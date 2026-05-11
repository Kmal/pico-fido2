#pragma once

#include "driver/i2c_master.h"
#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
  i2c_master_dev_handle_t dev;
} pf2_m5pm1_t;

esp_err_t pf2_m5pm1_init(pf2_m5pm1_t *ctx, i2c_master_bus_handle_t bus,
                         uint8_t addr);
esp_err_t pf2_m5pm1_gpio_set_output(pf2_m5pm1_t *ctx, uint8_t gpio, bool level);
esp_err_t pf2_m5pm1_enable_l3b_power(pf2_m5pm1_t *ctx, bool enable);
esp_err_t pf2_m5pm1_enable_speaker_amp(pf2_m5pm1_t *ctx, bool enable);
esp_err_t pf2_m5pm1_shutdown(pf2_m5pm1_t *ctx);
