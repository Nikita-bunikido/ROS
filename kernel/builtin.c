#include <avr/pgmspace.h>

#include <stdbool.h>
#include <inttypes.h>
#include <stddef.h>

#include "ros.h"
#include "video.h"

#define PROGRAM_NAME_MAX    32U

static const uint8_t test_rex[] PROGMEM = {
    0xa2, 0x08, 0x01, 0x00, 0x80, 0x03, 0x04, 0x02, 0x49, 0x66, 0x20, 0x65,
    0x76, 0x65, 0x72, 0x79, 0x74, 0x68, 0x69, 0x6e, 0x67, 0x20, 0x69, 0x73,
    0x20, 0x77, 0x6f, 0x72, 0x6b, 0x69, 0x6e, 0x67, 0x2c, 0x20, 0x74, 0x68,
    0x69, 0x73, 0x20, 0x73, 0x74, 0x72, 0x69, 0x6e, 0x67, 0x20, 0x73, 0x68,
    0x6f, 0x75, 0x6c, 0x64, 0x20, 0x61, 0x70, 0x70, 0x65, 0x61, 0x72, 0x2e,
    0x00
}, ver_rex[] PROGMEM = {
	0x00, 0xE0, 0x8E, 0xE3, 0x61, 0x57, 0x62, 0xFF, 0x03, 0x04, 0xA2, 0xE4,
	0xFE, 0x1E, 0x22, 0xEF, 0x40, 0x00, 0x12, 0x18, 0x7E, 0x01, 0x12, 0x0A,
	0xA2, 0x2E, 0x8C, 0xC3, 0x22, 0xC8, 0x7C, 0x01, 0x3C, 0x07, 0x12, 0x1C,
	0x61, 0x00, 0x62, 0x0B, 0x03, 0x03, 0x81, 0x13, 0x04, 0x02, 0x01, 0x01,
	0x52, 0x4F, 0x53, 0x20, 0x28, 0x52, 0x6F, 0x6D, 0x20, 0x6F, 0x70, 0x65,
	0x72, 0x61, 0x74, 0x69, 0x6E, 0x67, 0x20, 0x00, 0x01, 0x02, 0x73, 0x79,
	0x73, 0x74, 0x65, 0x6D, 0x29, 0x20, 0x76, 0x65, 0x72, 0x73, 0x69, 0x6F,
	0x6E, 0x20, 0x25, 0x64, 0x20, 0x00, 0x01, 0x04, 0x52, 0x4F, 0x53, 0x20,
	0x69, 0x73, 0x20, 0x61, 0x20, 0x73, 0x6D, 0x61, 0x6C, 0x6C, 0x2C, 0x20,
	0x44, 0x4F, 0x53, 0x00, 0x01, 0x05, 0x2D, 0x6C, 0x69, 0x6B, 0x65, 0x2C,
	0x20, 0x41, 0x56, 0x52, 0x2D, 0x74, 0x61, 0x72, 0x67, 0x65, 0x74, 0x74,
	0x69, 0x00, 0x01, 0x06, 0x6E, 0x67, 0x20, 0x6F, 0x70, 0x65, 0x72, 0x61,
	0x74, 0x69, 0x6E, 0x67, 0x20, 0x73, 0x79, 0x73, 0x74, 0x65, 0x6D, 0x00,
	0x01, 0x07, 0x66, 0x6F, 0x72, 0x20, 0x4E, 0x50, 0x41, 0x44, 0x2D, 0x35,
	0x20, 0x63, 0x6F, 0x6D, 0x70, 0x75, 0x74, 0x69, 0x6E, 0x00, 0x01, 0x08,
	0x67, 0x20, 0x6D, 0x61, 0x63, 0x68, 0x69, 0x6E, 0x65, 0x2E, 0x20, 0x20,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0xF1, 0x65, 0x83, 0x00,
	0x84, 0x10, 0x81, 0x30, 0x82, 0x40, 0x03, 0x03, 0x65, 0x02, 0xF5, 0x1E,
	0x61, 0x01, 0x01, 0x02, 0x65, 0x13, 0x75, 0x01, 0xF5, 0x1E, 0x00, 0xEE,
	0x00, 0x02, 0x02, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x04, 0xF0,
	0x65, 0x40, 0x04, 0x13, 0x19, 0x40, 0x00, 0x13, 0x05, 0x40, 0x02, 0x13,
	0x09, 0x40, 0x01, 0x13, 0x0D, 0x40, 0x03, 0x13, 0x11, 0xA3, 0x1D, 0x13,
	0x13, 0xA3, 0x23, 0x13, 0x13, 0xA3, 0x20, 0x13, 0x13, 0xA3, 0x26, 0x23,
	0x29, 0x60, 0x01, 0x00, 0xEE, 0x80, 0x03, 0x00, 0xEE, 0x98, 0x9C, 0x8A,
	0x96, 0x93, 0x85, 0x89, 0x20, 0x89, 0x97, 0x9C, 0x8B, 0x65, 0x01, 0xF0,
	0x65, 0x81, 0x00, 0x01, 0x01, 0xF5, 0x1E, 0x68, 0x13, 0xF0, 0x65, 0x81,
	0x00, 0x01, 0x01, 0x6F, 0x01, 0x88, 0xF5, 0x38, 0x00, 0x13, 0x39, 0xF5,
	0x1E, 0xF0, 0x65, 0x81, 0x00, 0x01, 0x01, 0x00, 0xEE
};

static const struct PACKED Builtin_Program {
    const char name[PROGRAM_NAME_MAX];

    uint8_t *raw;
    uint16_t size;
} programs[] = {
    { "test", (void *)test_rex, ARR_SIZE(test_rex) },
    { "ver",  (void *)ver_rex, ARR_SIZE(ver_rex) }
};

uint8_t *get_builtin_program(const struct Input_Buffer *ibuffer, uint16_t *size) {
    static char prog_name[PROGRAM_NAME_MAX];
    unsigned short prog_name_len = 0;

    const char *p = ibuffer->raw;
    for (; *p && (*p != ' ') && (*p != '\t'); p ++)
        prog_name[prog_name_len ++] = *p;
    prog_name[prog_name_len] = '\0';

    for (uint16_t i = 0; i < ARR_SIZE(programs); i ++) {
        if (caseless_cmp(programs[i].name, prog_name))
            continue;
    
        if (size)
            *size = programs[i].size;
    
        return programs[i].raw;
    }

    return NULL;
}