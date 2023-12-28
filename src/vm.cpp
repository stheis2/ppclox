#include <memory>
#include <cstdarg>
#include <ctime>

#include "common.hpp"
#include "compiler.hpp"
#include "vm.hpp"

// Global VM
// TODO: Refactor to make this not global somehow
VM g_vm;

// If we had more native functions, they would probably
// go into their own file, but we have just one.
static Value clock_native(std::size_t arg_count, NativeFnArgsIterator start, NativeFnArgsIterator end) {
    return Value((double)clock() / CLOCKS_PER_SEC);
}

VM::VM() {
    // Set our initial capacities
    reset_stack();

    // Define our native functions
    define_native("clock", clock_native);
}
VM::~VM() { }

InterpretResult VM::interpret(const char* source) {
    ObjFunction* function = Compiler::compile(source);
    if (function == nullptr) return InterpretResult::COMPILE_ERROR;

    // Set up our initial call frame
    push(function);
    call(function, 0);

    return run();
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

    for (auto frame_it = m_call_stack.rbegin(); frame_it != m_call_stack.rend(); ++frame_it) {
        // Get the instruction that was in the process of being executed
        size_t instruction = frame_it->current_instruction_offset();
        std::size_t line = frame_it->m_function->chunk().get_lines().at(instruction);
        fprintf(stderr, "[line %zd] in %s()\n", line, frame_it->m_function->name());
    }
    
    reset_stack();
}

void VM::define_native(const char* name, NativeFn function) {
    // Push objects after they are allocated to make sure the
    // GC won't collect them.
    ObjString* name_obj = ObjString::copy_string(name, strlen(name));
    push(name_obj);
    ObjNative* native = new ObjNative(function);
    push(native);

    auto it = m_globals.find(ObjStringRef(name_obj));
    if (it != m_globals.end()) {
        throw std::runtime_error("Native function with duplicate name.");
    }
    m_globals[ObjStringRef(name_obj)] = Value(native);

    // Clean up stack now that the fcn is safely inserted
    pop();
    pop();
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

bool VM::call_value(Value callee, std::size_t arg_count) {
    if (callee.is_obj()) {
        switch (callee.obj_type()) {
            case ObjType::FUNCTION:
                return call(callee.as_function(), arg_count);
            case ObjType::NATIVE: {
                NativeFn native = callee.as_native()->function();
                Value result = native(arg_count, m_stack.end() - arg_count, m_stack.end());
                // Clean up the value stack for this call.
                // NOTE! We must erase the args along with the native function that was pushed on the stack
                m_stack.erase(m_stack.end() - (arg_count + 1), m_stack.end());
                push(result);
                return true;
            }
            default:
                // Non-callable object type
                break;
        }
    }
    runtime_error("Can only call functions and classes.");
    return false;
}

bool VM::call(ObjFunction* function, std::size_t arg_count) {
    if (arg_count != function->m_arity) {
        runtime_error("Expected %zd arguments but got %zd.", function->m_arity, arg_count);
        return false;
    }

    if (m_call_stack.size() >= k_max_call_frames) {
        runtime_error("Call stack overflow.");
        return false;
    }

    // Push a new call frame onto the stack.
    // The base index for the new frame includes all the arguments, plus
    // the function Value that was being called (which is pushed before all arguments).
    std::size_t value_stack_base_index = m_stack.size() - arg_count - 1;
    m_call_stack.emplace_back(function, value_stack_base_index);
    return true;
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
        current_frame().disassemble_instruction();
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
                push(m_stack[current_frame().m_value_stack_base_index + slot]);
                break;
            }
            case std::to_underlying(OpCode::SET_LOCAL): {
                std::uint8_t slot = read_byte();
                // Store the top of the stack back into the local.
                // Since an assignment is an expression, we leave the
                // resulting value on the top of the stack.
                m_stack[current_frame().m_value_stack_base_index + slot] = peek(0);
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
                current_frame().m_ip += offset;
                break;
            }
            case std::to_underlying(OpCode::JUMP_IF_FALSE): {
                std::uint16_t offset = read_short();
                if (peek(0).is_falsey()) current_frame().m_ip += offset;
                break;
            }
            case std::to_underlying(OpCode::LOOP): {
                std::uint16_t offset = read_short();
                current_frame().m_ip -= offset;
                break;
            }
            case std::to_underlying(OpCode::CALL): {
                std::uint8_t arg_count = read_byte();
                if (!call_value(peek(arg_count), arg_count)) {
                    return InterpretResult::RUNTIME_ERROR;
                }
                // NOTE! If we were caching call frames some how instead
                //       of going through the m_call_stack vector,
                //       we would need to update that here.
                break;
            }
            case std::to_underlying(OpCode::RETURN): {
                // Pop the function return result from the stack.
                Value result = pop();

                // If this is the initial call frame...
                if (m_call_stack.size() == 1) {
                    // Clean up final frame
                    m_call_stack.pop_back();

                    // If there's more than one thing left on the value stack,
                    // something is very wrong.
                    if (m_stack.size() != 1) {
                        printf("Unexpected value stack size on program termination: %zd\n", m_stack.size());
                        reset_stack();
                        return InterpretResult::RUNTIME_ERROR;
                    }

                    // Pop the initial function from the value stack
                    pop();

                    // Call and value stacks should now be empty, and we're done.
                    return InterpretResult::OK;
                }

                // Clean up the value stack.
                // We need to erase all elements of the topmost callframe upwards.
                m_stack.erase(m_stack.begin() + current_frame().m_value_stack_base_index, m_stack.end());
                // Clean up call stack
                m_call_stack.pop_back();

                // Push function return result back on the value stack for the caller to find.
                push(result);

                // NOTE! If we were caching call frames some how instead
                //       of going through the m_call_stack vector,
                //       we would need to update that here.
                break;
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