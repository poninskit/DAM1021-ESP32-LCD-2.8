#pragma once
#include <stdbool.h>
#include "lvgl.h"

// Input sources
typedef enum {
    DAC_INPUT_AUTO  = 0,
    DAC_INPUT_USB   = 1,
    DAC_INPUT_SPDIF = 2,
    DAC_INPUT_OPT   = 3,
} dac_input_t;

// Digital filters
typedef enum {
    DAC_FILTER_LINEAR  = 0,
    DAC_FILTER_MIXED   = 1,
    DAC_FILTER_MINIMUM = 2,
    DAC_FILTER_SOFT    = 3,
} dac_filter_t;

// All user actions from touch
typedef enum {
    ACT_NONE = 0,
    ACT_VOL_UP,
    ACT_VOL_DOWN,
    ACT_MUTE,
    ACT_INPUT_AUTO,
    ACT_INPUT_USB,
    ACT_INPUT_SPDIF,
    ACT_INPUT_OPT,
    ACT_FILTER_LINEAR,
    ACT_FILTER_MIXED,
    ACT_FILTER_MINIMUM,
    ACT_FILTER_SOFT,
} dam_action_t;

typedef void (*dam_action_cb_t)(dam_action_t action);

// Build the full LVGL widget tree and register action callback.
void dam_ui_init(dam_action_cb_t cb);

// Update display to reflect new state (call after state changes).
void dam_ui_set_volume(int vol, bool muted);   // vol 0-99, dB = vol-99
void dam_ui_set_input (dac_input_t  input);
void dam_ui_set_filter(dac_filter_t filter);
