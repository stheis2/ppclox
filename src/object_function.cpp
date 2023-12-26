#include "object_function.hpp"
#include "chunk.hpp"

void ObjFunction::print() const {
    printf("<fn %s>", name());
}