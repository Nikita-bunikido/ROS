#ifndef _PSEUDOINS_H
#define _PSEUDOINS_H

#include "common.h"
#include "lexer.h"
#include "asm.h"

enum Pseudo_Instruction_Result {
    PSEUDO_INSTRUCTION_SUCCESS = 0,
    PSEUDO_INSTRUCTION_DO_NOT_PUSH,
    PSEUDO_INSTRUCTION_FAILURE
};

int try_parse_pseudo_instruction(struct Block *block, struct Token **root);

#endif /* _PSEUDOINS_H */