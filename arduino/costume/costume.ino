//#include <SPI.h> // Comment out this line if using Trinket or Gemma
#ifdef __AVR_ATtiny85__
#include <avr/power.h>
#endif

#include <stdlib.h>
#include <Adafruit_SleepyDog.h>

#include "comms.h"
#include "costume.h"
#include "tail.h"
#include "snout.h"

struct display_settings display;

bool reset_display;
/* Delay between refreshes (in ms) */
uint16_t freq_delay;

void show_error(void)
{
  for (;;) {
    digitalWrite(13, HIGH);
    delay(100);
    digitalWrite(13, LOW);
    delay(100);
  }
}

void setup() {
    pinMode(13, OUTPUT);

#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000L)
    clock_prescale_set(clock_div_1); // Enable 16 MHz on Trinket
#endif

    init_tail();
    init_eyes();
    if(init_comms() != 0) {
        show_error();
    }

    freq_delay = freq_perc_to_delay_ms(display.freq_perc);
    reset_display = true;
}

void loop() {
    unsigned long to_sleep_ms;
    unsigned long slept_ms = 0;
    unsigned long last_ble_poll = 0;
    unsigned long last_led_upd = 0;

    for (;;) {

        if ((slept_ms - last_led_upd) >= freq_delay) {
            display_tail();
            display_eyes();
            last_led_upd = slept_ms;
        }
        if ((slept_ms - last_ble_poll) >= BLE_POLLING_INTVL_MS) {
            last_ble_poll = slept_ms;
            pull_display_settings();
        }

        to_sleep_ms = min(freq_delay, BLE_POLLING_INTVL_MS);
        if (to_sleep_ms & (1ULL << 31)) {
            continue;
        }

        slept_ms += Watchdog.sleep(to_sleep_ms);
    }
}
