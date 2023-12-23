#include "object.hpp"


void Obj::print() const {
    printf("Object: %d", m_type);
}

void* Obj::operator new(std::size_t size) {
    void* ptr = ::operator new(size);
    // Whenever we allocate an Obj, we insert it into the linked list
    
    Obj* obj = static_cast<Obj*>(ptr);
    obj->m_next = s_objects_head;
    s_objects_head = obj;
    return ptr;;
}

void Obj::operator delete(void *memory) {
    ::operator delete(memory);
}

void Obj::free_objects() {
    Obj* object = s_objects_head;
    while (object != nullptr) {
        Obj* next = object->m_next;
        delete object;
        object = next;
    }
    s_objects_head = nullptr;
}

Obj* Obj::s_objects_head{};


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

/** Add the two strings and return the result (usually a new string) */
ObjString* ObjString::operator+(const ObjString& rhs) const {
    std::string combined = m_string + rhs.m_string;
    return ObjString::take_string(std::move(combined));
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