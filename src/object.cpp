#include "object.hpp"

void Obj::print() const {
    printf("Object: %d", m_type);
}

bool InternedStringKey::operator==(const InternedStringKey& key) const {
    // Thanks to de-duping of the ObjStrings,
    // equal ObjString* pointers indicate equivalent objects
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
std::unordered_map<InternedStringKey, ObjString*, InternedStringKeyHash> ObjString::s_interned_strings{};

void ObjString::print() const {
    printf("%s", chars());
}

ObjString::~ObjString() {
    // Construct the search key we will use to find ourselves in the map
    InternedStringKey search(this);
    // Upon destruction, we need to clean ourselves out of the map
//FIX - should probably protect this with a lock so these can be used across threads 
    s_interned_strings.erase(search);
}

ObjString* ObjString::copy_string(const char* chars, std::size_t length) {
    // Construct search key. Note that this will hash the string.
    InternedStringKey search(std::string_view(chars, length));

//FIX - should probably protect this with a lock so these can be used across threads      

    ObjString* existing = find_existing(search);
    if (existing != nullptr) return existing;

    // If it doesn't already exist, we need a new one
    ObjString* str = new ObjString(chars, length, search.hash());
    store_new(str);
    return str;
}

ObjString* ObjString::take_string(std::string&& text) {
    // Construct search key. Note that this will hash the string.
    InternedStringKey search{std::string_view(text)};

//FIX - should probably protect this with a lock so these can be used across threads    

    ObjString* existing = find_existing(search);
    if (existing != nullptr) return existing;

    // If it doesn't already exist, we need a new one
    ObjString* str = new ObjString(std::move(text), search.hash());
    store_new(str);
    return str;
}

ObjString* ObjString::find_existing(const InternedStringKey& search) {
    auto it = s_interned_strings.find(search);
    if (it != s_interned_strings.end()) {
        return it->second;
    }
    return nullptr;
}

void ObjString::store_new(ObjString* str) {
    // Construct the key we will use to find ourselves in the map later
    InternedStringKey key(str);
    s_interned_strings[key] = str;
}