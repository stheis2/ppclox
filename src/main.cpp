#include "common.hpp"
#include "chunk.hpp"

int main(int argc, const char* argv[]) {
    Chunk chunk{};
    chunk.write(static_cast<std::uint8_t>(OpCode::RETURN));
    chunk.dissassemble("test chunk");
    return 0;
}