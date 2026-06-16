#pragma once
#include <stdbool.h>
#include "lvgl.h"

// ─── Input sources ────────────────────────────────────────────────────────────
// Matches DAM 1021 I-command mapping
typedef enum {
    DAC_INPUT_AUTO  = 0,   // I3 – auto-detect
    DAC_INPUT_USB   = 1,   // I0 – USB
    DAC_INPUT_SPDIF = 2,   // I1 – S/PDIF optical
    DAC_INPUT_OPT   = 3,   // I2 – optical/coaxial
} dac_input_t;
#define DAC_INPUT_COUNT 4

// ─── Filter modes ─────────────────────────────────────────────────────────────
typedef enum {
    DAC_FILTER_LINEAR  = 0,  // F4
    DAC_FILTER_MIXED   = 1,  // F5
    DAC_FILTER_MINIMUM = 2,  // F6
    DAC_FILTER_SOFT    = 3,  // F7
} dac_filter_t;
#define DAC_FILTER_COUNT 4

// ─── Colour themes ────────────────────────────────────────────────────────────
#define THEME_COUNT 5

// ─── User actions ─────────────────────────────────────────────────────────────
typedef enum {
    ACT_NONE = 0,

    // Volume – arc widget sends ACT_VOL_SET with new value; remote sends UP/DOWN
    ACT_VOL_SET,        // value = new volume 0-99 (from arc drag)
    ACT_VOL_UP,         // +1 step (from IR remote)
    ACT_VOL_DOWN,       // -1 step (from IR remote)
    ACT_MUTE,

    // Input – direct select via touch buttons
    ACT_INPUT_AUTO,
    ACT_INPUT_USB,
    ACT_INPUT_SPDIF,
    ACT_INPUT_OPT,

    // Filter – direct select via touch buttons
    ACT_FILTER_LINEAR,
    ACT_FILTER_MIXED,
    ACT_FILTER_MINIMUM,
    ACT_FILTER_SOFT,

    // Input / filter cycle – from IR remote or touch button
    ACT_CHANNEL_LEFT,   // cycle input backward
    ACT_CHANNEL_RIGHT,  // cycle input forward
    ACT_FILTER_CYCLE,   // cycle filter forward
    ACT_FILTER_BACK,    // cycle filter backward

    // Style/theme cycle – from touch button
    ACT_STYLE_CYCLE,

    // Brightness – from UI slider
    ACT_BRIGHTNESS_SET, // value = new brightness 10-100
} dam_action_t;

// Callback – fires on touch or remote event.
// 'value' is only meaningful for ACT_VOL_SET (new volume 0-99), 0 otherwise.
typedef void (*dam_action_cb_t)(dam_action_t action, int value);

// ─── API ──────────────────────────────────────────────────────────────────────
void dam_ui_init(dam_action_cb_t cb);

// Sync display after state changes
void dam_ui_set_volume    (int vol, bool muted);   // vol 0-99
void dam_ui_set_input     (dac_input_t  input);
void dam_ui_set_filter    (dac_filter_t filter);
void dam_ui_set_brightness(int brightness);        // 10-100
void dam_ui_set_theme     (int theme_idx);         // 0 … THEME_COUNT-1
