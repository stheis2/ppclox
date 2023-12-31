#include "scanner.hpp"

const std::unordered_map<std::string_view, TokenType> Scanner::s_keywords{
    {"and", TokenType::AND},
    {"class", TokenType::CLASS},
    {"else", TokenType::ELSE},
    {"false", TokenType::FALSE},
    {"for", TokenType::FOR},
    {"fun", TokenType::FUN},
    {"if", TokenType::IF},
    {"nil", TokenType::NIL},
    {"or", TokenType::OR},
    {"print", TokenType::PRINT},
    {"return", TokenType::RETURN},
    {"super", TokenType::SUPER},
    {"this", TokenType::THIS},
    {"true", TokenType::TRUE},
    {"var", TokenType::VAR},
    {"while", TokenType::WHILE}
};

Scanner::Scanner(const char* source) {
    m_start = source;
    m_current = source;
    m_line = 1;
}

Token Scanner::scan_token() {
    skip_whitespace();
    m_start = m_current;

    if (is_at_end()) return make_token(TokenType::END_OF_FILE);

    char c = advance();
    if (is_alpha_or_underscore(c)) return identifier();
    if (is_digit(c)) return number();

    switch (c) {
        case '(': return make_token(TokenType::LEFT_PAREN);
        case ')': return make_token(TokenType::RIGHT_PAREN);
        case '{': return make_token(TokenType::LEFT_BRACE);
        case '}': return make_token(TokenType::RIGHT_BRACE);
        case ';': return make_token(TokenType::SEMICOLON);
        case ',': return make_token(TokenType::COMMA);
        case '.': return make_token(TokenType::DOT);
        case '-': return make_token(TokenType::MINUS);
        case '+': return make_token(TokenType::PLUS);
        case '/': return make_token(TokenType::SLASH);
        case '*': return make_token(TokenType::STAR);
        case '!':
            return make_token(
                match('=') ? TokenType::BANG_EQUAL : TokenType::BANG);
        case '=':
            return make_token(
                match('=') ? TokenType::EQUAL_EQUAL : TokenType::EQUAL);
        case '<':
            return make_token(
                match('=') ? TokenType::LESS_EQUAL : TokenType::LESS);
        case '>':
            return make_token(
                match('=') ? TokenType::GREATER_EQUAL : TokenType::GREATER);
        case '"':
            return string();
        default:
            // Break and return error token below
            break;
    }

    return error_token("Unexpected character.");
}

bool Scanner::is_alpha_or_underscore(char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
            c == '_';
}

char Scanner::advance() {
    m_current++;
    return m_current[-1];
}

char Scanner::peek_next() {
    if (is_at_end()) return '\0';
    return m_current[1];
}

bool Scanner::match(char expected) {
    if(is_at_end()) return false;
    if (*m_current != expected) return false;
    m_current++;
    return true;
}

Token Scanner::make_token(TokenType type) {
    Token token;
    token.type = type;
    token.start = m_start;
    token.length = token_len();
    token.line = m_line;
    return token;
}

Token Scanner::error_token(const char* message) {
    Token token;
    token.type = TokenType::ERROR;
    token.start = message;
    token.length = strlen(message);
    token.line = m_line;
    return token;
}

void Scanner::skip_whitespace() {
    for (;;) {
        char c = peek();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;
            case '\n':
                m_line++;
                advance();
                break;
            case '/':
                if (peek_next() == '/') {
                    // A comment goes until the end of the line
                    while (peek() != '\n' && !is_at_end())
                        advance();
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

TokenType Scanner::identifier_type() {
    auto it = s_keywords.find(std::string_view(m_start, token_len()));
    if (it != s_keywords.end()) {
        return it->second;
    }
    return TokenType::IDENTIFIER;
}

Token Scanner::identifier() {
    while (is_alpha_or_underscore(peek()) || is_digit(peek()))
        advance();
    return make_token(identifier_type());
}

Token Scanner::number() {
    while (Scanner::is_digit(peek()))
        advance();

    // Look for a fractional part
    if (peek() == '.' && Scanner::is_digit(peek_next())) {
        // Consume the "."
        advance();

        while (is_digit(peek())) advance();
    }

    return make_token(TokenType::NUMBER);
}

Token Scanner::string() {
    while (peek() != '"' && !is_at_end()) {
        if (peek() == '\n') m_line++;
        advance();
    }

    if (is_at_end()) return error_token("Unterminated string.");

    // The closing quote
    advance();
    return make_token(TokenType::STRING);
}