#ifndef ppclox_object_function_hpp
#define ppclox_object_function_hpp

#include <memory>
#include <optional>

#include "object.hpp"
#include "object_string.hpp"
#include "value.hpp"

// We forward declare these instead of including their headers to avoid circular
// dependencies.
class Chunk;

enum class FunctionType {
    FUNCTION,
    SCRIPT
};

class ObjFunction : public Obj {
public:
    /** 
     * Why do we pass in a shared pointer instead of directly living in the class?
     * Well it helps us to avoid circular dependencies. I tried it with unique_ptr
     * and that didn't work. Probably should reconsider the design to avoid these
     * circular dependencies, but I don't want to deviate from the Clox architecture
     * too much until after its been fully ported.
     */
    ObjFunction(std::shared_ptr<Chunk> chunk, ObjString* name) : Obj(ObjType::FUNCTION), m_chunk(chunk), m_name(name) {}

    void print() const override;

    /** Return a mutable reference to the Chunk for writing, etc. */
    Chunk& chunk() { return *m_chunk; };
    std::size_t m_arity{};
    std::size_t m_upvalue_count{};
    const char* name() const { return m_name != nullptr ? m_name->chars() : "<script>"; };
private:
    std::shared_ptr<Chunk> m_chunk{};
    /** @todo Can we make this safer than a raw pointer somehow? */
    ObjString* m_name{};
};

class ObjUpvalue : public Obj {
public:
    ObjUpvalue(std::size_t stack_index) : Obj(ObjType::UPVALUE), m_value_stack_index(stack_index) {}

    // Printing isn’t useful to end users. Upvalues are objects only so that we can take 
    // advantage of the VM’s memory management. They aren’t first-class values that a 
    // Lox user can directly access in a program. So this code will never actually execute
    void print() const override { printf("upvalue"); }

    /** Returns if upvalue contains index into value stack. If not, use closed_value. */
    bool is_stack_index() { return m_value_stack_index.has_value(); }
    /** Only use if is_stack_index() reports true */
    std::size_t stack_index() { return m_value_stack_index.value(); }
    Value& closed_value() { return m_value; }
private:
    std::optional<std::size_t> m_value_stack_index{};
    Value m_value{};
};

class ObjClosure : public Obj {
public:
    // m_upvalues is initialized with function->m_upvalue_count null pointers
    ObjClosure(ObjFunction* function) : Obj(ObjType::CLOSURE), m_function(function), m_upvalues(function->m_upvalue_count) {}
    void print() const override { m_function->print(); }
    ObjFunction* function() { return m_function; }
    std::vector<ObjUpvalue*>& upvalues() { return m_upvalues; }
private:
    ObjFunction* m_function{};
    std::vector<ObjUpvalue*> m_upvalues{};
};

typedef std::vector<Value>::iterator NativeFnArgsIterator;
typedef Value (*NativeFn)(std::size_t arg_count, NativeFnArgsIterator args_start, NativeFnArgsIterator args_end);

class ObjNative : public Obj {
public:
    ObjNative(NativeFn function) : Obj(ObjType::NATIVE), m_function(function) {}
    void print() const override { printf("<native fn>"); }
    NativeFn function() { return m_function; }
private:
    NativeFn m_function{};
};

#endif