#ifndef ppclox_object_hpp
#define ppclox_object_hpp

#include <functional>
#include <unordered_map>
#include <string>
#include <string_view>
#include <memory>

#include "common.hpp"

enum class ObjType {
    STRING
};

class Obj {
public:
    ObjType type() const { return m_type; }

    virtual void print() const;

    //https://azrael.digipen.edu/~mmead/www/Courses/CS225/OverloadingNewDelete.html#:~:text=You%20cannot%20overload%20the%20new,compiler)%20and%20cannot%20be%20changed.
    static void* operator new(size_t size);
    static void operator delete(void *memory);

    /** Free all allocated objects */
    static void free_objects();

    /** Virtual destructor ensures that deleting through base pointer will call derived destructors */
    virtual ~Obj() {}

protected:
    Obj(ObjType type) : m_type(type) {}
private:
    ObjType m_type{};
    /** 
     * NOTE! Do NOT initialize this field. It gets set by the overloaded new operator.
     * If we initialize it here, it will get initialized AFTER the new operator has run
     * and so will override what was done in the new operator.
     * @todo Maybe utilize an external list like a vector instead of an intrusive linked list?
    */
    Obj* m_next;

    /** Pointer to head of linked list of objects*/
    static Obj* s_objects_head;

//TODO: Implement custom new and delete for objs? And how do we account for the ObjStrings
// memory?   
};

#endif