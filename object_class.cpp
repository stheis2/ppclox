#include "object_class.hpp"

ObjClass::~ObjClass() {
    // When we are being destructed, inform the garbage collector of the
    // approximate number of additional bytes being removed due to the fields.
//FIX - Is there a better way to get the approximate number of bytes used by the field table?
//See https://stackoverflow.com/questions/25375202/how-to-measure-the-memory-usage-of-stdunordered-map   
    Obj::subtract_bytes_allocated(m_methods.size() * (sizeof(ObjStringRef) + sizeof(Value)));   
}

std::optional<Value> ObjClass::get_method(ObjString* name) {
    auto it = m_methods.find(ObjStringRef(name));
    if (it != m_methods.end()) {
        return it->second;
    }
    return std::nullopt;
}

void ObjClass::set_method(ObjString* name, Value value) {
    if (m_methods.insert_or_assign(ObjStringRef(name), value).second) {
        // If insertion took place, inform the garbage collector of the
        // approximate number of additional bytes used by the instance
//FIX - Is there a better way to get the approximate number of bytes used by the field table?
//See https://stackoverflow.com/questions/25375202/how-to-measure-the-memory-usage-of-stdunordered-map   
        Obj::add_bytes_allocated(sizeof(ObjStringRef) + sizeof(Value));
    }
}

void ObjClass::mark_methods_gc_gray() {
    for (auto pair : m_methods) {
        Obj::mark_gc_gray(pair.first.obj_string());
        pair.second.mark_obj_gc_gray();
    }
}

ObjInstance::~ObjInstance() {
    // When we are being destructed, inform the garbage collector of the
    // approximate number of additional bytes being removed due to the fields.
//FIX - Is there a better way to get the approximate number of bytes used by the field table?
//See https://stackoverflow.com/questions/25375202/how-to-measure-the-memory-usage-of-stdunordered-map   
    Obj::subtract_bytes_allocated(m_fields.size() * (sizeof(ObjStringRef) + sizeof(Value)));   
}

std::optional<Value> ObjInstance::get_field(ObjString* name) {
    auto it = m_fields.find(ObjStringRef(name));
    if (it != m_fields.end()) {
        return it->second;
    }
    return std::nullopt;
}

void ObjInstance::set_field(ObjString* name, Value value) {
    if (m_fields.insert_or_assign(ObjStringRef(name), value).second) {
        // If insertion took place, inform the garbage collector of the
        // approximate number of additional bytes used by the instance
//FIX - Is there a better way to get the approximate number of bytes used by the field table?
//See https://stackoverflow.com/questions/25375202/how-to-measure-the-memory-usage-of-stdunordered-map   
        Obj::add_bytes_allocated(sizeof(ObjStringRef) + sizeof(Value));
    }
}

void ObjInstance::mark_fields_gc_gray() {
    for (auto pair : m_fields) {
        Obj::mark_gc_gray(pair.first.obj_string());
        pair.second.mark_obj_gc_gray();
    }
}