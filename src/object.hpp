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
private:
    ObjType m_type{};

//TODO: Implement custom new and delete for objs?    
};

class ObjString : Obj {
public:
    /** 
     * Return an ObjString representing the given string, copying it if necessary
     * (e.g. not taking ownership)
    */
    ObjString* copy_string(const char* chars, std::size_t length) {

    }

    /** 
     * Return an ObjString representing the given string, taking ownership of it.
     * The returned ObjString may or may not use the actual given chars, though
     * they are guaranteed to be freed once no longer used.
    */
    ObjString* take_string(const char* chars, std::size_t length) {

    }

    const char* chars() { return m_chars; }
private:
    std::size_t m_length{};
    const char* m_chars{};

    ObjString(const char* chars, std::size_t length) {

    }

    ~ObjString() {

    }


    /** Map containing the actual interned string data */
    /** TODO: Investigate storing std::string in the map directly instead of a ptr? */
    static std::unordered_map<const std::string_view, std::shared_ptr<const std::string>> s_interned_strings;
    /** Map interned string data to ObjString referring to it */
    static std::unordered_map<const char*, ObjString*> s_obj_strings;
};

#endif