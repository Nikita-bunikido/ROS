#ifndef _PSEUDOINS_H
#define _PSEUDOINS_H

#include "common.h"
#include "lexer.h"
#include "asm.h"

int try_parse_pseudo_instruction(struct Block *block, struct Token **root);

#endif /* _PSEUDOINS_H */