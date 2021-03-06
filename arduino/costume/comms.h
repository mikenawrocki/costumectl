#ifndef __COMMS_H
#define __COMMS_H

#define GAPDEVNAME "Zaelyx Costume"
#define SUIT_CTL_SVC_UUID128 \
    "02-AE-00-7F-02-AE-02-AE-02-AE-02-AE-02-AE-02-AE"
#define SUIT_CTL_CHANGED_UUID128 \
    "02-AE-01-7F-02-AE-02-AE-02-AE-02-AE-02-AE-02-AE"
#define SUIT_CTL_FREQPERC_UUID128 \
    "02-AE-02-7F-02-AE-02-AE-02-AE-02-AE-02-AE-02-AE"
#define SUIT_CTL_TAIL_UUID128 \
    "02-AE-03-7F-02-AE-02-AE-02-AE-02-AE-02-AE-02-AE"
#define SUIT_CTL_EYES_UUID128 \
    "02-AE-04-7F-02-AE-02-AE-02-AE-02-AE-02-AE-02-AE"
#define SUIT_ADVDATA \
    "02-01-06-11-06-AE-02-AE-02-AE-02-AE-02-AE-02-AE-02-7F-00-AE-02"

#define SUIT_BATT_SVC_UUID 0x180F
#define SUIT_BATT_CHAR_UUID 0x2A19

#define BLE_POLLING_INTVL_MS 512

int init_comms(void);
int pull_display_settings(void);

#endif
