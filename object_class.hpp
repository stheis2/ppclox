#ifndef ppclox_object_class_hpp
#define ppclox_object_class_hpp

#include <unordered_map>

#include "common.hpp"
#include "value.hpp"
#include "object.hpp"
#include "object_string.hpp"

//class ObjString;

class ObjClass : public Obj {
public:
    ObjClass(ObjString* name) : Obj(ObjType::CLASS), m_name(name) {}

    void print() const override { printf("%s class", m_name->chars()); }

    ObjString* name() { return m_name; }
private:
    ObjString* m_name{};
};

class ObjInstance : public Obj {
public:
    ObjInstance(ObjClass* klass) : Obj(ObjType::INSTANCE), m_class(klass) {}

    void print() const override { printf("%s instance", m_class->name()->chars()); }

    ObjClass* get_class() { return m_class; }
    std::unordered_map<ObjStringRef, Value, ObjStringRefHash>& fields() { return m_fields; }
private:
    ObjClass* m_class{};
    std::unordered_map<ObjStringRef, Value, ObjStringRefHash> m_fields{};
};

#endif
