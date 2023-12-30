#ifndef ppclox_object_hpp
#define ppclox_object_hpp

#include <functional>
#include <unordered_map>
#include <string>
#include <string_view>
#include <memory>

#include "common.hpp"

enum class ObjType {
    CLOSURE,
    FUNCTION,
    NATIVE,
    STRING,
    UPVALUE
};

enum class ObjGcColor {
    // White color means we have not reached or processed the object at all.
    // When GC is done, white objects are the unreachable ones.
    WHITE,
    // Gray color means we know the object itself is reachable and should not be collected. 
    // But we have not yet finished tracing through it to see what other objects it references.
    GRAY,
    // When we take a gray object and mark all of the objects it references, we then turn 
    // the gray object black. This color means the mark phase is done processing that object.
    BLACK
};

class Obj {
public:
    ObjType type() const { return m_type; }

    // Mark gray for the purposes of garbage collection
    static void mark_gc_gray(Obj* obj);

    virtual void print() const;

    //https://azrael.digipen.edu/~mmead/www/Courses/CS225/OverloadingNewDelete.html#:~:text=You%20cannot%20overload%20the%20new,compiler)%20and%20cannot%20be%20changed.
    static void* operator new(size_t size);
    static void operator delete(void *memory);

    /** Collect all unreachable objects */
    static void collect_garbage();

    /** Free all allocated objects */
    static void free_objects();

    /** Virtual destructor ensures that deleting through base pointer will call derived destructors */
    virtual ~Obj() {
#ifdef DEBUG_LOG_GC
        printf("%p object type %d. Color: %d\n", this, m_type, m_gc_color);
#endif            
    }

protected:
    Obj(ObjType type) : m_type(type), m_gc_color(ObjGcColor::WHITE) {
#ifdef DEBUG_LOG_GC
        printf("%p object type %d\n", this, m_type);
#endif      
    }
private:
    ObjType m_type{};
    ObjGcColor m_gc_color{};

    /** 
     * Master list of all allocated objects
     * NOTE! Vector might not be the best choice for this. Ideally would want a collection
     *       optimized for these access patterns, so this would need to be investigated. 
     *       Clox uses an intrusive linked list but I'd prefer to avoid manual linked lists 
     *       in favor of a C++ collection of some sort.
     */
    static std::vector<Obj*> s_all_objects;

    // Gray objects needing processing during garbage collection
    static std::vector<Obj*> s_gray_worklist;

    static void mark_gc_roots();
    static void trace_gc_references();
    static void sweep();

    /** Blacken a gray object by first graying its references, then mark it as black */
    void blacken();
    /** Reset object color to white */
    void whiten() { m_gc_color = ObjGcColor::WHITE; };

//TODO: Implement custom new and delete for objs? And how do we account for the ObjStrings
// memory?   
};

#endif