#include "common.hpp"
#include "compiler.hpp"
#include "scanner.hpp"

void Compiler::compile(const char* source) {
    // TODO: Can this live here or does it need to be global?
    Scanner scanner(source);
    std::size_t line = std::numeric_limits<std::size_t>::max();
    for (;;) {
        Token token = scanner.scan_token();
        if (token.line != line) {
            printf("%4zu", token.line);
            line = token.line;
        } else {
            printf("   | ");
        }
        printf("%2d '%.*s'\n", token.type, token.length, token.start);

        if (token.type == TokenType::END_OF_FILE) break;
    }
}