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
        printf("%p object type %d\n", this, m_type);
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
     * NOTE! Do NOT initialize this field. It gets set by the overloaded new operator.
     * If we initialize it here, it will get initialized AFTER the new operator has run
     * and so will override what was done in the new operator.
     * @todo Maybe utilize an external list like a vector instead of an intrusive linked list?
    */
    Obj* m_next;

    /** Pointer to head of linked list of objects*/
    static Obj* s_objects_head;

    // Gray objects needing processing during garbage collection
    static std::vector<Obj*> s_gray_worklist;

    static void mark_gc_roots();

//TODO: Implement custom new and delete for objs? And how do we account for the ObjStrings
// memory?   
};

#endif