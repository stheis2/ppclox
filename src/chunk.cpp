#include "chunk.hpp"

void Chunk::write(std::uint8_t byte, std::size_t line) {
    m_code.push_back(byte);
    m_lines.push_back(line);
}

void Chunk::patch_at(std::size_t offset, std::uint8_t byte) {
    m_code.at(offset) = byte;
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
        case std::to_underlying(OpCode::JUMP):
            return jump_instruction("OP_JUMP", true, *this, offset);            
        case std::to_underlying(OpCode::JUMP_IF_FALSE):
            return jump_instruction("OP_JUMP_IF_FALSE", true, *this, offset);
        case std::to_underlying(OpCode::LOOP):
            return jump_instruction("OP_LOOP", false, *this, offset);
        case std::to_underlying(OpCode::CALL):
            return byte_instruction("OP_CALL", *this, offset);
        case std::to_underlying(OpCode::CLOSURE):
            return closure_instruction("OP_CLOSURE", *this, offset);
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

std::size_t Chunk::jump_instruction(const char* name, bool is_forward, const Chunk& chunk, std::size_t offset) {
    // Extract HO byte, then LO byte
    std::uint16_t ho_byte = chunk.get_code().at(offset + 1);
    std::uint16_t lo_byte = chunk.get_code().at(offset + 2);
    std::uint16_t jump = (ho_byte << 8) | lo_byte;

    // Calculate target
    std::size_t next_offset = offset + 3;
    std::size_t target = is_forward ? (next_offset + jump) : (next_offset - jump);
    printf("%-16s %4zd -> %zd\n", name, offset, target);

    return offset + 3;
}

std::size_t Chunk::constant_instruction(const char* name, const Chunk& chunk, std::size_t offset) {
    uint8_t constant = chunk.get_code().at(offset + 1);
    printf("%-16s %4d '", name, constant);
    chunk.get_constants().at(constant).print();
    printf("'\n");
    return offset + 2;
}

std::size_t Chunk::closure_instruction(const char* name, const Chunk& chunk, std::size_t offset) {
    offset++;
    std::uint8_t constant = chunk.get_code().at(offset++);
    printf("%-16s %4d ", name, constant);
    chunk.get_constants().at(constant).print();
    printf("\n");
    return offset;
}