#include "dam_serial.h"
#include <string.h>
#include <stdio.h>
#include "driver/uart.h"
#include "esp_log.h"

static const char *TAG = "DAM";

#define DAC_UART_NUM   UART_NUM_1
#define DAC_UART_TX    43
#define DAC_UART_RX    44
#define DAC_UART_BAUD  115200

void dam_serial_init(void)
{
    const uart_config_t cfg = {
        .baud_rate  = DAC_UART_BAUD,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_param_config  (DAC_UART_NUM, &cfg));
    ESP_ERROR_CHECK(uart_set_pin       (DAC_UART_NUM, DAC_UART_TX, DAC_UART_RX,
                                        UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(DAC_UART_NUM, 256, 0, 0, NULL, 0));
    ESP_LOGI(TAG, "UART1 ready (TX=%d RX=%d @ %d baud)",
             DAC_UART_TX, DAC_UART_RX, DAC_UART_BAUD);
}

static void _send(const char *cmd)
{
    uart_write_bytes(DAC_UART_NUM, cmd,    strlen(cmd));
    uart_write_bytes(DAC_UART_NUM, "\r\n", 2);
    ESP_LOGI(TAG, "→ %s", cmd);
}

void dam_serial_send_volume(int vol)
{
    // vol=99 → V0 (0 dB), vol=50 → V-49 (-49 dB), vol=0 → V-99 (-99 dB)
    // Buffer is 16 to satisfy -Wformat-truncation (GCC estimates max int width)
    char buf[16];
    snprintf(buf, sizeof(buf), "V%d", vol - 99);
    _send(buf);
}

void dam_serial_send_mute(bool muted, int vol)
{
    if (muted) {
        _send("V-99");
    } else {
        dam_serial_send_volume(vol);
    }
}

void dam_serial_send_input(dac_input_t input)
{
    // DAM 1021: I3=AUTO  I0=USB  I1=SPDIF  I2=OPT
    static const char *cmds[DAC_INPUT_COUNT] = { "I3", "I0", "I1", "I2" };
    if ((int)input >= 0 && (int)input < DAC_INPUT_COUNT) {
        _send(cmds[(int)input]);
    }
}

void dam_serial_send_filter(dac_filter_t filter)
{
    // DAM 1021: F4=Linear  F5=Mixed  F6=Minimum  F7=Soft
    static const char *cmds[DAC_FILTER_COUNT] = { "F4", "F5", "F6", "F7" };
    if ((int)filter >= 0 && (int)filter < DAC_FILTER_COUNT) {
        _send(cmds[(int)filter]);
    }
}
