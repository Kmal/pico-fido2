#pragma once

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
  PF2_M5_BOARD_NONE = 0,
  PF2_M5_BOARD_CARDPUTER_ADV,
  PF2_M5_BOARD_STICK_S3,
} pf2_m5_board_id_t;

typedef enum {
  PF2_USER_INPUT_GPIO = 0,
  PF2_USER_INPUT_TCA8418,
} pf2_user_input_type_t;

typedef struct {
  gpio_num_t sda;
  gpio_num_t scl;
  uint32_t hz;
} pf2_i2c_bus_desc_t;

typedef struct {
  gpio_num_t mosi;
  gpio_num_t miso;
  gpio_num_t sclk;
  gpio_num_t cs;
  gpio_num_t dc;
  gpio_num_t rst;
  gpio_num_t bl;
  uint16_t width;
  uint16_t height;
  bool invert_colors;
} pf2_lcd_desc_t;

typedef struct {
  gpio_num_t mclk;
  gpio_num_t dout;
  gpio_num_t bclk;
  gpio_num_t lrck;
  gpio_num_t din;
  uint8_t i2c_addr;
} pf2_es8311_desc_t;

typedef struct {
  uint8_t i2c_addr;
  gpio_num_t int_gpio;
} pf2_bmi270_desc_t;

typedef struct {
  gpio_num_t tx;
  gpio_num_t rx;
  bool rx_requires_rmt;
} pf2_ir_desc_t;

typedef struct {
  uint8_t i2c_addr;
  gpio_num_t irq_gpio;
  uint8_t gpio_l3b_power;
  uint8_t gpio_speaker_amp;
  uint8_t gpio_imu_int;
} pf2_m5pm1_desc_t;

typedef struct {
  pf2_user_input_type_t type;
  gpio_num_t gpio_a;
  gpio_num_t gpio_b;
  bool active_low;
  uint8_t tca8418_addr;
  gpio_num_t tca8418_int;
} pf2_user_input_desc_t;

typedef struct {
  pf2_m5_board_id_t id;
  const char *name;
  const char *usb_product;
  uint32_t flash_mb;
  uint32_t psram_mb;
  bool has_battery_adc;
  gpio_num_t battery_adc_gpio;
  pf2_i2c_bus_desc_t internal_i2c;
  pf2_lcd_desc_t lcd;
  pf2_es8311_desc_t audio;
  pf2_bmi270_desc_t imu;
  pf2_ir_desc_t ir;
  pf2_m5pm1_desc_t pm1;
  pf2_user_input_desc_t user_input;
} pf2_m5_board_desc_t;

const pf2_m5_board_desc_t *pf2_m5_board_get(void);
pf2_m5_board_id_t pf2_m5_board_id_from_kconfig(void);
esp_err_t pf2_m5_board_init_early(void);
esp_err_t pf2_m5_board_init_peripherals(void);
i2c_master_bus_handle_t pf2_m5_board_i2c_bus(void);
