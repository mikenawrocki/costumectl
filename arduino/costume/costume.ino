//#include <SPI.h> // Comment out this line if using Trinket or Gemma
#ifdef __AVR_ATtiny85__
#include <avr/power.h>
#endif

#include <SoftwareSerial.h>

#include <stdlib.h>

#include <Adafruit_BLE.h>
#include <Adafruit_BluefruitLE_UART.h>
#include <Adafruit_SleepyDog.h>

#include "color.h"
#include "display.h"
#include "costume.h"

#define DISPLAY_SERIALIZED_LEN (sizeof(struct display_settings) * 3)
#define MAX_CMD_LEN 128

struct display_settings display = {0x7F7F, MODE_FIXED, 85, RED, CYAN};
static char cached_display_settings[DISPLAY_SERIALIZED_LEN];

bool reset_display;
/* Delay between refreshes (in ms) */
static uint16_t freq_delay;

#define VERBOSE_MODE false  // If set to 'true' enables debug output

SoftwareSerial bluefruitSS = SoftwareSerial(BLUEFRUIT_SWUART_TXD_PIN,
                                            BLUEFRUIT_SWUART_RXD_PIN);
Adafruit_BluefruitLE_UART ble(bluefruitSS, BLUEFRUIT_UART_MODE_PIN,
                              BLUEFRUIT_UART_CTS_PIN, BLUEFRUIT_UART_RTS_PIN);

LPD8806 strip = LPD8806(NUM_LEDS << 1, LPD8806_DATA_PIN, LPD8806_CLOCK_PIN);

void show_error(void)
{
  for (;;) {
    digitalWrite(13, HIGH);
    delay(100);
    digitalWrite(13, LOW);
    delay(100);
  }
}


int setup_bluefruit(void)
{
    int32_t gattServiceId;
    int32_t gattNotifiableCharId;
    int32_t gattWritableResponseCharId;
    int32_t gattWritableNoResponseCharId;
    int32_t gattReadableCharId;
    boolean success;

    char cmd[MAX_CMD_LEN];
    randomSeed(micros());

    if (!ble.begin()) {
        return -1;
    }
    ble.reset();
    if (!ble.factoryReset() ) {
        return -1;
    }

    delay(1000);
    ble.sendCommandCheckOK("ATI");
    /* Disable command echo from Bluefruit */
    ble.echo(false);
    ble.setInterCharWriteDelay(5); /* 5MS delay */
    /* Add the Custom GATT Service definition */
    /* Service ID should be 1 */
    success = ble.sendCommandWithIntReply(F("AT+GATTADDSERVICE=UUID128="
                                            SUIT_SVC_UUID128), &gattServiceId);
    if (! success) {
        return -1;
    }

    /* Add the Writable characteristic - an external device writes to this
     * characteristic to change the LED color.
     * Characteristic ID should be 1
     */
    success = ble.sendCommandWithIntReply(
        F("AT+GATTADDCHAR=UUID128=" SUIT_CTL_CHAR_UUID128 ",PROPERTIES=0x08,"
          "MIN_LEN=10,MAX_LEN=20"), &gattWritableNoResponseCharId);
    if (!success) {
        return -1;
    }

    /* Add the Readable characteristic - external devices can query the
     * current LED color using this characteristic.
     * Characteristic ID should be 2
     */
    success = ble.sendCommandWithIntReply(
        F("AT+GATTADDCHAR=UUID128=" SUIT_STATUS_CHAR_UUID128
          ",PROPERTIES=0x02,MIN_LEN=10,MAX_LEN=20"), &gattReadableCharId);
    if (!success) {
        return -1;
    }

    /* Add the Custom GATT Service to the advertising data */
    ble.sendCommandCheckOK(F("AT+GAPSETADVDATA=" SUIT_ADVDATA));
    ble.sendCommandCheckOK(F("AT+GAPDEVNAME=" GAPDEVNAME));

    /* TODO more power savings? */
    /* AT+GAPINTERVALS config */
    ble.sendCommandCheckOK(F("AT+HWMODELED=DISABLE"));  // Disable Mode LED
    ble.sendCommandCheckOK(F("AT+BLEPOWERLEVEL=-20"));  // Lower TX power level

    /* Adjust intervals */
    ble.sendCommandCheckOK(F("AT+GAPINTERVALS=40,200,1024,80"));

    /* Reset the device for the new service setting changes to take effect */
    ble.reset();
    delay(1000);
    ble.echo(false);
    bluefruit_serialize(&display, sizeof(struct display_settings),
                        cached_display_settings, DISPLAY_SERIALIZED_LEN);
    snprintf(cmd, MAX_CMD_LEN, "AT+GATTCHAR=1,%s", cached_display_settings);

    ble.println(cmd);
    if (!ble.waitForOK()) {
        return -1;
    }
    snprintf(cmd, MAX_CMD_LEN, "AT+GATTCHAR=2,%s", cached_display_settings);

    ble.println(cmd);
    if (!ble.waitForOK()) {
        return -1;
    }
    return 0;
}

int is_ble_configured(void)
{
    char cmd[MAX_CMD_LEN];
    char serialized_display[DISPLAY_SERIALIZED_LEN];
    /* Currently uses device name to determine if setup is complete. */
    ble.println("AT+GAPDEVNAME");
    int n_read = ble.readline(5000, false);
    ble.waitForOK();

    return strcmp(ble.buffer, GAPDEVNAME) == 0;
}

void setup() {
    pinMode(13, OUTPUT);
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000L)
    clock_prescale_set(clock_div_1); // Enable 16 MHz on Trinket
#endif

    // Start up the LED strip
    strip.begin();
    strip.show();

    ble.echo(false);

    if (!ble.begin(VERBOSE_MODE)) {
        show_error();
        Serial.println("Failed to set BLE dev!");
    }

    if (!is_ble_configured) {
        if (setup_bluefruit()) {
            show_error();
            Serial.println("Failed to configure BLE device!");
        }
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
            display_funcs[display.mode]();
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

int is_display_valid(struct display_settings *t)
{
    if (!t) {
        return 0;
    }
    if (t->mode >= N_MODES) {
        return 0;
    }
    if (t->freq_perc > 100) {
        return 0;
    }
    if (!is_color_valid(t->primary_color)) {
        return 0;
    }
    if (!is_color_valid(t->secondary_color)) {
        return 0;
    }

    return 1;
}

/*
   Returns 0 for no change, 1 for change, -1 for error
 */
int pull_display_settings(void)
{
    struct display_settings tmp_display;
    char cmd[MAX_CMD_LEN];
    char serialized_display[DISPLAY_SERIALIZED_LEN];
    //check writable characteristic for a new color
    ble.println("AT+GATTCHAR=1");
    int n_read = ble.readline(5000, false);

    if (strncmp(ble.buffer, "OK", DISPLAY_SERIALIZED_LEN) == 0) {
        // no data
        return 0;
    }

    if (n_read < (3 * sizeof(struct display_settings)-1)) {
        ble.waitForOK();
        return 0; // SHORT READ!
    }

    if (!strncmp(ble.buffer, cached_display_settings, DISPLAY_SERIALIZED_LEN)) {
        ble.waitForOK();
        return 0;
    }

    if (bluefruit_deserialize(ble.buffer, BLE_BUFSIZE, &tmp_display,
                sizeof(struct display_settings)) < 0) {
        return -1;
    }

    if (!is_display_valid(&tmp_display)) {
        return -1;
    }

    display = tmp_display;
    strncpy(cached_display_settings, ble.buffer, DISPLAY_SERIALIZED_LEN);

    ble.waitForOK();

    bluefruit_serialize(&display, sizeof(struct display_settings),
                        serialized_display, DISPLAY_SERIALIZED_LEN);
    snprintf(cmd, MAX_CMD_LEN, "AT+GATTCHAR=2,%s", serialized_display);

    ble.println(cmd);
    if (!ble.waitForOK()) {
        return -1;
    }

    reset_display = true;
    freq_delay = freq_perc_to_delay_ms(display.freq_perc);
    return 0;
}

static const char *hex_chars = "0123456789ABCDEF";
static const uint8_t hex_vals[] = {
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xA, 0xB, 0xC,
    0xD, 0xE, 0xF, 0x0, 0x0
};

int bluefruit_serialize(void *src, size_t src_len, char *dst, size_t dst_len)
{
    uint8_t *srcbuf = (uint8_t *)src;
    int i;

    if (dst_len < (3 * src_len)) {
        return -1;
    }

    if (!dst || !src) {
        return -1;
    }

    for (i = 0; i < src_len - 1; ++i) {
        *dst++ = hex_chars[(srcbuf[i] >> 4) & 0xF];
        *dst++ = hex_chars[srcbuf[i] & 0xF];
        *dst++ = '-';
    }

    *dst++ = hex_chars[(srcbuf[i] >> 4) & 0xF];
    *dst++ = hex_chars[srcbuf[i] & 0xF];
    *dst = '\0';
    return 0;

}

int bluefruit_deserialize(char *src, size_t src_len, void *dst, size_t dst_len)
{
    uint8_t *dstbuf = (uint8_t *)dst;
    int i = 0, j = 0;

    if (!src || !dst) {
        return -1;
    }

    while (j + 1 < src_len && src[j]) {
        dstbuf[i++] = hex_vals[src[j]] << 4 | hex_vals[src[j + 1]];
        j += 3;
    }

    return i;
}
