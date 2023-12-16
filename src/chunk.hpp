#ifndef ppclox_chunk_hpp
#define ppclox_chunk_hpp

#include "common.hpp"

enum class OpCode {
    RETURN
};

class Chunk {
public:
    /** Append the byte to this chunk of bytecode */
    void write(std::uint8_t byte);
    void dissassemble(const char* name);

    inline const std::vector<std::uint8_t> get_code() const { return this->code; };
private:
    std::vector<std::uint8_t> code{};

    std::size_t disassembleInstruction(std::size_t offset);
    static std::size_t simpleInstruction(const char* name, std::size_t offset);
};

#endif
