#ifndef ppclox_value_hpp
#define ppclox_value_hpp

#include "common.hpp"
#include "object.hpp"
#include "object_string.hpp"

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
    bool is_string() const { return is_obj_type(ObjType::STRING); }

    ObjString* as_string() const { return (ObjString*)as_obj(); }
    const char* as_cstring() const { return ((ObjString*)as_obj())->chars(); }

    /** Lox follows Ruby in that nil and false are falsey and every other value behaves like true. */
    bool is_falsey() const { return is_nil() || (is_bool() && !as_bool()); }
private:
    ValueType m_type{ValueType::NIL};
    union {
        bool boolean;
        double number;
        Obj* obj;
    } m_as{};
};

#endif
