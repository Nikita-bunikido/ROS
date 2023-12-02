#ifndef _CHIP8_H
#define _CHIP8_H

#include <stdbool.h>
#include <inttypes.h>
#include <setjmp.h>

#include <drivers/memory.h>

#include <ros/ros-for-headers.h>

#define RAM_MAX             0xFFF
#define PROGRAM_START       0x200

#define INVALID_COMMAND     0xFFFF
#define REG_MAX             0x10

#define IMM8(c)             ((c) & 0xFF)
#define IMM12(c)            ((c) & 0xFFF)
#define REG8X(c)            (((c) >> 8) & 0xF)
#define REG8Y(c)            (((c) >> 4) & 0xF)

typedef uint16_t Command;

typedef enum {
    COMMAND_TYPE_CLS = 0,
    COMMAND_TYPE_RET,
    COMMAND_TYPE_CROS_IMM12,
    COMMAND_TYPE_GOTO_IMM12,
    COMMAND_TYPE_CALL_IMM12,
    COMMAND_TYPE_SE_REG8_IMM8,
    COMMAND_TYPE_SNE_REG8_IMM8,
    COMMAND_TYPE_SE_REG8_REG8,
    COMMAND_TYPE_SET_REG8_IMM8,
    COMMAND_TYPE_ADD_REG8_IMM8,
    COMMAND_TYPE_SET_REG8_REG8,
    COMMAND_TYPE_OR_REG8_REG8,
    COMMAND_TYPE_AND_REG8_REG8,
    COMMAND_TYPE_XOR_REG8_REG8,
    COMMAND_TYPE_ADD_REG8_REG8,
    COMMAND_TYPE_SUB_REG8_REG8,
    COMMAND_TYPE_SHR_REG8,
    COMMAND_TYPE_SUB_SPEC,
    COMMAND_TYPE_SHL_REG8,
    COMMAND_TYPE_SNE_REG8_REG8,
    COMMAND_TYPE_SET_REG12_IMM12,
    COMMAND_TYPE_GOTO_SPEC,
    COMMAND_TYPE_RND_IMM8,
    COMMAND_TYPE_DEPRECATED_DRAW_REG8_REG8_IMM4,
    COMMAND_TYPE_DEPRECATED_SKE_REG8,
    COMMAND_TYPE_DEPRECATED_SKNE_REG8,
    COMMAND_TYPE_SET_REG8_DELAY,
    COMMAND_TYPE_SET_REG8_KEY,
    COMMAND_TYPE_SET_DELAY_REG8,
    COMMAND_TYPE_DEPRECATED_SET_SOUND_REG8,
    COMMAND_TYPE_ADD_REG12_REG8,
    COMMAND_TYPE_DEPRECATED_SPR_REG8,
    COMMAND_TYPE_BCD_REG8,
    COMMAND_TYPE_DUMP_REG8,
    COMMAND_TYPE_LOAD_REG8,
    
    COMMANDS_MAX
} Command_Type;

typedef enum {
    EXCEPTION_TYPE_NONE = 0,
    EXCEPTION_TYPE_PC_OUT_OF_BOUNDS,
    EXCEPTION_TYPE_INVALID_COMMAND,
    EXCEPTION_TYPE_DEPRECATED_COMMAND,
    EXCEPTION_TYPE_UNKNOWN_SYSCALL,
    EXCEPTION_TYPE_NOT_IMPLEMENTED_SYSCALL,
    EXCEPTION_TYPE_STACK_SMASHED,
    EXCEPTION_TYPE_STACK_UNDERFLOW,
    EXCEPTION_TYPE_STACK_OVERFLOW
} Exception_Type;

struct PACKED Chip8_Context {
    /* Timers */
    uint8_t delay_timer;
    uint8_t sound_timer;

    /* Internal registers */
    unsigned int STP : 12;
    unsigned int PC  : 12;

    /* General purpose registers */
    unsigned int I  : 12;
    
    union {
        uint8_t raw[REG_MAX];
        struct {
            uint8_t v0, v1, v2, v3, v4, v5, v6, v7, v8, v9,
                    vA, vB, vC, vD, vE, vF;
        };
    } reg;

    /* Misc */
    bool halted;
    uint8_t exit_code;
    
    struct Chip8_Context *child;
    jmp_buf panic_buf;
};

void chip8_init(struct Chip8_Context *, Address);
void chip8_cycle(struct Chip8_Context *);
void chip8_panic(struct Chip8_Context *context, Exception_Type code);

#endif /* _CHIP8_H */