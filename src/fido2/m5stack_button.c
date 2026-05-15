/*
 * ESP-IDF M5Stack board bootstrap.
 *
 * pico-keys-sdk owns the main loop and default button symbols.  For the
 * M5Stack ESP32-S3 profiles this file overrides the weak picokey_init() hook
 * so board-specific I2C, input, power, and optional display peripherals are
 * initialized after TinyUSB descriptor setup.
 */

#include "pico_fido2_display.h"
#include "pico_fido2_m5_board.h"
#include "pico_fido2_presence.h"

int picokey_init(void) {
  const pf2_m5_board_desc_t *board = pf2_m5_board_get();
  if (!board) {
    return 0;
  }

  (void)pico_fido2_presence_init();
  (void)pf2_display_init(&board->lcd);
  pf2_display_set_status(PF2_STATUS_BOOT);
  pf2_display_set_message("Pico FIDO2", board->name);
  return 0;
}
