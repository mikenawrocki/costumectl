#include <Adafruit_NeoPixel.h>

#include "costume.h"
#include "snout.h"

// Number of LEDs present
// Two in the eyes
#define NUM_LEDS 2

/* Predefined colors - using RGB 8 bit format */
#define COLOR(r, g, b) ((uint32_t)(g) << 16 | \
                        (uint32_t)(r) << 8  | \
                        (uint32_t)(b))

#define BLACK 0x0
#define WHITE COLOR(0xFF, 0xFF, 0xFF)

#define RED     COLOR(0xFF, 0x00, 0x00)
#define GREEN   COLOR(0x00, 0xFF, 0x00)
#define BLUE    COLOR(0x00, 0x00, 0xFF)

#define CYAN    COLOR(0x00, 0xFF, 0xFF)
#define MAGENTA COLOR(0xFF, 0x00, 0xFF)
#define YELLOW  COLOR(0xFF, 0xFF, 0x00)

#define ORANGE  COLOR(0xFF, 0x40, 0x00)
#define PURPLE  COLOR(0xFF, 0x00, 0x85)

enum {
    MODE_OFF = 0,
    MODE_FIXED,
    MODE_STROBE,
    MODE_FADE,
    MODE_TWO_COLOR,
    MODE_RAINBOW,
    MODE_BLACKLIGHT,

    N_MODES /* Number of modes present */
};


static int is_color_valid(uint32_t color);
static void display_off(void);
static void display_fixed(void);
static void display_strobe(void);
static void display_fade(void);
static void display_two_color(void);
static void display_rainbow(void);
static void display_blacklight(void);
/* Based PaintYourDragon */
uint32_t Wheel(uint8_t WheelPos);

/* Display function to use */
static void (*display_funcs[N_MODES])() = {
    display_off,
    display_fixed,
    display_strobe,
    display_fade,
    display_two_color,
    display_rainbow,
    display_blacklight,
};

/* check me out homie */
Adafruit_NeoPixel eyes_strip;
static int uva_leds_enabled = 0;


static int is_color_valid(uint32_t color)
{
    return !(color & 0xFF000000);
}

static void _display_color(uint32_t color)
{
    for (uint8_t i = 0; i < NUM_LEDS; ++i) {
        eyes_strip.setPixelColor(i, color);
    } 
    eyes_strip.show();
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
    _display_color(display.eyes.primary_color);
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
    if ((s + 4) < e) {
        return s + 4;
    } else if ((s - 4) > e) {
        return s - 4;
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

        end_r = (end_color >> 16) & 0xFF;
        end_g = (end_color >>  8) & 0xFF;
        end_b = (end_color      ) & 0xFF;
    }

    uint32_t new_color = eyes_strip.getPixelColor(1);
    uint8_t new_r, new_g, new_b;
    new_r = (new_color >> 16) & 0xFF;
    new_g = (new_color >>  8) & 0xFF;
    new_b = (new_color) & 0xFF;

    new_r = _updated_color(new_r, end_r);
    new_g = _updated_color(new_g, end_g);
    new_b = _updated_color(new_b, end_b);
    new_color = eyes_strip.Color(new_r, new_g, new_b);
    _display_color(new_color);

    if (eyes_strip.getPixelColor(0) == end_color) {
        end_color = (end_color == color1) ? color2 : color1;
        end_r = (end_color >> 16) & 0xFF;
        end_g = (end_color >>  8) & 0xFF;
        end_b = (end_color      ) & 0xFF;
    }
}

static void display_fade(void)
{
    _display_two_color(display.eyes.primary_color, BLACK);
}

static void display_two_color()
{
    _display_two_color(display.eyes.primary_color,
                       display.eyes.secondary_color);
}

static void display_rainbow()
{
    static uint8_t r_ndx = 0;
    uint32_t r_color = Wheel(r_ndx);

    _display_color(r_color);

    ++r_ndx;
    eyes_strip.show();
}

/* Note: the UV/A LEDS are active low. */
static int enable_uva_leds(void)
{
    if (!uva_leds_enabled) {
        display_off();               /* Disable the RGB LEDs */
        pinMode(UVA_EYES_PIN, OUTPUT);
        digitalWrite(UVA_EYES_PIN, LOW);  /* Enable the UV/A LEDS */
        uva_leds_enabled = 1;
    }
}

static int disable_uva_leds(void)
{
    pinMode(UVA_EYES_PIN, INPUT);
    uva_leds_enabled = 0;
}


static void display_blacklight()
{
    enable_uva_leds();
}

/* Based PaintYourDragon */
uint32_t Wheel(uint8_t WheelPos)
{
    WheelPos = 255 - WheelPos;

    if(WheelPos < 85) {
        return Adafruit_NeoPixel::Color(255 - WheelPos * 3, 0, WheelPos * 3);
    }

    if(WheelPos < 170) {
        WheelPos -= 85;
        return Adafruit_NeoPixel::Color(0, WheelPos * 3, 255 - WheelPos * 3);
    }

    WheelPos -= 170;

    return Adafruit_NeoPixel::Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}


int validate_eyes(struct component_config *eyes)
{
    if (eyes->magic != 0x7f7f) {
        return 0;
    }
    if (eyes->mode >= N_MODES) {
        return 0;
    }
    if (!is_color_valid(eyes->primary_color)) {
        return 0;
    }
    if (!is_color_valid(eyes->secondary_color)) {
        return 0;
    }

    return 1;
}

void init_eyes(void)
{
    struct component_config *eyes = &display.eyes;
    //eyes->mode = MODE_FIXED;
    eyes->mode = MODE_RAINBOW;
    eyes->primary_color = RED;
    eyes->secondary_color = CYAN;

    /* Start up the LED strip */
    eyes_strip = Adafruit_NeoPixel(NUM_LEDS, NP_EYES_PIN, NEO_GRB + NEO_KHZ800);
    eyes_strip.begin();
    eyes_strip.show();

    /* Setup the UV/A LEDs */
    disable_uva_leds();
}

void display_eyes(void)
{
    static int prev_mode = -1;
    
    if (prev_mode < 0) {
        prev_mode = display.eyes.mode;
    }
    
    switch(prev_mode) {
    case MODE_BLACKLIGHT:
        if (display.eyes.mode != MODE_BLACKLIGHT) {
            /* Switching from blacklight mode to a new mode means disabling
             * the uva leds, lest they remain lit */
            disable_uva_leds();
        }
    default:
        display_funcs[display.eyes.mode]();
        prev_mode = display.eyes.mode;
    }
}
