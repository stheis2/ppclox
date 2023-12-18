#include "value.hpp"

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
        default:
            printf("Unhandled ValueType when printing Value");
            break;
    }
}