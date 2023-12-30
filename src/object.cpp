#include "object.hpp"
#include "compiler.hpp"
#include "vm.hpp"
#include "object_function.hpp"
#include "object_string.hpp"

void Obj::print() const {
    printf("Object: %d", m_type);
}

void Obj::mark_gc_gray(Obj* obj) {
    if (obj == nullptr) return;
    if (obj->m_gc_color != ObjGcColor::WHITE) return;

// TODO: An easy optimization we could do in markObject() is to skip adding strings and native functions to the gray stack at all since we know they don’t need to be processed. Instead, they could darken from white straight to black.

#ifdef DEBUG_LOG_GC
    printf("%p mark ", (void*)obj);
    obj->print();
    printf("\n");
#endif    

    // Mark this object gray and add it to the worklist for processing
    obj->m_gc_color = ObjGcColor::GRAY;
    s_gray_worklist.push_back(obj);
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

    mark_gc_roots();
    trace_gc_references();

#ifdef DEBUG_LOG_GC
    printf("-- gc end\n");
#endif
}

Obj* Obj::s_objects_head{};

std::vector<Obj*> Obj::s_gray_worklist{};

void Obj::mark_gc_roots() {
    // Tell the compiler to mark its roots
    Compiler::mark_gc_roots();

    // Tell the VM to mark its roots
    g_vm.mark_gc_roots();
}

void Obj::trace_gc_references() {
    while (s_gray_worklist.size() > 0) {
        Obj* gray = s_gray_worklist.back();
        s_gray_worklist.pop_back();
        gray->blacken();
    }
}

void Obj::blacken() {
#ifdef DEBUG_LOG_GC
    printf("%p blacken ", this);
    print();
    printf("\n");
#endif    

    switch (m_type) {
        case ObjType::CLOSURE: {
            ObjClosure* closure = (ObjClosure*)this;
            Obj::mark_gc_gray(closure->function());
            for (auto upvalue : closure->upvalues()) {
                Obj::mark_gc_gray(upvalue);
            }
            break;
        }
        case ObjType::FUNCTION: {
            ObjFunction* function = (ObjFunction*)this;
            Obj::mark_gc_gray(function->name_obj());
            for (auto val : function->chunk().get_constants()) {
                val.mark_obj_gc_gray();
            }
            break;
        }  
        case ObjType::UPVALUE: {
            ((ObjUpvalue*)this)->closed_value().mark_obj_gc_gray();
            break;
        }
        case ObjType::NATIVE:
        case ObjType::STRING:
            // These types have no outgoing references
            //TODO: An easy optimization we could do in markObject() is to skip adding strings and native functions to the gray stack at all since we know they don’t need to be processed. Instead, they could darken from white straight to black.
            break;
    }

    // Now that we are done graying our external references, the object is black
    m_gc_color = ObjGcColor::BLACK;
}