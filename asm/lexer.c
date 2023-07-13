#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#include "lexer.h"
#include "common.h"

static inline __attribute__((always_inline)) char *token_type_as_cstr(const enum Token_Type type) {
    ASM_ASSERT(type >= 0 && type < TOKEN_TYPES_NUMBER);
    
    return (char *[TOKEN_TYPES_NUMBER]){
        "SYMBOLIC",
        "HEX",
        "DEC",
        "STRING",
        "CHAR",
        "COLUMN",
        "OPBRACKET",
        "CLBRACKET",
        "PLUS",
        "MINUS",
        "DIVIDE",
        "MULTIPLY",
        "AT"
    }[(int)type];
}

static inline __attribute__((always_inline)) struct Token *talloc(void){
    return malloc(sizeof(struct Token));
}

char *token_dup_data(struct Token *tok) {
    char *buf;
    ASM_ASSERT_NOT_NULL(buf = malloc(tok->len + 1));

    memcpy(buf, tok->data, tok->len);
    buf[tok->len] = '\0';
    return buf;
}

static char *next_line(const char **stream) {
    if (!**stream)
        return NULL;
    
    const char *streamp = *stream, delim = 
    #if defined(_WIN32)
        '\r';
    #else
        '\n';
    #endif
    char *buf;

    while (*streamp++ != delim);
    size_t buf_size = streamp - *stream;
    ASM_ASSERT_NOT_NULL(buf = malloc(buf_size + 1));

    memcpy(buf, *stream, buf_size + 1);
    buf[buf_size --] = '\0';
    buf[buf_size ] = '\n';

    *stream += streamp - *stream;
    #if defined(_WIN32)
        ++*stream;
    #endif

    return buf;
}

static struct Token *next_token(const char **p, const char *file) {
    struct Token *tok;
    const char *operators = ":()+-/*@", *pp;
    bool character = false;
    long i = 0, j = 0;

    ASM_ASSERT_NOT_NULL(tok = talloc());

    for (j = 0, pp = *p; *pp && strchr(" ,\n\r\t\v\f", *pp); pp++, j++);
    if (*pp == '\0' || *pp == ';')
        return free(tok), NULL;

    *tok = (struct Token){
        .data = (char *)pp,
        .pos = {0, 0},
        .file = file,
        .next = NULL
    };

    switch (*pp) {
    case ':': case '(': case ')': case '+':
    case '-': case '/': case '*': case '@':
        i = tok->len = 1;
        tok->type = (enum Token_Type)(strchr(operators, *pp) - operators + TOKEN_TYPE_COLUMN);
        break;

    case '\'':
        character = true;
        /* fallthru */
    case '\"':
        for (i++; (character) ? (pp[i] != '\'') : (pp[i] != '\"'); i++);
        tok->len = ++i;
        tok->type = (enum Token_Type[2]){ TOKEN_TYPE_STRING, TOKEN_TYPE_CHAR }[character];
        break;

    case '0': case '1': case '2': case '3': case '4': 
    case '5': case '6': case '7': case '8': case '9':
        tok->type = TOKEN_TYPE_DEC;
        for (; isdigit(pp[i]) || ishex(pp[i]); i++);
        if (pp[i] == 'h'){
            tok->type = TOKEN_TYPE_HEX;
            i ++;
        }
        tok->len = i;
        break;

    default:
        for (; islower(pp[i]) || isupper(pp[i]) || isdigit(pp[i]) || pp[i] == '_' || pp[i] == '$'; i++);
        tok->type = TOKEN_TYPE_SYMBOLIC;
        tok->len = i;
        break;
    }

    *p += i + j;
    return tok;
}

void token_dump_list(const struct Token *list) {
    while (list != NULL) {
        printf("%u:%u:%12s:\"%.*s\"\n", list->pos.line, list->pos.character, token_type_as_cstr(list->type), list->len, list->data);
        list = list->next;
    }
}

void token_free_list(struct Token *list) {
    while (list != NULL) {
        struct Token *next_temp = list->next;
        if (list->data)
            free(list->data);
        free(list);
        list = next_temp;
    }
}

static bool is_end(const char *end, const char *line) {
    for(; *line != *end; line ++)
        if (*line == '\r' || *line == '\n') return false;
    return true;
}

struct Token *tokenize_data(const struct Input *input) {
    Position cpos = {0u, 0u};
    const char *line = NULL, *data = input->base;
    struct Token *t, *root, *p;
    t = p = root = NULL;

    while ((line = next_line(&data))) {
        const char *line_origin = line;

        while ((t = next_token(&line, input->file)) != NULL) {
            if (!root){
                root = p = t;
                t->data = token_dup_data(t);
                t->pos = (Position){ cpos.line + 1, cpos.character + 1 };
                continue;
            }

            cpos.character += t->data - line_origin;

            t->data = token_dup_data(t);
            t->pos = (Position){ cpos.line + 1, cpos.character + 1 };
            p->next = t;
            p = t;
        }

        if (is_end(data + strlen(data), line))
            break;

        cpos.character = 0;
        cpos.line ++;
        free((void *)line_origin);
    }

    return root;
} 

void input_delete(struct Input *self) {
    if (!self) return;
    if (self->base) {
        free(self->base);
        self->base = NULL;
    }
}

void input_create(struct Input *self, const char *fname) {
    struct Input input = {
        .file = fname,
        .pos = { 0u, 0u }
    };

    FILE *fhandle = fopen(fname, "rb");
    if (!fhandle) {
        ASM_ASSERT_NOT_NULL(input.file);
        ASM_ERROR(&input, "%s", strerror(errno));
    }

    fseek(fhandle, 0, SEEK_END);
    long fsize = ftell(fhandle);
    ASM_ASSERT(fsize >= 0);
    rewind(fhandle);

    void *data;
    ASM_ASSERT_NOT_NULL(data = malloc(fsize + 1), 
        fclose(fhandle));

    ASM_ASSERT(fsize == (long)fread(data, 1, fsize, fhandle), 
        free(data) COMMA 
        fclose(fhandle));

    ((unsigned char *)data)[fsize] = '\0';
    input.base = data;

    if (self) {
        *self = input;
        return;
    }

    free(data);
    fclose(fhandle);
}