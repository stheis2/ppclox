#include <memory>

#include "common.hpp"
#include "compiler.hpp"
#include "vm.hpp"

// Global VM
// TODO: Refactor to make this not global somehow
VM g_vm;

VM::VM() {
    // Set our initial capacities
    reset_stack();
}
VM::~VM() { }

InterpretResult VM::interpret(const char* source) {
    auto chunk_ptr = std::make_shared<Chunk>();

    if (!Compiler::compile(source, chunk_ptr)) {
        return InterpretResult::COMPILE_ERROR;
    }

    // If everything successfully compiles, assign this
    // as our chunk, and start executing it.
    m_chunk = chunk_ptr;
    m_ip = m_chunk->get_code().data();
    InterpretResult result = run();
    // Now that we are done with the chunk, reset our state to remove our reference to it
    m_chunk = nullptr;
    m_ip = nullptr;
    return result;
}

void VM::reset_stack() {
    m_stack.clear(); 
    m_stack.reserve(VALUE_STACK_INIT_CAPACITY);
}

void VM::push(Value value) {
    m_stack.push_back(value);
}

Value VM::pop() {
    Value val = m_stack.back();
    m_stack.pop_back();
    return val;
}

InterpretResult VM::run() {
    for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
        for (auto value : m_stack) {
            printf("[ ");
            printValue(value);
            printf(" ]");
        }
        printf("\n");
        m_chunk->disassemble_instruction((m_ip - m_chunk->get_code().data()));
#endif
        uint8_t instruction = read_byte();

        switch (instruction) {
            case std::to_underlying(OpCode::CONSTANT): {
                Value constant = read_constant();
                push(constant);
                break;
            }
            case std::to_underlying(OpCode::ADD): {
                Value b = pop();
                Value a = pop();
                push(a + b);
                break;
            }
            case std::to_underlying(OpCode::SUBTRACT): {
                Value b = pop();
                Value a = pop();
                push(a - b);
                break;
            }
            case std::to_underlying(OpCode::MULTIPLY): {
                Value b = pop();
                Value a = pop();
                push(a * b);
                break;
            }
            case std::to_underlying(OpCode::DIVIDE): {
                Value b = pop();
                Value a = pop();
                push(a / b);
                break;
            }
            case std::to_underlying(OpCode::NEGATE): {
                push(-pop());
                break;
            }
            case std::to_underlying(OpCode::RETURN): {
                printValue(pop());
                printf("\n");
                return InterpretResult::OK;
            }
            default:
                printf("Instruction not recognized: %d\n", instruction);
                return InterpretResult::RUNTIME_ERROR;
        }
    }
}