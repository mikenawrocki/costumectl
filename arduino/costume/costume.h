#ifndef __FURSUIT_H
#define __FURSUIT_H

#include <stdint.h>

#include <LPD8806.h>

// Number of LEDs present (please make it a power of 2...)
#define NUM_LEDS 8

/* PIN ASSIGNMENTS
 * NOTE: Trinket Pro pins 2 and 7 are unavailable
 */
#define LPD8806_DATA_PIN          9
#define LPD8806_CLOCK_PIN         10


#define BLUEFRUIT_UART_MODE_PIN   3
#define BLUEFRUIT_UART_CTS_PIN    4

#define BLUEFRUIT_SWUART_TXD_PIN  5
#define BLUEFRUIT_SWUART_RXD_PIN  6

#define BLUEFRUIT_UART_RTS_PIN    8


struct display_settings {
    uint16_t magic;
    uint8_t mode;
    uint8_t freq_perc;
    uint32_t primary_color;
    uint32_t secondary_color;
} __attribute__((packed));

extern struct display_settings display;

enum {
    MODE_OFF = 0,
    MODE_FIXED,
    MODE_STROBE,
    MODE_FADE,
    MODE_TWO_COLOR,
    MODE_TOP_DOWN,
    MODE_BOTTOM_UP,
    MODE_RAINBOW,

    N_MODES /* Number of modes present */
};


#define GAPDEVNAME "Zaelyx Costume"
#define SUIT_SVC_UUID128         "02-AE-00-7F-02-AE-02-AE-02-AE-02-AE-02-AE-02-AE"
#define SUIT_CTL_CHAR_UUID128    "02-AE-01-7F-02-AE-02-AE-02-AE-02-AE-02-AE-02-AE"
#define SUIT_STATUS_CHAR_UUID128 "02-AE-02-7F-02-AE-02-AE-02-AE-02-AE-02-AE-02-AE"
#define SUIT_ADVDATA         "02-01-06-11-06-AE-02-AE-02-AE-02-AE-02-AE-02-AE-02-7F-00-AE-02"
#define BLE_POLLING_INTVL_MS 512

/* Returns a delay between 40ms and 4096ms based on input percentage */
static inline uint16_t freq_perc_to_delay_ms(uint8_t freq_perc)
{
    return 4096/(freq_perc+1);
}

#define BAT_POLLING_INTVL_MS 8192

extern LPD8806 strip;
extern bool reset_display;
#endif
