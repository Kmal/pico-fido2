/*
 * ESP-IDF user-presence bridge for M5Stack ESP32-S3 boards.
 *
 * This file replaces the default ESP BOOT_PIN-only button implementation from
 * pico-keys-sdk when building the M5Stack profiles. It keeps the same public
 * symbols consumed by pico-keys-sdk while routing reads through the board
 * descriptors in src/m5boards.
 */

#include "button.h"
#include "fs/phy.h"
#include "led/led.h"
#include "pico_fido2_display.h"
#include "pico_fido2_m5_board.h"
#include "pico_fido2_presence.h"
#include "pico_time.h"
#include "picokeys.h"

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdbool.h>
#include <stdint.h>

extern void execute_tasks(void);
extern bool is_busy(void);

int (*button_pressed_cb)(uint8_t) = NULL;
bool cancel_button = false;

static bool req_button_pending = false;
static bool button_pressed_state = false;
static uint32_t button_pressed_time = 0;
static uint8_t button_press = 0;
static bool presence_initialized = false;

bool is_req_button_pending(void) { return req_button_pending; }

static bool m5_presence_read(void) {
  if (!presence_initialized) {
    presence_initialized = pico_fido2_presence_init() == ESP_OK;
  }
  return presence_initialized && pico_fido2_presence_poll();
}

static void m5_presence_service_delay(void) {
  execute_tasks();
#ifdef ESP_PLATFORM
  vTaskDelay(pdMS_TO_TICKS(10));
#endif
}

bool button_wait(void) {
  uint32_t button_timeout = 0;
  if (phy_data.up_btn_present) {
    button_timeout = phy_data.up_btn * 1000;
  }
  if (button_timeout == 0) {
    return false;
  }

  const uint32_t start_button = board_millis();
  bool timeout = false;
  cancel_button = false;
  const uint32_t led_mode = led_get_mode();

  led_set_mode(MODE_BUTTON);
  pf2_display_set_status(PF2_STATUS_WAITING_FOR_TOUCH);
  pf2_display_set_message("Touch key", "Confirm");
  req_button_pending = true;

  while (!cancel_button) {
    if (m5_presence_read()) {
      break;
    }
    if (start_button + button_timeout < board_millis()) {
      timeout = true;
      break;
    }
    m5_presence_service_delay();
  }

  led_set_mode(led_mode);
  const bool failed = timeout || cancel_button;
  pf2_display_set_status(failed ? PF2_STATUS_AUTH_FAILURE
                                : PF2_STATUS_AUTH_SUCCESS);
  pf2_display_set_message("Presence", failed ? "Timeout" : "Accepted");
  req_button_pending = false;
  return timeout || cancel_button;
}

void button_task(void) {
  if (button_pressed_cb && board_millis() > 1000 && !is_busy()) {
    const bool current_button_state = m5_presence_read();
    if (current_button_state != button_pressed_state) {
      if (!current_button_state) {
        if (button_pressed_time == 0 ||
            button_pressed_time + 1000 > board_millis()) {
          button_press++;
        }
        button_pressed_time = board_millis();
      }
      button_pressed_state = current_button_state;
    }
    if (button_pressed_time > 0 && button_press > 0 &&
        button_pressed_time + 1000 < board_millis() && !button_pressed_state) {
      button_pressed_cb(button_press);
      button_pressed_time = 0;
      button_press = 0;
    }
  }
}

int picokey_init(void) {
  const pf2_m5_board_desc_t *board = pf2_m5_board_get();
  if (!board) {
    return 0;
  }

  presence_initialized = pico_fido2_presence_init() == ESP_OK;
  (void)pf2_display_init(&board->lcd);
  pf2_display_set_status(PF2_STATUS_BOOT);
  pf2_display_set_message("Pico FIDO2", board->name);
  return 0;
}
