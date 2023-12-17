#include "common.hpp"
#include "vm.hpp"

// Global VM
// TODO: Refactor to make this not global
VM g_vm;

VM::VM() {
    // Set our initial capacities
    this->reset_stack();
}
VM::~VM() { }

InterpretResult VM::interpret(std::shared_ptr<Chunk>& chunk) {
    m_chunk = chunk;
    m_ip = m_chunk->get_code().data();
    InterpretResult result = this->run();
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
        uint8_t instruction = this->read_byte();

        // Static cast should be "safe" since we have a default clause
        switch (static_cast<OpCode>(instruction)) {
            case OpCode::CONSTANT: {
                Value constant = this->read_constant();
                this->push(constant);
                break;
            }
            case OpCode::ADD: {
                Value b = pop();
                Value a = pop();
                push(a + b);
                break;
            }
            case OpCode::SUBTRACT: {
                Value b = pop();
                Value a = pop();
                push(a - b);
                break;
            }
            case OpCode::MULTIPLY: {
                Value b = pop();
                Value a = pop();
                push(a * b);
                break;
            }
            case OpCode::DIVIDE: {
                Value b = pop();
                Value a = pop();
                push(a / b);
                break;
            }
            case OpCode::NEGATE: {
                push(-pop());
                break;
            }
            case OpCode::RETURN: {
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