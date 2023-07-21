#include <avr/pgmspace.h>
#include <util/delay.h>

#include <stdarg.h>
#include <stdbool.h>
#include <math.h>

#include "keyboard.h"
#include "log.h"
#include "video.h"
#include "memory.h"
#include "chip8.h"
#include "ros.h"

#define IMM8(c)         ((c) & 0xFF)
#define IMM12(c)        ((c) & 0xFFF)
#define REG8X(c)        (((c) >> 8) & 0xF)
#define REG8Y(c)        (((c) >> 4) & 0xF)

#define PUSH(con)       do{ memory_write_word((Address)((con)->STP), (uint16_t)((con)->PC)); (con)->STP -= 2; }while( 0 )
static inline __attribute__((always_inline)) void memory_write_word(Address addr, uint16_t word) {
    memory_write(addr, (uint8_t)(word & 0xFF));
    addr ++;
    memory_write(addr, (uint8_t)(word >> 8));
}

#define POP(con)        do{ (con)->STP += 2; (con)->PC = memory_read_word((Address)((con)->STP)); }while( 0 )
static inline __attribute__((always_inline)) uint16_t memory_read_word(Address addr) {
    return (uint16_t)memory_read(addr) | (uint16_t)memory_read(addr + 1) << 8;
}

/* --------------- API --------------- */

typedef void (* Api_Subroutine)(struct Chip8_Context * const);

uint8_t api_attribute_high = ATTRIBUTE_UNDERLINE,
        api_attribute_low  = ATTRIBUTE_DEFAULT;

static char print_buffer[0xFF] = { 0 };

/* imm8 r_puts(imm12 cstr) */
static void api_puts(struct Chip8_Context * const context) {
    uint16_t disp = 0;

    for (unsigned char ch; (ch = memory_read(context->I + disp)) != '\0'; )
        print_buffer[disp ++] = ch;

    print_buffer[disp] = '\0';
    context->reg.v0 = ros_puts(api_attribute_low, USTR(print_buffer), false);
}

/* imm8 r_putc(imm8 ch) */
static void api_putc(struct Chip8_Context * const context) {
    unsigned char ch = context->reg.v1;
    ros_putchar(api_attribute_low, ch);
    context->reg.v0 = ch;
}

/* imm8 r_putf(imm12 fmt, __VA_ARGS_OUT__) */
static void api_putf(struct Chip8_Context * const context) {
    uint16_t disp = 0;

    for (unsigned char ch; (disp < 0xFF) && (ch = memory_read(context->I + disp)) != '\0'; )
        print_buffer[disp++] = ch;
    
    print_buffer[disp] = '\0';
    context->reg.v0 = ros_printf(api_attribute_low, print_buffer, context->reg.v1, context->reg.v2, context->reg.v3, context->reg.v4, context->reg.v5);
}

/* imm8 r_putb(imm12 buf, imm8 max) */
static void api_putb(struct Chip8_Context * const context) {
    uint16_t disp = 0;

    for (unsigned char ch; (disp < 0xFF) && (disp < context->reg.v1) && ((ch = memory_read(context->I + disp ++)) != '\0'); )
        ros_putchar(api_attribute_low, ch);

    context->reg.v0 = disp;
}

static void api_gets(struct Chip8_Context * const context) {
    chip8_panic(context, EXCEPTION_TYPE_NOT_IMPLEMENTED_SYSCALL);
}

static void api_getc(struct Chip8_Context * const context) {
    chip8_panic(context, EXCEPTION_TYPE_NOT_IMPLEMENTED_SYSCALL);
}

static void api_getf(struct Chip8_Context * const context) {
    chip8_panic(context, EXCEPTION_TYPE_NOT_IMPLEMENTED_SYSCALL);
}

static void api_geta(struct Chip8_Context * const context) {
    chip8_panic(context, EXCEPTION_TYPE_NOT_IMPLEMENTED_SYSCALL);
}

static void api_getb(struct Chip8_Context * const context) {
    chip8_panic(context, EXCEPTION_TYPE_NOT_IMPLEMENTED_SYSCALL);
}

/* imm8 r_shcr(void) */
static void api_shcr(struct Chip8_Context * const context) {
    enable_cursor();
    context->reg.v0 = 0;
}

/* imm8 r_hdcr(void) */
static void api_hdcr(struct Chip8_Context * const context) {
    disable_cursor();
    context->reg.v0 = 0;
}

/* imm8 r_stcr(imm8 state) */
static void api_stcr(struct Chip8_Context * const context) {
    if (context->reg.v1)
        enable_cursor();
    else
        disable_cursor();
    context->reg.v0 = 0;
}

/* imm8 r_mvcr(imm8 row, imm8 col) */
static void api_mvcr(struct Chip8_Context * const context) {
    set_cursor_position((v2){context->reg.v1, context->reg.v2});
    context->reg.v0 = 0;
}

/* imm8 r_atcr(imm8 high, imm8 low) */
static void api_atcr(struct Chip8_Context * const context) {
    api_attribute_low = context->reg.v1;
    api_attribute_high = context->reg.v2;
    context->reg.v0 = 0;
}

/* void r_paus(void) */
static void api_paus(struct Chip8_Context * const context) {
    ros_puts(api_attribute_low, USTR("Press any key to continue. . ."), true);
    enable_cursor();
    sys_mode = SYSTEM_MODE_IDLE;

    while (idle_key == INVALID_KEY)
        ;

    disable_cursor();
    sys_mode = SYSTEM_MODE_BUSY;
    idle_key = INVALID_KEY;
}

/* void r_slms(imm12 ms) */
static void api_slms(struct Chip8_Context * const context) {
    if (context->I > 0)
        for (unsigned i = 0; i < context->I; i ++)
            _delay_ms(1);
}

/* void r_exit(imm8 code) */
static void api_exit(struct Chip8_Context * const context) {
    context->exit_code = context->reg.v1;
    context->halted = true;
}

/* void r_abrt(void) */
static void api_abrt(struct Chip8_Context * const context) {
    context->halted = true;
}

/* void r_log(imm8 type, imm12 fmt, __VA_ARGS_OUT__) */
static void api_logg(struct Chip8_Context * const context) {
    uint16_t disp = 0;

    for (unsigned char ch; (ch = memory_read(context->I + disp)) != '\0'; )
        print_buffer[disp++] = ch;

    ros_log(context->reg.v1, print_buffer, context->reg.v2, context->reg.v3, context->reg.v4, context->reg.v5);
}

/* void s_hder(imm8 code) */
static void api_hder(struct Chip8_Context * const context) {
    HARD_ERROR(context->reg.v1);
}

/* imm8 r_cplt(imm8 a, imm8 b) */
static void api_cplt(struct Chip8_Context * const context) {
    context->reg.v0 = context->reg.v1 < context->reg.v2;
}

/* imm8 r_cpgt(imm8 a, imm8 b) */
static void api_cpgt(struct Chip8_Context * const context) {
    context->reg.v0 = context->reg.v1 > context->reg.v2;
}

/* imm8 r_cple(imm8 a, imm8 b) */
static void api_cple(struct Chip8_Context * const context) {
    context->reg.v0 = context->reg.v1 <= context->reg.v2;
}

/* imm8 r_cpge(imm8 a, imm8 b) */
static void api_cpge(struct Chip8_Context * const context) {
    context->reg.v0 = context->reg.v1 >= context->reg.v2;
}

/* imm8 r_omul(imm8 a, imm8 b) */
static void api_omul(struct Chip8_Context * const context) {
    int mulres = context->reg.v1 * context->reg.v2;
    context->reg.vF = mulres > 0xFF;
    context->reg.v0 = mulres & 0xFF;
}

/* imm8 r_odiv(imm8 a, imm8 b) */
static void api_odiv(struct Chip8_Context * const context) {
    if (!context->reg.v2)
        HARD_ERROR(FAULT_KERNEL_ZERO_DIVIDER);

    context->reg.v0 = context->reg.v1 / context->reg.v2;
}

/* imm8 r_omod(imm8 a, imm8 b) */
static void api_omod(struct Chip8_Context * const context) {
    if (!context->reg.v2)
        HARD_ERROR(FAULT_KERNEL_ZERO_DIVIDER);

    context->reg.v0 = context->reg.v1 % context->reg.v2;
}

/* imm8 r_sqrt(imm8 a) */
static void api_sqrt(struct Chip8_Context * const context) {
    context->reg.v0 = sqrt(context->reg.v1);
}

static const struct PACKED Api_Lookup {
    Api_Subroutine addr;
    uint16_t logic_code;
} api_lookup_table[] PROGMEM = {
    { api_puts, 0x100 }, { api_putc, 0x101 }, { api_putf, 0x102 }, { api_putb, 0x103 },
    { api_gets, 0x104 }, { api_getc, 0x105 }, { api_getf, 0x106 }, { api_geta, 0x200 },
    { api_getb, 0x201 }, { api_shcr, 0x300 }, { api_hdcr, 0x301 }, { api_stcr, 0x302 },
    { api_mvcr, 0x303 }, { api_atcr, 0x304 }, { api_paus, 0x400 }, { api_slms, 0x401 },
    { api_exit, 0x402 }, { api_abrt, 0x403 }, { api_logg, 0x500 }, { api_hder, 0x600 },
    { api_cplt, 0x700 }, { api_cpgt, 0x701 }, { api_cple, 0x702 }, { api_cpge, 0x703 },
    { api_omul, 0x704 }, { api_odiv, 0x705 }, { api_omod, 0x706 }, { api_sqrt, 0x707 }
};

static Api_Subroutine get_api_address(struct Chip8_Context *context, Command com) {
    unsigned short i = 0;

    for (; (i < ARR_SIZE(api_lookup_table)) && (pgm_read_word(&api_lookup_table[i].logic_code) != IMM12(com)); i ++)
        ;
    
    if (i >= (ARR_SIZE(api_lookup_table) - 1))
        chip8_panic(context, EXCEPTION_TYPE_UNKNOWN_SYSCALL);
    
    return (Api_Subroutine)pgm_read_word(&api_lookup_table[i].addr);
}

/* --------------- RET --------------- */
void impl_ret(struct Chip8_Context *const context, Command com){
    (void) com;
    POP(context);
}

/* --------------- CLS --------------- */
void impl_cls(struct Chip8_Context *const context, Command com){
    (void) com;
    clear_screen(0x0000);
}

/* --------------- CROS --------------- */
void impl_cros_imm12(struct Chip8_Context *const context, Command com){
    get_api_address(context, com)(context);
}

/* --------------- GOTO --------------- */
void impl_goto_imm12(struct Chip8_Context *const context, Command com){
    context->PC = IMM12(com);
}

void impl_goto_spec(struct Chip8_Context *const context, Command com){

}

/* --------------- CALL --------------- */
void impl_call_imm12(struct Chip8_Context *const context, Command com){
    PUSH(context);
    context->PC = IMM12(com);
}

/* --------------- SE --------------- */
void impl_se_reg8_imm8(struct Chip8_Context *const context, Command com){
    context->PC += sizeof(Command) * (context->reg.raw[REG8X(com)] == IMM8(com));
}

void impl_se_reg8_reg8(struct Chip8_Context *const context, Command com){
    context->PC += sizeof(Command) * (context->reg.raw[REG8X(com)] == context->reg.raw[REG8Y(com)]);
}

/* --------------- SNE --------------- */
void impl_sne_reg8_imm8(struct Chip8_Context *const context, Command com){
    context->PC += sizeof(Command) * (context->reg.raw[REG8X(com)] != IMM8(com));
}

void impl_sne_reg8_reg8(struct Chip8_Context *const context, Command com){
    context->PC += sizeof(Command) * (context->reg.raw[REG8X(com)] != context->reg.raw[REG8Y(com)]);
}

/* --------------- SET --------------- */
void impl_set_reg8_imm8(struct Chip8_Context *const context, Command com){
    context->reg.raw[REG8X(com)] = IMM8(com);
}

void impl_set_reg8_reg8(struct Chip8_Context *const context, Command com){
    context->reg.raw[REG8X(com)] = context->reg.raw[REG8Y(com)];
}

void impl_set_reg12_imm12(struct Chip8_Context *const context, Command com){
    context->I = IMM12(com);
}

void impl_set_reg8_delay(struct Chip8_Context *const context, Command com){
    context->reg.raw[REG8X(com)] = context->delay_timer;
}

void impl_set_reg8_key(struct Chip8_Context *const context, Command com){
    /* TODO: call api */
}

void impl_set_delay_reg8(struct Chip8_Context *const context, Command com){
    context->delay_timer = context->reg.raw[REG8X(com)];
}

void impl_deprecated_set_sound_reg8(struct Chip8_Context *const context, Command com){
    (void) com;
    chip8_panic(context, EXCEPTION_TYPE_DEPRECATED_COMMAND);
}

/* --------------- ADD --------------- */
void impl_add_reg8_imm8(struct Chip8_Context *const context, Command com){
    context->reg.raw[REG8X(com)] += IMM8(com);
}

void impl_add_reg8_reg8(struct Chip8_Context *const context, Command com){
    unsigned sum = context->reg.raw[REG8X(com)] + context->reg.raw[REG8Y(com)];
    context->reg.vF = sum > 0xFF;
    context->reg.raw[REG8X(com)] = (uint8_t)(sum & 0xFF);
}

void impl_add_reg12_reg8(struct Chip8_Context *const context, Command com){
    context->I += context->reg.raw[REG8X(com)];
}

/* --------------- OR --------------- */
void impl_or_reg8_reg8(struct Chip8_Context *const context, Command com){
    context->reg.raw[REG8X(com)] |= context->reg.raw[REG8Y(com)];
}

/* --------------- AND --------------- */
void impl_and_reg8_reg8(struct Chip8_Context *const context, Command com){
    context->reg.raw[REG8X(com)] &= context->reg.raw[REG8Y(com)];
}

/* --------------- XOR --------------- */
void impl_xor_reg8_reg8(struct Chip8_Context *const context, Command com){
    context->reg.raw[REG8X(com)] ^= context->reg.raw[REG8Y(com)];
}

/* --------------- SUB --------------- */
void impl_sub_reg8_reg8(struct Chip8_Context *const context, Command com){
    uint8_t v1 = context->reg.raw[REG8X(com)], v2 = context->reg.raw[REG8Y(com)];
    context->reg.vF = v1 > v2;
    context->reg.raw[REG8X(com)] = v1 - v2;
}

void impl_sub_spec(struct Chip8_Context *const context, Command com){
    uint8_t v1 = context->reg.raw[REG8X(com)], v2 = context->reg.raw[REG8Y(com)];
    context->reg.vF = v2 > v1;
    context->reg.raw[REG8X(com)] = v2 - v1;
}

/* --------------- SHR --------------- */
void impl_shr_reg8(struct Chip8_Context *const context, Command com){
    context->reg.vF = context->reg.raw[REG8X(com)] & 1;
    context->reg.raw[REG8X(com)] >>= 1;
}

/* --------------- SHL --------------- */
void impl_shl_reg8(struct Chip8_Context *const context, Command com){
    context->reg.vF = context->reg.raw[REG8X(com)] >> 7;
    context->reg.raw[REG8X(com)] <<= 1;
}

/* --------------- RND --------------- */
void impl_rnd_imm8(struct Chip8_Context *const context, Command com){
    context->reg.raw[REG8X(com)] = rand() & IMM8(com);
}

/* --------------- DRAW --------------- */
void impl_deprecated_draw_reg8_reg8_imm4(struct Chip8_Context *const context, Command com){
    (void) com;
    chip8_panic(context, EXCEPTION_TYPE_DEPRECATED_COMMAND);
}

/* --------------- SKE --------------- */
void impl_deprecated_ske_reg8(struct Chip8_Context *const context, Command com){
    (void) com;
    chip8_panic(context, EXCEPTION_TYPE_DEPRECATED_COMMAND);
}

/* --------------- SKNE --------------- */
void impl_deprecated_skne_reg8(struct Chip8_Context *const context, Command com){
    (void) com;
    chip8_panic(context, EXCEPTION_TYPE_DEPRECATED_COMMAND);
}

/* --------------- SPR --------------- */
void impl_deprecated_spr_reg8(struct Chip8_Context *const context, Command com){
    (void) com;
    chip8_panic(context, EXCEPTION_TYPE_DEPRECATED_COMMAND);
}

/* --------------- BCD --------------- */
void impl_bcd_reg8(struct Chip8_Context *const context, Command com){
    uint8_t imm8 = IMM8(com);
    
    memory_write((Address)(context->I), imm8 / 100);
    imm8 %= 100;
    memory_write((Address)(context->I + 1), imm8 / 10);
    memory_write((Address)(context->I + 2), imm8 % 10);
}

/* --------------- DUMP --------------- */
void impl_dump_reg8(struct Chip8_Context *const context, Command com){
    for (uint8_t i = 0; i <= REG8X(com); i ++)
        memory_write((Address)(i + context->I), context->reg.raw[i]);
}

/* --------------- LOAD --------------- */
void impl_load_reg8(struct Chip8_Context *const context, Command com){
    for (uint8_t i = 0; i <= REG8X(com); i ++)
        context->reg.raw[i] = memory_read((Address)(i + context->I));
}