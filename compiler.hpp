#ifndef ppclox_compiler_hpp
#define ppclox_compiler_hpp

#include <memory>
#include <unordered_set>

#include "common.hpp"
#include "chunk.hpp"
#include "scanner.hpp"
#include "object.hpp"
#include "object_string.hpp"
#include "object_function.hpp"

class Parser {
public:
    Token current{};
    Token previous{};
    bool had_error{};
    bool panic_mode{};
};

class ClassCompiler {
public:
    bool m_has_superclass{};
};

enum class Precedence {
    NONE,
    ASSIGNMENT,  // =
    OR,          // or
    AND,         // and
    EQUALITY,    // == !=
    COMPARISON,  // < > <= >=
    TERM,        // + -
    FACTOR,      // * /
    UNARY,       // ! -
    CALL,        // . ()
    PRIMARY
};

typedef void (*ParseFn)(bool can_assign);

class ParseRule {
public:
    ParseFn prefix{};
    ParseFn infix{};
    Precedence precedence{};
};

class Local {
public:
    Token name{};
    int depth{};
    bool is_captured{};
};

class Upvalue {
public:
    std::uint8_t index{};
    bool is_local{};
};

//FIX - Should probably rework Compiler class to not use static.
//      Instead, there should be a Compiler class that gets instantiated,
//      and FunctionCompiler classes that contain the data for compiling
//      an individual function.
class Compiler {
public:
    static constexpr std::string_view k_init_string = "init";

    static ObjFunction* compile(const char* source);

    // NOTE! We pass in the ObjFunction instead of creating it inside
    //       the compiler constructor since we want to avoid
    //       GC potentially running when the Compiler is being constructed.
    //       If we are constructing the Compiler inside the s_compilers
    //       compiler stack, GC also has to iterate over that same stack
    //       to mark roots, which is a potentially messy situation that
    //       may or may not result in undefined behavior. We avoid this
    //       by decoupling the two operations.
    Compiler(ObjFunction* fun, FunctionType function_type);

    static void mark_gc_roots();

private:
    /** 
     * @todo Can we do this safer than just a raw pointer? Maybe not. The compiler constructs the
     *       function object, but when its done compiling it returns it to the caller.
     *       So the compiler doesn't "own" the function in any way. It will be cleaned up
     *       by the GC later. That said, we might be able to treat functions specially
     *       and manage them outside the GC. Since they are created at compile time,
     *       it seems like they don't necessarily need to be managed at runtime the same 
     *       way as objects created at runtime.
     */
    ObjFunction* m_function{};
    FunctionType m_function_type{};
    /** Locals are indexed by std::uint8_t at runtime, so thats the max we can currently support */
    static constexpr std::size_t k_locals_max = std::numeric_limits<std::uint8_t>::max() + 1;
    std::vector<Local> m_locals{};
    /** Upvalues are indexed by std::uint8_t at runtime, so thats the max we can currently support */
    static constexpr std::size_t k_upvalues_max = std::numeric_limits<std::uint8_t>::max() + 1;
    std::vector<Upvalue> m_upvalues{};
    int scope_depth{};

    /** During compilation, these will contain the scanner and parser for the current source */

    static std::unique_ptr<Scanner> s_scanner;
    static std::unique_ptr<Parser> s_parser;

    /** During compilation, we maintain a stack of compilers. */
    static std::vector<Compiler> s_compilers;
    typedef std::vector<Compiler>::reverse_iterator CompilerRevIterator;
    static Compiler& current() { return s_compilers.at(s_compilers.size() - 1); }
    static CompilerRevIterator compilers_rbegin() { return s_compilers.rbegin(); }
    static CompilerRevIterator compilers_rend() { return s_compilers.rend(); }

    /** During compilation, we also maintain a stack of ClassCompilers */
    static std::vector<ClassCompiler> s_class_compilers;
    static ClassCompiler& current_class_compiler() { return s_class_compilers.at(s_class_compilers.size() - 1); }

    /** 
     * During compilation, we sometimes need to hang on to some objects to prevent them
     * from being GC'd out from under us. We store pointers to these objects here.
    */
    static std::unordered_set<Obj*> s_temporary_roots;

    static ParseRule s_rules[];
    static ParseRule& get_rule(TokenType type) { return s_rules[std::to_underlying(type)]; }

    /** 
     * Gets a reference to the current chunk.
    */
    static Chunk& current_chunk() { return current().m_function->chunk(); }

    static void error_at(const Token& token, const char* message);
    static void error_at_current(const char* message);
    static void error(const char* message);
    static void synchronize();

    static void advance();
    static void consume(TokenType type, const char* message);
    static bool check(TokenType type);
    static bool match(TokenType type);

    static void emit_byte(std::uint8_t byte);
    static void emit_opcode(OpCode op_code);
    /** Emit an opcode followed by the given byte argument */
    static void emit_opcode_arg(OpCode op_code, std::uint8_t byte);
    static std::size_t emit_jump(OpCode instruction);
    static void patch_jump(std::size_t offset);
    static void emit_loop(std::size_t loop_start);
    static void emit_implicit_return();
    /** Add constant to the current chunk and return its index */
    static std::uint8_t make_constant(Value value);
    static std::uint8_t identifier_constant(const Token& name);
    static void emit_constant(Value value);
    /** Return index of local in given compiler's locals as output parameter. Boolean return indicates found or not found. */
    static bool resolve_local(const Compiler& compiler, const Token& name, std::uint8_t& out_index);
    /** Return index of upvalue in given compiler's upvalues as output parameter. Boolean return indicates found or not found. */
    static bool resolve_upvalue(const CompilerRevIterator compiler_rev_iter, const Token&name, std::uint8_t& out_index);
    /** Add upvalue to the given compiler's upvalues array with the given index. Returns index at which it was added. */
    static std::uint8_t add_upvalue(Compiler& compiler, std::uint8_t index, bool is_local);
    static std::uint8_t verify_index(std::size_t index, const char* message);
    static void add_local(Token name);
    static ObjFunction* end_compiler(std::vector<Upvalue>& out_upvalues);

    static void begin_scope();
    static void end_scope();

    static void parse_precedence(Precedence precedence);
    static std::uint8_t parse_variable(const char* error_message);
    static void declare_variable();
    static void define_variable(std::uint8_t global);
    static void mark_initialized();
    static void and_(bool can_assign);
    static void or_(bool can_assign);
    static void binary(bool can_assign);
    static void call(bool can_assign);
    static void dot(bool can_assign);
    static std::uint8_t argument_list();
    static void literal(bool can_assign);
    static void grouping(bool can_assign);
    static void number(bool can_assign);
    static void string(bool can_assign);
    static void named_variable(const Token& name, bool can_assign);
    static void variable(bool can_assign);
    static void super_(bool can_assign);
    static void this_(bool can_assign);
    static void unary(bool can_assign);
    static void expression();
    static void declaration();
    static void statement();
    static void class_declaration();
    static void fun_declaration();
    static void function(FunctionType type);
    static void method();
    static void var_declaration();
    static void print_statement();
    static void for_statement();
    static void if_statement();
    static void return_statement();
    static void while_statement();
    static void block();
    static void expression_statement();
};

#endif