#ifndef ppclox_chunk_hpp
#define ppclox_chunk_hpp

#include "common.hpp"
#include "value.hpp"

enum class OpCode {
    CONSTANT,
    RETURN
};

class Chunk {
public:
    /** Append the byte to this chunk of bytecode, and provide the line number */
    void write(std::uint8_t byte, std::size_t line);
    /** Append the constant to this chunk's constant array, returning it's index */
    std::size_t add_constant(Value value);
    void dissassemble(const char* name);

    inline const std::vector<std::uint8_t> get_code() const { return m_code; };
    inline const std::vector<std::size_t> get_lines() const { return m_lines; };
    inline const std::vector<Value> get_constants() const { return m_constants; };
private:
    std::vector<std::uint8_t> m_code{};
    std::vector<std::size_t> m_lines{};
    std::vector<Value> m_constants{};

    std::size_t disassemble_instruction(std::size_t offset);
    static std::size_t simple_instruction(const char* name, std::size_t offset);
    static std::size_t constant_instruction(const char* name, const Chunk& chunk, std::size_t offset);
};

#endif
