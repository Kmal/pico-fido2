#include "pico_fido2_m5_board.h"

#include "esp_bit_defs.h"
#include "esp_check.h"
#include "pico_fido2_m5pm1.h"
#include "sdkconfig.h"

static const char *TAG = "pf2_m5_board";
static i2c_master_bus_handle_t s_internal_i2c;
static pf2_m5pm1_t s_pm1;
static bool s_pm1_initialized;

static const pf2_m5_board_desc_t cardputer_adv_desc = {
    .id = PF2_M5_BOARD_CARDPUTER_ADV,
    .name = "m5stack-cardputer-adv",
    .usb_product = "Pico FIDO2 Cardputer-Adv",
    .flash_mb = 8,
    .psram_mb = 0,
    .has_battery_adc = true,
    .battery_adc_gpio = GPIO_NUM_10,
    .internal_i2c = {.sda = GPIO_NUM_8, .scl = GPIO_NUM_9, .hz = 400000},
    .lcd =
        {
            .mosi = GPIO_NUM_35,
            .miso = GPIO_NUM_NC,
            .sclk = GPIO_NUM_36,
            .cs = GPIO_NUM_37,
            .dc = GPIO_NUM_34,
            .rst = GPIO_NUM_33,
            .bl = GPIO_NUM_38,
            .width = 240,
            .height = 135,
            .invert_colors = true,
        },
    .audio =
        {
            .mclk = GPIO_NUM_NC,
            .dout = GPIO_NUM_46,
            .bclk = GPIO_NUM_41,
            .lrck = GPIO_NUM_43,
            .din = GPIO_NUM_42,
            .i2c_addr = 0x18,
        },
    .imu = {.i2c_addr = 0x69, .int_gpio = GPIO_NUM_NC},
    .ir = {.tx = GPIO_NUM_44, .rx = GPIO_NUM_NC, .rx_requires_rmt = false},
    .pm1 =
        {
            .i2c_addr = 0,
            .irq_gpio = GPIO_NUM_NC,
            .gpio_l3b_power = 0xff,
            .gpio_speaker_amp = 0xff,
            .gpio_imu_int = 0xff,
        },
    .user_input =
        {
            .type = PF2_USER_INPUT_TCA8418,
            .gpio_a = GPIO_NUM_NC,
            .gpio_b = GPIO_NUM_NC,
            .active_low = true,
            .tca8418_addr = 0x34,
            .tca8418_int = GPIO_NUM_11,
        },
};

static const pf2_m5_board_desc_t stick_s3_desc = {
    .id = PF2_M5_BOARD_STICK_S3,
    .name = "m5stack-stick-s3",
    .usb_product = "Pico FIDO2 StickS3",
    .flash_mb = 8,
    .psram_mb = 8,
    .has_battery_adc = false,
    .battery_adc_gpio = GPIO_NUM_NC,
    .internal_i2c = {.sda = GPIO_NUM_47, .scl = GPIO_NUM_48, .hz = 400000},
    .lcd =
        {
            .mosi = GPIO_NUM_39,
            .miso = GPIO_NUM_NC,
            .sclk = GPIO_NUM_40,
            .cs = GPIO_NUM_41,
            .dc = GPIO_NUM_45,
            .rst = GPIO_NUM_21,
            .bl = GPIO_NUM_38,
            .width = 135,
            .height = 240,
            .invert_colors = true,
        },
    .audio =
        {
            .mclk = GPIO_NUM_18,
            .dout = GPIO_NUM_14,
            .bclk = GPIO_NUM_17,
            .lrck = GPIO_NUM_15,
            .din = GPIO_NUM_16,
            .i2c_addr = 0x18,
        },
    .imu = {.i2c_addr = 0x68, .int_gpio = GPIO_NUM_NC},
    .ir = {.tx = GPIO_NUM_46, .rx = GPIO_NUM_42, .rx_requires_rmt = true},
    .pm1 =
        {
            .i2c_addr = 0x6e,
            .irq_gpio = GPIO_NUM_NC,
            .gpio_l3b_power = 2,
            .gpio_speaker_amp = 3,
            .gpio_imu_int = 4,
        },
    .user_input =
        {
            .type = PF2_USER_INPUT_GPIO,
            .gpio_a = GPIO_NUM_11,
            .gpio_b = GPIO_NUM_12,
            .active_low = true,
            .tca8418_addr = 0,
            .tca8418_int = GPIO_NUM_NC,
        },
};

pf2_m5_board_id_t pf2_m5_board_id_from_kconfig(void) {
#if PICO_FIDO2_BOARD_M5STACK_STICK_S3
  return PF2_M5_BOARD_STICK_S3;
#elif PICO_FIDO2_BOARD_M5STACK_CARDPUTER_ADV
  return PF2_M5_BOARD_CARDPUTER_ADV;
#else
  return PF2_M5_BOARD_NONE;
#endif
}

const pf2_m5_board_desc_t *pf2_m5_board_get(void) {
  switch (pf2_m5_board_id_from_kconfig()) {
  case PF2_M5_BOARD_CARDPUTER_ADV:
    return &cardputer_adv_desc;
  case PF2_M5_BOARD_STICK_S3:
    return &stick_s3_desc;
  default:
    return NULL;
  }
}

i2c_master_bus_handle_t pf2_m5_board_i2c_bus(void) { return s_internal_i2c; }

esp_err_t pf2_m5_board_init_early(void) {
  const pf2_m5_board_desc_t *board = pf2_m5_board_get();
  ESP_RETURN_ON_FALSE(board, ESP_ERR_NOT_SUPPORTED, TAG,
                      "no M5Stack board selected");

  if (s_internal_i2c) {
    return ESP_OK;
  }

  i2c_master_bus_config_t bus_cfg = {
      .i2c_port = I2C_NUM_0,
      .sda_io_num = board->internal_i2c.sda,
      .scl_io_num = board->internal_i2c.scl,
      .clk_source = I2C_CLK_SRC_DEFAULT,
      .glitch_ignore_cnt = 7,
      .flags.enable_internal_pullup = true,
  };
  return i2c_new_master_bus(&bus_cfg, &s_internal_i2c);
}

esp_err_t pf2_m5_board_init_peripherals(void) {
  const pf2_m5_board_desc_t *board = pf2_m5_board_get();
  ESP_RETURN_ON_FALSE(board, ESP_ERR_NOT_SUPPORTED, TAG,
                      "no M5Stack board selected");
  ESP_RETURN_ON_ERROR(pf2_m5_board_init_early(), TAG, "early init");

  if (board->id == PF2_M5_BOARD_STICK_S3) {
    if (!s_pm1_initialized) {
      ESP_RETURN_ON_ERROR(
          pf2_m5pm1_init(&s_pm1, s_internal_i2c, board->pm1.i2c_addr), TAG,
          "PM1 init");
      s_pm1_initialized = true;
    }
    ESP_RETURN_ON_ERROR(
        pf2_m5pm1_gpio_set_output(&s_pm1, board->pm1.gpio_l3b_power, true), TAG,
        "L3B power");
    ESP_RETURN_ON_ERROR(
        pf2_m5pm1_gpio_set_output(&s_pm1, board->pm1.gpio_speaker_amp, false),
        TAG, "speaker amp off");
  }

  if (board->user_input.type == PF2_USER_INPUT_GPIO) {
    gpio_config_t cfg = {
        .pin_bit_mask =
            BIT64(board->user_input.gpio_a) | BIT64(board->user_input.gpio_b),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = board->user_input.active_low ? GPIO_PULLUP_ENABLE
                                                   : GPIO_PULLUP_DISABLE,
        .pull_down_en = board->user_input.active_low ? GPIO_PULLDOWN_DISABLE
                                                     : GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&cfg), TAG, "presence GPIOs");
  }

  return ESP_OK;
}
