#ifndef ppclox_scanner_hpp
#define ppclox_scanner_hpp

#include "common.hpp"

enum class TokenType {
    // Single-character tokens.
    LEFT_PAREN, RIGHT_PAREN,
    LEFT_BRACE, RIGHT_BRACE,
    COMMA, DOT, MINUS, PLUS,
    SEMICOLON, SLASH, STAR,
    // One or two character tokens.
    BANG, BANG_EQUAL,
    EQUAL, EQUAL_EQUAL,
    GREATER, GREATER_EQUAL,
    LESS, LESS_EQUAL,
    // Literals.
    IDENTIFIER, STRING, NUMBER,
    // Keywords.
    AND, CLASS, ELSE, FALSE,
    FOR, FUN, IF, NIL, OR,
    PRINT, RETURN, SUPER, THIS,
    TRUE, VAR, WHILE,

    ERROR, 
    // EOF is #define'd in stdio.h, so we need to use something else
    END_OF_FILE
};

// TODO: Can we implement this using more C++ idioms?
class Token {
public:
    TokenType type{};
    const char* start{};
    std::size_t length{};
    std::size_t line{};
};

// TODO: Can we implement this using more C++ idioms?
class Scanner {
public:
    Scanner(const char* source);

    Token scan_token();
private:
    const char* m_start{};
    const char* m_current{};
    std::size_t m_line{};

    bool is_at_end() { return *m_current == '\0'; }
    char advance();
    char peek() { return *m_current; }
    bool match(char expected);
    Token make_token(TokenType type);
    Token error_token(const char* message);
    void skip_whitespace();
};

#endif