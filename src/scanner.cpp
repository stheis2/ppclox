#include "scanner.hpp"

Scanner::Scanner(const char* source) {
    m_start = source;
    m_current = source;
    m_line = 1;
}

Token Scanner::scan_token() {
    this->skip_whitespace();
    m_start = m_current;

    if (this->is_at_end()) return this->make_token(TokenType::END_OF_FILE);

    char c = this->advance();

    switch (c) {
        case '(': return this->make_token(TokenType::LEFT_PAREN);
        case ')': return this->make_token(TokenType::RIGHT_PAREN);
        case '{': return this->make_token(TokenType::LEFT_BRACE);
        case '}': return this->make_token(TokenType::RIGHT_BRACE);
        case ';': return this->make_token(TokenType::SEMICOLON);
        case ',': return this->make_token(TokenType::COMMA);
        case '.': return this->make_token(TokenType::DOT);
        case '-': return this->make_token(TokenType::MINUS);
        case '+': return this->make_token(TokenType::PLUS);
        case '/': return this->make_token(TokenType::SLASH);
        case '*': return this->make_token(TokenType::STAR);
        case '!':
            return this->make_token(
                this->match('=') ? TokenType::BANG_EQUAL : TokenType::BANG);
        case '=':
            return this->make_token(
                this->match('=') ? TokenType::EQUAL_EQUAL : TokenType::EQUAL);
        case '<':
            return this->make_token(
                this->match('=') ? TokenType::LESS_EQUAL : TokenType::LESS);
        case '>':
            return this->make_token(
                this->match('=') ? TokenType::GREATER_EQUAL : TokenType::GREATER);
        default:
            // Break and return error token below
            break;
    }

    return this->error_token("Unexpected character.");
}

char Scanner::advance() {
    m_current++;
    return m_current[-1];
}

bool Scanner::match(char expected) {
    if(this->is_at_end()) return false;
    if (*m_current != expected) return false;
    m_current++;
    return true;
}

Token Scanner::make_token(TokenType type) {
    Token token;
    token.type = type;
    token.start = m_start;
    token.length = m_current - m_start;
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
        char c = this->peek();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                this->advance();
                break;
            case '\n':
                m_line++;
                this->advance();
                break;
            default:
                return;
        }
    }
}