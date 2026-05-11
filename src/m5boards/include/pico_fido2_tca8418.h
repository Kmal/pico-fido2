#pragma once

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
  i2c_master_dev_handle_t dev;
  gpio_num_t int_gpio;
  uint8_t last_event;
} pf2_tca8418_t;

esp_err_t pf2_tca8418_init(pf2_tca8418_t *ctx, i2c_master_bus_handle_t bus,
                           uint8_t addr, gpio_num_t int_gpio);
esp_err_t pf2_tca8418_read_key_event(pf2_tca8418_t *ctx, uint8_t *key_event,
                                     bool *pressed);
bool pf2_tca8418_any_key_pressed(pf2_tca8418_t *ctx);
