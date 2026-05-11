#include "pico_fido2_presence.h"

#include "esp_check.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "pico_fido2_m5_board.h"
#include "pico_fido2_tca8418.h"

static const char *TAG = "pf2_presence";
static const pf2_m5_board_desc_t *s_board;
static pf2_tca8418_t s_keyboard;
static bool s_gpio_last_pressed;
static bool s_presence_initialized;

static bool gpio_is_pressed(gpio_num_t gpio, bool active_low) {
  if (gpio < 0) {
    return false;
  }
  const int level = gpio_get_level(gpio);
  return active_low ? (level == 0) : (level != 0);
}

esp_err_t pf2_user_presence_init(const pf2_m5_board_desc_t *board) {
  ESP_RETURN_ON_FALSE(board, ESP_ERR_INVALID_ARG, TAG, "missing board");
  if (s_presence_initialized && s_board == board) {
    return ESP_OK;
  }

  s_board = board;
  s_gpio_last_pressed = false;

  ESP_RETURN_ON_ERROR(pf2_m5_board_init_peripherals(), TAG,
                      "board peripherals");

  if (board->user_input.type == PF2_USER_INPUT_TCA8418) {
    ESP_RETURN_ON_ERROR(pf2_tca8418_init(&s_keyboard, pf2_m5_board_i2c_bus(),
                                         board->user_input.tca8418_addr,
                                         board->user_input.tca8418_int),
                        TAG, "TCA8418 init");
  }

  if (board->user_input.type == PF2_USER_INPUT_GPIO) {
    s_gpio_last_pressed =
        gpio_is_pressed(board->user_input.gpio_a,
                        board->user_input.active_low) ||
        gpio_is_pressed(board->user_input.gpio_b, board->user_input.active_low);
  }

  s_presence_initialized = true;
  return ESP_OK;
}

bool pf2_user_presence_poll(void) {
  if (!s_board) {
    return false;
  }

  if (s_board->user_input.type == PF2_USER_INPUT_TCA8418) {
    return pf2_tca8418_any_key_pressed(&s_keyboard);
  }

  if (s_board->user_input.type == PF2_USER_INPUT_GPIO) {
    const bool now_pressed = gpio_is_pressed(s_board->user_input.gpio_a,
                                             s_board->user_input.active_low) ||
                             gpio_is_pressed(s_board->user_input.gpio_b,
                                             s_board->user_input.active_low);
    const bool rising_edge = now_pressed && !s_gpio_last_pressed;
    s_gpio_last_pressed = now_pressed;
    return rising_edge;
  }

  return false;
}

bool pf2_user_presence_wait(uint32_t timeout_ms) {
  const int64_t deadline_us =
      esp_timer_get_time() + ((int64_t)timeout_ms * 1000);
  do {
    if (pf2_user_presence_poll()) {
      return true;
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  } while (timeout_ms == 0 || esp_timer_get_time() < deadline_us);
  return false;
}

esp_err_t pico_fido2_presence_init(void) {
  const pf2_m5_board_desc_t *board = pf2_m5_board_get();
  ESP_RETURN_ON_FALSE(board, ESP_ERR_NOT_SUPPORTED, TAG,
                      "no M5Stack board selected");
  ESP_RETURN_ON_ERROR(pf2_m5_board_init_early(), TAG, "early init");
  return pf2_user_presence_init(board);
}

bool pico_fido2_presence_poll(void) {
  if (!s_board && pico_fido2_presence_init() != ESP_OK) {
    return false;
  }
  return pf2_user_presence_poll();
}

bool pico_fido2_presence_wait(uint32_t timeout_ms) {
  if (!s_board && pico_fido2_presence_init() != ESP_OK) {
    return false;
  }
  return pf2_user_presence_wait(timeout_ms);
}

bool __attribute__((weak)) user_presence_wait(uint32_t timeout_ms) {
  return pico_fido2_presence_wait(timeout_ms);
}
