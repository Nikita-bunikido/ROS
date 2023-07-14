#ifndef _STACK_H
#define _STACK_H

#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stddef.h>

typedef void (* Cleanup_Stack_Callback)(void *item);

typedef struct Cleanup_Stack Cleanup_Stack;
struct __attribute__((__packed__)) Cleanup_Stack {
    size_t item_number;
    size_t item_size;
    void *raw;
};

#define STACK_PUSH(stack,item)      _stack_push((Cleanup_Stack *)&(stack), (const void *)&(item), sizeof(__typeof__((item))))
void _stack_push(Cleanup_Stack * const, const void *, size_t);

#define STACK_CLEANUP(stack,proc)        _stack_cleanup((Cleanup_Stack *)&(stack),(Cleanup_Stack_Callback)(proc))
void _stack_cleanup(Cleanup_Stack * const, Cleanup_Stack_Callback);

#endif /* _STACK_H */