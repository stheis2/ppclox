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

    switch (instruction) {
        case std::to_underlying(OpCode::CONSTANT):
            return constant_instruction("OP_CONSTANT", *this, offset);
        case std::to_underlying(OpCode::NIL):
            return simple_instruction("OP_NIL", offset);            
        case std::to_underlying(OpCode::TRUE):
            return simple_instruction("OP_TRUE", offset);            
        case std::to_underlying(OpCode::FALSE):
            return simple_instruction("OP_FALSE", offset);
        case std::to_underlying(OpCode::POP):
            return simple_instruction("OP_POP", offset);
        case std::to_underlying(OpCode::GET_LOCAL):
            return byte_instruction("OP_GET_LOCAL", *this, offset);
        case std::to_underlying(OpCode::SET_LOCAL):
            return byte_instruction("OP_SET_LOCAL", *this, offset);
        case std::to_underlying(OpCode::GET_GLOBAL):
            return constant_instruction("OP_GET_GLOBAL", *this, offset);
        case std::to_underlying(OpCode::DEFINE_GLOBAL):
            return constant_instruction("OP_DEFINE_GLOBAL", *this, offset);
        case std::to_underlying(OpCode::SET_GLOBAL):
            return constant_instruction("OP_SET_GLOBAL", *this, offset);
        case std::to_underlying(OpCode::EQUAL):
            return simple_instruction("OP_EQUAL", offset);
        case std::to_underlying(OpCode::GREATER):
            return simple_instruction("OP_GREATER", offset);
        case std::to_underlying(OpCode::LESS):
            return simple_instruction("OP_LESS", offset);       
        case std::to_underlying(OpCode::ADD):
            return simple_instruction("OP_ADD", offset);
        case std::to_underlying(OpCode::SUBTRACT):
            return simple_instruction("OP_SUBTRACT", offset);
        case std::to_underlying(OpCode::MULTIPLY):
            return simple_instruction("OP_MULTIPLY", offset);
        case std::to_underlying(OpCode::DIVIDE):
            return simple_instruction("OP_DIVIDE", offset);            
        case std::to_underlying(OpCode::NOT):
            return simple_instruction("OP_NOT", offset);
        case std::to_underlying(OpCode::NEGATE):
            return simple_instruction("OP_NEGATE", offset);
        case std::to_underlying(OpCode::PRINT):
            return simple_instruction("OP_PRINT", offset);
        case std::to_underlying(OpCode::RETURN):
            return simple_instruction("OP_RETURN", offset);
        default:
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
    }
}

std::size_t Chunk::simple_instruction(const char* name, std::size_t offset) {
    printf("%s\n", name);
    return offset + 1;
}

std::size_t Chunk::byte_instruction(const char* name, const Chunk& chunk, std::size_t offset) {
    std::uint8_t slot = chunk.get_code().at(offset + 1);
    printf("%-16s %4d\n", name, slot);
    return offset + 2;
}

std::size_t Chunk::constant_instruction(const char* name, const Chunk& chunk, std::size_t offset) {
    uint8_t constant = chunk.get_code().at(offset + 1);
    printf("%-16s %4d '", name, constant);
    chunk.get_constants().at(constant).print();
    printf("'\n");
    return offset + 2;
}
