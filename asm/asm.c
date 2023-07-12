#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>

#include "common.h"
#include "lexer.h"
#include "asm.h"

#define MAX_INPUTS      32

volatile struct Warning_Info warning_info = { 0 };

void __attribute__((noreturn)) usage(FILE *f, int argc, const char **argv) {
    ASM_UNUSED(argc);
    
    fprintf(f, "usage: %s <files> <options>\n", argv[0]);
    fprintf(f, "options:\n\t-warn_error - Turn all warnings into errors\n\t"
               "-warn_separate - Auto separating one complex instruction into two\n\t"
               "-warn_conversions - IMM8 to IMM12 auto conversions\n\t"
               "-warn_range - Auto defaulting to IMM12/IMM8 range using \'and\' operation\n\t"
               "-warn_all - Turn on all warnings\n\t"
               "-loadaddr <load address> - Change load address ( default - 200h )\n");
    exit(EXIT_FAILURE);
}

enum Warn_Type {
    WARN_TYPE_WERROR = 0,
    WARN_TYPE_WSEPARATE,
    WARN_TYPE_WCONVERSIONS,
    WARN_TYPE_WRANGE,
    WARN_TYPE_WALL,
    WARN_TYPE_UNKNOWN
};
typedef enum Warn_Type Warn_Type;

static Warn_Type option_as_warn_type(const char *opt) {
    for (int i = 0; i < WARN_TYPE_UNKNOWN; ++i)
        if (!strcmp(opt, (const char *[WARN_TYPE_UNKNOWN]){
            [WARN_TYPE_WERROR]       = "-warn_error",
            [WARN_TYPE_WSEPARATE]    = "-warn_separate",
            [WARN_TYPE_WCONVERSIONS] = "-warn_conversions",
            [WARN_TYPE_WRANGE]       = "-warn_range",
            [WARN_TYPE_WALL]         = "-warn_all"
        }[i])) return i;
    return WARN_TYPE_UNKNOWN;
}

int main(int argc, const char *argv[]) {

    if (argc < 2)
        usage(stderr, argc, argv);

    struct Input input_stack[MAX_INPUTS] = { 0 };
    size_t input_stack_size = 0;

    for (int i = 1; i < argc; i ++) {
        const char *arg = argv[i];

        if (*arg != '-') {
            input_create(&input_stack[input_stack_size ++], arg);
            continue;
        }

        Warn_Type wt;
        if ((wt = option_as_warn_type(arg)) != WARN_TYPE_UNKNOWN) {
            if (wt == WARN_TYPE_WALL){
                memset(warning_info.w_raw, 1, sizeof(warning_info.w_raw));
                warning_info.w_error = false;
                continue;
            }
            
            warning_info.w_raw[(unsigned)wt] = true;
            continue;
        }

        if (!strcmp(arg, "-loadaddr")) {
            uint16_t addr;
            i ++;

            if (1 != sscanf(argv[i], "%" PRIx16 "h", &addr))
                if (1 != sscanf(argv[i], "%" PRIX16 "h", &addr)) {
                    fprintf(stderr, "Value \"%s\" cannot be treated as address.\n", argv[i]);
                    for (size_t i = 0; i < input_stack_size - 1; i++)
                        input_delete(input_stack + i);
                    exit(EXIT_FAILURE);
                }

            _load_addr = addr;
            continue;
        }

        fprintf(stderr, "Unknown option: \"%s\".\n", arg);
        for (size_t i = 0; i < input_stack_size - 1; i++)
            input_delete(input_stack + i);
        exit(EXIT_FAILURE);
    }

    dump_assembler();

    for (size_t i = 0; i < input_stack_size; i++) {
        clear_defenitions();

        struct Token *troot = tokenize_data(input_stack + i);
        //token_dump_list(troot);

        size_t blocks_num;
        struct Block *blocks = blocks_parse(troot, &blocks_num);

        puts("RAW CODE\n========");
        for (size_t i = 0; i < blocks_num; i++)
            if (assemble_block(blocks + i) < 0) {
                printf("[%zu instruction assembling fault.]\n", i);
                exit(EXIT_FAILURE);
            }

        for (size_t i = 0; i < blocks_num; i++)
            if (blocks[i].is_data && !blocks[i].data.is_reserved) free(blocks[i].data.raw);

        token_free_list(troot);
        input_delete(input_stack + i);
    }
   
    return 0;
}