#ifndef __COSTUME_H
#define __COSTUME_H

#include <stdint.h>

/* PIN ASSIGNMENTS
 * NOTE: Trinket Pro pins 2 and 7 are unavailable
 */
#define LPD8806_DATA_PIN          9
#define LPD8806_CLOCK_PIN         10

#define NP_EYES_PIN               11
#define UVA_EYES_PIN              12

#define BLUEFRUIT_UART_MODE_PIN   3
#define BLUEFRUIT_UART_CTS_PIN    4
#define BLUEFRUIT_SWUART_TXD_PIN  5
#define BLUEFRUIT_SWUART_RXD_PIN  6
#define BLUEFRUIT_UART_RTS_PIN    8


struct component_config {
    uint16_t magic;
    uint16_t mode;
    uint32_t primary_color;
    uint32_t secondary_color;
} __attribute__((packed));

struct display_settings {
    uint8_t  freq_perc;
    struct component_config tail;
    struct component_config eyes;
};

extern struct display_settings display;

/* Returns a delay between 40ms and 4096ms based on input percentage */
static inline uint16_t freq_perc_to_delay_ms(uint8_t freq_perc)
{
    return 4096/(freq_perc+1);
}

#define BAT_POLLING_INTVL_MS 8192

extern bool reset_display;
extern uint16_t freq_delay;
#endif
