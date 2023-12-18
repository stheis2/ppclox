#ifndef ppclox_value_hpp
#define ppclox_value_hpp

#include "common.hpp"

enum class ValueType {
    BOOL,
    NIL,
    NUMBER
};

class Value {
public:
    ValueType type() { return m_type; }

    Value(bool val);
    /** Construct NIL Value */
    Value();
    Value(double val);

    void print() const;

    bool as_bool() const { return m_as.boolean; }
    double as_number() const { return m_as.number; }

    bool is_bool() const { return m_type == ValueType::BOOL; }
    bool is_nil() const { return m_type == ValueType::NIL; }
    bool is_number() const { return m_type == ValueType::NUMBER; }
private:
    ValueType m_type{ValueType::NIL};
    union {
        bool boolean;
        double number;
    } m_as{};
};

#endif
