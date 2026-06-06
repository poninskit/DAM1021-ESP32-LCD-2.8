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
    uart_write_bytes(DAC_UART_NUM, cmd,   strlen(cmd));
    uart_write_bytes(DAC_UART_NUM, "\r\n", 2);
    ESP_LOGI(TAG, "→ %s", cmd);
}

void dam_serial_send_volume(int vol)
{
    char buf[12];
    snprintf(buf, sizeof(buf), "V%d", vol - 99);   // vol=99 → V0, vol=50 → V-49
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

void dam_serial_send_input(int input)
{
    // DAM 1021: I3=AUTO  I0=USB  I1=SPDIF  I2=OPT
    static const char *cmds[4] = { "I3", "I0", "I1", "I2" };
    if (input >= 0 && input < 4) _send(cmds[input]);
}

void dam_serial_send_filter(int filter)
{
    // DAM 1021: F4=Linear  F5=Mixed  F6=Minimum  F7=Soft
    static const char *cmds[4] = { "F4", "F5", "F6", "F7" };
    if (filter >= 0 && filter < 4) _send(cmds[filter]);
}
