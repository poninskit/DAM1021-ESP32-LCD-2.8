#pragma once
#include <stdbool.h>
#include "dam_ui.h"    // for dac_input_t, dac_filter_t

// Volume scale: 0–99  (vol=99 → 0 dB, vol=50 → -49 dB, vol=0 → -99 dB)
#define VOL_MIN      0
#define VOL_MAX      99
#define VOL_DEFAULT  50

// LCD backlight: 10–100 %
#define BRIGHTNESS_MIN      10
#define BRIGHTNESS_MAX     100
#define BRIGHTNESS_DEFAULT  80

// DAC state bundle (persisted to NVS flash)
typedef struct {
    int          volume;      // 0-99
    bool         muted;
    dac_input_t  input;
    dac_filter_t filter;
    int          brightness;  // 10-100
} dam_state_t;

// Default initialiser
#define DAM_STATE_DEFAULT { .volume = VOL_DEFAULT, .muted = false, \
                            .input = DAC_INPUT_AUTO, .filter = DAC_FILTER_LINEAR, \
                            .brightness = BRIGHTNESS_DEFAULT }

// Initialise NVS subsystem (call once in app_main before load/save).
void dam_nvs_init(void);

// Load persisted state into *state.  Fills defaults for missing keys.
void dam_nvs_load(dam_state_t *state);

// Persist current state to flash.
void dam_nvs_save(const dam_state_t *state);
