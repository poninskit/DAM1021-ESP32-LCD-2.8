#include "dam_nvs.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

static const char *TAG      = "NVS";
static const char *NS       = "dam1021";

void dam_nvs_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition erased, reinitialising");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    } else {
        ESP_ERROR_CHECK(err);
    }
    ESP_LOGI(TAG, "NVS ready");
}

void dam_nvs_load(dam_state_t *state)
{
    // Start from defaults so missing keys are handled gracefully
    dam_state_t def = DAM_STATE_DEFAULT;
    *state = def;

    nvs_handle_t h;
    if (nvs_open(NS, NVS_READONLY, &h) != ESP_OK) {
        ESP_LOGW(TAG, "No saved state, using defaults");
        return;
    }

    int32_t vol;
    if (nvs_get_i32(h, "volume", &vol) == ESP_OK) {
        state->volume = (int)vol;
        if (state->volume < VOL_MIN) state->volume = VOL_MIN;
        if (state->volume > VOL_MAX) state->volume = VOL_MAX;
    }

    uint8_t muted;
    if (nvs_get_u8(h, "muted", &muted) == ESP_OK) {
        state->muted = (muted != 0);
    }

    int8_t input;
    if (nvs_get_i8(h, "input", &input) == ESP_OK &&
        input >= 0 && input < DAC_INPUT_COUNT) {
        state->input = (dac_input_t)input;
    }

    int8_t filter;
    if (nvs_get_i8(h, "filter", &filter) == ESP_OK &&
        filter >= 0 && filter < DAC_FILTER_COUNT) {
        state->filter = (dac_filter_t)filter;
    }

    nvs_close(h);
    ESP_LOGI(TAG, "Loaded vol=%d muted=%d input=%d filter=%d",
             state->volume, state->muted, state->input, state->filter);
}

void dam_nvs_save(const dam_state_t *state)
{
    nvs_handle_t h;
    if (nvs_open(NS, NVS_READWRITE, &h) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS for write");
        return;
    }

    nvs_set_i32(h, "volume", (int32_t)state->volume);
    nvs_set_u8 (h, "muted",  state->muted ? 1 : 0);
    nvs_set_i8 (h, "input",  (int8_t)state->input);
    nvs_set_i8 (h, "filter", (int8_t)state->filter);
    nvs_commit(h);
    nvs_close(h);
    ESP_LOGD(TAG, "Saved vol=%d muted=%d input=%d filter=%d",
             state->volume, state->muted, state->input, state->filter);
}
