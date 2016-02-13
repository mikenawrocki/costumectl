#include "costume.h"
#include "color.h"
#include "display.h"

/* Display function to use */
void (*display_funcs[N_MODES])(void) = {
    display_off,
    display_fixed,
    display_strobe,
    display_fade,
    display_two_color,
    display_top_down,
    display_bottom_up,
    display_rainbow
};


static void _display_color(uint32_t color)
{
    for (uint8_t i = 0; i < NUM_LEDS; ++i) {
        strip.setPixelColor((i<<1)+1, color);
    } 
    strip.show();
}

static void display_clear(void)
{
    _display_color(BLACK);
}

void display_off(void)
{
    display_clear();
}

void display_fixed(void)
{
    _display_color(display.primary_color);
}

void display_strobe(void)
{
    static uint8_t i = 0;
    if (i++ & 1) {
        display_fixed();
    }
    else {
        display_clear();
    }
}

static uint8_t _updated_color(uint8_t s, uint8_t e)
{
    if ((s + 2) < e) {
        return s + 2;
    } else if ((s - 2) > e) {
        return s - 2;
    } else {
        return e;
    }
}

static void _display_two_color(uint32_t color1, uint32_t color2)
{
    static uint32_t end_color;
    static uint8_t end_r, end_g, end_b;

    if (reset_display) {
        reset_display = false;
        end_color = color2;
        display_fixed();

        end_r = (end_color >>  8) & 0x7F;
        end_g = (end_color >> 16) & 0x7F;
        end_b = (end_color      ) & 0x7F;
    }

    uint32_t new_color = strip.getPixelColor(1);
    uint8_t new_r, new_g, new_b;
    new_r = (new_color >> 8) & 0x7F;
    new_g = (new_color >> 16) & 0x7F;
    new_b = (new_color) & 0x7F;

    new_r = _updated_color(new_r, end_r);
    new_g = _updated_color(new_g, end_g);
    new_b = _updated_color(new_b, end_b);
    new_color = strip.Color(new_r, new_g, new_b);
    _display_color(new_color);

    if (strip.getPixelColor(1) == end_color) {
        end_color = (end_color == color1) ? color2 : color1;
        end_r = (end_color >>  8) & 0x7F;
        end_g = (end_color >> 16) & 0x7F;
        end_b = (end_color      ) & 0x7F;
    }
}

/* If dir, display from nearest to farthest, else do reverse. */
static void _display_wipe(int dir)
{
    static uint8_t ndx = 0;
    if (!ndx) {
        display_clear();
    }

    uint8_t display_pixel = (dir) ? ndx : (NUM_LEDS - ndx -1);

    strip.setPixelColor((display_pixel << 1) + 1, display.primary_color);
    strip.show();
    strip.setPixelColor((display_pixel << 1) + 1, BLACK);
    ndx = (ndx + 1) & (NUM_LEDS - 1);
}

void display_fade(void)
{
    _display_two_color(display.primary_color, BLACK);
}


void display_two_color(void)
{
    _display_two_color(display.primary_color, display.secondary_color);
}

void display_top_down(void)
{
    _display_wipe(1);
}

void display_bottom_up(void)
{
    _display_wipe(0);
}

void display_rainbow(void)
{
    static uint8_t r_ndx = 0;

    for (uint8_t i = 0; i < NUM_LEDS; ++i, ++r_ndx) {
        strip.setPixelColor((i<<1) + 1, rainbow_colors[r_ndx & (NUM_LEDS-1)]);
    }

    --r_ndx;
    strip.show();
}
