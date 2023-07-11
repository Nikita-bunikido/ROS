#ifndef _ASM_H
#define _ASM_H

#include <inttypes.h>
#include <assert.h>
#include <stdbool.h>

#define MAX_OPERANDS    3
#define SUFFIX_UNUSED   0

#define OP_REG_IMM8     OP_REG8 | OP_IMM8 | OP_V0
enum Operand_Type {
    OP_KEY = (1 << 0),
    OP_REG8 = (1 << 1),
    OP_IMM8 = (1 << 2),
    OP_IMM12 = (1 << 3),
    OP_REG12 = (1 << 4),
    OP_V0 = (1 << 5),
    OP_SPEC = (1 << 6),
    OP_TIMER = (1 << 7)
};

enum Template {
    TEMPL_NONE = 0,
    TEMPL_ONNN,
    TEMPL_ONNN2,    /* ONNN, but imm12 is in 2nd argument */
    TEMPL_OXNN,
    TEMPL_OXYS,
    TEMPL_OXSS,
    TEMPL_OXSS2,    /* OXSS, but reg8 is in 2nd argument */
    TEMPL_OOOO,
};

union __attribute__((__packed__)) Instruction_Raw {
    struct __attribute__((__packed__)) {
        unsigned int NNN : 12;
        unsigned int O   : 4;
    } ONNN;

    struct __attribute__((__packed__)) { 
        unsigned int NN  : 8;
        unsigned int X   : 4;
        unsigned int O   : 4;
    } OXNN;
        
    struct __attribute__((__packed__)) {
        unsigned int S   : 4;
        unsigned int Y   : 4;
        unsigned int X   : 4;
        unsigned int O   : 4;
    } OXYS;

    struct __attribute__((__packed__)) {
        unsigned int SS  : 8;
        unsigned int X   : 4;
        unsigned int O   : 4;
    } OXSS;
};
static_assert( sizeof(union Instruction_Raw) == 2 );

/* For assembling */
struct Real_Instruction_Info {
    int noperands;
    uint8_t types[MAX_OPERANDS];
    enum Template template;

    union {
        struct {
            unsigned int opcode4 : 4;
            unsigned int suffix : 8;
        };
        uint16_t opcode16;
    };
};

enum Instruction_Type {
    INS_CROS = 0, INS_RET, INS_CLS, INS_GOTO,
    INS_CALL, INS_SE, INS_SNE, INS_SET,
    INS_ADD, INS_OR, INS_AND, INS_XOR,
    INS_SUB, INS_SHR, INS_SHL, INS_RND,
    INS_BCD, INS_DUMP, INS_LOAD,

    INS_DRAW, INS_SKE, INS_SKNE, INS_SPR,

    INS_MAX
};

/* For checking */
struct Instruction_Info {
    char mnemonic[20];
    int noperands;
    uint8_t types[MAX_OPERANDS];
};

union Ros_Chip8_Value {
    uint8_t imm8, reg8;
    uint16_t imm12;
    struct Token *symbolic;
};

struct Definition {
    struct Token *tok;
    union Ros_Chip8_Value;
    uint8_t type;
};
extern void defenition_push(const struct Definition *);
extern void clear_defenitions(void);

struct Instruction {
    struct Token *tok;
    enum Instruction_Type type;

    int noperands;
    struct Operand {
        uint8_t type;

        union Ros_Chip8_Value;
        bool is_symbolic;

        struct Token *tok;
    } operands[MAX_OPERANDS];
};

struct Dataseg {
    bool is_reserved;
    size_t len;
    uint8_t *raw;
};

struct Block {
    bool is_data;

    union {
        struct Instruction ins;
        struct Dataseg data;
    };
};

struct Block *blocks_parse(const struct Token *root, size_t *nbl);
int assemble_block(const struct Block *block);

/* Conversions */
void token_as_imm(struct Operand *self, struct Token *tok);
void trim_string_quotes(struct Token *tok);

#endif /* _ASM_H */