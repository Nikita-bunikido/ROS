#include <stdlib.h>
#include <stdio.h>

#include "lexer.h"
#include "common.h"

int main(int argc, const char *argv[]) {

    if (argc < 2) {
        fprintf(stdout, "usage: %s <files> <options>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    struct Input base_input;
    const char *fname = argv[1];
    input_create(&base_input, fname);

    struct Token *troot = tokenize_data(base_input.base);

    token_dump_list(troot);
    token_free_list(troot);
    input_delete(&base_input);
    return 0;
}