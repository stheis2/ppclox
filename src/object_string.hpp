#ifndef ppclox_object_string_hpp
#define ppclox_object_string_hpp

#include "common.hpp"
#include "object.hpp"

// Forward declare this to appease the compiler
class ObjString;

class InternedStringKey {
public:
    bool operator==(const InternedStringKey& key) const;

    /** Construct key for storing or searching. This will utilize cached hash. */
    InternedStringKey(ObjString* obj);
    /** Construct key for searching only. This will hash the string. */
    InternedStringKey(std::string_view string_view);

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
public:    
    size_t operator()(const InternedStringKey& key) const
    {
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

    /** Add the two strings and return the result (usually a new string) */
    ObjString* operator+(const ObjString& rhs) const;

    std::size_t length() const { return m_string.size(); }
    const char* chars() const { return m_string.c_str(); }
    std::size_t hash() const { return m_hash; }

    ~ObjString();
private:
    const std::string m_string{};
    /** 
     * NOTE! Be sure to list this after m_string so it gets initialized after!
     * See https://stackoverflow.com/questions/1242830/what-is-the-order-of-evaluation-in-a-member-initializer-list
    */ 
    const std::size_t m_hash{};

    ObjString(const char* chars, std::size_t length, std::size_t hash) : 
        Obj(ObjType::STRING),       
        m_string(chars, length),
        m_hash(hash) { }
    ObjString(std::string&& text, std::size_t hash) : 
        Obj(ObjType::STRING), 
        m_string(std::move(text)),
        m_hash(hash) {}

    static ObjString* find_existing(const InternedStringKey& search);
    static void store_new(ObjString* str);

    /** Map used for de-deping ObjStrings */
    static std::unordered_map<InternedStringKey, ObjString*, InternedStringKeyHash> s_interned_strings;
};

/** 
 * Reference to an ObjString that can be used as a key in a hash map.
 * Stores a pointer to an ObjString so the object is only valid as long
 * as the ObjString it refers to is.
 */
class ObjStringRef {
public:
    ObjStringRef(ObjString* obj_string) : m_obj_string(obj_string) {}
    ObjString* obj_string() const { return m_obj_string; }

    bool operator==(const ObjStringRef& key) const {
        /** Thanks to ObjString de-duping, two ObjStrings are equal exactly when their pointers are equal. */
        return m_obj_string == key.m_obj_string;
    }
private:
    ObjString* m_obj_string{};
};

// Function object that knows how to hash ObjStringRef
// See method 3 at https://marknelson.us/posts/2011/09/03/hash-functions-for-c-unordered-containers.html
class ObjStringRefHash {
public:    
    size_t operator()(const ObjStringRef& obj_string_ref) const
    {
        return obj_string_ref.obj_string()->hash();
    }
};

#endif