#include "compiler.hpp"

/** Zero initialize these to start */
std::unique_ptr<Scanner> Compiler::s_scanner{};
std::unique_ptr<Parser> Compiler::s_parser{};
std::shared_ptr<Chunk> Compiler::s_current_chunk{};

// NOTE! Unfortunately C++ doesn't support array initialization
//       with enum indices, so we must resort to comments.
//       Make sure any changes to the TokenType enum get reflected here!
// TODO: Maybe switch to a map and generate the array at compilation start.
ParseRule Compiler::s_rules[] = {
    {grouping,    nullptr,   Precedence::NONE},         // [TokenType::LEFT_PAREN]
    {nullptr,     nullptr,   Precedence::NONE},         // [TokenType::RIGHT_PAREN]
    {nullptr,     nullptr,   Precedence::NONE},         // [TokenType::LEFT_BRACE]     
    {nullptr,     nullptr,   Precedence::NONE},         // [TokenType::RIGHT_BRACE]   
    {nullptr,     nullptr,   Precedence::NONE},         // [TokenType::COMMA]         
    {nullptr,     nullptr,   Precedence::NONE},         // [TokenType::DOT]           
    {unary,       binary,    Precedence::TERM},         // [TokenType::MINUS]         
    {nullptr,     binary,    Precedence::TERM},         // [TokenType::PLUS]          
    {nullptr,     nullptr,   Precedence::NONE},         // [TokenType::SEMICOLON]     
    {nullptr,     binary,    Precedence::FACTOR},       // [TokenType::SLASH]         
    {nullptr,     binary,    Precedence::FACTOR},       // [TokenType::STAR]          
    {unary,       nullptr,   Precedence::NONE},         // [TokenType::BANG]          
    {nullptr,     binary,    Precedence::EQUALITY},     // [TokenType::BANG_EQUAL]    
    {nullptr,     nullptr,   Precedence::NONE},         // [TokenType::EQUAL]         
    {nullptr,     binary,    Precedence::EQUALITY},     // [TokenType::EQUAL_EQUAL]   
    {nullptr,     binary,    Precedence::COMPARISON},   // [TokenType::GREATER]       
    {nullptr,     binary,    Precedence::COMPARISON},   // [TokenType::GREATER_EQUAL] 
    {nullptr,     binary,    Precedence::COMPARISON},   // [TokenType::LESS]          
    {nullptr,     binary,    Precedence::COMPARISON},   // [TokenType::LESS_EQUAL]    
    {nullptr,     nullptr,   Precedence::NONE},         // [TokenType::IDENTIFIER]    
    {string,      nullptr,   Precedence::NONE},         // [TokenType::STRING]        
    {number,      nullptr,   Precedence::NONE},         // [TokenType::NUMBER]        
    {nullptr,     nullptr,   Precedence::NONE},         // [TokenType::AND]           
    {nullptr,     nullptr,   Precedence::NONE},         // [TokenType::CLASS]         
    {nullptr,     nullptr,   Precedence::NONE},         // [TokenType::ELSE]          
    {literal,     nullptr,   Precedence::NONE},         // [TokenType::FALSE]         
    {nullptr,     nullptr,   Precedence::NONE},         // [TokenType::FOR]           
    {nullptr,     nullptr,   Precedence::NONE},         // [TokenType::FUN]           
    {nullptr,     nullptr,   Precedence::NONE},         // [TokenType::IF]            
    {literal,     nullptr,   Precedence::NONE},         // [TokenType::NIL]           
    {nullptr,     nullptr,   Precedence::NONE},         // [TokenType::OR]            
    {nullptr,     nullptr,   Precedence::NONE},         // [TokenType::PRINT]         
    {nullptr,     nullptr,   Precedence::NONE},         // [TokenType::RETURN]        
    {nullptr,     nullptr,   Precedence::NONE},         // [TokenType::SUPER]         
    {nullptr,     nullptr,   Precedence::NONE},         // [TokenType::THIS]          
    {literal,     nullptr,   Precedence::NONE},         // [TokenType::TRUE]          
    {nullptr,     nullptr,   Precedence::NONE},         // [TokenType::VAR]           
    {nullptr,     nullptr,   Precedence::NONE},         // [TokenType::WHILE]         
    {nullptr,     nullptr,   Precedence::NONE},         // [TokenType::ERROR]         
    {nullptr,     nullptr,   Precedence::NONE},         // [TokenType::END_OF_FILE]   
};

bool Compiler::compile(const char* source, const std::shared_ptr<Chunk>& chunk) {
    // Create a scanner and parser we can use for this source
    s_scanner = std::make_unique<Scanner>(source);
    s_parser = std::make_unique<Parser>();

    // Store our chunk while we operate on it
    s_current_chunk = chunk;

    advance();
    expression();
    consume(TokenType::END_OF_FILE, "Expect end of expression.");
    end_compiler();
    bool had_error = s_parser->had_error;

    // Now that we are done compiling, destroy the scanner and parser,
    // and release our reference to the chunk
    s_scanner = nullptr;
    s_parser = nullptr;
    s_current_chunk = nullptr;

    return !had_error;
}

void Compiler::error_at(const Token& token, const char* message) {
    if (s_parser->panic_mode) return;
    s_parser->panic_mode = true;
    fprintf(stderr, "[line %zu] Error", token.line);

    if (token.type == TokenType::END_OF_FILE) {
        fprintf(stderr, " at end");
    } else if (token.type == TokenType::ERROR) {
        // Nothing, as the message will contain the info
    } else {
        if (token.length <= std::numeric_limits<int>::max()) {
            fprintf(stderr, " at '%.*s'", (int)token.length, token.start);
        }
        else {
            fprintf(stderr, " at %s", "Token too long to display.");
        }
    }

    fprintf(stderr, ": %s\n", message);
    s_parser->had_error = true;
}

void Compiler::error_at_current(const char* message) {
    error_at(s_parser->current, message);
}

void Compiler::error(const char* message) {
    error_at(s_parser->previous, message);
}

void Compiler::advance() {
    s_parser->previous = s_parser->current;

    for (;;) {
        s_parser->current = s_scanner->scan_token();
        if (s_parser->current.type != TokenType::ERROR) break;

        // An error token will contain the error message to display
        error_at_current(s_parser->current.start);
    }
}

void Compiler::consume(TokenType type, const char* message) {
    if (s_parser->current.type == type) {
        advance();
        return;
    }

    error_at_current(message);
}

void Compiler::emit_byte(std::uint8_t byte) {
    current_chunk()->write(byte, s_parser->previous.line);
}

void Compiler::emit_opcode(OpCode op_code) {
    emit_byte(std::to_underlying(op_code));
}

void Compiler::emit_opcode(OpCode op_code, std::uint8_t byte) {
    emit_byte(std::to_underlying(op_code));
    emit_byte(byte);
}

void Compiler::emit_return() {
    return emit_opcode(OpCode::RETURN);
}

std::uint8_t Compiler::make_constant(Value value) {
    std::size_t index = current_chunk()->add_constant(value);
    if (index > std::numeric_limits<std::uint8_t>::max()) {
        // NOTE! If this were a full-sized language implementation, we’d want to add another 
        //       instruction like OP_CONSTANT_16 that stores the index as a two-byte operand 
        //       so we could handle more constants when needed.
        error("Too many constants in one chunk.");
        return 0;
    }
    return (std::uint8_t)index;
}

void Compiler::emit_constant(Value value) {
    emit_opcode(OpCode::CONSTANT, make_constant(value));
}

void Compiler::end_compiler() {
    emit_return();
#ifdef DEBUG_PRINT_CODE
    if (!s_parser->had_error) {
        current_chunk()->dissassemble("code");
    }
#endif    
}

void Compiler::parse_precedence(Precedence precedence) {
    advance();
    ParseFn prefix_rule = get_rule(s_parser->previous.type).prefix;
    if (prefix_rule == nullptr) {
        error("Expect expression. No prefix rule found in parse_precedence.");
        return;
    }

    prefix_rule();

    while (precedence <= get_rule(s_parser->current.type).precedence) {
        advance();
        ParseFn infix_rule = get_rule(s_parser->previous.type).infix;
        infix_rule();
    }
}

void Compiler::binary() {
    TokenType operator_type = s_parser->previous.type;
    ParseRule& rule = get_rule(operator_type);
    parse_precedence(static_cast<Precedence>(std::to_underlying(rule.precedence) + 1));

    switch (operator_type) {
        case TokenType::BANG_EQUAL: {
            emit_opcode(OpCode::EQUAL);
            emit_opcode(OpCode::NOT);
            break;
        }
        case TokenType::EQUAL_EQUAL: emit_opcode(OpCode::EQUAL); break;
        case TokenType::GREATER: emit_opcode(OpCode::GREATER); break;
        case TokenType::GREATER_EQUAL: {
            emit_opcode(OpCode::LESS);
            emit_opcode(OpCode::NOT);  
            break;
        }
        case TokenType::LESS: emit_opcode(OpCode::LESS); break;
        case TokenType::LESS_EQUAL: {
            emit_opcode(OpCode::GREATER);
            emit_opcode(OpCode::NOT);  
            break;
        }
        case TokenType::PLUS:  emit_opcode(OpCode::ADD); break;
        case TokenType::MINUS: emit_opcode(OpCode::SUBTRACT); break;
        case TokenType::STAR:  emit_opcode(OpCode::MULTIPLY); break;
        case TokenType::SLASH: emit_opcode(OpCode::DIVIDE); break;
        default:
            // Should be unreachable
            error("Unhandled operator type after compiling binary expressions.");
            return;
    }
}

void Compiler::literal() {
    switch (s_parser->previous.type) {
        case TokenType::FALSE: emit_opcode(OpCode::FALSE); break;
        case TokenType::NIL: emit_opcode(OpCode::NIL); break;
        case TokenType::TRUE: emit_opcode(OpCode::TRUE); break;
        default:
            // Should be unreachable
            error("Unhandled token type for literal.");
            return;
    }
}

void Compiler::grouping() {
    expression();
    consume(TokenType::RIGHT_PAREN, "Expect ')' after expression.");
}

void Compiler::number() {
    double value = strtod(s_parser->previous.start, nullptr);
    emit_constant(value);
}

void Compiler::string() {
    // Copy the chars between the ""
    // NOTE! If Lox supported string escape sequences like \n, 
    //       we’d translate those here. Since it doesn’t, we can 
    //       take the characters as they are.
    emit_constant(Value(ObjString::copy_string(s_parser->previous.start + 1, 
        s_parser->previous.length - 2)));
}

void Compiler::unary() {
    TokenType operator_type = s_parser->previous.type;

    // Compile the operand
    parse_precedence(Precedence::UNARY);

    // Emit the operator instruction
    switch (operator_type) {
        case TokenType::BANG: emit_opcode(OpCode::NOT); break;
        case TokenType::MINUS: emit_opcode(OpCode::NEGATE); break;
        default:
            error("Unhandled operator type after compiling unary expression.");
            return;
    }
}

void Compiler::expression() {
//TODO: Should this really be Precedence::NONE?
    parse_precedence(Precedence::ASSIGNMENT);
}