#pragma once

#include "esp_err.h"
#include "pico_fido2_m5_board.h"

typedef enum {
  PF2_STATUS_BOOT = 0,
  PF2_STATUS_WAITING_FOR_TOUCH,
  PF2_STATUS_AUTH_SUCCESS,
  PF2_STATUS_AUTH_FAILURE,
  PF2_STATUS_LOCKED,
} pf2_status_t;

esp_err_t pf2_display_init(const pf2_lcd_desc_t *lcd);
void pf2_display_set_status(pf2_status_t status);
void pf2_display_set_message(const char *line1, const char *line2);
