#include <optional>

#include "compiler.hpp"

/** Zero initialize these to start */
std::unique_ptr<Scanner> Compiler::s_scanner{};
std::unique_ptr<Parser> Compiler::s_parser{};
std::vector<Compiler> Compiler::s_compilers{};

// NOTE! Unfortunately C++ doesn't support array initialization
//       with enum indices, so we must resort to comments.
//       Make sure any changes to the TokenType enum get reflected here!
// TODO: Maybe switch to a map and generate the array at compilation start.
ParseRule Compiler::s_rules[] = {
    {grouping,    call,      Precedence::CALL},         // [TokenType::LEFT_PAREN]
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
    {variable,    nullptr,   Precedence::NONE},         // [TokenType::IDENTIFIER]    
    {string,      nullptr,   Precedence::NONE},         // [TokenType::STRING]        
    {number,      nullptr,   Precedence::NONE},         // [TokenType::NUMBER]        
    {nullptr,     and_,      Precedence::AND},          // [TokenType::AND]           
    {nullptr,     nullptr,   Precedence::NONE},         // [TokenType::CLASS]         
    {nullptr,     nullptr,   Precedence::NONE},         // [TokenType::ELSE]          
    {literal,     nullptr,   Precedence::NONE},         // [TokenType::FALSE]         
    {nullptr,     nullptr,   Precedence::NONE},         // [TokenType::FOR]           
    {nullptr,     nullptr,   Precedence::NONE},         // [TokenType::FUN]           
    {nullptr,     nullptr,   Precedence::NONE},         // [TokenType::IF]            
    {literal,     nullptr,   Precedence::NONE},         // [TokenType::NIL]           
    {nullptr,     or_,       Precedence::OR},           // [TokenType::OR]            
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

ObjFunction* Compiler::compile(const char* source) {
    // Create a scanner and parser we can use for this source
    s_scanner = std::make_unique<Scanner>(source);
    s_parser = std::make_unique<Parser>();

    // Create our initial compiler on the compiler stack
    s_compilers.emplace_back(FunctionType::SCRIPT);

    // From now on, the compiler implicitly claims stack slot zero
    // for the VM's own internal use. It does this in form of a dummy local.
    Token dummy{};
    current().add_local(dummy);

    advance();
    while (!match(TokenType::END_OF_FILE)) {
        declaration();
    }
    ObjFunction* function = end_compiler();
    bool had_error = s_parser->had_error;

    // Now that we are done compiling, destroy the scanner and parser,
    // and release our reference to the chunk
    s_scanner = nullptr;
    s_parser = nullptr;

    return had_error ? nullptr : function;
}

Compiler::Compiler(FunctionType function_type) : 
    m_function_type(function_type) {
    // Make a new chunk and function to compile into
    std::shared_ptr<Chunk> chunk = std::make_shared<Chunk>();

    // When compiling a function declaration, we call Compiler() ctor right 
    // after we parse the function’s name. That means we can grab the name 
    // right then from the previous token.
    ObjString* name = nullptr;
    if (function_type != FunctionType::SCRIPT) {
        name = ObjString::copy_string(s_parser->previous.start, s_parser->previous.length);
    }

    m_function = new ObjFunction(chunk, name);
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

void Compiler::synchronize() {
    s_parser->panic_mode = false;

    /** 
     * Skip tokens indiscriminantly until we reach something that
     * looks like a statement boundary.
    */
    while (s_parser->current.type != TokenType::END_OF_FILE) {
        if (s_parser->previous.type == TokenType::SEMICOLON) return;
        switch(s_parser->current.type) {
            case TokenType::CLASS:
            case TokenType::FUN:
            case TokenType::VAR:
            case TokenType::FOR:
            case TokenType::IF:
            case TokenType::WHILE:
            case TokenType::PRINT:
            case TokenType::RETURN:
                return;
            default:
                break;
        }

        advance();
    }
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

bool Compiler::check(TokenType type) {
    return s_parser->current.type == type;
}

bool Compiler::match(TokenType type) {
    if (!check(type)) return false;
    advance();
    return true;
}

void Compiler::emit_byte(std::uint8_t byte) {
    current_chunk().write(byte, s_parser->previous.line);
}

void Compiler::emit_opcode(OpCode op_code) {
    emit_byte(std::to_underlying(op_code));
}

void Compiler::emit_opcode_arg(OpCode op_code, std::uint8_t byte) {
    emit_byte(std::to_underlying(op_code));
    emit_byte(byte);
}

std::size_t Compiler::emit_jump(OpCode instruction) {
    emit_opcode(instruction);
    // Emit two bytes that will be filled in later
    emit_byte(0xff);
    emit_byte(0xff);
    return current_chunk().get_code().size() - 2;
}

void Compiler::patch_jump(std::size_t offset) {
    // -2 to adjust for the bytecode for the jump offset itself.
    // The given offset is to the jump offset, but the jump is relative to
    // AFTER the jump offset.
    std::size_t jump = current_chunk().get_code().size() - offset - 2;

    if (jump > std::numeric_limits<std::uint16_t>::max()) {
        error("Too much code to jump over.");
    }

    std::uint8_t ho_byte = static_cast<std::uint8_t>((jump >> 8) & 0xff);
    std::uint8_t lo_byte = static_cast<std::uint8_t>(jump & 0xff);

    current_chunk().patch_at(offset, ho_byte);
    current_chunk().patch_at(offset + 1, lo_byte);
}

void Compiler::emit_loop(std::size_t loop_start) {
    emit_opcode(OpCode::LOOP);

    // +2 is to take into account the size of the OpCode::LOOP operands
    // which we also need to jump over.
    std::size_t offset = current_chunk().get_code().size() - loop_start + 2;
    if (offset > std::numeric_limits<std::uint16_t>::max()) {
        error("Loop body too large.");
    }

    std::uint8_t ho_byte = static_cast<std::uint8_t>((offset >> 8) & 0xff);
    std::uint8_t lo_byte = static_cast<std::uint8_t>(offset & 0xff);

    emit_byte(ho_byte);
    emit_byte(lo_byte);
}

void Compiler::emit_return() {
    return emit_opcode(OpCode::RETURN);
}

std::uint8_t Compiler::make_constant(Value value) {
    std::size_t index = current_chunk().add_constant(value);
    if (index > std::numeric_limits<std::uint8_t>::max()) {
        // NOTE! If this were a full-sized language implementation, we’d want to add another 
        //       instruction like OP_CONSTANT_16 that stores the index as a two-byte operand 
        //       so we could handle more constants when needed.
        error("Too many constants in one chunk.");
        return 0;
    }
    return (std::uint8_t)index;
}

std::uint8_t Compiler::identifier_constant(const Token& name) {
    return make_constant(ObjString::copy_string(name.start, name.length));
}

void Compiler::emit_constant(Value value) {
    emit_opcode_arg(OpCode::CONSTANT, make_constant(value));
}

bool Compiler::resolve_local(const Compiler& compiler, const Token& name, std::size_t& out_index) {
    // Iterate backwards and find the index of the local.
    // Use it = index + 1 to avoid underflow.
    for (std::size_t it = compiler.m_locals.size(); it > 0; --it) {
        std::size_t index = it - 1;
        const Local& local = compiler.m_locals.at(index);
        if (name.as_string_view() == local.name.as_string_view()) {
            if (local.depth == -1) {
                error("Can't read local variable in its own initializer");
            }
            out_index = index;
            return true;
        }
    }

    return false;
}

void Compiler::add_local(Token name) {
    if (current().m_locals.size() >= k_locals_max) {
        error("Too many local variables in function in add local.");
        return;
    }
    current().m_locals.emplace_back(Local {
        .name = name,
        .depth = -1
    });
}


ObjFunction* Compiler::end_compiler() {
    emit_return();
    ObjFunction* function = current().m_function;

#ifdef DEBUG_PRINT_CODE
    if (!s_parser->had_error) {
        current_chunk().dissassemble(function->name());
    }
#endif

    // Pop the compiler that compiled this function from the stack
    s_compilers.pop_back();
    return function;
}

void Compiler::begin_scope() {
    current().scope_depth++;
}

void Compiler::end_scope() {
    current().scope_depth--;

    // We need to pop all local variables when we leave the scope
    while (current().m_locals.size() > 0 && current().m_locals.back().depth > current().scope_depth) {
        emit_opcode(OpCode::POP);
        current().m_locals.pop_back();
    }
}

void Compiler::parse_precedence(Precedence precedence) {
    advance();
    ParseFn prefix_rule = get_rule(s_parser->previous.type).prefix;
    if (prefix_rule == nullptr) {
        error("Expect expression. No prefix rule found in parse_precedence.");
        return;
    }

    bool can_assign = precedence <= Precedence::ASSIGNMENT;
    prefix_rule(can_assign);

    while (precedence <= get_rule(s_parser->current.type).precedence) {
        advance();
        ParseFn infix_rule = get_rule(s_parser->previous.type).infix;
        infix_rule(can_assign);
    }

    if (can_assign && match(TokenType::EQUAL)) {
        error("invalid assignment target.");
    }
}

std::uint8_t Compiler::parse_variable(const char* error_message) {
    consume(TokenType::IDENTIFIER, error_message);

    declare_variable();
    // Return dummy index if we're in a local scope
    if (current().scope_depth > 0) return 0;

    return identifier_constant(s_parser->previous);
}

void Compiler::declare_variable() {
    // Globals should not be added to the locals
    if (current().scope_depth == 0) return;

    const Token& name = s_parser->previous;

    /** Iterate the locals in reverse order, checking for duplicates */
    for (auto it = current().m_locals.rbegin(); it != current().m_locals.rend(); ++it) {
        const Local& local = *it;

        // Once we reach a lower scope, we're done since we allow
        // same named variables in different scopes.
        if (local.depth != -1 && local.depth < current().scope_depth) {
            break;
        }

        if (name.as_string_view() == local.name.as_string_view()) {
            error("Already a variable with this name in this scope.");
        }
    }

    add_local(name);
}

void Compiler::define_variable(std::uint8_t global) {
    // Locals don't need to be explicitly defined since they
    // live on the value stack
    if (current().scope_depth > 0) {
        mark_initialized();
        return;
    }
    emit_opcode_arg(OpCode::DEFINE_GLOBAL, global);
}

void Compiler::mark_initialized() {
    // This may be called when compiling functions declared at the top level,
    // so bail early in that case.
    if (current().scope_depth == 0) return;
    current().m_locals.at(current().m_locals.size() - 1).depth = current().scope_depth;
}

void Compiler::and_(bool can_assign) {
    std::size_t end_jump = emit_jump(OpCode::JUMP_IF_FALSE);

    emit_opcode(OpCode::POP);
    parse_precedence(Precedence::AND);

    patch_jump(end_jump);
}

void Compiler::or_(bool can_assign) {
    std::size_t else_jump = emit_jump(OpCode::JUMP_IF_FALSE);
    std::size_t end_jump = emit_jump(OpCode::JUMP);

    patch_jump(else_jump);
    emit_opcode(OpCode::POP);

    parse_precedence(Precedence::OR);
    patch_jump(end_jump);
}

void Compiler::binary(bool can_assign) {
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

void Compiler::call(bool can_assign) {
    std::uint8_t arg_count = argument_list();
    emit_opcode_arg(OpCode::CALL, arg_count);
}

std::uint8_t Compiler::argument_list() {
    std::uint8_t arg_count = 0;
    if (!check(TokenType::RIGHT_PAREN)) {
        do {
            expression();
            if (arg_count == std::numeric_limits<std::uint8_t>::max()) {
                error("Can't have more than 255 arguments");
            }
            arg_count++;
        } while (match(TokenType::COMMA));
    }
    consume(TokenType::RIGHT_PAREN, "Expect ')' after arguments.");
    return arg_count;
}

void Compiler::literal(bool can_assign) {
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

void Compiler::grouping(bool can_assign) {
    expression();
    consume(TokenType::RIGHT_PAREN, "Expect ')' after expression.");
}

void Compiler::number(bool can_assign) {
    double value = strtod(s_parser->previous.start, nullptr);
    emit_constant(value);
}

void Compiler::string(bool can_assign) {
    // Copy the chars between the ""
    // NOTE! If Lox supported string escape sequences like \n, 
    //       we’d translate those here. Since it doesn’t, we can 
    //       take the characters as they are.
    emit_constant(Value(ObjString::copy_string(s_parser->previous.start + 1, 
        s_parser->previous.length - 2)));
}

void Compiler::named_variable(const Token& name, bool can_assign) {
    OpCode get_op{};
    OpCode set_op{};
    std::uint8_t arg{};

    std::size_t local_index{};
    if (resolve_local(current(), name, local_index)) {
        get_op = OpCode::GET_LOCAL;
        set_op = OpCode::SET_LOCAL;

        // This should never happend, but verify here
        if (local_index >= std::numeric_limits<std::uint8_t>::max()) {
            error("Too many local variables in function in named_variable.");
            return;
        }
        arg = static_cast<std::uint8_t>(local_index);
    } else {
        get_op = OpCode::GET_GLOBAL;
        set_op = OpCode::SET_GLOBAL;
        arg = identifier_constant(name);
    }


    if (can_assign && match(TokenType::EQUAL)) {
        expression();
        emit_opcode_arg(set_op, arg);
    } else {
        emit_opcode_arg(get_op, arg);
    }
}

void Compiler::variable(bool can_assign) {
    named_variable(s_parser->previous, can_assign);
}

void Compiler::unary(bool can_assign) {
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

void Compiler::declaration() {
    if (match(TokenType::FUN)) {
        fun_declaration();
    } else if (match(TokenType::VAR)) {
        var_declaration();
    } else {
        statement();
    }

    if (s_parser->panic_mode) synchronize();
}

void Compiler::statement() {
    if (match(TokenType::PRINT)) {
        print_statement();
    } else if (match(TokenType::FOR)) {
        for_statement();
    } else if (match(TokenType::IF)) {
        if_statement();
    } else if (match(TokenType::WHILE)) {
        while_statement();
    } else if (match(TokenType::LEFT_BRACE)) {
        begin_scope();
        block();
        end_scope();
    }
    else {
        expression_statement();
    }
}

void Compiler::fun_declaration() {
    std::uint8_t global = parse_variable("Expect function name.");
    // It’s safe for a function to refer to its own name inside its body. 
    // You can’t call the function and execute the body until after it’s fully defined, 
    // so you’ll never see the variable in an uninitialized state.
    mark_initialized();
    function(FunctionType::FUNCTION);
    define_variable(global);
}

void Compiler::function(FunctionType type) {
    // When we start compiling a function, we need
    // to instantiate a new compiler on the stack.
    s_compilers.emplace_back(type);
    begin_scope();

    consume(TokenType::LEFT_PAREN, "Expect '(' after function name.");
    if (!check(TokenType::RIGHT_PAREN)) {
        do {
            current().m_function->m_arity++;
            if (current().m_function->m_arity > std::numeric_limits<std::uint8_t>::max()) {
                error_at_current("Can't have more than 255 parameters.");
            }
            std::uint8_t constant = parse_variable("Expect parameter name.");
            define_variable(constant);
        } while (match(TokenType::COMMA));
    }
    consume(TokenType::RIGHT_PAREN, "Expect ')' after parameters.");
    consume(TokenType::LEFT_BRACE, "Expect '{' before function body.");
    block();

    // This beginScope() doesn’t have a corresponding endScope() call. 
    // Because we end Compiler completely when we reach the end of the function body, 
    // there’s no need to close the lingering outermost scope.
    ObjFunction* function = end_compiler();
    emit_opcode_arg(OpCode::CONSTANT, make_constant(function));
}

void Compiler::var_declaration() {
    std::uint8_t global = parse_variable("Expect variable name.");

    if (match(TokenType::EQUAL)) {
        expression();
    } else {
        emit_opcode(OpCode::NIL);
    }
    consume(TokenType::SEMICOLON, "Expect ';' after variable declaration.");
    define_variable(global);
}

void Compiler::print_statement() {
    expression();
    consume(TokenType::SEMICOLON, "Expect ';' after value in print statement.");
    emit_opcode(OpCode::PRINT);
}

void Compiler::for_statement() {
    begin_scope();
    consume(TokenType::LEFT_PAREN, "Expect '(' after 'for'.");
    if (match(TokenType::SEMICOLON)) {
        // No initializer.
    } else if (match(TokenType::VAR)) {
        var_declaration();
    } else {
        expression_statement();
    }

    std::size_t loop_start = current_chunk().get_code().size();
    std::optional<std::size_t> exit_jump = std::nullopt;
    if (!match(TokenType::SEMICOLON)) {
        expression();
        consume(TokenType::SEMICOLON, "expect ';' after loop condition.");

        // Jump out of the loop if the condition is false
        exit_jump = emit_jump(OpCode::JUMP_IF_FALSE);
        emit_opcode(OpCode::POP);
    }

    if (!match(TokenType::RIGHT_PAREN)) {
        std::size_t body_jump = emit_jump(OpCode::JUMP);
        std::size_t increment_start = current_chunk().get_code().size();
        expression();
        emit_opcode(OpCode::POP);
        consume(TokenType::RIGHT_PAREN, "Expect ')' after for clauses.");

        emit_loop(loop_start);
        loop_start = increment_start;
        patch_jump(body_jump);
    }

    statement();
    emit_loop(loop_start);

    if (exit_jump != std::nullopt) {
        patch_jump(exit_jump.value());
        emit_opcode(OpCode::POP);
    }

    end_scope();
}

void Compiler::if_statement() {
    consume(TokenType::LEFT_PAREN, "Expect '(' after 'if'.");
    expression();
    consume(TokenType::RIGHT_PAREN, "Expect ')' after condition.");

    std::size_t then_jump = emit_jump(OpCode::JUMP_IF_FALSE);
    emit_opcode(OpCode::POP);
    statement();

    std::size_t else_jump = emit_jump(OpCode::JUMP);
    patch_jump(then_jump);
    emit_opcode(OpCode::POP);

    if (match(TokenType::ELSE)) statement();
    patch_jump(else_jump);
}

void Compiler::while_statement() {
    std::size_t loop_start = current_chunk().get_code().size();
    consume(TokenType::LEFT_PAREN, "Expect '(' after 'while'.");
    expression();
    consume(TokenType::RIGHT_PAREN, "Expect ')' after condition,");

    std::size_t exit_jump = emit_jump(OpCode::JUMP_IF_FALSE);
    emit_opcode(OpCode::POP);
    statement();
    emit_loop(loop_start);

    patch_jump(exit_jump);
    emit_opcode(OpCode::POP);
}

void Compiler::block() {
    while (!check(TokenType::RIGHT_BRACE) && !check(TokenType::END_OF_FILE)) {
        declaration();
    }

    consume(TokenType::RIGHT_BRACE, "Expect '}' after block.");
}

void Compiler::expression_statement() {
    expression();
    consume(TokenType::SEMICOLON, "Expect ';' after expression.");
    emit_opcode(OpCode::POP);
}