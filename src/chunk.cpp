#include "chunk.hpp"

void Chunk::write(std::uint8_t byte) {
    this->code.push_back(byte);
}

void Chunk::dissassemble(const char* name) {
    printf("== %s ==\n", name);

    for (std::size_t offset = 0; offset < this->code.size();) {
        offset = this->disassembleInstruction(offset);
    }
}

std::size_t Chunk::disassembleInstruction(std::size_t offset) {
    printf("%04d ", offset);

    std::uint8_t instruction = this->code.at(offset);

    // Static cast is safe since we have a default case
    switch (static_cast<OpCode>(instruction)) {
        case OpCode::RETURN:
            return Chunk::simpleInstruction("OP_RETURN", offset);
        default:
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
    }
}

std::size_t Chunk::simpleInstruction(const char* name, std::size_t offset) {
    printf("%s\n", name);
    return offset + 1;
}
