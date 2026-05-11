/*
 * M5Stack ESP-IDF PHY defaults wrapper.
 *
 * The upstream pico-keys-sdk phy_init() runs before usb_init(), while
 * picokey_init() runs after ESP TinyUSB descriptors are installed. Include the
 * SDK implementation with only phy_init renamed, then expose a wrapper that
 * applies board defaults early enough for USB descriptors and button timeout.
 */

#define phy_init pico_fido2_sdk_phy_init
#include "../../pico-keys-sdk/src/fs/phy.c"
#undef phy_init

#include "pico_fido2_m5_board.h"

#include <stdio.h>

int phy_init(void) {
  const int ret = pico_fido2_sdk_phy_init();
  const pf2_m5_board_desc_t *board = pf2_m5_board_get();
  if (!board) {
    return ret;
  }

  if (!phy_data.up_btn_present) {
    phy_data.up_btn = 15;
    phy_data.up_btn_present = true;
  }
  if (!phy_data.usb_product_present && board->usb_product) {
    snprintf(phy_data.usb_product, sizeof(phy_data.usb_product), "%s",
             board->usb_product);
    phy_data.usb_product_present = true;
  }
  return ret;
}
