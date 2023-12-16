#include "common.hpp"
#include "chunk.hpp"

int main(int argc, const char* argv[]) {
    Chunk chunk{};

    std::size_t constant_index = chunk.add_constant(1.2);
    chunk.write(static_cast<std::uint8_t>(OpCode::CONSTANT), 123);
    chunk.write(constant_index, 123);

    chunk.write(static_cast<std::uint8_t>(OpCode::RETURN), 123);
    chunk.dissassemble("test chunk");
    return 0;
}