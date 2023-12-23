#ifndef ppclox_chunk_hpp
#define ppclox_chunk_hpp

#include "common.hpp"
#include "value.hpp"

enum class OpCode : std::uint8_t {
    CONSTANT,
    NIL,
    TRUE,
    FALSE,
    POP,
    EQUAL,
    GREATER,
    LESS,
    ADD,
    SUBTRACT,
    MULTIPLY,
    DIVIDE,
    NOT,
    NEGATE,
    PRINT,
    RETURN
};

class Chunk {
public:
    /** Append the byte to this chunk of bytecode, and provide the line number */
    void write(std::uint8_t byte, std::size_t line);
    /** Append the constant to this chunk's constant array, returning it's index */
    std::size_t add_constant(Value value);
    void dissassemble(const char* name);
    /** Disassemble the instruction at the given offset into the chunk's code vector */
    std::size_t disassemble_instruction(std::size_t offset);

    const std::vector<std::uint8_t>& get_code() { return m_code; };
    const std::vector<std::size_t>& get_lines() { return m_lines; };
    const std::vector<Value>& get_constants() { return m_constants; };
private:
    std::vector<std::uint8_t> m_code{};
    std::vector<std::size_t> m_lines{};
    std::vector<Value> m_constants{};
    
    static std::size_t simple_instruction(const char* name, std::size_t offset);
    static std::size_t constant_instruction(const char* name, Chunk& chunk, std::size_t offset);
};

#endif
