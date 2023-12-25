#include <memory>
#include <cstdarg>

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

void VM::runtime_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    // Get the instruction that was in the process of being executed
    size_t instruction = m_ip - m_chunk->get_code().data() - 1;
    int line = m_chunk->get_lines()[instruction];
    fprintf(stderr, "[line %d] in script\n", line);
    reset_stack();
}

void VM::push(Value value) {
    m_stack.push_back(value);
}

Value VM::pop() {
    // TODO: Add bounds checking in debug builds?
    Value val = m_stack.back();
    m_stack.pop_back();
    return val;
}

Value VM::peek(std::size_t distance) {
    // TODO: Add bounds checking in debug builds?
    return m_stack[m_stack.size() - 1 - distance];
}

InterpretResult VM::run() {
    for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
        for (auto value : m_stack) {
            printf("[ ");
            value.print();
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
            case std::to_underlying(OpCode::NIL): push(Value()); break;
            case std::to_underlying(OpCode::TRUE): push(Value(true)); break;
            case std::to_underlying(OpCode::FALSE): push(Value(false)); break;
            case std::to_underlying(OpCode::POP): pop(); break;
            case std::to_underlying(OpCode::GET_LOCAL): {
                std::uint8_t slot = read_byte();
                // Push copy of the local to the top of the stack where
                // other instructions will be able to find it.
                // We're not a register based VM, so we must use the stack.
                push(m_stack[slot]);
                break;
            }
            case std::to_underlying(OpCode::SET_LOCAL): {
                std::uint8_t slot = read_byte();
                // Store the top of the stack back into the local.
                // Since an assignment is an expression, we leave the
                // resulting value on the top of the stack.
                m_stack[slot] = peek(0);
                break;
            }
            case std::to_underlying(OpCode::GET_GLOBAL): {
                ObjString* name = read_string();
                auto it = m_globals.find(ObjStringRef(name));
                if (it == m_globals.end()) {
                    runtime_error("Undefined variable '%s'.", name->chars());
                    return InterpretResult::RUNTIME_ERROR;
                }
                push(it->second);
                break;
            }
            case std::to_underlying(OpCode::DEFINE_GLOBAL): {
                ObjString* name = read_string();
                // NOTE! Peek/pop was done in the C implementation
                // out of worry that GC might be triggered
                // by the hash table insert. That shouldn't
                // be an issue here since resizing of the
                // globals hash table is independent of our GC.
                m_globals[ObjStringRef(name)] = pop();
                break;
            }
            case std::to_underlying(OpCode::SET_GLOBAL): {
                ObjString* name = read_string();
                auto it = m_globals.find(ObjStringRef(name));
                if (it == m_globals.end()) {
                    runtime_error("Undefined variable '%s'.", name->chars());
                    return InterpretResult::RUNTIME_ERROR;
                }
                it->second = peek(0);
                break;
            }
            case std::to_underlying(OpCode::EQUAL): {
                Value b = pop();
                Value a = pop();
                push(a == b);
                break;
            }
            case std::to_underlying(OpCode::GREATER): {
                if (!verify_binary_op_types()) {
                    return InterpretResult::RUNTIME_ERROR;
                }
                double b = pop().as_number();
                double a = pop().as_number();
                push(a > b);
                break;
            }
            case std::to_underlying(OpCode::LESS): {
                if (!verify_binary_op_types()) {
                    return InterpretResult::RUNTIME_ERROR;
                }
                double b = pop().as_number();
                double a = pop().as_number();
                push(a < b);
                break;
            }
            case std::to_underlying(OpCode::ADD): {
                // Handle String concatenation
                if (peek(0).is_string() && peek(1).is_string()) {
                    Value b = pop();
                    Value a = pop();
                    ObjString* result = *a.as_string() + *b.as_string();
                    push(result);
                    break;
                }

                if (!verify_binary_op_types()) {
                    return InterpretResult::RUNTIME_ERROR;
                }
                double b = pop().as_number();
                double a = pop().as_number();
                push(a + b);
                break;
            }
            case std::to_underlying(OpCode::SUBTRACT): {
                if (!verify_binary_op_types()) {
                    return InterpretResult::RUNTIME_ERROR;
                }
                double b = pop().as_number();
                double a = pop().as_number();
                push(a - b);
                break;
            }
            case std::to_underlying(OpCode::MULTIPLY): {
                if (!verify_binary_op_types()) {
                    return InterpretResult::RUNTIME_ERROR;
                }
                double b = pop().as_number();
                double a = pop().as_number();
                push(a * b);
                break;
            }
            case std::to_underlying(OpCode::DIVIDE): {
                if (!verify_binary_op_types()) {
                    return InterpretResult::RUNTIME_ERROR;
                }
                double b = pop().as_number();
                double a = pop().as_number();
                push(a / b);
                break;
            }
            case std::to_underlying(OpCode::NOT): {
                push(pop().is_falsey());
                break;
            }
            case std::to_underlying(OpCode::NEGATE): {
                if (!peek(0).is_number()) {
                    runtime_error("Operand must be a number.");
                    return InterpretResult::RUNTIME_ERROR;
                }
                push(-pop().as_number());
                break;
            }
            case std::to_underlying(OpCode::PRINT): {
                pop().print();
                printf("\n");
                break;
            }
            case std::to_underlying(OpCode::JUMP): {
                std::uint16_t offset = read_short();
                m_ip += offset;
                break;
            }
            case std::to_underlying(OpCode::JUMP_IF_FALSE): {
                std::uint16_t offset = read_short();
                if (peek(0).is_falsey()) m_ip += offset;
                break;
            }
            case std::to_underlying(OpCode::LOOP): {
                std::uint16_t offset = read_short();
                m_ip -= offset;
                break;
            }
            case std::to_underlying(OpCode::RETURN): {
                // Exit interpreter
                return InterpretResult::OK;
            }
            default:
                printf("Instruction not recognized: %d\n", instruction);
                return InterpretResult::RUNTIME_ERROR;
        }
    }
}

bool VM::verify_binary_op_types() {
    if (!peek(0).is_number() || !peek(1).is_number()) {
        runtime_error("Operands must be numbers.");
        return false;
    }
    return true;
}