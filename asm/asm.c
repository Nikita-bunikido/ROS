#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

#include "common.h"
#include "lexer.h"
#include "asm.h"

#define MAX_INPUTS      32

int main(int argc, const char *argv[]) {

    if (argc < 2) {
        fprintf(stdout, "usage: %s <files> <options>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    struct Input input_stack[MAX_INPUTS] = { 0 };
    size_t input_stack_size = 0;

    while (argc > 0)
        input_create(&input_stack[input_stack_size ++], argv[--argc]);

    for (size_t i = 0; i < input_stack_size - 1; i++) {
        struct Token *troot = tokenize_data(input_stack + i);
        token_dump_list(troot);

        size_t instructions_num;
        struct Instruction *instructions = instructions_parse(troot, &instructions_num);

        puts("RAW CODE\n========");
        for (size_t i = 0; i < instructions_num; i++) {
            uint16_t raw;
            if (assemble_instruction(instructions + i, &raw) < 0) {
                printf("[%zu instruction assembling fault.]\n", i);
                exit(EXIT_FAILURE);
            }
        
            printf(ANSI_MAGENTA "%04X" ANSI_RESET " : %04X\n", i * 2 + 512, raw);
        }

        free(instructions);
        token_free_list(troot);
        input_delete(input_stack + i);
    }
   
    return 0;
}