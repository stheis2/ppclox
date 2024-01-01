#include <algorithm>

#include "object.hpp"
#include "compiler.hpp"
#include "vm.hpp"
#include "object_function.hpp"
#include "object_string.hpp"
#include "object_class.hpp"

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

//FIX - Put practical limit on object heap size

    // If the previous allocation put us over the limit, run the collector before
    // before allocating more.
    if (s_bytes_allocated > s_next_gc) {
        collect_garbage();
    }

    void* ptr = ::operator new(size);

    // Whenever we allocate an Obj, add it to the master list
    Obj* obj = static_cast<Obj*>(ptr);
    s_all_objects.push_back(obj);

    // Accumulate bytes allocated and save for later
    s_bytes_allocated += size;
    s_bytes_map[obj] = size;

#ifdef DEBUG_LOG_GC
    printf("%p allocated %zu\n", ptr, size);
#endif

    return ptr;;
}

void Obj::operator delete(void *memory) {
    // Find this object to decerement its bytes allocated and
    // also remove it from the map.
    auto it = s_bytes_map.find((Obj*)memory);
    if (it != s_bytes_map.end()) {
        s_bytes_allocated -= it->second;
        s_bytes_map.erase(it);
    }
    else {
#ifdef DEBUG_LOG_GC
    printf("%p unable to find in bytes map for removing allocated bytes!\n", memory);
#endif        
    }

#ifdef DEBUG_LOG_GC
    printf("%p free\n", memory);
#endif   
    ::operator delete(memory);
}

void Obj::free_objects() {
    while (s_all_objects.size() > 0) {
        Obj* obj = s_all_objects.back();
        delete obj;
        s_all_objects.pop_back();
    }
}

void Obj::collect_garbage() {
#ifdef DEBUG_LOG_GC
    printf("-- gc begin\n");
    std::size_t before = s_bytes_allocated;
#endif

    mark_gc_roots();
    trace_gc_references();
    // NOTE! We don't need to release weak references to ObjStrings
    //       like Clox because ObjString automatically removes itself
    //       from the de-duping table upon destruction.
    sweep();

    // Now that we're done, adjust next GC threshold based
    // on total (estimated) heap size.
    // NOTE! In the unlikely event that the heap is so large that multiplying by the growth factor might overflow,
    //       choose the midpoint between the heap size and the max
    if (s_bytes_allocated < (std::numeric_limits<std::size_t>::max() / k_gc_heap_grow_factor)) {
        s_next_gc = s_bytes_allocated * k_gc_heap_grow_factor;
    }
    else {
        std::size_t increment = (std::numeric_limits<std::size_t>::max() - s_bytes_allocated) / 2;
        s_next_gc = s_bytes_allocated + increment;
    }

#ifdef DEBUG_LOG_GC
    printf("-- gc end\n");
    printf("   collected %zu bytes (from %zu to %zu) next at %zu\n",
        before - s_bytes_allocated, before, s_bytes_allocated,
        s_next_gc);
#endif
}

void Obj::add_bytes_allocated(std::size_t bytes) {
    s_bytes_allocated += bytes;
}

void Obj::subtract_bytes_allocated(std::size_t bytes) {
    s_bytes_allocated -= bytes;
}

std::vector<Obj*> Obj::s_all_objects{};
std::unordered_map<Obj*, std::size_t> Obj::s_bytes_map{};
std::vector<Obj*> Obj::s_gray_worklist{};
std::size_t Obj::s_bytes_allocated{};
std::size_t Obj::s_next_gc = Obj::k_initial_gc_threshold;

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

void Obj::sweep() {
    // Partition the list into non-white followed by white objects
    auto white_base_iter = partition(s_all_objects.begin(), s_all_objects.end(), [](Obj* obj) { return obj->m_gc_color != ObjGcColor::WHITE; });

    // Free all the white objects
    for (auto free_iter = white_base_iter; free_iter != s_all_objects.end(); ++free_iter) {
        Obj* free_me = *free_iter;
        delete free_me;
    }

    // Remove all the now dangling pointers to white objects
    s_all_objects.erase(white_base_iter, s_all_objects.end());

    // Reset all remaining objects to white for the next GC
    for (auto obj : s_all_objects) {
        obj->whiten();
    }
}

void Obj::blacken() {
#ifdef DEBUG_LOG_GC
    printf("%p blacken ", this);
    print();
    printf("\n");
#endif    

// TODO: Could move responsibility for tracing references down into each subclass,
// e.g. make it a virtual method.
    switch (m_type) {
        case ObjType::CLASS: {
            ObjClass* klass = (ObjClass*)this;
            Obj::mark_gc_gray(klass->name());
            break;
        }
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
        case ObjType::INSTANCE: {
            ObjInstance* instance = (ObjInstance*)this;
            Obj::mark_gc_gray(instance->get_class());
            instance->mark_fields_gc_gray();
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