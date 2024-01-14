#ifndef ppclox_object_class_hpp
#define ppclox_object_class_hpp

#include <unordered_map>
#include <optional>

#include "common.hpp"
#include "value.hpp"
#include "object.hpp"
#include "object_string.hpp"

class ObjClass : public Obj {
public:
    ObjClass(ObjString* name) : Obj(ObjType::CLASS), m_name(name) {}
    ~ObjClass();

    void print() const override { printf("%s class", m_name->chars()); }

    ObjString* name() { return m_name; }
    std::optional<Value> get_method(ObjString* name);
    void set_method(ObjString* name, Value value);
    void mark_methods_gc_gray();
    // Inherit all methods from the given superclass
    void inherit_methods_from(ObjClass* superclass);
private:
    ObjString* m_name{};
//TODO: I think these Values are always ObjClosures, so we could store them
//      as ObjClosure* directly potentially.
    std::unordered_map<ObjStringRef, Value, ObjStringRefHash> m_methods{};
};

class ObjInstance : public Obj {
public:
    ObjInstance(ObjClass* klass) : Obj(ObjType::INSTANCE), m_class(klass) {}
     ~ObjInstance();

    void print() const override { printf("%s instance", m_class->name()->chars()); }

    ObjClass* get_class() { return m_class; }
    std::optional<Value> get_field(ObjString* name);
    void set_field(ObjString* name, Value value);
    void mark_fields_gc_gray();
private:
    ObjClass* m_class{};
    std::unordered_map<ObjStringRef, Value, ObjStringRefHash> m_fields{};
};

#endif
