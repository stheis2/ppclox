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
class ObjInstance;

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
    ObjString* name_obj() { return m_name; }
private:
    // NOTE! Although Chunks and their constituent parts do take up memory,
    //       we don't worry about including them in GC memory pressure analysis.
    //       Any objects actually included in the chunk's contants will be included as expected.
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

    void close(const std::vector<Value>& stack) { m_value = stack[m_value_stack_index.value()]; m_value_stack_index = std::nullopt; }

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
    ObjClosure(ObjFunction* function) : Obj(ObjType::CLOSURE), m_function(function), m_upvalues(function->m_upvalue_count) {
        // Although small, the upvalues vector itself does add some additional memory we should account for.
        // Since we don't modify the size of the vector after construction, this *should* remain constant.
        Obj::add_bytes_allocated(upvalues_vector_bytes());

    }
    ~ObjClosure() {
        Obj::subtract_bytes_allocated(upvalues_vector_bytes());
    }
    void print() const override { m_function->print(); }
    ObjFunction* function() { return m_function; }
    std::vector<ObjUpvalue*>& upvalues() { return m_upvalues; }
    std::size_t upvalues_vector_bytes() { return m_upvalues.capacity() * sizeof(ObjUpvalue*); }
private:
    ObjFunction* m_function{};
    std::vector<ObjUpvalue*> m_upvalues{};
};

class ObjBoundMethod : public Obj {
public:
    ObjBoundMethod(ObjInstance* receiver, ObjClosure* method) : 
        Obj(ObjType::BOUND_METHOD), m_receiver(receiver), m_method(method) {}

    // A bound method prints exactly the same way as a function. From the user’s perspective, 
    // a bound method is a function. It’s an object they can call. We don’t expose that the VM 
    // implements bound methods using a different object type.
    void print() const override { m_method->function()->print(); }

    ObjInstance* receiver() { return m_receiver; }
    ObjClosure* method() { return m_method; }
private:
    // Clox types this as Value, but its always an ObjInstance.
    // Let's type it that way and see how painful it is for now.
    ObjInstance* m_receiver{};
    ObjClosure* m_method{};
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