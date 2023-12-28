#ifndef ppclox_object_function_hpp
#define ppclox_object_function_hpp

#include <memory>

#include "object.hpp"
#include "object_string.hpp"

// We need to forward declare this due to circular dependencies
class Chunk;
class Value;

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

class ObjClosure : public Obj {
public:
    ObjClosure(ObjFunction* function) : Obj(ObjType::CLOSURE), m_function(function) {}
    void print() const override { m_function->print(); }
    ObjFunction* function() { return m_function; }
private:
    ObjFunction* m_function{};
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