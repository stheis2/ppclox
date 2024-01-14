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
    GET_LOCAL,
    SET_LOCAL,
    GET_GLOBAL,
    DEFINE_GLOBAL,
    SET_GLOBAL,
    GET_UPVALUE,
    SET_UPVALUE,
    GET_PROPERTY,
    SET_PROPERTY,
    GET_SUPER,
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
    JUMP,
    JUMP_IF_FALSE,
    LOOP,
    CALL,
    INVOKE,
    SUPER_INVOKE,
    CLOSURE,
    CLOSE_UPVALUE,
    RETURN,
    CLASS,
    INHERIT,
    METHOD
};

class Chunk {
public:
    /** Append the byte to this chunk of bytecode, and provide the line number */
    void write(std::uint8_t byte, std::size_t line);
    /** Patch the byte at the given offset, assumed to have been previously written to */
    void patch_at(std::size_t offset, std::uint8_t byte);
    /** Append the constant to this chunk's constant array, returning it's index */
    std::size_t add_constant(Value value);
    void dissassemble(const char* name);
    /** Disassemble the instruction at the given offset into the chunk's code vector */
    std::size_t disassemble_instruction(std::size_t offset);

    const std::vector<std::uint8_t>& get_code() const { return m_code; };
    const std::vector<std::size_t>& get_lines() const { return m_lines; };
    const std::vector<Value>& get_constants() const { return m_constants; };
private:
    std::vector<std::uint8_t> m_code{};
    std::vector<std::size_t> m_lines{};
    std::vector<Value> m_constants{};
    
    static std::size_t simple_instruction(const char* name, std::size_t offset);
    static std::size_t byte_instruction(const char* name, const Chunk& chunk, std::size_t offset);
    static std::size_t jump_instruction(const char* name, bool is_forward, const Chunk& chunk, std::size_t offset);
    static std::size_t constant_instruction(const char* name, const Chunk& chunk, std::size_t offset);
    static std::size_t closure_instruction(const char* name, const Chunk& chunk, std::size_t offset);
    static std::size_t invoke_instruction(const char* name, const Chunk& chunk, std::size_t offset);
};

#endif
