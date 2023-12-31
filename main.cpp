#include <memory>

#include "common.hpp"
#include "chunk.hpp"
#include "vm.hpp"

// TODO: Implement using C++ idioms instead of C
static void repl() {
    char line[1024];
    for (;;) {
        printf("> ");

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }

        g_vm.interpret(line);
    }
}

// TODO: Implement using C++ idioms instead of C
static char* readFile(const char* path) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        std::exit(74);
    }

    fseek(file, 0L, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(file_size + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        std::exit(74);
    }

    size_t bytes_read = fread(buffer, sizeof(char), file_size, file);
    if (bytes_read < file_size) {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        std::exit(74);
    }
    buffer[bytes_read] = '\0';

    fclose(file);
    return buffer;
}

// TODO: Implement using C++ idioms instead of C
static void runFile(const char* path) {
    char* source = readFile(path);
    InterpretResult result = g_vm.interpret(source);
    free(source);

    if (result == InterpretResult::COMPILE_ERROR) std::exit(65);
    if (result == InterpretResult::RUNTIME_ERROR) std::exit(70);
}

int main(int argc, const char* argv[]) {

    if (argc == 1) {
        repl();
    } else if (argc == 2) {
        runFile(argv[1]);
    } else {
        fprintf(stderr, "Usage: ppclox [path]\n");
        std::exit(64);
    }

    /** Free all objects before program exit */
    Obj::free_objects();

#if false
    auto chunk_ptr = std::make_shared<Chunk>();

    std::size_t constant_index = chunk_ptr->add_constant(1.2);
    chunk_ptr->write(static_cast<std::uint8_t>(OpCode::CONSTANT), 123);
    chunk_ptr->write(constant_index, 123);

    constant_index = chunk_ptr->add_constant(3.4);
    chunk_ptr->write(static_cast<std::uint8_t>(OpCode::CONSTANT), 123);
    chunk_ptr->write(constant_index, 123);

    chunk_ptr->write(static_cast<std::uint8_t>(OpCode::SUBTRACT), 123);

    constant_index = chunk_ptr->add_constant(5.6);
    chunk_ptr->write(static_cast<std::uint8_t>(OpCode::CONSTANT), 123);
    chunk_ptr->write(constant_index, 123);

    chunk_ptr->write(static_cast<std::uint8_t>(OpCode::MULTIPLY), 123);

    chunk_ptr->write(static_cast<std::uint8_t>(OpCode::NEGATE), 123);

    chunk_ptr->write(static_cast<std::uint8_t>(OpCode::RETURN), 123);
    chunk_ptr->dissassemble("test chunk");
    std::cout << std::endl;
    g_vm.interpret(chunk_ptr);
#endif    
    return 0;
}