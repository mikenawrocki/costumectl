#include <LPD8806.h>

#include "costume.h"
#include "tail.h"

// Number of LEDs present (please make it a power of 2...)
#define NUM_LEDS 8

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
#define MAGENTA COLOR(0x7F, 0x00, 0x7F)
#define YELLOW  COLOR(0x7F, 0x7F, 0x00)

#define ORANGE  COLOR(0x7F, 0x20, 0x00)
#define PURPLE  COLOR(0x7F, 0x00, 0x45)

enum {
    MODE_OFF = 0,
    MODE_FIXED,
    MODE_STROBE,
    MODE_FADE,
    MODE_TWO_COLOR,
    MODE_TOP_DOWN,
    MODE_BOTTOM_UP,
    MODE_RAINBOW,
    MODE_BI_PRIDE,

    N_MODES /* Number of modes present */
};


static int is_color_valid(uint32_t color);
static void display_off(void);
static void display_fixed(void);
static void display_strobe(void);
static void display_fade(void);
static void display_two_color(void);
static void display_top_down(void);
static void display_bottom_up(void);
static void display_rainbow(void);
static void display_bi_pride(void);

/* Display function to use */
static void (*display_funcs[N_MODES])(void) = {
    display_off,
    display_fixed,
    display_strobe,
    display_fade,
    display_two_color,
    display_top_down,
    display_bottom_up,
    display_rainbow,
    display_bi_pride
};

static uint32_t rainbow_colors[8] = {
    RED, ORANGE, YELLOW, GREEN, BLUE, CYAN, MAGENTA, PURPLE
};

LPD8806 strip = LPD8806(NUM_LEDS << 1, LPD8806_DATA_PIN, LPD8806_CLOCK_PIN);


static int is_color_valid(uint32_t color)
{
    return !(color & 0xFF808080);
}

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

static void display_off(void)
{
    display_clear();
}

static void display_fixed(void)
{
    _display_color(display.tail.primary_color);
}

static void display_strobe(void)
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

    strip.setPixelColor((display_pixel << 1) + 1, display.tail.primary_color);
    strip.show();
    strip.setPixelColor((display_pixel << 1) + 1, BLACK);
    ndx = (ndx + 1) & (NUM_LEDS - 1);
}

static void display_fade(void)
{
    _display_two_color(display.tail.primary_color, BLACK);
}


static void display_two_color(void)
{
    _display_two_color(display.tail.primary_color,
                       display.tail.secondary_color);
}

static void display_top_down(void)
{
    _display_wipe(1);
}

static void display_bottom_up(void)
{
    _display_wipe(0);
}

static void display_rainbow(void)
{
    static uint8_t r_ndx = 0;

    for (uint8_t i = 0; i < NUM_LEDS; ++i, ++r_ndx) {
        strip.setPixelColor((i<<1) + 1, rainbow_colors[r_ndx & (NUM_LEDS-1)]);
    }

    --r_ndx;
    strip.show();
}

static void display_bi_pride(void)
{
    static uint8_t n_magenta = 0.375 * NUM_LEDS;
    static uint8_t n_blue = 0.375 * NUM_LEDS;
    static uint8_t n_purple = 0.25 * NUM_LEDS;

    uint8_t px_ndx = 0;

    for (uint8_t i = 0; i < n_magenta; ++i, ++px_ndx) {
        strip.setPixelColor((px_ndx<<1) + 1, MAGENTA);
    }
    for (uint8_t i = 0; i < n_purple; ++i, ++px_ndx) {
        strip.setPixelColor((px_ndx<<1) + 1, PURPLE);
    }
    for (uint8_t i = 0; i < n_blue; ++i, ++px_ndx) {
        strip.setPixelColor((px_ndx<<1) + 1, BLUE);
    }

    strip.show();
}


int validate_tail(struct component_config *tail)
{
    if (tail->magic != 0x7f7f) {
        return 0;
    }
    if (tail->mode >= N_MODES) {
        return 0;
    }
    if (!is_color_valid(tail->primary_color)) {
        return 0;
    }
    if (!is_color_valid(tail->secondary_color)) {
        return 0;
    }

    return 1;
}

void init_tail(void)
{
    struct component_config *tail = &display.tail;


    
    tail->mode = MODE_FIXED;
    tail->primary_color = RED;
    tail->secondary_color = CYAN;

    // Start up the LED strip
    strip = LPD8806(NUM_LEDS << 1, LPD8806_DATA_PIN, LPD8806_CLOCK_PIN);
    strip.begin();
    display_clear();
    strip.show();
}

void display_tail(void)
{
    display_funcs[display.tail.mode]();
}
