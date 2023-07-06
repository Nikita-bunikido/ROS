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

#define MAX_VARIANTS    12

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

    if (strlen(tok->data) <= 2) {
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

int assemble_block(const struct Block *block) {
    if (!(block->is_data)) {
        uint16_t raw;
        if (assemble_instruction(&(block->ins), &raw) < 0)
            return -1;

        fprintf(stdout, "%02X %02X\n", raw >> 8, raw & 0xFF);
        return 0;
    }

    if (block->data.is_reserved) {
        const char *reserved_stub = "Built by ROS-CHIP-8", *p = reserved_stub;
        for (size_t i = 0; i < block->data.len; i++) {
            fprintf(stdout, "%02X ", *p++);
            if (p[-1] == '\0')
                p = reserved_stub;
        }
    
        fputc('\n', stdout);
        return 0;
    }

    for (size_t i = 0; i < block->data.len; i++)
        fprintf(stdout, "%02X ", block->data.raw[i]);

    fputc('\n', stdout);
    return 0;
}

static void *push_block(struct Block *base, size_t *n, const struct Block *bl) {
    void *temp;
    ASM_ASSERT((temp = realloc(base, sizeof(struct Block) * ++*n)) != NULL, 
        if (base){ free(base); base = NULL; });

    base = temp;
    base[*n - 1] = *bl;
    return base;
}

struct Block *blocks_parse(const struct Token *root, size_t *nbl) {
    struct Block *bl_arr = NULL, bl;
    size_t bl_num = 0;

    for (const struct Token *tok = root; tok != NULL; tok = tok->next) {
        memset(&bl, 0, sizeof(struct Block));
        
        if (try_parse_pseudo_instruction(&bl, &tok) >= 0) {
            bl_arr = push_block(bl_arr, &bl_num, &bl);
            continue;
        }

        bl.ins.tok = tok;
        if (token_as_instruction_type(bl.ins.tok, &(bl.ins.type)) < 0)
            ASM_ERROR(bl.ins.tok, "Invalid instruction: \"%s\".", bl.ins.tok->data);

        bl.ins.noperands = asm_info[bl.ins.type].noperands;

        bool match = true;
        for (int op = 0; op < asm_info[bl.ins.type].noperands; op ++) {
            ASM_ASSERT_NOT_NULL(tok = tok->next);
            token_as_operand(bl.ins.operands + op, tok);

            /* Check for vF temporary use */
            if ((op == 1) && (bl.ins.operands[op].type == OP_IMM8) &&
                (bl.ins.type == INS_OR || bl.ins.type == INS_AND || bl.ins.type == INS_XOR || bl.ins.type == INS_SUB))
            {
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

            match &= (asm_info[bl.ins.type].types[op] & bl.ins.operands[op].type) != 0;
            /* IMM8 -> IMM12 ( if instruction requires ) */
            if (!match) {
                if ((asm_info[bl.ins.type].types[op] & OP_IMM12) && bl.ins.operands[op].type == OP_IMM8) {
                    bl.ins.operands[op].type = OP_IMM12;
                    bl.ins.operands[op].imm12 = (uint16_t)(bl.ins.operands[op].imm8);
                }
                match = (asm_info[bl.ins.type].types[op] & bl.ins.operands[op].type) != 0;

                ASM_WARNING(tok, "Autoconverted value %x from IMM8 to IMM12.", bl.ins.operands[op].imm8);
            }

            if (!match)
                ASM_ERROR(tok, "Instruction %s requires argument %d to be %s, but argument \"%s\" is %s.", asm_info[bl.ins.type].mnemonic, op + 1, operand_type_as_cstr(asm_info[bl.ins.type].types[op]), tok->data, operand_type_as_cstr(bl.ins.operands[op].type));
        }

        /* Constraints */
        if ((bl.ins.type == INS_SET) && (bl.ins.operands[0].type == OP_REG8) && (bl.ins.operands[1].type == OP_IMM12))
            ASM_ERROR(tok, "Instruction set with operands <reg8>, <imm12> is constraint and cannot be used.");
        if ((bl.ins.type == INS_SET) && (bl.ins.operands[0].type == OP_REG12) && ((bl.ins.operands[1].type == OP_REG8) || (bl.ins.operands[1].type == OP_TIMER) || (bl.ins.operands[1].type == OP_KEY) || (bl.ins.operands[1].type == OP_SPEC)))
            ASM_ERROR(tok, "Instruction set with operands <reg12>, <reg8>/delay/key/<reg8 - dst8> is constraint and cannot be used.");
        if ((bl.ins.type == INS_SET) && (bl.ins.operands[0].type == OP_TIMER) && (bl.ins.operands[1].type != OP_REG8))
            ASM_ERROR(tok, "Instruction set with operands delay, <imm8>/<reg8 - dst8>/<imm12>/delay/key is constraint and cannot be used.");

        bl_arr = push_block(bl_arr, &bl_num, &bl);
_skip_default_parse_step:
    }

    if (nbl)
        *nbl = bl_num;

    return bl_arr;
}