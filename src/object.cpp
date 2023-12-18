#include "object.hpp"

// Initialize these maps to empty
std::unordered_map<const std::string_view, std::shared_ptr<const std::string>> ObjString::s_interned_strings{};
std::unordered_map<const char*, ObjString*> ObjString::s_obj_strings{};

ObjString* ObjString::copy_string(const char* chars, std::size_t length) {
    // First, we need to see if this string already exists
    std::string_view search(chars, length);
    auto it = s_interned_strings.find(search);
    if(it != s_interned_strings.end()) {
        // String already exists. See if an ObjString is already stored for this data
        auto obj_it = s_obj_strings.find(it->second->c_str());
        if(obj_it != s_obj_strings.end()) {
            return obj_it->second;
        }
    }

    // Make a copy of the given string into a std::string allocated on the heap

    std::string copied_string{chars};

    // Make a string_view referring to this std::string that can be used as the key
    std::string_view key{copied_string};



    // Allocate a std::string on the heap to contain this data.
    // NOTE! I'd like to have the map's key be a string_view pointing to
    //       the std::string, but 
}

ObjString* ObjString::take_string(const char* chars, std::size_t length) {
    
}