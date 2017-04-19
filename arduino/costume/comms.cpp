#ifdef __AVR_ATtiny85__
#include <avr/power.h>
#endif
#include <stdlib.h>
#include <SoftwareSerial.h>

#include <Adafruit_BLE.h>
#include <Adafruit_BluefruitLE_UART.h>
#include <Adafruit_BLEGatt.h>

#include "costume.h"
#include "comms.h"
#include "tail.h"
#include "snout.h"

#define MAX_CMD_LEN 128
#define VERBOSE_MODE false  // If set to 'true' enables debug output

/* UUIDS */
static uint8_t suit_ctl_svc_uuid[] = {
    0x02, 0xAE, 0x00, 0x7F, 0x02, 0xAE, 0x02, 0xAE, 0x02, 0xAE, 0x02, 0xAE,
    0x02, 0xAE, 0x02, 0xAE
};
static uint8_t suit_ctl_changed_uuid[] = {
    0x02, 0xAE, 0x01, 0x7F, 0x02, 0xAE, 0x02, 0xAE, 0x02, 0xAE, 0x02, 0xAE,
    0x02, 0xAE, 0x02, 0xAE
};
static uint8_t suit_ctl_freqperc_uuid[] = {
    0x02, 0xAE, 0x02, 0x7F, 0x02, 0xAE, 0x02, 0xAE, 0x02, 0xAE, 0x02, 0xAE,
    0x02, 0xAE, 0x02, 0xAE
};
static uint8_t suit_ctl_tail_uuid[] = {
    0x02, 0xAE, 0x03, 0x7F, 0x02, 0xAE, 0x02, 0xAE, 0x02, 0xAE, 0x02, 0xAE,
    0x02, 0xAE, 0x02, 0xAE
};
static uint8_t suit_ctl_eyes_uuid[] = {
    0x02, 0xAE, 0x04, 0x7F, 0x02, 0xAE, 0x02, 0xAE, 0x02, 0xAE, 0x02, 0xAE,
    0x02, 0xAE, 0x02, 0xAE
};


static SoftwareSerial bluefruitSS(BLUEFRUIT_SWUART_TXD_PIN,
                                  BLUEFRUIT_SWUART_RXD_PIN);
static Adafruit_BluefruitLE_UART ble(bluefruitSS, BLUEFRUIT_UART_MODE_PIN,
                                     BLUEFRUIT_UART_CTS_PIN,
                                     BLUEFRUIT_UART_RTS_PIN);

static Adafruit_BLEGatt gatt(ble);

static uint8_t cached_changed = 0;


static int bluefruit_serialize(void *src, size_t src_len, char *dst,
                               size_t dst_len);
static int bluefruit_deserialize(char *src, size_t src_len, void *dst,
                                 size_t dst_len);


static int setup_bluefruit(void)
{
    int8_t gattServiceId;
    int8_t gattChangedCharId;
    int8_t gattTailCharId;
    int8_t gattEyesCharId;
    int8_t gattFreqPercCharId;
    boolean success;

    char cmd[MAX_CMD_LEN];
    randomSeed(micros());

    //if (!ble.begin()) {
    //    return -1;
    //}
    ble.reset();
    delay(500);
    if (!ble.factoryReset() ) {
        /* Try one last time... */
        delay(200);
        if (!ble.factoryReset()) {
            return -1;
        }
    }

    delay(1000);
    /* Disable command echo from Bluefruit */
    ble.echo(false);
    ble.setInterCharWriteDelay(5); /* 5MS delay */
    /* Add the Custom GATT Service definition */
    /* Service ID should be 1 */
    gattServiceId = gatt.addService(suit_ctl_svc_uuid);
    if (!gattServiceId) {
        return -1;
    }

    /* Changed characteristic - indicates that configuration has changed and
     * the settings must be pulled from the bluetooth chip.
     * Characteristic ID should be 1
     */
    gattChangedCharId = gatt.addCharacteristic(suit_ctl_changed_uuid,
            GATT_CHARS_PROPERTIES_READ | GATT_CHARS_PROPERTIES_WRITE |
            GATT_CHARS_PROPERTIES_WRITE_WO_RESP, 1, 1, BLE_DATATYPE_AUTO);
    if (!gattChangedCharId) {
        return -1;
    }

    /* FREQPERC characteristic - Stores the speed in terms of frequency
     * percentage.
     * Characteristic ID should be 2
     */
    gattFreqPercCharId = gatt.addCharacteristic(suit_ctl_freqperc_uuid,
            GATT_CHARS_PROPERTIES_READ | GATT_CHARS_PROPERTIES_WRITE |
            GATT_CHARS_PROPERTIES_WRITE_WO_RESP, 1, 1, BLE_DATATYPE_AUTO);
    if (!gattFreqPercCharId) {
        return -1;
    }

    /* TAIL characteristic - Stores the configuration settings of the tail.
     * Characteristic ID should be 3
     */
    gattTailCharId = gatt.addCharacteristic(suit_ctl_tail_uuid,
            GATT_CHARS_PROPERTIES_READ | GATT_CHARS_PROPERTIES_WRITE |
            GATT_CHARS_PROPERTIES_WRITE_WO_RESP,
            sizeof(struct component_config), sizeof(struct component_config),
            BLE_DATATYPE_AUTO);
    if (!gattTailCharId) {
        return -1;
    }

    /* EYES characteristic - Stores the configuration settings of the eyes.
     * Characteristic ID should be 4
     */
    gattEyesCharId = gatt.addCharacteristic(suit_ctl_eyes_uuid,
            GATT_CHARS_PROPERTIES_READ | GATT_CHARS_PROPERTIES_WRITE |
            GATT_CHARS_PROPERTIES_WRITE_WO_RESP,
            sizeof(struct component_config), sizeof(struct component_config),
            BLE_DATATYPE_AUTO);
    if (!gattEyesCharId) {
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

    return 0;
}

static int is_ble_configured(void)
{
    char cmd[MAX_CMD_LEN];
    /* Currently uses device name to determine if setup is complete. */
    ble.println("AT+GAPDEVNAME");
    int n_read = ble.readline(5000, false);
    ble.waitForOK();

    return strcmp(ble.buffer, GAPDEVNAME) == 0;
}

static int is_display_valid(struct display_settings *t)
{
    if (!t) {
        return 0;
    }
    if (t->freq_perc > 100) {
        return 0;
    }
    if (!validate_tail(&t->tail)) {
        return 0;
    }
    if (!validate_eyes(&t->eyes)) {
        return 0;
    }

    return 1;
}

/* Returns 0 for no change, 1 for change, -1 for error */
int pull_display_settings(void)
{
    struct display_settings tmp_display;
    char cmd[MAX_CMD_LEN];
    uint8_t changed;

    //check changed characteristic to see if things have changed
    changed = gatt.getCharInt8(1);

    if (changed == cached_changed) {
        return 0;
    }

    cached_changed = changed;

    gatt.getChar(2, (uint8_t *)&tmp_display.freq_perc, 1);
    gatt.getChar(3, (uint8_t *)&tmp_display.tail,
                 sizeof(struct component_config));
    gatt.getChar(4, (uint8_t *)&tmp_display.eyes,
                 sizeof(struct component_config));

    if (!is_display_valid(&tmp_display)) {
        return -1;
    }

    gatt.setChar(1, changed);
    gatt.setChar(2, tmp_display.freq_perc);
    gatt.setChar(3, (uint8_t *)&tmp_display.tail,
                 sizeof(struct component_config));
    gatt.setChar(4, (uint8_t *)&tmp_display.eyes,
                 sizeof(struct component_config));

    /* TODO these shouldn't be here...? */
    display = tmp_display;
    reset_display = true;
    freq_delay = freq_perc_to_delay_ms(display.freq_perc);

    return 0;
}

int init_comms(void)
{
    ble.echo(false);
    if (!ble.begin(VERBOSE_MODE)) {
        return -1;
    }
    if (!is_ble_configured()) {
        if (setup_bluefruit()) {
            return -2;
        }
    }

    return 0;
}
