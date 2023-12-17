#ifndef ppclox_vm_hpp
#define ppclox_vm_hpp

#include <memory>

#include "chunk.hpp"

enum class InterpretResult {
    OK,
    COMPILE_ERROR,
    RUNTIME_ERROR
};

class VM {
public:
    VM();
    ~VM();

    InterpretResult interpret(std::shared_ptr<Chunk>& chunk);
private:
    std::shared_ptr<Chunk> m_chunk{};
    /** If non-null, this is a pointer into the chunk's code */
    /** @todo Is there any way to make this safer? Maybe not. */
    const std::uint8_t* m_ip{};

    InterpretResult run();

    /** Return the current byte pointed to, and increment the IP */
    std::uint8_t read_byte() { return *(m_ip++); }
    /** 
     * Read/increment the current byte and assume it is an index into the chunk's 
     * constants array, returning the constant at that index. 
     * NOTE! We do not do any bounds checking here to ensure fast execution, so it's 
     * important that the compiled code produce correct, in-bound indexes.
     */
    Value read_constant() { return m_chunk->get_constants()[this->read_byte()]; }
};

// Exposes the global g_vm variable from vm.cpp
extern VM g_vm;

#endif