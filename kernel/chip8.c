#include <string.h>
#include <setjmp.h>

#include <avr/pgmspace.h>

#include "chip8.h"
#include "log.h"
#include "memory.h"
#include "video.h"
#include "ros.h"

jmp_buf chip8_panic_buf;

void __callback impl_ret(struct Chip8_Context * const, Command);
void __callback impl_cls(struct Chip8_Context * const, Command);
void __callback impl_cros_imm12(struct Chip8_Context * const, Command);
void __callback impl_goto_imm12(struct Chip8_Context * const, Command);
void __callback impl_call_imm12(struct Chip8_Context * const, Command);
void __callback impl_se_reg8_imm8(struct Chip8_Context * const, Command);
void __callback impl_sne_reg8_imm8(struct Chip8_Context * const, Command);
void __callback impl_se_reg8_reg8(struct Chip8_Context * const, Command);
void __callback impl_set_reg8_imm8(struct Chip8_Context * const, Command);
void __callback impl_add_reg8_imm8(struct Chip8_Context * const, Command);
void __callback impl_set_reg8_reg8(struct Chip8_Context * const, Command);
void __callback impl_or_reg8_reg8(struct Chip8_Context * const, Command);
void __callback impl_and_reg8_reg8(struct Chip8_Context * const, Command);
void __callback impl_xor_reg8_reg8(struct Chip8_Context * const, Command);
void __callback impl_add_reg8_reg8(struct Chip8_Context * const, Command);
void __callback impl_sub_reg8_reg8(struct Chip8_Context * const, Command);
void __callback impl_shr_reg8(struct Chip8_Context * const, Command);
void __callback impl_sub_spec(struct Chip8_Context * const, Command);
void __callback impl_shl_reg8(struct Chip8_Context * const, Command);
void __callback impl_sne_reg8_reg8(struct Chip8_Context * const, Command);
void __callback impl_set_reg12_imm12(struct Chip8_Context * const, Command);
void __callback impl_goto_spec(struct Chip8_Context * const, Command);
void __callback impl_rnd_imm8(struct Chip8_Context * const, Command);
void __callback impl_deprecated_draw_reg8_reg8_imm4(struct Chip8_Context * const, Command);
void __callback impl_deprecated_ske_reg8(struct Chip8_Context * const, Command);
void __callback impl_deprecated_skne_reg8(struct Chip8_Context * const, Command);
void __callback impl_set_reg8_delay(struct Chip8_Context * const, Command);
void __callback impl_set_reg8_key(struct Chip8_Context * const, Command);
void __callback impl_set_delay_reg8(struct Chip8_Context * const, Command);
void __callback impl_deprecated_set_sound_reg8(struct Chip8_Context * const, Command);
void __callback impl_add_reg12_reg8(struct Chip8_Context * const, Command);
void __callback impl_deprecated_spr_reg8(struct Chip8_Context * const, Command);
void __callback impl_bcd_reg8(struct Chip8_Context * const, Command);
void __callback impl_dump_reg8(struct Chip8_Context * const, Command);
void __callback impl_load_reg8(struct Chip8_Context * const, Command);

typedef void (*__callback Command_Impl)(struct Chip8_Context * const, Command);

static const Command_Impl command_impl[COMMANDS_MAX] PROGMEM = {
    [COMMAND_TYPE_RET] =                            impl_ret,
    [COMMAND_TYPE_CLS] =                            impl_cls,
    [COMMAND_TYPE_CROS_IMM12] =                     impl_cros_imm12,
    [COMMAND_TYPE_GOTO_IMM12] =                     impl_goto_imm12,
    [COMMAND_TYPE_CALL_IMM12] =                     impl_call_imm12,
    [COMMAND_TYPE_SE_REG8_IMM8] =                   impl_se_reg8_imm8,
    [COMMAND_TYPE_SNE_REG8_IMM8] =                  impl_sne_reg8_imm8,
    [COMMAND_TYPE_SE_REG8_REG8] =                   impl_se_reg8_reg8,
    [COMMAND_TYPE_SET_REG8_IMM8] =                  impl_set_reg8_imm8,
    [COMMAND_TYPE_ADD_REG8_IMM8] =                  impl_add_reg8_imm8,
    [COMMAND_TYPE_SET_REG8_REG8] =                  impl_set_reg8_reg8,
    [COMMAND_TYPE_OR_REG8_REG8] =                   impl_or_reg8_reg8,
    [COMMAND_TYPE_AND_REG8_REG8] =                  impl_and_reg8_reg8,
    [COMMAND_TYPE_XOR_REG8_REG8] =                  impl_xor_reg8_reg8,
    [COMMAND_TYPE_ADD_REG8_REG8] =                  impl_add_reg8_reg8,
    [COMMAND_TYPE_SUB_REG8_REG8] =                  impl_sub_reg8_reg8,
    [COMMAND_TYPE_SHR_REG8] =                       impl_shr_reg8,
    [COMMAND_TYPE_SUB_SPEC] =                       impl_sub_spec,
    [COMMAND_TYPE_SHL_REG8] =                       impl_shl_reg8,
    [COMMAND_TYPE_SNE_REG8_REG8] =                  impl_sne_reg8_reg8,
    [COMMAND_TYPE_SET_REG12_IMM12] =                impl_set_reg12_imm12,
    [COMMAND_TYPE_GOTO_SPEC] =                      impl_goto_spec,
    [COMMAND_TYPE_RND_IMM8] =                       impl_rnd_imm8,
    [COMMAND_TYPE_DEPRECATED_DRAW_REG8_REG8_IMM4] = impl_deprecated_draw_reg8_reg8_imm4,
    [COMMAND_TYPE_DEPRECATED_SKE_REG8] =            impl_deprecated_ske_reg8,
    [COMMAND_TYPE_DEPRECATED_SKNE_REG8] =           impl_deprecated_skne_reg8,
    [COMMAND_TYPE_SET_REG8_DELAY] =                 impl_set_reg8_delay,
    [COMMAND_TYPE_SET_REG8_KEY] =                   impl_set_reg8_key,
    [COMMAND_TYPE_SET_DELAY_REG8] =                 impl_set_delay_reg8,
    [COMMAND_TYPE_DEPRECATED_SET_SOUND_REG8] =      impl_deprecated_set_sound_reg8,
    [COMMAND_TYPE_ADD_REG12_REG8] =                 impl_add_reg12_reg8,
    [COMMAND_TYPE_DEPRECATED_SPR_REG8] =            impl_deprecated_spr_reg8,
    [COMMAND_TYPE_BCD_REG8] =                       impl_bcd_reg8,
    [COMMAND_TYPE_DUMP_REG8] =                      impl_dump_reg8,
    [COMMAND_TYPE_LOAD_REG8] =                      impl_load_reg8,
};

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
    ros_printf(ATTRIBUTE_DEFAULT, "*** STOP <%x>\n", code);

    ros_printf(ATTRIBUTE_DEFAULT, "dt:%x\nst:%x\npc:%x\nsp:%x\ni:%x\n", context->delay_timer,
                                                                                         context->sound_timer,
                                                                                         context->PC,
                                                                                         context->STP,
                                                                                         context->I);
    for (unsigned short i = 0; i < REG_MAX; i++)
        ros_printf(ATTRIBUTE_DEFAULT, "%x,", context->reg.raw[i]);
    
    context->exit_code = 1;
    longjmp(chip8_panic_buf, ~0);
}

static Command peek_command(struct Chip8_Context *context) {
    if (context->PC < PROGRAM_START || context->PC >= RAM_MAX - 2)
        chip8_panic(context, EXCEPTION_TYPE_PC_OUT_OF_BOUNDS);

    Command com = ((uint16_t)memory_read((Address)(context->PC)) << 8) | ((uint16_t)memory_read((Address)(context->PC + 1)));
    context->PC += 2;
    return com;
}

static Command_Type get_command_type(const Command com){
    Command_Type i = COMMAND_TYPE_CLS;
    for (; i < ARR_SIZE(command_lookup_table); i ++)
        if  ((com & ((Command)pgm_read_word(&command_lookup_table[i].mask))) == (Command)pgm_read_word(&command_lookup_table[i].template)) break;

    return i;
}

void chip8_init(struct Chip8_Context *context) {
    memset(context, 0, sizeof(*context));
    
    context->STP  = RAM_MAX - 2;
    context->PC   = PROGRAM_START;
}

void chip8_cycle(struct Chip8_Context *context) {
    Command com = peek_command(context);

    if (get_command_type(com) >= COMMANDS_MAX)
        chip8_panic(context, EXCEPTION_TYPE_INVALID_COMMAND);
 
    ((Command_Impl)(pgm_read_word(&command_impl[get_command_type(com)])))(context, com);
}