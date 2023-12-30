#include "object.hpp"


void Obj::print() const {
    printf("Object: %d", m_type);
}

void* Obj::operator new(std::size_t size) {
#ifdef DEBUG_STRESS_GC
    collect_garbage();
#endif

    void* ptr = ::operator new(size);

    // Whenever we allocate an Obj, we insert it into the linked list
    Obj* obj = static_cast<Obj*>(ptr);
    obj->m_next = s_objects_head;
    s_objects_head = obj;

#ifdef DEBUG_LOG_GC
    printf("%p allocated %zu\n", ptr, size);
#endif

    return ptr;;
}

void Obj::operator delete(void *memory) {
#ifdef DEBUG_LOG_GC
    printf("%p free\n", memory);
#endif   
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

void Obj::collect_garbage() {
#ifdef DEBUG_LOG_GC
    printf("-- gc begin\n");
#endif

#ifdef DEBUG_LOG_GC
    printf("-- gc end\n");
#endif
}

Obj* Obj::s_objects_head{};