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

void Value::print() const {
    //FIX - Need to handle non-number values
    printf("%g", as_number());
}