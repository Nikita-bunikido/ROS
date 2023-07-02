#ifndef _LEXER_H
#define _LEXER_H

#include <stddef.h>
#include "common.h"

enum Token_Type {
    TOKEN_TYPE_SYMBOLIC = 0,
    /* Constants */
    TOKEN_TYPE_HEX,
    TOKEN_TYPE_DEC,

    /* Characters */
    TOKEN_TYPE_STRING,
    TOKEN_TYPE_CHAR,

    /* Operators */
    TOKEN_TYPE_COLUMN,
    TOKEN_TYPE_OPBRACKET,
    TOKEN_TYPE_CLBRACKET,
    TOKEN_TYPE_PLUS,
    TOKEN_TYPE_MINUS,
    TOKEN_TYPE_DIVIDE,
    TOKEN_TYPE_MULTIPLY,
};

typedef struct Token Token;
struct Token {
    char *data;
    size_t len;
    enum Token_Type type;

    Position pos;
    struct Token *next;
};

void input_delete(struct Input *);
void input_create(struct Input *, const char *);

void token_dump_list(const struct Token *);
void token_free_list(struct Token *);
struct Token *tokenize_data(const char *);

#endif /* _LEXER_H */