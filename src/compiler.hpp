#ifndef ppclox_compiler_hpp
#define ppclox_compiler_hpp

#include <memory>

#include "common.hpp"
#include "chunk.hpp"
#include "scanner.hpp"
#include "object.hpp"
#include "object_string.hpp"

class Parser {
public:
    Token current{};
    Token previous{};
    bool had_error{};
    bool panic_mode{};
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
};

class Compiler {
public:
    static ObjFunction* compile(const char* source);

    Compiler(FunctionType function_type);

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
    ObjFunction* m_function;
    FunctionType m_function_type;
    /** Locals are indexed by std::uint8_t at runtime, so thats the max we can currently support */
    static constexpr std::uint32_t k_locals_max = std::numeric_limits<std::uint8_t>::max() + 1;
    std::vector<Local> m_locals{};
    int scope_depth{};

    /** During compilation, these will contain the scanner and parser for the current source */

    static std::unique_ptr<Scanner> s_scanner;
    static std::unique_ptr<Parser> s_parser;

    /** During compilation, we maintain a stack of compilers. */
    static std::vector<Compiler> s_compilers;
    static Compiler& current() { return s_compilers.at(s_compilers.size() - 1); }

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
    static void emit_nil_return();
    /** Add constant to the current chunk and return its index */
    static std::uint8_t make_constant(Value value);
    static std::uint8_t identifier_constant(const Token& name);
    static void emit_constant(Value value);
    /** Return index of local in given compiler's locals as output parameter. Boolean return indicates found or not found. */
    static bool resolve_local(const Compiler& compiler, const Token& name, std::size_t& out_index);
    static void add_local(Token name);
    static ObjFunction* end_compiler();

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
    static std::uint8_t argument_list();
    static void literal(bool can_assign);
    static void grouping(bool can_assign);
    static void number(bool can_assign);
    static void string(bool can_assign);
    static void named_variable(const Token& name, bool can_assign);
    static void variable(bool can_assign);
    static void unary(bool can_assign);
    static void expression();
    static void declaration();
    static void statement();
    static void fun_declaration();
    static void function(FunctionType type);
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