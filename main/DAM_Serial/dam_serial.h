#pragma once
#include <stdbool.h>
#include "dam_ui.h"

// Initialise UART1 to talk to the Soekris DAM 1021 at 115200 baud.
// TX → GPIO43 (board UART header TXD)
// RX ← GPIO44 (board UART header RXD)
void dam_serial_init(void);

// Send volume command.  vol 0-99 maps to dB = vol-99 (-99..0 dB).
void dam_serial_send_volume(int vol);

// Send mute: V-99 when muted, otherwise restore vol.
void dam_serial_send_mute(bool muted, int vol);

// Send input select.
//   DAC_INPUT_AUTO=0  → I3 (auto-detect)
//   DAC_INPUT_USB=1   → I0 (USB)
//   DAC_INPUT_SPDIF=2 → I1 (S/PDIF optical)
//   DAC_INPUT_OPT=3   → I2 (optical/coaxial)
void dam_serial_send_input(dac_input_t input);

// Send filter select.
//   DAC_FILTER_LINEAR=0  → F4
//   DAC_FILTER_MIXED=1   → F5
//   DAC_FILTER_MINIMUM=2 → F6
//   DAC_FILTER_SOFT=3    → F7
void dam_serial_send_filter(dac_filter_t filter);
