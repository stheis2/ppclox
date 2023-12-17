#include "common.hpp"
#include "vm.hpp"

// Global VM
// TODO: Refactor to make this not global
VM g_vm;

VM::VM() { }
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

InterpretResult VM::run() {
    for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
        m_chunk->disassemble_instruction((m_ip - m_chunk->get_code().data()));
#endif
        uint8_t instruction = this->read_byte();

        // Static cast should be "safe" since we have a default clause
        switch (static_cast<OpCode>(instruction)) {
            case OpCode::CONSTANT: {
                Value constant = this->read_constant();
                printValue(constant);
                printf("\n");
                break;
            }
            case OpCode::RETURN: {
                return InterpretResult::OK;
            }
            default:
                printf("Instruction not recognized: %d\n", instruction);
                return InterpretResult::RUNTIME_ERROR;
        }
    }
}