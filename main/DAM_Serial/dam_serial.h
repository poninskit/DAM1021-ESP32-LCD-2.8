#pragma once
#include <stdbool.h>

// Initialise UART1 to talk to the Soekris DAM 1021 at 115200 baud.
// TX → GPIO43 (board UART header TXD)
// RX ← GPIO44 (board UART header RXD)
void dam_serial_init(void);

// Send volume command.  vol 0-99 maps to dB = vol-99 (-99..0 dB).
void dam_serial_send_volume(int vol);

// Send mute: V-99 when muted, otherwise restore vol.
void dam_serial_send_mute(bool muted, int vol);

// Send input select.  0=AUTO(I3)  1=USB(I0)  2=SPDIF(I1)  3=OPT(I2)
void dam_serial_send_input(int input);

// Send filter select.  0=Linear(F4)  1=Mixed(F5)  2=Minimum(F6)  3=Soft(F7)
void dam_serial_send_filter(int filter);
