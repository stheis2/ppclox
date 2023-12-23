#include "chunk.hpp"

void Chunk::write(std::uint8_t byte, std::size_t line) {
    m_code.push_back(byte);
    m_lines.push_back(line);
}

std::size_t Chunk::add_constant(Value value) {
    m_constants.push_back(value);
    return m_constants.size() - 1;
}

void Chunk::dissassemble(const char* name) {
    printf("== %s ==\n", name);

    for (std::size_t offset = 0; offset < m_code.size();) {
        offset = this->disassemble_instruction(offset);
    }
}

std::size_t Chunk::disassemble_instruction(std::size_t offset) {
    printf("%04zu ", offset);
    if (offset > 0 && m_lines.at(offset) == m_lines.at(offset - 1)) {
        printf("   | ");
    } else {
        printf("%4zu ", m_lines.at(offset));
    }

    std::uint8_t instruction = m_code.at(offset);

    // Static cast is safe since we have a default case
    switch (instruction) {
        case std::to_underlying(OpCode::CONSTANT):
            return Chunk::constant_instruction("OP_CONSTANT", *this, offset);
        case std::to_underlying(OpCode::NIL):
            return Chunk::simple_instruction("OP_NIL", offset);            
        case std::to_underlying(OpCode::TRUE):
            return Chunk::simple_instruction("OP_TRUE", offset);            
        case std::to_underlying(OpCode::FALSE):
            return Chunk::simple_instruction("OP_FALSE", offset);     
        case std::to_underlying(OpCode::EQUAL):
            return Chunk::simple_instruction("OP_EQUAL", offset);
        case std::to_underlying(OpCode::GREATER):
            return Chunk::simple_instruction("OP_GREATER", offset);
        case std::to_underlying(OpCode::LESS):
            return Chunk::simple_instruction("OP_LESS", offset);       
        case std::to_underlying(OpCode::ADD):
            return Chunk::simple_instruction("OP_ADD", offset);
        case std::to_underlying(OpCode::SUBTRACT):
            return Chunk::simple_instruction("OP_SUBTRACT", offset);
        case std::to_underlying(OpCode::MULTIPLY):
            return Chunk::simple_instruction("OP_MULTIPLY", offset);
        case std::to_underlying(OpCode::DIVIDE):
            return Chunk::simple_instruction("OP_DIVIDE", offset);            
        case std::to_underlying(OpCode::NOT):
            return Chunk::simple_instruction("OP_NOT", offset);
        case std::to_underlying(OpCode::NEGATE):
            return Chunk::simple_instruction("OP_NEGATE", offset);
        case std::to_underlying(OpCode::RETURN):
            return Chunk::simple_instruction("OP_RETURN", offset);
        default:
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
    }
}

std::size_t Chunk::simple_instruction(const char* name, std::size_t offset) {
    printf("%s\n", name);
    return offset + 1;
}

std::size_t Chunk::constant_instruction(const char* name, Chunk& chunk, std::size_t offset) {
    uint8_t constant = chunk.get_code().at(offset + 1);
    printf("%-16s %4d '", name, constant);
    chunk.get_constants().at(constant).print();
    printf("'\n");
    return offset + 2;
}
