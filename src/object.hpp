#ifndef ppclox_object_hpp
#define ppclox_object_hpp

#include <unordered_map>
#include <string>
#include <string_view>
#include <memory>

#include "common.hpp"

enum class ObjType {
    STRING
};

class Obj {
public:
    ObjType type() const { return m_type; }

    virtual void print() const;

//FIX - Do we need a virtual destructor?    
protected:
    Obj(ObjType type) : m_type(type) {}
private:
    ObjType m_type{};

//TODO: Implement custom new and delete for objs? And how do we account for the ObjStrings
// memory?   
};

class ObjString : public Obj {
public:
    void print() const override;

    /** 
     * Return an ObjString representing the given string, copying it if necessary
     * (e.g. not taking ownership)
    */
    static ObjString* copy_string(const char* chars, std::size_t length);

    /** 
     * Return an ObjString representing the given (moved) string.
    */
    static ObjString* take_string(std::string&& text);

    const std::size_t length() { return m_string.size(); }
    const char* chars() const { return m_string.c_str(); }

    ~ObjString();
private:
    const std::string m_string{};

    ObjString(const char* chars, std::size_t length) : Obj(ObjType::STRING), m_string(std::move(std::string(chars, length))) { }
    ObjString(std::string&& text) : Obj(ObjType::STRING), m_string(std::move(text)) {}

    static ObjString* find_existing(const char* chars, std::size_t length);
    static ObjString* find_existing(std::string_view search);
    static void store_new(ObjString* str);

    /** Map used for de-deping ObjStrings */
    static std::unordered_map<std::string_view, ObjString*> s_interned_strings;
};

#endif