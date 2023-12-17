#include <memory>

#include "common.hpp"
#include "chunk.hpp"
#include "vm.hpp"

int main(int argc, const char* argv[]) {

    auto chunk_ptr = std::make_shared<Chunk>();

    std::size_t constant_index = chunk_ptr->add_constant(1.2);
    chunk_ptr->write(static_cast<std::uint8_t>(OpCode::CONSTANT), 123);
    chunk_ptr->write(constant_index, 123);

    constant_index = chunk_ptr->add_constant(3.4);
    chunk_ptr->write(static_cast<std::uint8_t>(OpCode::CONSTANT), 123);
    chunk_ptr->write(constant_index, 123);

    chunk_ptr->write(static_cast<std::uint8_t>(OpCode::SUBTRACT), 123);

    constant_index = chunk_ptr->add_constant(5.6);
    chunk_ptr->write(static_cast<std::uint8_t>(OpCode::CONSTANT), 123);
    chunk_ptr->write(constant_index, 123);

    chunk_ptr->write(static_cast<std::uint8_t>(OpCode::MULTIPLY), 123);

    chunk_ptr->write(static_cast<std::uint8_t>(OpCode::NEGATE), 123);

    chunk_ptr->write(static_cast<std::uint8_t>(OpCode::RETURN), 123);
    chunk_ptr->dissassemble("test chunk");
    std::cout << std::endl;
    g_vm.interpret(chunk_ptr);
    return 0;
}