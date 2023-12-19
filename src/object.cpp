#include "object.hpp"

void Obj::print() const {
    printf("Object: %d", m_type);
}

bool InternedStringKey::operator==(const InternedStringKey& key) const {
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

InternedStringKey::InternedStringKey(ObjString* obj) {
    m_string_view = std::string_view(obj->chars(), obj->length());
    m_obj_string = obj;
    m_hash = obj->hash();
}

InternedStringKey::InternedStringKey(std::string_view string_view) {
    m_string_view = string_view;
    m_obj_string = nullptr;
    m_hash = std::hash<std::string_view>()(string_view);
}

// Initialize map to empty
std::unordered_map<std::string_view, ObjString*> ObjString::s_interned_strings{};

void ObjString::print() const {
    printf("%s", chars());
}

ObjString::~ObjString() {
    // Upon destruction, we need to clean ourselves out of the map
//FIX - This has to hash the string again to find it. Can we avoid re-hashing
//      via a custom key object that caches the hash???
//FIX - should probably protect this with a lock so these can be used across threads 
    s_interned_strings.erase(std::string_view(this->chars(), this->length()));
}

ObjString* ObjString::copy_string(const char* chars, std::size_t length) {
//FIX - should probably protect this with a lock so these can be used across threads    
    ObjString* existing = find_existing(chars, length);
    if (existing != nullptr) return existing;

    // If it doesn't already exist, we need a new one
    ObjString* str = new ObjString(chars, length);
    store_new(str);
    return str;
}

ObjString* ObjString::take_string(std::string&& text) {
//FIX - should probably protect this with a lock so these can be used across threads    
    ObjString* existing = find_existing(text);
    if (existing != nullptr) return existing;

    // If it doesn't already exist, we need a new one
    ObjString* str = new ObjString(std::move(text));
    store_new(str);
    return str;
}

ObjString* ObjString::find_existing(const char* chars, std::size_t length) {
    return find_existing(std::string_view(chars, length));
}

ObjString* ObjString::find_existing(std::string_view search) {
    auto it = s_interned_strings.find(search);
    if (it != s_interned_strings.end()) {
        return it->second;
    }
    return nullptr;
}

void ObjString::store_new(ObjString* str) {
    s_interned_strings[std::string_view(str->chars(), str->length())] = str;
}