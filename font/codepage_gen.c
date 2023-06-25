#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <inttypes.h>

#define USAGE(str, ...) fprintf(stderr, str __VA_OPT__(,) __VA_ARGS__)

#define PPM_VERSION         "6"
#define PPM_DELIMITER       "\n"

#define CHARACTER_WIDTH     6
#define CHARACTER_HEIGHT    8

#define TO1(x,y,width)      ((x) + (y) * (width))

typedef struct __attribute__((__packed__)) PPM_Color {
    uint8_t r, g, b;
} PPM_Color;
static_assert( sizeof(PPM_Color) == 3 );

typedef PPM_Color PPM_Character[CHARACTER_WIDTH * CHARACTER_HEIGHT];

typedef struct __attribute__((__packed__)) PPM_Image {
    long width, height;
    uint8_t color_depth;

    PPM_Color *colors;
} PPM_Image;

PPM_Image load_ppm_image(const char *path){
    assert(path != NULL);
    FILE *f = fopen(path, "rb");

    if (!f){
        perror("popen");
        exit(EXIT_FAILURE);
    }

    PPM_Image ppm_image = { 0 };
    assert( fscanf(f, "P"PPM_VERSION PPM_DELIMITER "%li" PPM_DELIMITER "%li" PPM_DELIMITER "%lu" PPM_DELIMITER, 
            &(ppm_image.width), &(ppm_image.height), &(ppm_image.color_depth)) == 3 );

    printf("Width - %ld\nHeight - %ld\nDepth - %ld\n", ppm_image.width, ppm_image.height, ppm_image.color_depth);

    size_t ppm_size = sizeof(PPM_Color) * ppm_image.width * ppm_image.height;
    ppm_image.colors = malloc(ppm_size);
    assert(ppm_image.colors);

    fread(ppm_image.colors, ppm_size, 1, f);
    fclose(f);
    return ppm_image;
}

void get_ppm_character(const PPM_Image *base, uint32_t px, uint32_t py, PPM_Character ch){
    for (uint32_t read = 0; read < CHARACTER_HEIGHT; read ++, py++){
        PPM_Color *col = base->colors + TO1(px,py,base->width);
        memcpy(ch + read * CHARACTER_WIDTH, col, sizeof(PPM_Color) * CHARACTER_WIDTH);
    }
}

void dump_ppm_character(const PPM_Character ch){
    printf("{ ");
    const char *delim = "";

    for (uint32_t py = 0; py < CHARACTER_HEIGHT; py++, delim = ","){
        uint8_t byte = 0;
        for (uint32_t px = 0; px < CHARACTER_WIDTH; px++)
            if (ch[TO1(px,py,CHARACTER_WIDTH)].r > 0)
                byte |= (1 << px);
        printf("%s0x%"PRIX8"", delim, byte);
    }

    printf(" },\n");
}

void unload_ppm_image(PPM_Image * const pi){
    if (!pi->colors) return;

    free(pi->colors);
    pi->colors = NULL;
}

int main(int argc, char *argv[]){
    if (argc < 2){
        USAGE("%s <filename>", *argv);
        exit(EXIT_FAILURE);
    }

    PPM_Image pi = load_ppm_image(argv[1]);
    
    for (int j = 0; j < pi.height / CHARACTER_HEIGHT; j++){
        for (int i = 0; i < pi.width / CHARACTER_WIDTH; i++){
            PPM_Character ch = { 0 };
            get_ppm_character(&pi, i * 6, j * 8, ch);
            dump_ppm_character(ch);
        }
    }

    unload_ppm_image(&pi);
    return 0;
}