#include <string.h>
#include <setjmp.h>

#include <avr/pgmspace.h>

#include <drivers/memory.h>
#include <ros/chip8.h>
#include <ros/log.h>
#include <ros/video.h>
#include <ros/ros-for-modules.h>

#if defined(_SECURITY_KERNEL_ADVANCED_STACK_PROTECTION)
    #define CHIP8_STACK_CAP 12
    static Address chip8_stack[CHIP8_STACK_CAP] = { 0 };

    #define _GET_REAL_STACK_ADDRESS(idx) (RAM_MAX - CHIP8_STACK_CAP * sizeof(__typeof__(*chip8_stack)) - (idx) * sizeof(__typeof__(*chip8_stack)))

    #define STACK_PROTECTION_SAVE(con)                                          \
    do {                                                                        \
        for (unsigned short i = 0; i < CHIP8_STACK_CAP; i ++)                   \
            chip8_stack[i] = memory_read_word(_GET_REAL_STACK_ADDRESS(i));      \
    } while ( 0 )

    #define STACK_PROTECTION_COMPARE(con)                                       \
    do {                                                                        \
        for (unsigned short i = 0; i < CHIP8_STACK_CAP; i ++)                   \
            if (chip8_stack[i] != memory_read_word(_GET_REAL_STACK_ADDRESS(i))) \
                chip8_panic((con), EXCEPTION_TYPE_STACK_SMASHED);               \
    } while ( 0 )
#else
    #define STACK_PROTECTION_SAVE(...)                 /* Nothing */
    #define STACK_PROTECTION_COMPARE(...)              /* Nothing */
#endif

#if defined(_SECURITY_KERNEL_STACK_PROTECTION)
    #define STACK_PROTECTION_OVERFLOW_CHECK(con)                                \
    do {                                                                        \
        if ((con)->STP <= RAM_MAX - CHIP8_STACK_CAP * 2)                        \
            chip8_panic((con), EXCEPTION_TYPE_STACK_OVERFLOW);                  \
    } while ( 0 )

    #define STACK_PROTECTION_UNDERFLOW_CHECK(con)                               \
    do {                                                                        \
        if ((con)->STP >= RAM_MAX - 2)                                          \
            chip8_panic((con), EXCEPTION_TYPE_STACK_UNDERFLOW);                 \
    } while ( 0 )
#else
    #define STACK_PROTECTION_OVERFLOW_CHECK(...)       /* Nothing */
    #define STACK_PROTECTION_UNDERFLOW_CHECK(...)      /* Nothing */
#endif

#define PUSH(con)       do{ STACK_PROTECTION_OVERFLOW_CHECK(con); memory_write_word((Address)((con)->STP), (con)->PC); (con)->STP -= 2; STACK_PROTECTION_SAVE(con); }while( 0 )
#define POP(con)        do{ STACK_PROTECTION_UNDERFLOW_CHECK(con); STACK_PROTECTION_COMPARE(con); (con)->STP += 2; (con)->PC = memory_read_word((con)->STP); }while( 0 )

extern int chip8_perform_syscall(struct Chip8_Context *, uint16_t);

static const struct PACKED Command_Lookup {
    Command mask, template;
} command_lookup_table[] PROGMEM = {
    [COMMAND_TYPE_RET] =                            { 0xFFFF, 0x00EE },
    [COMMAND_TYPE_CLS] =                            { 0xFFFF, 0x00E0 },
    [COMMAND_TYPE_CROS_IMM12] =                     { 0xF000, 0x0000 },
    [COMMAND_TYPE_GOTO_IMM12] =                     { 0xF000, 0x1000 },
    [COMMAND_TYPE_CALL_IMM12] =                     { 0xF000, 0x2000 },
    [COMMAND_TYPE_SE_REG8_IMM8] =                   { 0xF000, 0x3000 },
    [COMMAND_TYPE_SNE_REG8_IMM8] =                  { 0xF000, 0x4000 },
    [COMMAND_TYPE_SE_REG8_REG8] =                   { 0xF00F, 0x5000 },
    [COMMAND_TYPE_SET_REG8_IMM8] =                  { 0xF000, 0x6000 },
    [COMMAND_TYPE_ADD_REG8_IMM8] =                  { 0xF000, 0x7000 },
    [COMMAND_TYPE_SET_REG8_REG8] =                  { 0xF00F, 0x8000 },
    [COMMAND_TYPE_OR_REG8_REG8] =                   { 0xF00F, 0x8001 },
    [COMMAND_TYPE_AND_REG8_REG8] =                  { 0xF00F, 0x8002 },
    [COMMAND_TYPE_XOR_REG8_REG8] =                  { 0xF00F, 0x8003 },
    [COMMAND_TYPE_ADD_REG8_REG8] =                  { 0xF00F, 0x8004 },
    [COMMAND_TYPE_SUB_REG8_REG8] =                  { 0xF00F, 0x8005 },
    [COMMAND_TYPE_SHR_REG8] =                       { 0xF00F, 0x8006 },
    [COMMAND_TYPE_SUB_SPEC] =                       { 0xF00F, 0x8007 },
    [COMMAND_TYPE_SHL_REG8] =                       { 0xF00F, 0x800E },
    [COMMAND_TYPE_SNE_REG8_REG8] =                  { 0xF00F, 0x9000 },
    [COMMAND_TYPE_SET_REG12_IMM12] =                { 0xF000, 0xA000 },
    [COMMAND_TYPE_GOTO_SPEC] =                      { 0xF000, 0xB000 },
    [COMMAND_TYPE_RND_IMM8] =                       { 0xF000, 0xC000 },
    [COMMAND_TYPE_DEPRECATED_DRAW_REG8_REG8_IMM4] = { 0xF000, 0xD000 },
    [COMMAND_TYPE_DEPRECATED_SKE_REG8] =            { 0xF0FF, 0xE09E },
    [COMMAND_TYPE_DEPRECATED_SKNE_REG8] =           { 0xF0FF, 0xE0A1 },
    [COMMAND_TYPE_SET_REG8_DELAY] =                 { 0xF0FF, 0xF007 },
    [COMMAND_TYPE_SET_REG8_KEY] =                   { 0xF0FF, 0xF00A },
    [COMMAND_TYPE_SET_DELAY_REG8] =                 { 0xF0FF, 0xF015 },
    [COMMAND_TYPE_DEPRECATED_SET_SOUND_REG8] =      { 0xF0FF, 0xF018 },
    [COMMAND_TYPE_ADD_REG12_REG8] =                 { 0xF0FF, 0xF01E },
    [COMMAND_TYPE_DEPRECATED_SPR_REG8] =            { 0xF0FF, 0xF029 },
    [COMMAND_TYPE_BCD_REG8] =                       { 0xF0FF, 0xF033 },
    [COMMAND_TYPE_DUMP_REG8] =                      { 0xF0FF, 0xF055 },
    [COMMAND_TYPE_LOAD_REG8] =                      { 0xF0FF, 0xF065 },
};

void chip8_panic(struct Chip8_Context *context, Exception_Type code) {
    context->halted = true;
    ros_log(LOG_TYPE_ERROR, "Runtime exception has occured. Context dumped. Aborted.");
    ros_printf(ATTRIBUTE_DEFAULT, "Code <%x>\n", code);

    ros_printf(ATTRIBUTE_DEFAULT, "dt:%x\nst:%x\npc:%x\nsp:%x\ni:%x\n", context->delay_timer,
                                                                                         context->sound_timer,
                                                                                         context->PC,
                                                                                         context->STP,
                                                                                         context->I);
    for (unsigned short i = 0; i < REG_MAX; i++)
        ros_printf(ATTRIBUTE_DEFAULT, "%x,", context->reg.raw[i]);

    context->exit_code = 1;
    longjmp(context->panic_buf, ~0);
}

static Command peek_command(struct Chip8_Context *context) {
    if (context->PC >= RAM_MAX - 2)
        chip8_panic(context, EXCEPTION_TYPE_PC_OUT_OF_BOUNDS);

    Command com = ((uint16_t)memory_read(context->PC) << 8) | (uint16_t)memory_read(context->PC + 1);
    context->PC += 2;
    return com;
}

static Command_Type get_command_type(const Command com){
    Command_Type i;
    for (i = COMMAND_TYPE_CLS; i < ARR_SIZE(command_lookup_table); i ++)
        if  ((com & pgm_read_word(&command_lookup_table[i].mask)) == pgm_read_word(&command_lookup_table[i].template))
            break;

    return i;
}

static void chip8_execute_command(struct Chip8_Context * const context, Command com, Command_Type type) {
    unsigned sum;
    uint8_t v1, v2;
    div_t div_result;
    
    switch (type) {
    case COMMAND_TYPE_CLS:
        clear_screen(0x0000);
        break;

    case COMMAND_TYPE_RET:
        POP(context);
        break;
    
    case COMMAND_TYPE_CROS_IMM12:
       chip8_perform_syscall(context, IMM12(com));
       break;

    case COMMAND_TYPE_CALL_IMM12:
        PUSH(context);

        /* Fallthru */    
    case COMMAND_TYPE_GOTO_IMM12:
        context->PC = IMM12(com);
        break;

    case COMMAND_TYPE_SE_REG8_IMM8: context->PC += sizeof(Command) * (context->reg.raw[REG8X(com)] == IMM8(com)); break;
    case COMMAND_TYPE_SE_REG8_REG8: context->PC += sizeof(Command) * (context->reg.raw[REG8X(com)] == context->reg.raw[REG8Y(com)]); break;
    
    case COMMAND_TYPE_SNE_REG8_IMM8: context->PC += sizeof(Command) * (context->reg.raw[REG8X(com)] != IMM8(com)); break;
    case COMMAND_TYPE_SNE_REG8_REG8: context->PC += sizeof(Command) * (context->reg.raw[REG8X(com)] != context->reg.raw[REG8Y(com)]); break;

    case COMMAND_TYPE_SET_REG8_IMM8: context->reg.raw[REG8X(com)] = IMM8(com); break;
    case COMMAND_TYPE_SET_REG8_REG8: context->reg.raw[REG8X(com)] = context->reg.raw[REG8Y(com)]; break;
    case COMMAND_TYPE_SET_REG12_IMM12: context->I = IMM12(com); break;
    case COMMAND_TYPE_SET_REG8_DELAY: context->reg.raw[REG8X(com)] = context->delay_timer; break;
    case COMMAND_TYPE_SET_REG8_KEY: /* TODO: call api */ break;
    case COMMAND_TYPE_SET_DELAY_REG8: context->delay_timer = context->reg.raw[REG8X(com)]; break;

    case COMMAND_TYPE_ADD_REG8_IMM8: context->reg.raw[REG8X(com)] += IMM8(com); break;
    case COMMAND_TYPE_ADD_REG8_REG8:
        sum = context->reg.raw[REG8X(com)] + context->reg.raw[REG8Y(com)];
        context->reg.vF = sum > 0xFF;
        context->reg.raw[REG8X(com)] = (uint8_t)(sum & 0xFF);
        break;
    
    case COMMAND_TYPE_ADD_REG12_REG8:
        context->I += context->reg.raw[REG8X(com)];
        break;

    case COMMAND_TYPE_OR_REG8_REG8: context->reg.raw[REG8X(com)] |= context->reg.raw[REG8Y(com)]; break;
    case COMMAND_TYPE_AND_REG8_REG8: context->reg.raw[REG8X(com)] &= context->reg.raw[REG8Y(com)]; break;
    case COMMAND_TYPE_XOR_REG8_REG8: context->reg.raw[REG8X(com)] ^= context->reg.raw[REG8Y(com)]; break;

    case COMMAND_TYPE_SUB_REG8_REG8:
        v1 = context->reg.raw[REG8X(com)];
        v2 = context->reg.raw[REG8Y(com)];
        context->reg.vF = v1 > v2;
        context->reg.raw[REG8X(com)] = v1 - v2;
        break;

    case COMMAND_TYPE_SUB_SPEC:
        v1 = context->reg.raw[REG8X(com)];
        v2 = context->reg.raw[REG8Y(com)];
        context->reg.vF = v2 > v1;
        context->reg.raw[REG8X(com)] = v2 - v1;
        break;

    case COMMAND_TYPE_SHR_REG8:
        context->reg.vF = context->reg.raw[REG8X(com)] & 1;
        context->reg.raw[REG8X(com)] >>= 1;
        break;

    case COMMAND_TYPE_SHL_REG8:
        context->reg.vF = context->reg.raw[REG8X(com)] >> 7;
        context->reg.raw[REG8X(com)] <<= 1;
        break;

    case COMMAND_TYPE_RND_IMM8:
        context->reg.raw[REG8X(com)] = rand() & IMM8(com);
        break;

    case COMMAND_TYPE_GOTO_SPEC:
        break;

    case COMMAND_TYPE_BCD_REG8:
        v1 = context->reg.raw[REG8X(com)];
        div_result = div(v1, 100);

        memory_write(context->I, div_result.quot);
        div_result = div(div_result.rem, 10);
        memory_write(context->I + 1, div_result.quot);
        memory_write(context->I + 2, div_result.rem);
        break;

    case COMMAND_TYPE_DUMP_REG8:
        for (uint8_t i = 0; i <= REG8X(com); i ++)
            memory_write(context->I + i, context->reg.raw[i]);
        break;

    case COMMAND_TYPE_LOAD_REG8:
        for (uint8_t i = 0; i <= REG8X(com); i ++)
            context->reg.raw[i] = memory_read(context->I + i);
        break;

    case COMMAND_TYPE_DEPRECATED_SET_SOUND_REG8:
    case COMMAND_TYPE_DEPRECATED_DRAW_REG8_REG8_IMM4:
    case COMMAND_TYPE_DEPRECATED_SKE_REG8:
    case COMMAND_TYPE_DEPRECATED_SKNE_REG8:
    case COMMAND_TYPE_DEPRECATED_SPR_REG8:
        chip8_panic(context, EXCEPTION_TYPE_DEPRECATED_COMMAND);
        UNREACHABLE();

    default:
        chip8_panic(context, EXCEPTION_TYPE_INVALID_COMMAND);
        break;
    }
}

void chip8_init(struct Chip8_Context *context, Address entry) {
    memset(context, 0, sizeof(*context));
    
    context->STP = RAM_MAX - 2;
    context->PC = entry;
}

void chip8_cycle(struct Chip8_Context *context) {
    Command com = peek_command(context);
    chip8_execute_command(context, com, get_command_type(com));

    context->delay_timer -= ( context->delay_timer > 0 );
}