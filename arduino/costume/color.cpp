#include "color.h"
#include "costume.h"

uint32_t rainbow_colors[8] = {
    RED, ORANGE, YELLOW, GREEN, BLUE, CYAN, VIOLET, PURPLE
};

uint32_t convert_24bpp_rgb(uint32_t color)
{
    uint32_t r, g, b;
    r = color >> 17 & 0x7F;
    g = color >> 9  & 0x7F;
    b = color >> 1  & 0x7F;
    return (g << 16 | r << 8 | b);
}

int is_color_valid(uint32_t color)
{
    return !(color & 0xFF808080);
}
