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
inline void _stack_push(Cleanup_Stack * const stack, const void *item, size_t item_size) {
    assert(stack);
    if (!stack->item_size)
        stack->item_size = item_size;

    stack->item_number += 1;
    void *temp = realloc(stack->raw, stack->item_number * stack->item_size);

    if (!temp) {
        free(stack->raw);
        stack->raw = NULL;
        assert(0 && "Internal error: realloc failed.");
    }

    stack->raw = temp;
    memcpy((uint8_t *)stack->raw + (stack->item_number - 1) * stack->item_size, item, stack->item_size);
}

#define STACK_CLEANUP(stack,proc)        _stack_cleanup((Cleanup_Stack *)&(stack),(Cleanup_Stack_Callback)(proc))
static inline void _stack_cleanup(Cleanup_Stack * const stack, Cleanup_Stack_Callback callback) {
    assert(stack != NULL);
    assert(callback != NULL);

    if (stack->item_size == 0 || stack->raw == NULL)
        return;

    for (size_t i = 0; i < stack->item_number; i ++) {
        callback((uint8_t *)stack->raw + i * stack->item_size);
    }

    stack->item_number = stack->item_size = 0ULL;
    stack->raw = NULL;
}

#endif /* _STACK_H */