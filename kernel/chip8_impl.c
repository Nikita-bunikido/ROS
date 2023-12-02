#include <avr/pgmspace.h>
#include <util/delay.h>

#include <stdarg.h>
#include <stdbool.h>
#include <math.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>

#include <drivers/keyboard.h>
#include <drivers/memory.h>
#include <ros/log.h>
#include <ros/video.h>
#include <ros/chip8.h>
#include <ros/ros-for-modules.h>

#define API_BUFFER_CAP       256

#define API_SUB(_name)       static void __callback api_##_name(struct Chip8_Context * const context)
#define API_ARG(_num)        (context->reg.raw[(_num) + 1])
#define API_RET(_val)        context->reg.v0 = (_val)

typedef void (*Api_Subroutine)(struct Chip8_Context * const);

static uint8_t api_attribute_high = ATTRIBUTE_UNDERLINE,
               api_attribute_low  = ATTRIBUTE_DEFAULT;

static char api_buffer[API_BUFFER_CAP] = { 0 };

/* imm8 r_puts(imm12 cstr) */
API_SUB(puts) {
    uint16_t disp = 0;

    for (unsigned char ch; (ch = memory_read(context->I + disp)) != '\0'; )
        api_buffer[disp ++] = ch;

    api_buffer[disp] = '\0';
    API_RET(0xFF & ros_puts(api_attribute_low, USTR(api_buffer), false));
}

/* imm8 r_putc(imm8 ch) */
API_SUB(putc) {
    API_RET(ros_putchar(api_attribute_low, API_ARG(0)));
}

/* imm8 r_putf(imm12 fmt, __VA_ARGS_OUT__) */
API_SUB(putf) {
    uint16_t disp = 0;

    for (unsigned char ch; (disp < API_BUFFER_CAP) && (ch = memory_read(context->I + disp)) != '\0'; )
        api_buffer[disp++] = ch;
    
    api_buffer[disp] = '\0';
    API_RET(0xFF & ros_printf(api_attribute_low, api_buffer, 
                                                        API_ARG(0),
                                                        API_ARG(1),
                                                        API_ARG(2),
                                                        API_ARG(3),
                                                        API_ARG(4))
    );
}

/* imm8 r_putb(imm12 buf, imm8 max) */
API_SUB(putb) {
    uint16_t disp = 0;

    for (unsigned char ch; (disp < API_BUFFER_CAP) && (disp < API_ARG(0)) && ((ch = memory_read(context->I + disp ++)) != '\0'); )
        ros_putchar(api_attribute_low, ch);

    API_RET(0xFF & disp);
}

/* imm8 r_gets(imm12 buf, imm8 max) */
API_SUB(gets) {
    chip8_panic(context, EXCEPTION_TYPE_NOT_IMPLEMENTED_SYSCALL);
}

/* imm8 r_getc(void) */
API_SUB(getc) {
    sys_mode = SYSTEM_MODE_IDLE;

    while (idle_key == INVALID_KEY)
        ;

    API_RET(0xFF & vk_as_char(idle_key));
    sys_mode = SYSTEM_MODE_BUSY;
    idle_key = INVALID_KEY;
}

/* imm8 r_getf(imm12 fmt, __VA_ARGS_IN__) */
API_SUB(getf) {
    chip8_panic(context, EXCEPTION_TYPE_NOT_IMPLEMENTED_SYSCALL);
}

/* imm8 r_geta(imm12 addr, imm8 idx) */
API_SUB(geta) {
#if 0
    Address addr = 0;

    for (int8_t idx = API_ARG(0); (idx > 0) && (addr < INPUT_BUFFER_CAP); addr ++)
        if (strchr(" \t", memory_read(addr))) idx --;

    if (addr == INPUT_BUFFER_CAP)
        API_RET(0);

    uint8_t len = 0;
    for (uint16_t disp = 0; ((context->I + disp) < RAM_MAX) && (memory_write(context->I + disp++, memory_read(addr ++)) != '\0'); len ++)
        ;

    /* TODO: copy and terminate */
    memory_write_buffer();
    API_RET(len);
#endif
}

/* imm8 r_getb(void) */
API_SUB(getb) {
    context->I = 0;
}

/* imm8 r_shcr(void) */
API_SUB(shcr) {
    enable_cursor();
    API_RET(0);
}

/* imm8 r_hdcr(void) */
API_SUB(hdcr) {
    disable_cursor();
    API_RET(0);
}

/* imm8 r_stcr(imm8 state) */
API_SUB(stcr) {
    if (API_ARG(0)) {
        enable_cursor();
        API_RET(0);
        return;
    }
    
    disable_cursor();
    API_RET(0);
}

/* imm8 r_mvcr(imm8 row, imm8 col) */
API_SUB(mvcr) {
    set_cursor_position((v2){API_ARG(0), API_ARG(1)});
    API_RET(0);
}

/* imm8 r_atcr(imm8 high, imm8 low) */
API_SUB(atcr) {
    api_attribute_low = API_ARG(0);
    api_attribute_high = API_ARG(1);
    API_RET(0);
}

/* void r_paus(void) */
API_SUB(paus) {
    (void) context;

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
API_SUB(slms) {
    if (context->I > 0)
        for (unsigned short i = 0; i < context->I; i ++)
            _delay_ms(1);
}

/* void r_exit(imm8 code) */
API_SUB(exit) {
    context->exit_code = API_ARG(0);
    api_attribute_high = ATTRIBUTE_UNDERLINE;
    api_attribute_low = ATTRIBUTE_DEFAULT;

    extern void chip8_perform_syscall(struct Chip8_Context *, uint16_t);
    chip8_perform_syscall(context, 0x403); /* abrt */
}

/* void r_abrt(void) */
API_SUB(abrt) {
    context->halted = true;
}

/* void r_logg(imm8 type, imm12 fmt, __VA_ARGS_OUT__) */
API_SUB(logg) {
    uint16_t disp = 0;

    for (unsigned char ch; (ch = memory_read(context->I + disp)) != '\0'; )
        api_buffer[disp++] = ch;

    ros_log(API_ARG(0), api_buffer,
                                    API_ARG(1), 
                                    API_ARG(2),
                                    API_ARG(3),
                                    API_ARG(4));
}

/* void s_hder(imm8 code) */
__attribute__((noreturn)) API_SUB(hder) {
    HARD_ERROR(context->reg.v1);
    UNREACHABLE();
}

/* imm8 r_cplt(imm8 a, imm8 b) */
API_SUB(cplt) { API_RET(API_ARG(0) < API_ARG(1)); }

/* imm8 r_cpgt(imm8 a, imm8 b) */
API_SUB(cpgt) { API_RET(API_ARG(0) > API_ARG(1)); }

/* imm8 r_cple(imm8 a, imm8 b) */
API_SUB(cple) { API_RET(API_ARG(0) <= API_ARG(1)); }

/* imm8 r_cpge(imm8 a, imm8 b) */
API_SUB(cpge) { API_RET(API_ARG(0) >= API_ARG(1)); }

/* imm8 r_omul(imm8 a, imm8 b) */
API_SUB(omul) {
    int mulres = API_ARG(0) * API_ARG(1);
    context->reg.vF = mulres > 0xFF;
    API_RET(0xFF & mulres);
}

/* imm8 r_odiv(imm8 a, imm8 b) */
API_SUB(odiv) {
    #if defined(_SECURITY_API_ZERO_DIVIDER_CHECK)
        if (!API_ARG(1))
            HARD_ERROR(FAULT_KERNEL_ZERO_DIVIDER);
    #endif

    API_RET(API_ARG(0) / API_ARG(1));
}

/* imm8 r_omod(imm8 a, imm8 b) */
API_SUB(omod) {
    #if defined(_SECURITY_API_ZERO_DIVIDER_CHECK)
        if (!API_ARG(1))
            HARD_ERROR(FAULT_KERNEL_ZERO_DIVIDER);
    #endif

    API_RET(API_ARG(0) % API_ARG(1));
}

/* imm8 r_sqrt(imm8 a) */
API_SUB(sqrt) {
    API_RET(sqrt(API_ARG(0)));
}

static const struct PACKED Api_Lookup {
    Api_Subroutine addr;
    uint16_t logic_code;
} api_lookup_table[] PROGMEM = {
    { api_puts, 0x100 },
    { api_putc, 0x101 },
    { api_putf, 0x102 },
    { api_putb, 0x103 },
    { api_gets, 0x104 },
    { api_getc, 0x105 },
    { api_getf, 0x106 },
    
    { api_geta, 0x200 },
    { api_getb, 0x201 },
    
    { api_shcr, 0x300 },
    { api_hdcr, 0x301 },
    { api_stcr, 0x302 },
    { api_mvcr, 0x303 },
    { api_atcr, 0x304 },
    { api_paus, 0x400 },
    { api_slms, 0x401 },
    { api_exit, 0x402 },
    { api_abrt, 0x403 },
    
    { api_logg, 0x500 },
    
    { api_hder, 0x600 },

    { api_cplt, 0x700 },
    { api_cpgt, 0x701 },
    { api_cple, 0x702 },
    { api_cpge, 0x703 },
    { api_omul, 0x704 },
    { api_odiv, 0x705 },
    { api_omod, 0x706 },
    { api_sqrt, 0x707 },
};

void chip8_perform_syscall(struct Chip8_Context *context, uint16_t code) {
    uint16_t c_code;
    size_t i;

    for (i = 0; (i < ARR_SIZE(api_lookup_table)) && ((c_code = pgm_read_word(&api_lookup_table[i].logic_code)) != code); i ++)
        if ((c_code & 0xF00) > (code & 0xF00))
            chip8_panic(context, EXCEPTION_TYPE_UNKNOWN_SYSCALL);

    if (i >= ARR_SIZE(api_lookup_table) - 1)
        chip8_panic(context, EXCEPTION_TYPE_UNKNOWN_SYSCALL);

    Api_Subroutine api = (Api_Subroutine)pgm_read_word(&api_lookup_table[i].addr);
    api(context);
}