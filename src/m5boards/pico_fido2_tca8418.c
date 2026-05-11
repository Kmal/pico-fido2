#include "pico_fido2_tca8418.h"

#include "esp_bit_defs.h"
#include "esp_check.h"

#define TCA8418_REG_CFG 0x01
#define TCA8418_REG_INT_STAT 0x02
#define TCA8418_REG_KEY_LCK_EC 0x03
#define TCA8418_REG_KEY_EVENT_A 0x04
#define TCA8418_REG_KP_GPIO1 0x1d
#define TCA8418_REG_KP_GPIO2 0x1e
#define TCA8418_REG_KP_GPIO3 0x1f
#define TCA8418_REG_DEBOUNCE_DIS1 0x29
#define TCA8418_REG_DEBOUNCE_DIS2 0x2a
#define TCA8418_REG_DEBOUNCE_DIS3 0x2b
#define TCA8418_CFG_KE_IEN BIT(0)
#define TCA8418_INT_STAT_K_INT BIT(0)
#define TCA8418_KEY_EVENT_PRESS BIT(7)
#define TCA8418_KEY_EVENT_CODE_MASK 0x7f
#define TCA8418_I2C_TIMEOUT_MS 100

static const char *TAG = "pf2_tca8418";

static esp_err_t tca8418_read_u8(pf2_tca8418_t *ctx, uint8_t reg,
                                 uint8_t *value) {
  ESP_RETURN_ON_FALSE(ctx && ctx->dev && value, ESP_ERR_INVALID_ARG, TAG,
                      "invalid argument");
  return i2c_master_transmit_receive(ctx->dev, &reg, 1, value, 1,
                                     TCA8418_I2C_TIMEOUT_MS);
}

static esp_err_t tca8418_write_u8(pf2_tca8418_t *ctx, uint8_t reg,
                                  uint8_t value) {
  ESP_RETURN_ON_FALSE(ctx && ctx->dev, ESP_ERR_INVALID_ARG, TAG,
                      "invalid argument");
  const uint8_t payload[] = {reg, value};
  return i2c_master_transmit(ctx->dev, payload, sizeof(payload),
                             TCA8418_I2C_TIMEOUT_MS);
}

esp_err_t pf2_tca8418_init(pf2_tca8418_t *ctx, i2c_master_bus_handle_t bus,
                           uint8_t addr, gpio_num_t int_gpio) {
  ESP_RETURN_ON_FALSE(ctx && bus && addr, ESP_ERR_INVALID_ARG, TAG,
                      "invalid argument");

  i2c_device_config_t dev_cfg = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = addr,
      .scl_speed_hz = 400000,
  };
  ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(bus, &dev_cfg, &ctx->dev), TAG,
                      "add device");

  ctx->int_gpio = int_gpio;
  ctx->last_event = 0;

  if (int_gpio >= 0) {
    gpio_config_t int_cfg = {
        .pin_bit_mask = BIT64(int_gpio),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&int_cfg), TAG, "interrupt GPIO");
  }

  ESP_RETURN_ON_ERROR(tca8418_write_u8(ctx, TCA8418_REG_CFG, 0x00), TAG,
                      "disable cfg");
  ESP_RETURN_ON_ERROR(tca8418_write_u8(ctx, TCA8418_REG_KP_GPIO1, 0xff), TAG,
                      "keypad gpio1");
  ESP_RETURN_ON_ERROR(tca8418_write_u8(ctx, TCA8418_REG_KP_GPIO2, 0xff), TAG,
                      "keypad gpio2");
  ESP_RETURN_ON_ERROR(tca8418_write_u8(ctx, TCA8418_REG_KP_GPIO3, 0x03), TAG,
                      "keypad gpio3");
  ESP_RETURN_ON_ERROR(tca8418_write_u8(ctx, TCA8418_REG_DEBOUNCE_DIS1, 0x00),
                      TAG, "debounce1");
  ESP_RETURN_ON_ERROR(tca8418_write_u8(ctx, TCA8418_REG_DEBOUNCE_DIS2, 0x00),
                      TAG, "debounce2");
  ESP_RETURN_ON_ERROR(tca8418_write_u8(ctx, TCA8418_REG_DEBOUNCE_DIS3, 0x00),
                      TAG, "debounce3");
  ESP_RETURN_ON_ERROR(tca8418_write_u8(ctx, TCA8418_REG_INT_STAT, 0xff), TAG,
                      "clear interrupts");
  return tca8418_write_u8(ctx, TCA8418_REG_CFG, TCA8418_CFG_KE_IEN);
}

esp_err_t pf2_tca8418_read_key_event(pf2_tca8418_t *ctx, uint8_t *key_event,
                                     bool *pressed) {
  ESP_RETURN_ON_FALSE(ctx && key_event && pressed, ESP_ERR_INVALID_ARG, TAG,
                      "invalid argument");

  uint8_t count = 0;
  ESP_RETURN_ON_ERROR(tca8418_read_u8(ctx, TCA8418_REG_KEY_LCK_EC, &count), TAG,
                      "event count");
  count &= 0x0f;
  if (count == 0) {
    *key_event = 0;
    *pressed = false;
    return ESP_OK;
  }

  uint8_t event = 0;
  ESP_RETURN_ON_ERROR(tca8418_read_u8(ctx, TCA8418_REG_KEY_EVENT_A, &event),
                      TAG, "event fifo");
  ESP_RETURN_ON_ERROR(
      tca8418_write_u8(ctx, TCA8418_REG_INT_STAT, TCA8418_INT_STAT_K_INT), TAG,
      "clear key interrupt");

  *key_event = event & TCA8418_KEY_EVENT_CODE_MASK;
  *pressed = (event & TCA8418_KEY_EVENT_PRESS) != 0;
  ctx->last_event = event;
  return ESP_OK;
}

bool pf2_tca8418_any_key_pressed(pf2_tca8418_t *ctx) {
  for (uint8_t i = 0; i < 10; ++i) {
    uint8_t key_event = 0;
    bool pressed = false;
    if (pf2_tca8418_read_key_event(ctx, &key_event, &pressed) != ESP_OK) {
      return false;
    }
    if (pressed && key_event != 0) {
      return true;
    }
    if (key_event == 0) {
      return false;
    }
  }
  return false;
}
