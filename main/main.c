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

// ─── DAC state ────────────────────────────────────────────────────────────────
#define VOL_MIN      0
#define VOL_MAX     99
#define VOL_DEFAULT 50     // -49 dB

static int          s_vol    = VOL_DEFAULT;
static bool         s_muted  = false;
static dac_input_t  s_input  = DAC_INPUT_AUTO;
static dac_filter_t s_filter = DAC_FILTER_LINEAR;

// ─── Action handler ───────────────────────────────────────────────────────────
static void handle_action(dam_action_t action)
{
    switch (action) {

    case ACT_VOL_UP:
        if (!s_muted && s_vol < VOL_MAX) {
            s_vol++;
            dam_serial_send_volume(s_vol);
            dam_ui_set_volume(s_vol, s_muted);
        }
        break;

    case ACT_VOL_DOWN:
        if (!s_muted && s_vol > VOL_MIN) {
            s_vol--;
            dam_serial_send_volume(s_vol);
            dam_ui_set_volume(s_vol, s_muted);
        }
        break;

    case ACT_MUTE:
        s_muted = !s_muted;
        dam_serial_send_mute(s_muted, s_vol);
        dam_ui_set_volume(s_vol, s_muted);
        break;

    case ACT_INPUT_AUTO:  s_input = DAC_INPUT_AUTO;  dam_serial_send_input(s_input);  dam_ui_set_input(s_input);  break;
    case ACT_INPUT_USB:   s_input = DAC_INPUT_USB;   dam_serial_send_input(s_input);  dam_ui_set_input(s_input);  break;
    case ACT_INPUT_SPDIF: s_input = DAC_INPUT_SPDIF; dam_serial_send_input(s_input);  dam_ui_set_input(s_input);  break;
    case ACT_INPUT_OPT:   s_input = DAC_INPUT_OPT;   dam_serial_send_input(s_input);  dam_ui_set_input(s_input);  break;

    case ACT_FILTER_LINEAR:  s_filter = DAC_FILTER_LINEAR;  dam_serial_send_filter(s_filter); dam_ui_set_filter(s_filter); break;
    case ACT_FILTER_MIXED:   s_filter = DAC_FILTER_MIXED;   dam_serial_send_filter(s_filter); dam_ui_set_filter(s_filter); break;
    case ACT_FILTER_MINIMUM: s_filter = DAC_FILTER_MINIMUM; dam_serial_send_filter(s_filter); dam_ui_set_filter(s_filter); break;
    case ACT_FILTER_SOFT:    s_filter = DAC_FILTER_SOFT;    dam_serial_send_filter(s_filter); dam_ui_set_filter(s_filter); break;

    default: break;
    }
}

// ─── Entry point ──────────────────────────────────────────────────────────────
void app_main(void)
{
    I2C_Init();
    EXIO_Init();
    dam_serial_init();
    LCD_Init();
    Touch_Init();
    LVGL_Init();

    // Build UI and apply initial state
    dam_ui_init(handle_action);
    dam_ui_set_volume(s_vol,    s_muted);
    dam_ui_set_input (s_input);
    dam_ui_set_filter(s_filter);

    // Push initial state to DAM 1021
    dam_serial_send_volume(s_vol);
    vTaskDelay(pdMS_TO_TICKS(50));
    dam_serial_send_input(s_input);
    vTaskDelay(pdMS_TO_TICKS(50));
    dam_serial_send_filter(s_filter);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10));
        lv_timer_handler();
    }
}
