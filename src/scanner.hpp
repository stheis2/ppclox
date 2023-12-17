#ifndef ppclox_scanner_hpp
#define ppclox_scanner_hpp

#include <string_view>
#include <unordered_map>

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
    /** Start of characters for the current token */
    const char* m_start{};
    /** Next character to consume */
    const char* m_current{};
    std::size_t m_line{};

    static bool is_alpha_or_underscore(char c);
    static bool is_digit(char c) { return c >= '0' && c <= '9'; }

    bool is_at_end() { return *m_current == '\0'; }
    char advance();
    char peek() { return *m_current; }
    char peek_next();
    bool match(char expected);
    /** Length of the current token, based on characters we've consumed */
    std::size_t token_len() { return m_current - m_start; }
    Token make_token(TokenType type);
    Token error_token(const char* message);
    void skip_whitespace();
    TokenType identifier_type();
    Token identifier();
    Token number();
    Token string();

    // Map of keyword strings to their token type.
    // Keys are views of string literals which have static lifetime.
    // NOTE! This is probably slower than the Clox implementation,
    //       though it's easier to modify.
    //       For a real language, we'd probably use the faster
    //       approach (like Clox or V8 does).
    static const std::unordered_map<std::string_view, TokenType> s_keywords;
};

#endif