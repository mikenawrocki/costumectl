#ifndef __COLOR_H
#define __COLOR_H

#include <stdint.h>
#include "costume.h"

/* Predefined colors - using GRB 7 bit format */
#define COLOR(r, g, b) ((uint32_t)(g) << 16 | \
                        (uint32_t)(r) << 8  | \
                        (uint32_t)(b))

#define BLACK 0x0
#define WHITE COLOR(0x7F, 0x7F, 0x7F)

#define RED     COLOR(0x7F, 0x00, 0x00)
#define GREEN   COLOR(0x00, 0x7F, 0x00)
#define BLUE    COLOR(0x00, 0x00, 0x7F)

#define CYAN    COLOR(0x00, 0x7F, 0x7F)
#define VIOLET  COLOR(0x7F, 0x00, 0x7F)
#define YELLOW  COLOR(0x7F, 0x7F, 0x00)

#define ORANGE  COLOR(0x7F, 0x20, 0x00)
#define PURPLE  COLOR(0x7F, 0x00, 0x45)

// Convert a 24bpp little endian rgb color to the native color forma
uint32_t convert_24bpp_rgb(uint32_t color);

int is_color_valid(uint32_t color);

extern uint32_t rainbow_colors[8];

#endif
