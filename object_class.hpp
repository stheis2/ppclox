#ifndef ppclox_object_class_hpp
#define ppclox_object_class_hpp

#include "common.hpp"
#include "object.hpp"

class ObjString;

class ObjClass : public Obj {
public:
    ObjClass(ObjString* name) : Obj(ObjType::CLASS), m_name(name) {}

    void print() const override { printf("%s", m_name->chars()); }

    ObjString* name() { return m_name; }
private:
    ObjString* m_name{};
};

#endif
