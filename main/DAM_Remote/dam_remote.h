#pragma once
#include "dam_ui.h"    // for dam_action_t

/*******************************************************************************
 * IR Remote – Apple aluminium remote, NEC 32-bit extended protocol.
 *
 * Wiring: IR receiver (e.g. TSOP4838) signal pin → IR_GPIO (default GPIO0).
 *   TSOP output is active-LOW: LOW when IR burst is received.
 *
 * Apple remote NEC mapping (address = 0x87EE):
 *   Up     (0x0B) → ACT_CHANNEL_LEFT   cycle input backward
 *   Down   (0x0D) → ACT_CHANNEL_RIGHT  cycle input forward
 *   Right  (0x07) → ACT_VOL_UP
 *   Left   (0x08) → ACT_VOL_DOWN
 *   Centre (0x5D) → ACT_MUTE
 *   Menu   (0x02) → ACT_FILTER_CYCLE
 *
 * Initialise dam_remote_init() once.  Then call dam_remote_poll() from the
 * main loop; it is non-blocking and returns ACT_NONE when the queue is empty.
 * dam_remote_is_repeat() returns true if the last poll returned a repeat frame
 * (useful to implement volume ramping without adding a 200 ms delay).
 ******************************************************************************/

#define DAM_IR_GPIO    0    // BOOT pad - pulled high, free as GPIO input after boot

// Initialise RMT RX channel and spawn the decoder task.
void dam_remote_init(int gpio);

// Non-blocking poll – returns the next decoded action, or ACT_NONE.
dam_action_t dam_remote_poll(void);

// True if the last poll() result was a NEC auto-repeat frame.
bool dam_remote_is_repeat(void);
