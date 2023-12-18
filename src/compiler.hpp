#ifndef ppclox_compiler_hpp
#define ppclox_compiler_hpp

#include <memory>

#include "common.hpp"
#include "chunk.hpp"
#include "scanner.hpp"
#include "object.hpp"

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

typedef void (*ParseFn)();

class ParseRule {
public:
    ParseFn prefix{};
    ParseFn infix{};
    Precedence precedence{};
};

class Compiler {
public:
    static bool compile(const char* source, const std::shared_ptr<Chunk>& chunk);

private:
    /** During compilation, these will contain the scanner and parser for the current source */

    static std::unique_ptr<Scanner> s_scanner;
    static std::unique_ptr<Parser> s_parser;

    /** For now, the current chunk will just be a static member. Later on this will change */
    static std::shared_ptr<Chunk> s_current_chunk;

    static ParseRule s_rules[];
    static ParseRule& get_rule(TokenType type) { return s_rules[std::to_underlying(type)]; }

    /** 
     * Gets a pointer to the current chunk.
    */
    static std::shared_ptr<Chunk> current_chunk() { return s_current_chunk; }

    static void error_at(const Token& token, const char* message);
    static void error_at_current(const char* message);
    static void error(const char* message);

    static void advance();
    static void consume(TokenType type, const char* message);

    static void emit_byte(std::uint8_t byte);
    static void emit_opcode(OpCode op_code);
    /** Emit an opcode followed by the given byte */
    static void emit_opcode(OpCode op_code, std::uint8_t byte);
    static void emit_return();
    /** Add constant to the current chunk and return its index */
    static std::uint8_t make_constant(Value value);
    static void emit_constant(Value value);
    static void end_compiler();

    static void parse_precedence(Precedence precedence);
    static void binary();
    static void literal();
    static void grouping();
    static void number();
    static void string();
    static void unary();
    static void expression();
};

#endif