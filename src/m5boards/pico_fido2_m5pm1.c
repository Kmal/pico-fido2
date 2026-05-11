#include "pico_fido2_m5pm1.h"

#include "esp_bit_defs.h"
#include "esp_check.h"

#define M5PM1_REG_DEVICE_ID 0x00
#define M5PM1_REG_GPIO_MODE 0x10
#define M5PM1_REG_GPIO_OUT 0x11
#define M5PM1_REG_GPIO_FUNC0 0x16
#define M5PM1_REG_GPIO_FUNC1 0x17
#define M5PM1_REG_SYS_CMD 0x0c
#define M5PM1_SYS_CMD_KEY 0xa0
#define M5PM1_SYS_CMD_SHUTDOWN 0x01
#define M5PM1_I2C_TIMEOUT_MS 100

static const char *TAG = "pf2_m5pm1";

static esp_err_t m5pm1_read_u8(pf2_m5pm1_t *ctx, uint8_t reg, uint8_t *value) {
  ESP_RETURN_ON_FALSE(ctx && ctx->dev && value, ESP_ERR_INVALID_ARG, TAG,
                      "invalid argument");
  return i2c_master_transmit_receive(ctx->dev, &reg, 1, value, 1,
                                     M5PM1_I2C_TIMEOUT_MS);
}

static esp_err_t m5pm1_write_u8(pf2_m5pm1_t *ctx, uint8_t reg, uint8_t value) {
  ESP_RETURN_ON_FALSE(ctx && ctx->dev, ESP_ERR_INVALID_ARG, TAG,
                      "invalid argument");
  const uint8_t payload[] = {reg, value};
  return i2c_master_transmit(ctx->dev, payload, sizeof(payload),
                             M5PM1_I2C_TIMEOUT_MS);
}

static esp_err_t m5pm1_update_bits(pf2_m5pm1_t *ctx, uint8_t reg, uint8_t mask,
                                   uint8_t value) {
  uint8_t old_value = 0;
  ESP_RETURN_ON_ERROR(m5pm1_read_u8(ctx, reg, &old_value), TAG,
                      "read reg 0x%02x", reg);
  const uint8_t new_value = (old_value & ~mask) | (value & mask);
  if (new_value == old_value) {
    return ESP_OK;
  }
  return m5pm1_write_u8(ctx, reg, new_value);
}

esp_err_t pf2_m5pm1_init(pf2_m5pm1_t *ctx, i2c_master_bus_handle_t bus,
                         uint8_t addr) {
  ESP_RETURN_ON_FALSE(ctx && bus && addr, ESP_ERR_INVALID_ARG, TAG,
                      "invalid argument");

  i2c_device_config_t dev_cfg = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = addr,
      .scl_speed_hz = 400000,
  };
  ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(bus, &dev_cfg, &ctx->dev), TAG,
                      "add device");

  uint8_t device_id = 0;
  return m5pm1_read_u8(ctx, M5PM1_REG_DEVICE_ID, &device_id);
}

esp_err_t pf2_m5pm1_gpio_set_output(pf2_m5pm1_t *ctx, uint8_t gpio,
                                    bool level) {
  ESP_RETURN_ON_FALSE(gpio < 5, ESP_ERR_INVALID_ARG, TAG, "invalid PM1 GPIO");
  const uint8_t bit = (uint8_t)BIT(gpio);

  if (gpio < 4) {
    const uint8_t func_mask = (uint8_t)(0x03u << (gpio * 2));
    ESP_RETURN_ON_ERROR(
        m5pm1_update_bits(ctx, M5PM1_REG_GPIO_FUNC0, func_mask, 0), TAG,
        "gpio func0");
  } else {
    ESP_RETURN_ON_ERROR(m5pm1_update_bits(ctx, M5PM1_REG_GPIO_FUNC1, 0x03, 0),
                        TAG, "gpio func1");
  }

  ESP_RETURN_ON_ERROR(
      m5pm1_update_bits(ctx, M5PM1_REG_GPIO_OUT, bit, level ? bit : 0), TAG,
      "gpio out");
  return m5pm1_update_bits(ctx, M5PM1_REG_GPIO_MODE, bit, bit);
}

esp_err_t pf2_m5pm1_enable_l3b_power(pf2_m5pm1_t *ctx, bool enable) {
  return pf2_m5pm1_gpio_set_output(ctx, 2, enable);
}

esp_err_t pf2_m5pm1_enable_speaker_amp(pf2_m5pm1_t *ctx, bool enable) {
  return pf2_m5pm1_gpio_set_output(ctx, 3, enable);
}

esp_err_t pf2_m5pm1_shutdown(pf2_m5pm1_t *ctx) {
  return m5pm1_write_u8(ctx, M5PM1_REG_SYS_CMD,
                        M5PM1_SYS_CMD_KEY | M5PM1_SYS_CMD_SHUTDOWN);
}
