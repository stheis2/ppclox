#include "value.hpp"
#include "object.hpp"

Value::Value(bool val) {
    m_type = ValueType::BOOL;
    m_as.boolean = val;
}

Value::Value() {
    m_type = ValueType::NIL;
}

Value::Value(double val) {
    m_type = ValueType::NUMBER;
    m_as.number = val;
}

Value::Value(Obj* obj) {
    m_type = ValueType::OBJ;
    m_as.obj = obj;
}

void Value::print() const {
    switch (m_type) {
        case ValueType::BOOL:
            printf(as_bool() ? "true" : "false");
            break;
        case ValueType::NIL:
            printf("nil");
            break;
        case ValueType::NUMBER:
            printf("%g", as_number());
            break;
        case ValueType::OBJ:
            as_obj()->print();
            break;
        default:
            printf("Unhandled ValueType when printing Value");
            break;
    }
}

bool Value::operator==(const Value& rhs) const {
    if (m_type != rhs.m_type) return false;
    switch (m_type) {
        case ValueType::BOOL: return as_bool() == rhs.as_bool();
        case ValueType::NIL: return true;
        case ValueType::NUMBER: return as_number() == rhs.as_number();
        // Objects are considered distinct if they point to 
        // different objects on the heap. Thanks to ObjString
        // de-duping/interning, this is equivalent to comparing
        // character by character for ObjStrings, but much faster.
        case ValueType::OBJ: return as_obj() == rhs.as_obj();
        default:
            // Should be unreachable, but just assume false
// TODO: Throw an exception instead?
            return false;
    }
}

void Value::mark_obj_gc_gray() {
    if (is_obj()) {
        Obj::mark_gc_gray(as_obj());
    }
}