#pragma once

#include "esp_err.h"
#include "pico_fido2_m5_board.h"
#include <stdbool.h>
#include <stdint.h>

esp_err_t pf2_user_presence_init(const pf2_m5_board_desc_t *board);
bool pf2_user_presence_poll(void);
bool pf2_user_presence_wait(uint32_t timeout_ms);

esp_err_t pico_fido2_presence_init(void);
bool pico_fido2_presence_poll(void);
bool pico_fido2_presence_wait(uint32_t timeout_ms);
