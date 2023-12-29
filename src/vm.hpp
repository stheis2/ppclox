#ifndef ppclox_vm_hpp
#define ppclox_vm_hpp

#include <memory>

#include "chunk.hpp"
#include "object_function.hpp"

#define VALUE_STACK_INIT_CAPACITY 256

class CallFrame {
public:
    ObjClosure* m_closure{};
    /** If non-null, this is a pointer into the chunk's code */
    /** @todo Is there any way to make this safer? Maybe using an iterator? */
    const std::uint8_t* m_ip{};
    /** Base index into the VM's value stack for this call frame's locals etc. */
    std::size_t m_value_stack_base_index{};

    CallFrame(ObjClosure* closure, std::size_t value_stack_base_index) : 
        m_closure(closure), 
        m_ip(closure->function()->chunk().get_code().data()), 
        m_value_stack_base_index(value_stack_base_index) {}

    std::size_t next_instruction_offset() {
        return m_ip - m_closure->function()->chunk().get_code().data();
    }

    /** Offset of currently executing instruction. Assumes at least 1 instruction has been read. */
    std::size_t current_instruction_offset() {
        return m_ip - m_closure->function()->chunk().get_code().data() - 1;
    }

    void disassemble_instruction() {
        m_closure->function()->chunk().disassemble_instruction(next_instruction_offset());
    }
};

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
    /** 
     * There should be a practical limit on the number of stack frames so as to
     * detect runaway recursion and avoid memory exhaustion.
    */
    static constexpr std::size_t k_max_call_frames = 1024;

    std::vector<CallFrame> m_call_stack{};
    std::vector<Value> m_stack{};
    std::unordered_map<ObjStringRef, Value, ObjStringRefHash> m_globals{};

    void reset_stack();
    void runtime_error(const char* format, ...);
    void define_native(const char* name, NativeFn function);
    void push(Value value);
    Value pop();
    Value peek(std::size_t distance);
    bool call_value(Value callee, std::size_t arg_count);
    bool call(ObjClosure* closure, std::size_t arg_count);
    ObjUpvalue* capture_upvalue(std::size_t stack_index);

    InterpretResult run();

    /** 
     * Get reference to current call frame.
     * NOTE! Caller must ensure that there IS a call frame to get!
    */
    CallFrame& current_frame() { return m_call_stack.back(); }
    /** Return the current byte pointed to, and increment the IP */
    std::uint8_t read_byte() { return *(current_frame().m_ip++); }
    /** Return the short pointed to, and increment the IP to after it */
    std::uint16_t read_short() {
        current_frame().m_ip += 2;
        // Read the HO byte, followed by the LO byte
        std::uint16_t ho_byte = current_frame().m_ip[-2];
        std::uint16_t lo_byte = current_frame().m_ip[-1];
        return (ho_byte << 8) | lo_byte;
    }
    /** 
     * Read/increment the current byte and assume it is an index into the chunk's 
     * constants array, returning the constant at that index. 
     * NOTE! We do not do any bounds checking here to ensure fast execution, so it's 
     * important that the compiled code produce correct, in-bound indexes.
     */
    Value read_constant() { return current_frame().m_closure->function()->chunk().get_constants()[this->read_byte()]; }
    ObjString* read_string() { return read_constant().as_string(); }
    bool verify_binary_op_types();
};

// Exposes the global g_vm variable from vm.cpp
extern VM g_vm;

#endif