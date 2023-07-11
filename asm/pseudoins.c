#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "common.h"
#include "lexer.h"
#include "asm.h"
#include "pseudoins.h"

#define CHUNK_SIZE      512

#define DATASEG_PUSH_ARRAY(data,array,real,n)   _dataseg_push_value(&(data),(array),(n),&(real))
#define DATASEG_PUSH_VALUE(data,value,real)     _dataseg_push_value(&(data),&(value),sizeof(value),&(real))
static void _dataseg_push_value(struct Dataseg *data, const void *value, size_t sizeof_value, size_t *real_len) {
    if ((data->len + sizeof_value) >= (*real_len - 1)) {
        void *temp = realloc(data->raw, *real_len + CHUNK_SIZE);
        *real_len += CHUNK_SIZE;

        if (!temp) {
            free(data->raw);
            data->raw = NULL;
            ASM_ASSERT_NOT_NULL(0 && "Internal error: realloc failed.");
        }

        data->raw = temp;
    }
    
    memcpy(data->raw + data->len, value, sizeof_value);
    data->len += sizeof_value;
}

static void parse_pseudo_instruction_reserve(struct Block *block, struct Token **root) {
    struct Token *t = *root;
    struct Operand temp;

    block->is_data = true;
    ASM_ASSERT_NOT_NULL(t = t->next);

    token_as_imm(&temp, t);
    block->data = (struct Dataseg){
        .is_reserved = true,
        .len = (temp.type == OP_IMM12) ? (size_t)(temp.imm12) : (size_t)(temp.imm8)
    };

    *root = t;
}

static void parse_pseudo_instruction_dataseg(struct Block *block, struct Token **root) {
    struct Token *t = *root;
    struct Operand temp;
    size_t real_len;

    block->is_data = true;
    block->data.raw = malloc(CHUNK_SIZE);
    ASM_ASSERT_NOT_NULL(block->data.raw);
    
    block->data.len = 0ULL;
    real_len = CHUNK_SIZE;

    ASM_ASSERT_NOT_NULL(t = t->next);

    for(; t != NULL; t = t->next) {
        if (!token_cmp_cstr(t, "dataend"))
            break;
        
        switch (t->type) {
        case TOKEN_TYPE_HEX:
        case TOKEN_TYPE_DEC:
            token_as_imm(&temp, t);

            if (temp.type == OP_IMM8) {
                DATASEG_PUSH_VALUE(block->data, temp.imm8, real_len);
                break;
            }

            ASM_ERROR(t, "IMM12 constant cannot be used in data segment declaration.");

        case TOKEN_TYPE_STRING:
        case TOKEN_TYPE_CHAR:
            if (strlen(t->data) == 2)
                ASM_ERROR(t, "Empty %s literal: \"\" in data segment declaration.", t->type == TOKEN_TYPE_CHAR ? "character" : "string");

            trim_string_quotes(t);

            if (t->type == TOKEN_TYPE_CHAR) {
                DATASEG_PUSH_VALUE(block->data, t->data[0], real_len);
                break;
            }

            DATASEG_PUSH_ARRAY(block->data, t->data, real_len, strlen(t->data));
            break;

        default:
            ASM_ERROR(t, "Unknown name: \"%s\" in data segment declaration.", t->data);
            break;
        }
    }

    *root = t;
}

static void parse_pseudo_instruction_define(struct Block *block, struct Token **root) {
    struct Token *t = *root;
    struct Operand temp;
    (void) block;
    
    ASM_ASSERT_NOT_NULL(t = t->next);
    struct Definition def = (struct Definition) {
        .tok = t
    };

    ASM_ASSERT_NOT_NULL(t = t->next);
    switch (t->type) {
    case TOKEN_TYPE_HEX:
    case TOKEN_TYPE_DEC:
        token_as_imm(&temp, t);
    
        if (temp.type == OP_IMM8) {
            def.imm8 = temp.imm8;
            def.type = OP_IMM8;
            break;
        }

        def.imm12 = temp.imm12;
        def.type = OP_IMM12;
        break;

    case TOKEN_TYPE_CHAR:
        if (strlen(t->data) == 2)
            ASM_ERROR(t, "Empty character literal: \"\" in data segment declaration.");

        trim_string_quotes(t);        

        def.imm8 = (uint8_t)*(t->data);
        def.type = OP_IMM8;
        break;

    default:
        ASM_ERROR(t, "Unknown name: \"%s\" in define statement.", t->data);
    }

    defenition_push(&def);
    *root = t;
}

int try_parse_pseudo_instruction(struct Block *block, struct Token **root) {
    if (!token_cmp_cstr(*root, "dataseg")) {
        parse_pseudo_instruction_dataseg(block, root);
        return PSEUDO_INSTRUCTION_SUCCESS;
    }

    if (!token_cmp_cstr(*root, "reserve")) {
        parse_pseudo_instruction_reserve(block, root);
        return PSEUDO_INSTRUCTION_SUCCESS;
    }

    if (!token_cmp_cstr(*root, "define")) {
        parse_pseudo_instruction_define(block, root);
        return PSEUDO_INSTRUCTION_DO_NOT_PUSH;
    }

    return PSEUDO_INSTRUCTION_FAILURE;
}