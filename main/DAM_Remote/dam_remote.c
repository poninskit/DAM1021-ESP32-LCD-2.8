#include "dam_remote.h"
#include "driver/rmt_rx.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string.h>

static const char *TAG = "IR";

// ─── NEC timing constants (μs, 1MHz RMT resolution) ─────────────────────────
#define NEC_RES_HZ         1000000   // 1 MHz → 1 tick = 1 μs
#define NEC_LEADER_MIN     8000      // leader burst ≥ 8 ms
#define NEC_LEADER_MAX     10000     // leader burst ≤ 10 ms
#define NEC_DATA_SPACE     3500      // leader space > 3.5 ms → data frame
#define NEC_REPEAT_MAX     3000      // leader space < 3 ms → repeat frame
#define NEC_BIT_PULSE_MIN  300       // data bit pulse ≥ 300 μs
#define NEC_BIT_PULSE_MAX  900       // data bit pulse ≤ 900 μs (nominal 562)
#define NEC_BIT1_SPACE_MIN 1100      // bit-1 space ≥ 1.1 ms (nominal 1687)
#define NEC_BITS           32

// Apple aluminium remote address (standard NEC LSB-first decoding)
#define APPLE_ADDR  0x77E1U

// ─── Module state ─────────────────────────────────────────────────────────────
static rmt_channel_handle_t  s_rx_ch   = NULL;
static QueueHandle_t          s_rmt_q   = NULL;  // RMT ISR → decoder task
static QueueHandle_t          s_act_q   = NULL;  // decoder task → poll()

// Buffer for RMT symbols – static; must remain valid until decoder task copies it
static rmt_symbol_word_t s_symbols[64];

// Last decoded action for repeat frames
static dam_action_t s_last_action = ACT_NONE;
static bool         s_is_repeat   = false;

// 200 ms debounce (applies to non-repeat frames)
static int64_t s_last_us = 0;
#define DEBOUNCE_US  200000LL

// ─── RMT ISR callback (called from ISR context) ───────────────────────────────
static bool IRAM_ATTR rmt_rx_done(rmt_channel_handle_t ch,
    const rmt_rx_done_event_data_t *edata, void *ctx)
{
    BaseType_t hp = pdFALSE;
    xQueueSendFromISR((QueueHandle_t)ctx, edata, &hp);
    return hp == pdTRUE;
}

// ─── NEC decoder ─────────────────────────────────────────────────────────────
static bool _in_range(uint32_t v, uint32_t lo, uint32_t hi)
{
    return (v >= lo && v <= hi);
}

// Returns the decoded action, or ACT_NONE on failure.
// Sets s_is_repeat before returning.
static dam_action_t _decode_nec(const rmt_symbol_word_t *syms, size_t n)
{
    if (n < 2) return ACT_NONE;

    // sym[0]: level0=0 (LOW burst), level1=1 (HIGH space)
    uint32_t burst = syms[0].duration0;
    uint32_t space = syms[0].duration1;

    // Leader burst must be ~9 ms
    if (!_in_range(burst, NEC_LEADER_MIN, NEC_LEADER_MAX)) return ACT_NONE;

    // ── Repeat frame: ~9 ms burst + ~2.25 ms space ───────────────────────────
    if (space < NEC_REPEAT_MAX) {
        s_is_repeat = true;
        return s_last_action;
    }

    // ── Data frame: ~9 ms burst + ~4.5 ms space ──────────────────────────────
    if (space < NEC_DATA_SPACE) return ACT_NONE;
    s_is_repeat = false;

    if (n < (size_t)(NEC_BITS + 1)) return ACT_NONE;  // need 33 symbols

    uint32_t raw = 0;
    for (int i = 0; i < NEC_BITS; i++) {
        uint32_t bp = syms[i + 1].duration0;   // burst pulse
        uint32_t bs = syms[i + 1].duration1;   // following space

        if (!_in_range(bp, NEC_BIT_PULSE_MIN, NEC_BIT_PULSE_MAX)) return ACT_NONE;

        if (bs >= NEC_BIT1_SPACE_MIN) {
            raw |= (1u << i);   // bit = 1 (long space)
        }
        // else bit = 0 (short space, nominal 562 μs)
    }

    // NEC 32-bit layout (LSB first):
    //   bits  0-15 = 16-bit address  (addr_lo | addr_hi<<8)
    //   bits 16-23 = command byte
    //   bits 24-31 = ~command byte   (ignored for extended NEC)
    uint16_t address = (uint16_t)(raw & 0xFFFF);
    uint8_t  command = (uint8_t)((raw >> 16) & 0xFF);

    ESP_LOGD(TAG, "NEC addr=0x%04X cmd=0x%02X", address, command);

    if (address != APPLE_ADDR) {
        ESP_LOGD(TAG, "Unknown remote 0x%04X, ignoring", address);
        return ACT_NONE;
    }

    switch (command) {
        case 0x0B: return ACT_CHANNEL_LEFT;  // Up    → cycle input backward
        case 0x0D: return ACT_CHANNEL_RIGHT; // Down  → cycle input forward
        case 0x07: return ACT_VOL_UP;        // Right → volume up
        case 0x08: return ACT_VOL_DOWN;      // Left  → volume down
        case 0x5D: return ACT_MUTE;          // Centre→ mute toggle
        case 0x02: return ACT_FILTER_CYCLE;  // Menu  → cycle filter
        default:
            ESP_LOGD(TAG, "Unmapped cmd=0x%02X", command);
            return ACT_NONE;
    }
}

// ─── Decoder task ─────────────────────────────────────────────────────────────
static void remote_task(void *arg)
{
    static const rmt_receive_config_t recv_cfg = {
        .signal_range_min_ns = 1250,        // filter glitches < 1.25 μs
        .signal_range_max_ns = 12500000,    // max: 12.5 ms (NEC leader = 9 ms)
    };

    // Arm first receive
    ESP_ERROR_CHECK(rmt_receive(s_rx_ch, s_symbols, sizeof(s_symbols), &recv_cfg));

    rmt_rx_done_event_data_t edata;
    while (1) {
        if (xQueueReceive(s_rmt_q, &edata, portMAX_DELAY) != pdTRUE) continue;

        // Decode *before* re-arming (s_symbols may be overwritten afterwards)
        dam_action_t act = _decode_nec(edata.received_symbols, edata.num_symbols);

        // Re-arm RMT receive
        rmt_receive(s_rx_ch, s_symbols, sizeof(s_symbols), &recv_cfg);

        if (act == ACT_NONE) continue;

        // Debounce non-repeat frames
        int64_t now = esp_timer_get_time();
        if (!s_is_repeat && (now - s_last_us) < DEBOUNCE_US) continue;
        s_last_us = now;

        s_last_action = act;

        // Post to action queue (drop if full – caller is too slow)
        xQueueSend(s_act_q, &act, 0);
        ESP_LOGD(TAG, "action=%d repeat=%d", act, s_is_repeat);
    }
}

// ─── Public API ───────────────────────────────────────────────────────────────
void dam_remote_init(int gpio)
{
    // Queues
    s_rmt_q = xQueueCreate(4, sizeof(rmt_rx_done_event_data_t));
    s_act_q = xQueueCreate(8, sizeof(dam_action_t));

    // RMT RX channel
    rmt_rx_channel_config_t rx_cfg = {
        .gpio_num           = gpio,
        .clk_src            = RMT_CLK_SRC_DEFAULT,
        .resolution_hz      = NEC_RES_HZ,
        .mem_block_symbols  = 64,
    };
    ESP_ERROR_CHECK(rmt_new_rx_channel(&rx_cfg, &s_rx_ch));

    // Register done-callback
    rmt_rx_event_callbacks_t cbs = {
        .on_recv_done = rmt_rx_done,
    };
    ESP_ERROR_CHECK(rmt_rx_register_event_callbacks(s_rx_ch, &cbs, s_rmt_q));

    // Enable channel
    ESP_ERROR_CHECK(rmt_enable(s_rx_ch));

    // Decoder task
    xTaskCreate(remote_task, "ir_dec", 3072, NULL, 5, NULL);

    ESP_LOGI(TAG, "IR remote ready on GPIO%d", gpio);
}

dam_action_t dam_remote_poll(void)
{
    dam_action_t act = ACT_NONE;
    if (s_act_q) {
        xQueueReceive(s_act_q, &act, 0);   // non-blocking
    }
    return act;
}

bool dam_remote_is_repeat(void)
{
    return s_is_repeat;
}
