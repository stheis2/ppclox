#ifndef ppclox_vm_hpp
#define ppclox_vm_hpp

#include <memory>

#include "chunk.hpp"

#define VALUE_STACK_INIT_CAPACITY 256

enum class InterpretResult {
    OK,
    COMPILE_ERROR,
    RUNTIME_ERROR
};

class VM {
public:
    VM();
    ~VM();

    InterpretResult interpret(const char* source);
private:
    std::shared_ptr<Chunk> m_chunk{};
    /** If non-null, this is a pointer into the chunk's code */
    /** @todo Is there any way to make this safer? Maybe using an iterator? */
    const std::uint8_t* m_ip{};
    std::vector<Value> m_stack{};
    std::unordered_map<ObjStringRef, Value, ObjStringRefHash> m_globals{};

    void reset_stack();
    void runtime_error(const char* format, ...);
    void push(Value value);
    Value pop();
    Value peek(std::size_t distance);

    InterpretResult run();

    /** Return the current byte pointed to, and increment the IP */
    std::uint8_t read_byte() { return *(m_ip++); }
    /** Return the short pointed to, and increment the IP to after it */
    std::uint16_t read_short() {
        m_ip += 2;
        // Read the HO byte, followed by the LO byte
        std::uint16_t ho_byte = m_ip[-2];
        std::uint16_t lo_byte = m_ip[-1];
        return (ho_byte << 8) | lo_byte;
    }
    /** 
     * Read/increment the current byte and assume it is an index into the chunk's 
     * constants array, returning the constant at that index. 
     * NOTE! We do not do any bounds checking here to ensure fast execution, so it's 
     * important that the compiled code produce correct, in-bound indexes.
     */
    Value read_constant() { return m_chunk->get_constants()[this->read_byte()]; }
    ObjString* read_string() { return read_constant().as_string(); }
    bool verify_binary_op_types();
};

// Exposes the global g_vm variable from vm.cpp
extern VM g_vm;

#endif