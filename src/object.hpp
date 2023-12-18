#ifndef ppclox_object_hpp
#define ppclox_object_hpp

#include "common.hpp"

enum class ObjType {
    STRING
};

class Obj {
public:
    ObjType type() const { return m_type; }
private:
    ObjType m_type{};

//TODO: Implement custom new and delete for objs?    
};

class ObjString : Obj {
public:
    const char* chars() { return m_chars; }
private:
    std::size_t m_length{};
    const char* m_chars{};
};

#endif