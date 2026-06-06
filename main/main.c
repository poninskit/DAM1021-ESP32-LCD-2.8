/*******************************************************************************
 * DAM 1021 Controller – ESP32-S3 / 480×640 RGB LCD
 *
 * Controls the Soekris DAM 1021 DAC over hardware UART (GPIO43/44).
 * UI: LVGL v8 on a 480×640 RGB touchscreen.
 * Remote: Apple aluminium IR remote (NEC protocol) on GPIO0.
 * State: persisted to NVS flash across power cycles.
 *
 * DAM 1021 serial protocol (@115200 baud):
 *   V<n>   Volume   (V0=0dB … V-99=-99dB)
 *   I<n>   Input    (I3=AUTO  I0=USB  I1=SPDIF  I2=OPT)
 *   F<n>   Filter   (F4=Linear  F5=Mixed  F6=Minimum  F7=Soft)
 ******************************************************************************/

#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "I2C_Driver.h"
#include "TCA9554PWR.h"
#include "ST7701S.h"
#include "GT911.h"
#include "LVGL_Driver.h"
#include "lvgl.h"
#include "dam_ui.h"
#include "dam_serial.h"
#include "dam_nvs.h"
#include "dam_remote.h"

// ─── DAC state ────────────────────────────────────────────────────────────────
static dam_state_t s_state = DAM_STATE_DEFAULT;

// ─── Forward declarations ─────────────────────────────────────────────────────
static void handle_action(dam_action_t action, int value);

// ─── Action handler ───────────────────────────────────────────────────────────
static void handle_action(dam_action_t action, int value)
{
    switch (action) {

    // ── Volume (arc) ──────────────────────────────────────────────────────────
    case ACT_VOL_SET:
        if (value >= VOL_MIN && value <= VOL_MAX) {
            s_state.volume = value;
            if (!s_state.muted) dam_serial_send_volume(s_state.volume);
            dam_ui_set_volume(s_state.volume, s_state.muted);
            dam_nvs_save(&s_state);
        }
        break;

    // ── Volume (remote ±1 step) ───────────────────────────────────────────────
    case ACT_VOL_UP:
        if (!s_state.muted && s_state.volume < VOL_MAX) {
            s_state.volume++;
            dam_serial_send_volume(s_state.volume);
            dam_ui_set_volume(s_state.volume, s_state.muted);
            dam_nvs_save(&s_state);
        }
        break;

    case ACT_VOL_DOWN:
        if (!s_state.muted && s_state.volume > VOL_MIN) {
            s_state.volume--;
            dam_serial_send_volume(s_state.volume);
            dam_ui_set_volume(s_state.volume, s_state.muted);
            dam_nvs_save(&s_state);
        }
        break;

    // ── Mute ──────────────────────────────────────────────────────────────────
    case ACT_MUTE:
        s_state.muted = !s_state.muted;
        dam_serial_send_mute(s_state.muted, s_state.volume);
        dam_ui_set_volume(s_state.volume, s_state.muted);
        dam_nvs_save(&s_state);
        break;

    // ── Input (direct touch select) ───────────────────────────────────────────
    case ACT_INPUT_AUTO:  s_state.input = DAC_INPUT_AUTO;  goto send_input;
    case ACT_INPUT_USB:   s_state.input = DAC_INPUT_USB;   goto send_input;
    case ACT_INPUT_SPDIF: s_state.input = DAC_INPUT_SPDIF; goto send_input;
    case ACT_INPUT_OPT:   s_state.input = DAC_INPUT_OPT;   goto send_input;
    send_input:
        dam_serial_send_input(s_state.input);
        dam_ui_set_input(s_state.input);
        dam_nvs_save(&s_state);
        break;

    // ── Input (remote cycle) ──────────────────────────────────────────────────
    case ACT_CHANNEL_LEFT:
        s_state.input = (dac_input_t)(((int)s_state.input - 1 + DAC_INPUT_COUNT)
                                       % DAC_INPUT_COUNT);
        dam_serial_send_input(s_state.input);
        dam_ui_set_input(s_state.input);
        dam_nvs_save(&s_state);
        break;

    case ACT_CHANNEL_RIGHT:
        s_state.input = (dac_input_t)(((int)s_state.input + 1) % DAC_INPUT_COUNT);
        dam_serial_send_input(s_state.input);
        dam_ui_set_input(s_state.input);
        dam_nvs_save(&s_state);
        break;

    // ── Filter (direct touch select) ──────────────────────────────────────────
    case ACT_FILTER_LINEAR:  s_state.filter = DAC_FILTER_LINEAR;  goto send_filter;
    case ACT_FILTER_MIXED:   s_state.filter = DAC_FILTER_MIXED;   goto send_filter;
    case ACT_FILTER_MINIMUM: s_state.filter = DAC_FILTER_MINIMUM; goto send_filter;
    case ACT_FILTER_SOFT:    s_state.filter = DAC_FILTER_SOFT;    goto send_filter;
    send_filter:
        dam_serial_send_filter(s_state.filter);
        dam_ui_set_filter(s_state.filter);
        dam_nvs_save(&s_state);
        break;

    // ── Filter (remote / nav button cycle) ───────────────────────────────────
    case ACT_FILTER_CYCLE:
        s_state.filter = (dac_filter_t)(((int)s_state.filter + 1)
                                         % DAC_FILTER_COUNT);
        dam_serial_send_filter(s_state.filter);
        dam_ui_set_filter(s_state.filter);
        dam_nvs_save(&s_state);
        break;

    case ACT_FILTER_BACK:
        s_state.filter = (dac_filter_t)(((int)s_state.filter - 1 + DAC_FILTER_COUNT)
                                         % DAC_FILTER_COUNT);
        dam_serial_send_filter(s_state.filter);
        dam_ui_set_filter(s_state.filter);
        dam_nvs_save(&s_state);
        break;

    default:
        break;
    }
}

// ─── Entry point ──────────────────────────────────────────────────────────────
void app_main(void)
{
    // NVS must come first so state is ready before UI is built
    dam_nvs_init();
    dam_nvs_load(&s_state);

    // Hardware peripherals
    I2C_Init();
    EXIO_Init();
    dam_serial_init();
    LCD_Init();
    Touch_Init();
    LVGL_Init();

    // Build UI and apply persisted state
    dam_ui_init(handle_action);
    dam_ui_set_volume(s_state.volume, s_state.muted);
    dam_ui_set_input (s_state.input);
    dam_ui_set_filter(s_state.filter);

    // IR remote (GPIO0 – BOOT pad, usable as input after boot)
    dam_remote_init(DAM_IR_GPIO);

    // Push initial state to DAM 1021
    dam_serial_send_mute(s_state.muted, s_state.volume);
    vTaskDelay(pdMS_TO_TICKS(50));
    dam_serial_send_input (s_state.input);
    vTaskDelay(pdMS_TO_TICKS(50));
    dam_serial_send_filter(s_state.filter);

    // Main loop: drive LVGL + poll IR remote
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10));
        lv_timer_handler();

        dam_action_t act = dam_remote_poll();
        if (act != ACT_NONE) {
            // Skip 200 ms delay between non-repeat volume steps for ramping
            handle_action(act, 0);
        }
    }
}
