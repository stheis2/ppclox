#ifndef ppclox_value_hpp
#define ppclox_value_hpp

#include "common.hpp"
#include "object.hpp"
#include "object_string.hpp"

// We forward declare these instead of including their headers to avoid circular
// dependencies.
class ObjBoundMethod;
class ObjClass;
class ObjInstance;
class ObjFunction;
class ObjClosure;
class ObjNative;

enum class ValueType {
    BOOL,
    NIL,
    NUMBER,
    OBJ
};

class Value {
public:
    ValueType type() { return m_type; }

    Value(bool val);
    /** Construct NIL Value */
    Value();
    Value(double val);
    Value(Obj* obj);

    void print() const;
    bool operator==(const Value& rhs) const;

    bool as_bool() const { return m_as.boolean; }
    double as_number() const { return m_as.number; }
    Obj* as_obj() const { return m_as.obj; }

    bool is_bool() const { return m_type == ValueType::BOOL; }
    bool is_nil() const { return m_type == ValueType::NIL; }
    bool is_number() const { return m_type == ValueType::NUMBER; }
    bool is_obj() const { return m_type == ValueType::OBJ; }

    ObjType obj_type() const { return as_obj()->type(); }
    bool is_obj_type(ObjType type) const { return is_obj() && as_obj()->type() == type; }
    bool is_bound_method() const { return is_obj_type(ObjType::BOUND_METHOD); }
    bool is_string() const { return is_obj_type(ObjType::STRING); }
    bool is_class() const { return is_obj_type(ObjType::CLASS); }
    bool is_closure() const { return is_obj_type(ObjType::CLOSURE); }
    bool is_function() const { return is_obj_type(ObjType::FUNCTION); }
    bool is_instance() const { return is_obj_type(ObjType::INSTANCE); }
    bool is_native() const { return is_obj_type(ObjType::NATIVE); }

    ObjBoundMethod* as_bound_method() const { return (ObjBoundMethod*)as_obj(); }
    ObjClass* as_class() const { return (ObjClass*)as_obj(); }
    ObjClosure* as_closure() const { return (ObjClosure*)as_obj(); }
    ObjFunction* as_function() const { return (ObjFunction*)as_obj(); }
    ObjInstance* as_instance() const { return (ObjInstance*)as_obj(); }
    ObjNative* as_native() const { return (ObjNative*)as_obj(); }
    ObjString* as_string() const { return (ObjString*)as_obj(); }
    const char* as_cstring() const { return ((ObjString*)as_obj())->chars(); }

    /** Lox follows Ruby in that nil and false are falsey and every other value behaves like true. */
    bool is_falsey() const { return is_nil() || (is_bool() && !as_bool()); }

    // If type is Obj, mark the value as gray for GC
    void mark_obj_gc_gray();
private:
    ValueType m_type{ValueType::NIL};
    union {
        bool boolean;
        double number;
        Obj* obj;
    } m_as{};
};

#endif
