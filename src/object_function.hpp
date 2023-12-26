#ifndef ppclox_object_function_hpp
#define ppclox_object_function_hpp

#include <memory>

//#include "chunk.hpp"
#include "object.hpp"
#include "object_string.hpp"

// We need to forward declare this due to circular dependencies
class Chunk;

enum class FunctionType {
    FUNCTION,
    SCRIPT
};

class ObjFunction : public Obj {
public:
    ObjFunction() : Obj(ObjType::FUNCTION), m_chunk(std::make_unique<Chunk>()) {}

    void print() const override;

    /** Return a mutable reference to the Chunk for writing, etc. */
    Chunk& chunk() { return *m_chunk; };
    const char* name() const { return m_name != nullptr ? m_name->chars() : "<script>"; };
private:
    std::size_t m_arity{};
    /** 
     * Why make this a unique pointer instead of directly living in the class?
     * Well it helps us to avoid circular dependencies.
     */
    std::unique_ptr<Chunk> m_chunk{};
    //Chunk m_chunk{};
    /** @todo Can we make this safer than a raw pointer somehow? */
    ObjString* m_name{};
};

#endif