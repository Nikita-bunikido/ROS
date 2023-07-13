#include <stdbool.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "common.h"
#include "lexer.h"
#include "asm.h"
#include "pseudoins.h"
#include "stack.h"

#define MAX_VARIANTS        12
#define DEFINES_STACK_CAP   256

volatile uint16_t _load_addr = 0x200;

static struct Definition defines_stack[DEFINES_STACK_CAP] = { 0 };
static size_t defines_stack_size = 0;

static inline __attribute__((always_inline)) char *bool_as_cstr(const bool b) {
    return (char *[2]){ "false", "true" }[(int)b];
}

void dump_assembler(void) {
    fprintf(stdout, ANSI_CYAN "ROS-CHIP-8 Assembler v1.0 ( ALPHA )\n" ANSI_CYAN
                    ANSI_MAGENTA "-warn_error" ANSI_RESET "\t\t = %s\n"
                    ANSI_MAGENTA "-warn_separate" ANSI_RESET "\t\t = %s\n"
                    ANSI_MAGENTA "-warn_conversions" ANSI_RESET "\t = %s\n"
                    ANSI_MAGENTA "-warn_range" ANSI_RESET "\t\t = %s\n"
                    ANSI_MAGENTA "-loadaddr " ANSI_RESET "\t\t = %"PRIx16 "\n\n",
                    bool_as_cstr(warning_info.w_error), bool_as_cstr(warning_info.w_separate),
                    bool_as_cstr(warning_info.w_conversions), bool_as_cstr(warning_info.w_range),
                    _load_addr);
    fprintf(stdout, "Thank you for using ROS-CHIP-8 Assembler!\n");
}

void defenition_push(const struct Definition *def) {
    ASM_ASSERT(defines_stack_size <= DEFINES_STACK_CAP - 1);
    defines_stack[defines_stack_size ++] = *def;
}

void clear_defenitions(void) {
    memset(defines_stack, 0, defines_stack_size * sizeof(struct Definition));
    defines_stack_size = 0;
}

static struct Definition *definition_lookup(const struct Token *tok) {
    struct Definition *d = defines_stack;
    for(d = defines_stack; (d < (defines_stack + defines_stack_size)) && token_cmp_token(d->tok, tok); d ++);
    return (d == (defines_stack + defines_stack_size)) ? NULL : d;
}

static uint16_t measure_len_between_blocks(const struct Block *block1, const struct Block *block2, const struct Token *label) {
    int length = 0;
    ASM_ASSERT_NOT_NULL(block1);
    ASM_ASSERT_NOT_NULL(block2);
    ASM_ASSERT(block1 <= block2);

    for (const struct Block *bl = block1; bl != block2; bl ++) {
        if (!(bl->is_data)) {
            length += 2;
            continue;
        }

        length += bl->data.len;
    }

    length += _load_addr + 2;
    if (warning_info.w_range && (length > 0xFFF))
        ASM_WARNING(label, "Value %x is not in imm8/imm12 range. Defaulting to %x.", length, (unsigned)length & 0xFFF);

    return (uint16_t)((unsigned)length & 0xFFF);
}

static void create_label(struct Definition *self, const struct Block *bl_arr, const struct Block *bl_current, const struct Token *base) {
    ASM_ASSERT_NOT_NULL(self);

    *self = (struct Definition) {
        .tok = (struct Token *)base,
        .imm12 = measure_len_between_blocks(bl_arr, bl_current, base),
        .type = OP_IMM12
    };
}

/* For assembling */
static const struct Real_Instruction_Info asm_real_info[INS_MAX][MAX_VARIANTS] = {
    [INS_CROS] = {
        /* IMM12 */
        { 
            .noperands = 1, 
            .types = { OP_IMM12 }, 
            .template = TEMPL_ONNN, 
            .opcode4 = 0x0,
            .suffix = SUFFIX_UNUSED
        }
    },
    [INS_RET] = {
        /* No operands */
        {
            .noperands = 0,
            .types = { 0 },
            .template = TEMPL_OOOO,
            .opcode16 = 0x00EE,
        }
    },
    [INS_CLS] = {
        /* No operands */
        {
            .noperands = 0,
            .types = { 0 },
            .template = TEMPL_OOOO,
            .opcode16 = 0x00E0,
        }
    },
    [INS_GOTO] = {
        /* IMM12 */
        {
            .noperands = 1,
            .types = { OP_IMM12 },
            .template = TEMPL_ONNN,
            .opcode4 = 0x1,
            .suffix = SUFFIX_UNUSED
        },
        /* V0 + IMM12 ( SPEC ) */
        {
            .noperands = 1,
            .types = { OP_SPEC },
            .template = TEMPL_ONNN,
            .opcode4 = 0xB,
            .suffix = SUFFIX_UNUSED
        }
    },
    [INS_CALL] = {
        /* IMM12 */
        {
            .noperands = 1,
            .types = { OP_IMM12 },
            .template = TEMPL_ONNN,
            .opcode4 = 0x2,
            .suffix = SUFFIX_UNUSED
        }
    },
    [INS_SE] = {
        /* REG8, REG8 */
        {
            .noperands = 2,
            .types = { OP_REG8 | OP_V0, OP_REG8 | OP_V0 },
            .template = TEMPL_OXYS,
            .opcode4 = 0x5,
            .suffix = 0x0
        },
        /* REG8, IMM8 */
        {
            .noperands = 2,
            .types = { OP_REG8 | OP_V0, OP_IMM8 },
            .template = TEMPL_OXNN,
            .opcode4 = 0x3,
            .suffix = SUFFIX_UNUSED
        }
    },
    [INS_SNE] = {
        /* REG8, REG8 */
        {
            .noperands = 2,
            .types = { OP_REG8 | OP_V0, OP_REG8 | OP_V0 },
            .template = TEMPL_OXYS,
            .opcode4 = 0x9,
            .suffix = 0x0
        },
        /* REG8, IMM8 */
        {
            .noperands = 2,
            .types = { OP_REG8 | OP_V0, OP_IMM8 },
            .template = TEMPL_OXNN,
            .opcode4 = 0x4,
            .suffix = SUFFIX_UNUSED
        }
    },
    [INS_SET] = {
        /* REG8, REG8 */
        {
            .noperands = 2,
            .types = { OP_REG8 | OP_V0, OP_REG8 | OP_V0 },
            .template = TEMPL_OXYS,
            .opcode4 = 0x8,
            .suffix = 0x0
        },
        /* REG8, IMM8 */
        {
            .noperands = 2,
            .types = { OP_REG8 | OP_V0, OP_IMM8 },
            .template = TEMPL_OXNN,
            .opcode4 = 0x6,
            .suffix = SUFFIX_UNUSED
        },
        /* REG8, REG8 - DST8 ( SPEC )*/
        {
            .noperands = 2,
            .types = { OP_REG8 | OP_V0, OP_SPEC },
            .template = TEMPL_OXYS,
            .opcode4 = 0x8,
            .suffix = 0x7
        },
        /* REG8, delay/sound */
        {
            .noperands = 2,
            .types = { OP_REG8 | OP_V0, OP_TIMER },
            .template = TEMPL_OXSS,
            .opcode4 = 0xF,
            .suffix = 0x07
        },
        /* REG8, key */
        {
            .noperands = 2,
            .types = { OP_REG8 | OP_V0, OP_KEY },
            .template = TEMPL_OXSS,
            .opcode4 = 0xF,
            .suffix = 0x0A
        },
        /* REG12, IMM12 */
        {
            .noperands = 2,
            .types = { OP_REG12, OP_IMM12 },
            .template = TEMPL_ONNN2,
            .opcode4 = 0xA,
            .suffix = SUFFIX_UNUSED
        },
        /* REG12, IMM8 ( Pseudo-case ) */
        {
            .noperands = 2,
            .types = { OP_REG12, OP_IMM8 },
            .template = TEMPL_ONNN2,
            .opcode4 = 0xA,
            .suffix = SUFFIX_UNUSED
        },
        /* delay, REG8 */
        {
            .noperands = 2,
            .types = { OP_TIMER, OP_REG8 | OP_V0 },
            .template = TEMPL_OXSS2,
            .opcode4 = 0xF,
            .suffix = 0x15
        },
    },
    [INS_ADD] = {
        /* REG8, REG8 */
        {
            .noperands = 2,
            .types = { OP_REG8 | OP_V0, OP_REG8 | OP_V0 },
            .template = TEMPL_OXYS,
            .opcode4 = 0x8,
            .suffix = 0x4
        },
        /* REG8, IMM8 */
        {
            .noperands = 2,
            .types = { OP_REG8 | OP_V0, OP_IMM8 },
            .template = TEMPL_OXNN,
            .opcode4 = 0x7,
            .suffix = SUFFIX_UNUSED
        },
        /* REG12, REG8 */
        {
            .noperands = 2,
            .types = { OP_REG12, OP_REG8 | OP_V0 },
            .template = TEMPL_OXSS,
            .opcode4 = 0xF,
            .suffix = 0x1E
        }
    },
    [INS_OR] = {
        /* REG8, REG8 */
        {
            .noperands = 2,
            .types = { OP_REG8 | OP_V0, OP_REG8 | OP_V0 },
            .template = TEMPL_OXYS,
            .opcode4 = 0x8,
            .suffix = 0x1
        }
    },
    [INS_AND] = {
        /* REG8, REG8 */
        {
            .noperands = 2,
            .types = { OP_REG8 | OP_V0, OP_REG8 | OP_V0 },
            .template = TEMPL_OXYS,
            .opcode4 = 0x8,
            .suffix = 0x2
        }
    },
    [INS_XOR] = {
        /* REG8, REG8 */
        {
            .noperands = 2,
            .types = { OP_REG8 | OP_V0, OP_REG8 | OP_V0 },
            .template = TEMPL_OXYS,
            .opcode4 = 0x8,
            .suffix = 0x3
        }
    },
    [INS_SUB] = {
        /* REG8, REG8 */
        {
            .noperands = 2,
            .types = { OP_REG8 | OP_V0, OP_REG8 | OP_V0 },
            .template = TEMPL_OXYS,
            .opcode4 = 0x8,
            .suffix = 0x5
        }
    },
    [INS_SHR] = {
        /* REG8 */
        {
            .noperands = 1,
            .types = { OP_REG8 | OP_V0 },
            .template = TEMPL_OXSS,
            .opcode4 = 0x8,
            .suffix = 0xF6
        }
    },
    [INS_SHL] = {
        /* REG8 */
        {
            .noperands = 1,
            .types = { OP_REG8 | OP_V0 },
            .template = TEMPL_OXSS,
            .opcode4 = 0x8,
            .suffix = 0xFE
        }
    },
    [INS_RND] = {
        /* REG8, IMM8 */
        {
            .noperands = 2,
            .types = { OP_REG8 | OP_V0, OP_IMM8 },
            .template = TEMPL_OXNN,
            .opcode4 = 0xC,
            .suffix = SUFFIX_UNUSED
        }
    },
    [INS_BCD] = {
        /* REG8 */
        {
            .noperands = 1,
            .types = { OP_REG8 | OP_V0 },
            .template = TEMPL_OXSS,
            .opcode4 = 0xF,
            .suffix = 0x33
        }
    },
    [INS_DUMP] = {
        /* V0, REG8 */
        {
            .noperands = 2,
            .types = { OP_V0, OP_REG8 | OP_V0 },
            .template = TEMPL_OXSS2,
            .opcode4 = 0xF,
            .suffix = 0x55
        }
    },
    [INS_LOAD] = {
        /* V0, REG8 */
        {
            .noperands = 2,
            .types = { OP_V0, OP_REG8 | OP_V0 },
            .template = TEMPL_OXSS2,
            .opcode4 = 0xF,
            .suffix = 0x65
        }
    },

    #define ASM_REAL_INFO_NOP       \
    {                               \
        {                           \
            .noperands = 0,         \
            .types = { 0 },         \
            .template = TEMPL_OOOO, \
            .opcode16 = 0x8000      \
        }                           \
    }                               \

    [INS_DRAW] = ASM_REAL_INFO_NOP,
    [INS_SKE]  = ASM_REAL_INFO_NOP,
    [INS_SKNE] = ASM_REAL_INFO_NOP,
    [INS_SPR]  = ASM_REAL_INFO_NOP

    #undef ASM_REAL_INFO_NOP
};

/* For type & mnemonic checking */
static const struct Instruction_Info asm_info[INS_MAX] = {
    [INS_CROS] = { "cros", 1, { OP_IMM12 } },
    [INS_RET]  = { "ret",  0, { 0 } },
    [INS_CLS]  = { "cls",  0, { 0 } },
    [INS_GOTO] = { "goto", 1, { OP_IMM12 | OP_SPEC } },
    [INS_CALL] = { "call", 1, { OP_IMM12 } },
    [INS_SE]   = { "se",   2, { OP_REG8 | OP_V0, OP_REG_IMM8 } },
    [INS_SNE]  = { "sne",  2, { OP_REG8 | OP_V0, OP_REG_IMM8 } },
    [INS_SET]  = { "set",  2, { OP_REG8 | OP_V0 | OP_REG12 | OP_TIMER, OP_REG_IMM8 | OP_SPEC | OP_IMM12 | OP_TIMER | OP_KEY } },
    [INS_ADD]  = { "add",  2, { OP_REG8 | OP_V0 | OP_REG12, OP_REG_IMM8 } },
    [INS_OR]   = { "or",   2, { OP_REG8 | OP_V0, OP_REG_IMM8 } },
    [INS_AND]  = { "and",  2, { OP_REG8 | OP_V0, OP_REG_IMM8 } },
    [INS_XOR]  = { "xor",  2, { OP_REG8 | OP_V0, OP_REG_IMM8 } },
    [INS_SUB]  = { "sub",  2, { OP_REG8 | OP_V0, OP_REG_IMM8 } },
    [INS_SHR]  = { "shr",  1, { OP_REG8 | OP_V0 } },
    [INS_SHL]  = { "shl",  1, { OP_REG8 | OP_V0 } },
    [INS_RND]  = { "rnd",  2, { OP_REG8 | OP_V0, OP_IMM8 } },
    [INS_BCD]  = { "bcd",  1, { OP_REG8 | OP_V0 } },
    [INS_DUMP] = { "dump", 2, { OP_V0, OP_REG8 | OP_V0 } },
    [INS_LOAD] = { "load", 2, { OP_V0, OP_REG8 | OP_V0 } },

    [INS_DRAW] = { "draw", 3, { OP_REG8 | OP_V0, OP_REG8 | OP_V0, 0 } },
    [INS_SKE]  = { "ske",  1, { OP_REG8 | OP_V0 } },
    [INS_SKNE] = { "skne", 1, { OP_REG8 | OP_V0 } },
    [INS_SPR]  = { "spr", 1, { OP_REG8 | OP_V0 } }
};

static bool is_decimal(const char *buf) {
    for(; *buf && isdigit(*buf); buf ++);
    return *buf == '\0';
}

static bool is_hexadecimal(const char *buf) {
    for(; *buf && (isdigit(*buf) || ishex(*buf)); buf ++);
    return *buf == '\0';
}

static bool is_register(const Token *tok) {
    if (strlen(tok->data) == 1 && (tolower(*(tok->data)) == 'i')) return true;
    if (strlen(tok->data) == 2 && (tolower(*(tok->data)) == 'v') && (is_decimal(tok->data + 1) || is_hexadecimal(tok->data + 1))) return true;
    return false;
}

static char *operand_type_as_cstr(const enum Operand_Type type) {
    char temp_buf[260] = { 0 };
    *temp_buf = '\0';

    if (type & OP_KEY)   strcat(temp_buf, "KEY/");
    if (type & OP_REG8)  strcat(temp_buf, "REG8/");
    if (type & OP_IMM8)  strcat(temp_buf, "IMM8/");
    if (type & OP_IMM12) strcat(temp_buf, "IMM12/");
    if (type & OP_REG12) strcat(temp_buf, "REG12/");
    if (type & OP_V0)    strcat(temp_buf, "V0/");
    if (type & OP_SPEC)  strcat(temp_buf, "<expression>/");
    if (type & OP_TIMER) strcat(temp_buf, "DELAY/");

    return strdup(temp_buf);
}

void trim_string_quotes(struct Token *tok) {
    const char beg = tok->data[0], end = tok->data[strlen(tok->data) - 1];

    if ((beg == '\'' && end == '\'') || (beg == '\"' && end == '\"')) {
        memmove(tok->data, tok->data + 1, strlen(tok->data));
        tok->data[strlen(tok->data) - 1] = '\0';
    }
}

static int token_as_instruction_type(struct Token *tok, enum Instruction_Type *type) {
    for (long i = 0; i < (long)(sizeof(asm_info) / sizeof(__typeof__(asm_info[0]))); i ++)
        if (!token_cmp_cstr(tok, asm_info[i].mnemonic)) {
            if (type) *type = (enum Instruction_Type)(i);
            return 0;
        }

    return -1;
}

static void token_as_register(struct Operand *self, struct Token *tok) {
    if (!token_cmp_cstr(tok, "i")) {
        self->type = OP_REG12;
        return;
    }

    const char *tokp = tok->data;
    if (tolower(*tokp++) != 'v')
        return;

    char reg_num_char = tolower(*tokp);
    if (!(reg_num_char >= 'a' && reg_num_char <= 'f') && !(reg_num_char >= '0' && reg_num_char <= '9'))
        ASM_ERROR(tok, "Invalid register specifier: \"%c\".", reg_num_char);

    self->type = OP_REG8;
    if (reg_num_char == '0')
        self->type |= OP_V0;

    self->reg8 = islower(reg_num_char) ? ((reg_num_char - 'a') + 10) : (reg_num_char - '0');
}

void token_as_imm(struct Operand *self, struct Token *tok) {
    char temp_buf[128] = { 0 };
    int tempi;

    strncpy(temp_buf, tok->data, sizeof(temp_buf) - 1);
    if (tok->type == TOKEN_TYPE_HEX) {
        temp_buf[strlen(temp_buf) - 1] = '\0'; /* h */
        if (!is_hexadecimal(temp_buf))
            ASM_ERROR(tok, "Invalid hexadecimal value: \"%s\".", temp_buf);

        if (sscanf(temp_buf, "%x", (unsigned int *)&tempi) != 1)
            if (sscanf(temp_buf, "%X", (unsigned int *)&tempi) != 1)
                ASM_ASSERT_NOT_NULL(0 && "Internal error: sscanf fault.");
    } else {
        if (!is_decimal(temp_buf))
            ASM_ERROR(tok, "Invalid decimal value: \"%s\".", temp_buf);

        tempi = atoi(temp_buf);
    }

    if (tempi >= 0 && tempi <= 0xFF) {
        self->imm8 = (uint8_t)tempi;
        self->type = OP_IMM8;
        return;
    } else if (tempi >= 0 && tempi <= 0xFFF) {
        self->imm12 = (uint16_t)tempi;
        self->type = OP_IMM12;
        return;
    }

    if (warning_info.w_range)
        ASM_WARNING(tok, "Value %x is not in imm8/imm12 range. Defaulting to %x.", tempi, (unsigned)tempi & 0xFFF);
    
    self->imm12 = (uint16_t)(tempi & 0xFFF);
    self->type = OP_IMM12;
}

static void token_as_operand(struct Operand *self, struct Token *tok) {
    ASM_ASSERT_NOT_NULL(self);
    ASM_ASSERT_NOT_NULL(tok);

    self->tok = tok;
    if ((tok->type == TOKEN_TYPE_HEX) || (tok->type == TOKEN_TYPE_DEC)) {
        /* IMM8/IMM12 */
        token_as_imm(self, tok);
        return;
    }

    if (tok->type == TOKEN_TYPE_CHAR) {
        /* Character constant */
        trim_string_quotes(tok);
        self->imm8 = *(uint8_t *)tok->data;
        self->type = OP_IMM8;
        return;
    }

    if (is_register(tok)) {
        /* Register */
        token_as_register(self, tok);
        return;
    }

    if (!token_cmp_cstr(tok, "delay")) {
        /* Timer */
        self->type = OP_TIMER;
        return;
    }

    if (!token_cmp_cstr(tok, "key")) {
        /* Key */
        self->type = OP_KEY;
        return;
    }

    if (tok->type == TOKEN_TYPE_SYMBOLIC) {
        self->is_symbolic = true;
        self->symbolic = tok;
        return;
    }

    ASM_ERROR(tok, "Value \"%s\" cannot be treated as operand.", tok->data);
}

int assemble_instruction(const struct Instruction *instruction, uint16_t *result) {
    const struct Real_Instruction_Info *variantp = asm_real_info[instruction->type];
    union Instruction_Raw stencill;

    for (; variantp->template != TEMPL_NONE; variantp += 1) {
        if (instruction->noperands != variantp->noperands)
            continue;

        bool match = true;
        for (int i = 0; i < instruction->noperands; i ++)
            match &= (instruction->operands[i].type & variantp->types[i]) != 0;
    
        if (match)
            break;
    }

    switch (variantp->template) {
    case TEMPL_ONNN2:
        stencill.ONNN = (__typeof__(stencill.ONNN)){
            .O   = variantp->opcode4,
            .NNN = instruction->operands[1].imm12
        };
        break;

    case TEMPL_ONNN:
        stencill.ONNN = (__typeof__(stencill.ONNN)){
            .O   = variantp->opcode4,
            .NNN = instruction->operands[0].imm12
        };
        break;
    
    case TEMPL_OXNN:
        stencill.OXNN = (__typeof__(stencill.OXNN)){
            .O  = variantp->opcode4,
            .X  = instruction->operands[0].reg8,
            .NN = instruction->operands[1].imm8
        };
        break;

    case TEMPL_OXYS:
        stencill.OXYS = (__typeof__(stencill.OXYS)){
            .O = variantp->opcode4,
            .X = instruction->operands[0].reg8,
            .Y = instruction->operands[1].reg8,
            .S = variantp->suffix
        };
        break;
    
    case TEMPL_OXSS:
        stencill.OXSS = (__typeof__(stencill.OXSS)){
            .O  = variantp->opcode4,
            .X  = instruction->operands[0].reg8,
            .SS = variantp->suffix
        };
        break;

    case TEMPL_OXSS2:
        stencill.OXSS = (__typeof__(stencill.OXSS)){
            .O  = variantp->opcode4,
            .X  = instruction->operands[1].reg8,
            .SS = variantp->suffix
        };
        break;

    case TEMPL_OOOO:
        *(uint16_t *)&stencill = variantp->opcode16;
        break;

    case TEMPL_NONE:
    default:
        return -1;
    }

    if (result)
        *(union Instruction_Raw *)result = stencill;
    return 0;
}

int assemble_block(const struct Block *block, FILE *f) {
    ASM_ASSERT_NOT_NULL(f);
    
    if (!(block->is_data)) {
        uint16_t raw;
        if (assemble_instruction(&(block->ins), &raw) < 0)
            return -1;

        raw = ((raw & 0xFF) << 8) | (raw >> 8);
        fwrite(&raw, 2, 1, f);
        return 0;
    }

    if (block->data.is_reserved) {
        const char *reserved_stub = "Built by ROS-CHIP-8 Assembler", *p = reserved_stub;
        for (size_t i = 0; i < block->data.len; i++) {
            fwrite(p++, 1, 1, f);
            if (p[-1] == '\0')
                p = reserved_stub;
        }
    
        return 0;
    }

    fwrite(block->data.raw, 1, block->data.len, f);
    return 0;
}

static void *push_block(struct Block *base, size_t *n, const struct Block *bl) {
    STACK_PUSH(blocks_cleanup, *bl);
    void *temp;
    ASM_ASSERT((temp = realloc(base, sizeof(struct Block) * ++*n)) != NULL);

    base = temp;
    base[*n - 1] = *bl;
    return base;
}

ASM_WARNING

static inline __attribute__((always_inline)) void constraint_check(const struct Instruction ins) {
    if ((ins.type == INS_SET) && (ins.operands[0].type == OP_REG8) && (ins.operands[1].type == OP_IMM12))
        ASM_ERROR(ins.tok, "Instruction set with operands <reg8>, <imm12> is constraint and cannot be used.");
    if ((ins.type == INS_SET) && (ins.operands[0].type == OP_REG12) && ((ins.operands[1].type == OP_REG8) || (ins.operands[1].type == OP_TIMER) || (ins.operands[1].type == OP_KEY) || (ins.operands[1].type == OP_SPEC)))
        ASM_ERROR(ins.tok, "Instruction set with operands <reg12>, <reg8>/delay/key/<reg8 - dst8> is constraint and cannot be used.");
    if ((ins.type == INS_SET) && (ins.operands[0].type == OP_TIMER) && (ins.operands[1].type != OP_REG8))
        ASM_ERROR(ins.tok, "Instruction set with operands delay, <imm8>/<reg8 - dst8>/<imm12>/delay/key is constraint and cannot be used.");
}

static void block_correct_symbolic(struct Block *bl) {
    for (int op = 0; op < bl->ins.noperands; op ++) {
        if (!bl->ins.operands[op].is_symbolic) continue;

        struct Token *symb = bl->ins.operands[op].symbolic;
        struct Definition *def = definition_lookup(symb);
        if (!def)
            ASM_ERROR(symb, "Unknown name \"%s\".", symb->data);

        bool conv_imm12 = false;
        if ((asm_info[bl->ins.type].types[op] == OP_IMM12) && (def->type == OP_IMM8)) {
            conv_imm12 = true;
            def->type = OP_IMM12;
            def->imm12 = def->imm8;

            if (warning_info.w_conversions)
                ASM_WARNING(symb, "Autoconverted value %x from IMM8 to IMM12.", def->imm8);
        }

        if ((asm_info[bl->ins.type].types[op] & def->type) == 0)
            ASM_ERROR(symb, "Instruction %s requires argument %d to be %s, but argument \"%s\" is %s.", asm_info[bl->ins.type].mnemonic, op + 1, operand_type_as_cstr(asm_info[bl->ins.type].types[op]), symb->data, operand_type_as_cstr(def->type)); 

        bl->ins.operands[op] = (struct Operand) {
            .type = def->type,
            .tok = NULL
        };

       switch (def->type) {
        case OP_REG8 | OP_V0:
        case OP_REG8:
            bl->ins.operands[op].reg8 = def->reg8;
            break;
        case OP_IMM8:
            bl->ins.operands[op].imm8 = def->imm8;
            break;
        case OP_IMM12:
            bl->ins.operands[op].imm12 = def->imm12;
            break;
        default:
            ASM_ERROR(symb, "Cannot parse definition \"%s\" of type %s.", symb->data, operand_type_as_cstr(def->type));
        }
    }

    constraint_check(bl->ins);
}

static void blocks_correct_symbolic(struct Block *bl_arr, size_t nbl) {
    for (struct Block *bl = bl_arr; bl < bl_arr + nbl; bl ++) {
        if (bl->is_data) continue;
        block_correct_symbolic(bl);
    }
}

static void parse_includes(struct Token *root) {
    struct Input input;
    
    for (struct Token *tok = root; tok != NULL; tok = tok->next) {
        if (token_cmp_cstr(tok, "include")) continue;
        const struct Token base = *tok;
        struct Token *const basep = tok;

        tok = tok->next;
        if (tok->type != TOKEN_TYPE_STRING)
            ASM_ERROR(tok, "Include path must be string");

        trim_string_quotes((struct Token *)tok);
        input_create(&input, tok->data);
        STACK_PUSH(inputs_cleanup, input);

        struct Token *include_root = tokenize_data(&input);
        parse_includes(include_root);

        basep->next = include_root;
        for(; include_root->next != NULL; include_root = include_root->next);

        /*                          include    path */
        include_root->next = base . next    -> next;
        tok = include_root;
    }
}

struct Block *blocks_parse(const struct Token *root, size_t *nbl) {
    struct Block *bl_arr = NULL, bl;
    size_t bl_num = 0;

    parse_includes(root);

    for (const struct Token *tok = root; tok != NULL; tok = tok->next) {
        memset(&bl, 0, sizeof(struct Block));

        if (!token_cmp_cstr(tok, "include")) continue;

        int ret_pseudo;
        if ((ret_pseudo = try_parse_pseudo_instruction(&bl, (struct Token **)&tok)) != PSEUDO_INSTRUCTION_FAILURE) {
            if (ret_pseudo == PSEUDO_INSTRUCTION_DO_NOT_PUSH) continue;

            bl_arr = push_block(bl_arr, &bl_num, &bl);
            continue;
        }

        if (token_as_instruction_type(tok, &(bl.ins.type)) < 0) {
            struct Token *label = tok;
            struct Definition *def;

            if ((def = definition_lookup(label)) != NULL)
                ASM_ERROR(label, "Label \"%s\" redefined. Previous definition was on line %u", def->tok->data, def->tok->pos.line);

            if (label->next->type != TOKEN_TYPE_COLUMN)
                ASM_ERROR(label, "Expected \':\' right after \"%s\" label declaration.", label->data);

            tok = tok->next;

            if (!bl_arr) {
                /* No instructions yet */
                defines_stack[defines_stack_size ++] = (struct Definition) {
                    .tok = label,
                    .imm12 = _load_addr,
                    .type = OP_IMM12
                };
                continue;
            }

            create_label(&defines_stack[defines_stack_size ++], bl_arr, bl_arr + bl_num - 1, label);
            continue;
        }

        bl.ins.tok = tok;
        bl.ins.noperands = asm_info[bl.ins.type].noperands;

        bool match = true;
        for (int op = 0; op < asm_info[bl.ins.type].noperands; op ++) {
            ASM_ASSERT_NOT_NULL(tok = tok->next);
            token_as_operand(bl.ins.operands + op, tok);

            if ((op == 1) && ((bl.ins.operands[op].type == OP_IMM8) || (bl.ins.operands[op].is_symbolic)) &&
                 (bl.ins.type == INS_OR || bl.ins.type == INS_AND || bl.ins.type == INS_XOR || bl.ins.type == INS_SUB))
            {
                 if (warning_info.w_separate)
                    ASM_WARNING(bl.ins.tok, "Separated instruction: %s, vf register affected.", asm_info[bl.ins.type].mnemonic);

                /* vF = imm8 */
                struct Instruction temp_instruction = (struct Instruction){
                    .type = INS_SET,
                    .noperands = 2,
                    
                    .operands = {
                        {
                            .type = OP_REG8,
                            .reg8 = 0xF,
                            .tok = NULL
                        },
                        {
                            .type = OP_IMM8,
                            .imm8 = bl.ins.operands[op].imm8,
                            .tok = NULL
                        }
                    }
                };

                if (bl.ins.operands[1].is_symbolic) {
                    temp_instruction.operands[1].is_symbolic = true;
                    temp_instruction.operands[1].symbolic = bl.ins.operands[1].tok;
                }

                struct Block temp_block;
                memset(&temp_block, 0, sizeof(temp_block));
                temp_block.ins = temp_instruction;

                bl_arr = push_block(bl_arr, &bl_num, &temp_block);
            
                /* Operation */
                temp_instruction = (struct Instruction){
                    .type = bl.ins.type,
                    .noperands = 2,

                    .operands = {
                        {
                            .type = OP_REG8,
                            .reg8 = bl.ins.operands[0].reg8,
                            .tok = NULL
                        },
                        {
                            .type = OP_REG8,
                            .reg8 = 0xF,
                            .tok = NULL
                        }
                    }
                };
                temp_block.ins = temp_instruction;

                bl_arr = push_block(bl_arr, &bl_num, &temp_block);
                goto _skip_default_parse_step;
            }

            /* symbolic */
            if (bl.ins.operands[op].is_symbolic)
                continue;

            match &= (asm_info[bl.ins.type].types[op] & bl.ins.operands[op].type) != 0;
            /* IMM8 -> IMM12 ( if instruction requires ) */
            if (!match) {
                if ((asm_info[bl.ins.type].types[op] & OP_IMM12) && bl.ins.operands[op].type == OP_IMM8) {
                    bl.ins.operands[op].type = OP_IMM12;
                    bl.ins.operands[op].imm12 = (uint16_t)(bl.ins.operands[op].imm8);
                }
                match = (asm_info[bl.ins.type].types[op] & bl.ins.operands[op].type) != 0;

                if (warning_info.w_conversions)
                    ASM_WARNING(tok, "Autoconverted value %x from IMM8 to IMM12.", bl.ins.operands[op].imm8);
            }

            if (!match)
                ASM_ERROR(tok, "Instruction %s requires argument %d to be %s, but argument \"%s\" is %s.", asm_info[bl.ins.type].mnemonic, op + 1, operand_type_as_cstr(asm_info[bl.ins.type].types[op]), tok->data, operand_type_as_cstr(bl.ins.operands[op].type));
        }

        constraint_check(bl.ins);
        bl_arr = push_block(bl_arr, &bl_num, &bl);
_skip_default_parse_step:
    }

    blocks_correct_symbolic(bl_arr, bl_num);

    if (nbl)
        *nbl = bl_num;

    return bl_arr;
}