#ifndef ppclox_object_hpp
#define ppclox_object_hpp

#include <functional>
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

class InternedStringKey {
public:
//FIX - Move this to CPP file.
    bool operator==(const InternedStringKey& key) const {
        // Thanks to de-duping as well as the way these keys are used, 
        // ObjString* pointers indicate equivalent objects
        if (this->m_obj_string != nullptr && key.m_obj_string != nullptr) {
            return this->m_obj_string == key.m_obj_string;
        }

        // If the hashes differ, we know we are different
        if (this->m_hash != key.m_hash) {
            return false;
        }

        // If hashes are the same, we must compare strings directly
        return this->m_string_view == key.m_string_view;
    }

    // TODO: Need to add constructors that make sense for this type.
    // Constructors must compute a hash, unless its been cached somewhere (e.g.
    // on the ObjString).

    const std::string_view string_view() const { return m_string_view; }
    const ObjString* obj_string() const  { return m_obj_string; }
    std::size_t hash() const { return m_hash; }
private:
    /** 
     * When actually stored in the map, this will point to the actual 
     * string data owned by the corresponding m_obj_string.
    */
    std::string_view m_string_view{};
    /** This will be non-null when actually stored in the map. */
    /** It can be null or non-null when searching within the map for a given key. */
    ObjString* m_obj_string{};
    std::size_t m_hash{};    
};

// Function object that knows how to hash InternedStringKey
// See method 3 at https://marknelson.us/posts/2011/09/03/hash-functions-for-c-unordered-containers.html
class InternedStringKeyHash {
    size_t operator()(const InternedStringKey& key) const
    {
        //return std::hash<std::string_view>()(key.m_string_view);
        return key.hash();
    }
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