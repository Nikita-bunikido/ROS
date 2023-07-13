#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>

#include "common.h"
#include "lexer.h"
#include "asm.h"
#include "stack.h"

#define MAX_INPUTS      32

enum Warn_Type {
    WARN_TYPE_WERROR = 0,
    WARN_TYPE_WSEPARATE,
    WARN_TYPE_WCONVERSIONS,
    WARN_TYPE_WRANGE,
    WARN_TYPE_WALL,
    WARN_TYPE_UNKNOWN
};
typedef enum Warn_Type Warn_Type;

static void __attribute__((noinline)) _blocks_cleanup_callback(void *item) {
    struct Block block = *(struct Block *)item;

    if (block.is_data && !block.data.is_reserved)
        free(block.data.raw);
}

static void __attribute__((noinline)) _tokens_cleanup_callback(void *item) {
    struct Token *token_list = *(struct Token **)item;

    if (token_list)
        token_free_list(token_list);
}

static void __attribute__((noinline)) _inputs_cleanup_callback(void *item) {
    struct Input input = *(struct Input *)item;
    input_delete(&input);
}

volatile struct Warning_Info warning_info = { 0 };

volatile Cleanup_Stack blocks_cleanup = { 0 };
volatile Cleanup_Stack tokens_cleanup = { 0 };
volatile Cleanup_Stack inputs_cleanup = { 0 };

volatile Cleanup_Stack_Callback blocks_cleanup_callback = _blocks_cleanup_callback;
volatile Cleanup_Stack_Callback tokens_cleanup_callback = _tokens_cleanup_callback;
volatile Cleanup_Stack_Callback inputs_cleanup_callback = _inputs_cleanup_callback;

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

static inline __attribute__((always_inline)) const char *extension(const char *path) {
    return strrchr(path, '.');
}

static inline __attribute__((always_inline)) void generate_output_name(char *buf, const char *path) {
    ASM_ASSERT_NOT_NULL(buf);

    strcpy(buf, path);
    *strrchr(buf, '.') = '\0';
    strcat(buf, ".rex");
}

int main(int argc, const char *argv[]) {

    if (argc < 2)
        usage(stderr, argc, argv);

    struct Input input_stack[MAX_INPUTS] = { 0 };
    size_t input_stack_size = 0;

    for (int i = 1; i < argc; i ++) {
        const char *arg = argv[i];

        if (*arg != '-') {
            if (strcmp(extension(arg), ".rch8") && strcmp(extension(arg), ".RCH8")) {
                fprintf(stderr, "Unknown file extension: \"%s\"\n", extension(arg));
                TOTAL_CLEANUP();
                exit(EXIT_FAILURE);
            }

            input_create(&input_stack[input_stack_size ++], arg);
            STACK_PUSH(inputs_cleanup, input_stack[input_stack_size - 1]);
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
                    TOTAL_CLEANUP();
                    exit(EXIT_FAILURE);
                }

            _load_addr = addr;
            continue;
        }

        fprintf(stderr, "Unknown option: \"%s\".\n", arg);
        TOTAL_CLEANUP();
        exit(EXIT_FAILURE);
    }

    dump_assembler();

    for (size_t i = 0; i < input_stack_size; i++) {
        clear_defenitions();

        struct Token *troot = tokenize_data(input_stack + i);
        STACK_PUSH(tokens_cleanup, troot);

        size_t blocks_num;
        struct Block *blocks = blocks_parse(troot, &blocks_num);
        for (size_t i = 0; i < blocks_num; i ++)
            STACK_PUSH(blocks_cleanup, blocks[i]);

        char result_path[PATH_MAX];
        generate_output_name(result_path, input_stack[i].file);
        FILE *f = fopen(result_path, "wb");

        for (size_t i = 0; i < blocks_num; i++)
            if (assemble_block(blocks + i, f) < 0) {
                printf("[%zu instruction assembling fault.]\n", i);
                TOTAL_CLEANUP();
                exit(EXIT_FAILURE);
            }
        
        fclose(f);
    }
    TOTAL_CLEANUP();

    return 0;
}