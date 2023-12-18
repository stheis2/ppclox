#include "object.hpp"

void Obj::print() const {
    printf("Object: %d", m_type);
}

// Initialize map to empty
std::unordered_map<std::string_view, ObjString*> ObjString::s_interned_strings{};

void ObjString::print() const {
    printf("%s", chars());
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