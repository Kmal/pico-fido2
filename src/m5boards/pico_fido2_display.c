#include "pico_fido2_display.h"

#include "sdkconfig.h"

#if CONFIG_PICO_FIDO2_M5_DISPLAY_STATUS
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_bit_defs.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>
#endif

static const pf2_lcd_desc_t *s_lcd;
static pf2_status_t s_status;
static const char *s_line1;
static const char *s_line2;

#if CONFIG_PICO_FIDO2_M5_DISPLAY_STATUS
static spi_device_handle_t s_lcd_spi;
static QueueHandle_t s_status_queue;
static TaskHandle_t s_display_task;

typedef struct {
  pf2_status_t status;
  char line1[24];
  char line2[24];
} pf2_display_state_t;

static pf2_display_state_t s_display_state = {
    .status = PF2_STATUS_BOOT,
};

static esp_err_t lcd_cmd(uint8_t cmd) {
  gpio_set_level(s_lcd->dc, 0);
  spi_transaction_t txn = {
      .length = 8,
      .tx_buffer = &cmd,
  };
  return spi_device_transmit(s_lcd_spi, &txn);
}

static esp_err_t lcd_data(const void *data, size_t len) {
  if (len == 0) {
    return ESP_OK;
  }
  gpio_set_level(s_lcd->dc, 1);
  spi_transaction_t txn = {
      .length = len * 8,
      .tx_buffer = data,
  };
  return spi_device_transmit(s_lcd_spi, &txn);
}

static esp_err_t lcd_cmd_data(uint8_t cmd, const void *data, size_t len) {
  esp_err_t ret = lcd_cmd(cmd);
  if (ret != ESP_OK) {
    return ret;
  }
  return lcd_data(data, len);
}

static esp_err_t lcd_set_window(uint16_t x0, uint16_t y0, uint16_t x1,
                                uint16_t y1) {
  uint8_t col[] = {(uint8_t)(x0 >> 8), (uint8_t)x0, (uint8_t)(x1 >> 8),
                   (uint8_t)x1};
  uint8_t row[] = {(uint8_t)(y0 >> 8), (uint8_t)y0, (uint8_t)(y1 >> 8),
                   (uint8_t)y1};
  esp_err_t ret = lcd_cmd_data(0x2a, col, sizeof(col));
  if (ret != ESP_OK) {
    return ret;
  }
  ret = lcd_cmd_data(0x2b, row, sizeof(row));
  if (ret != ESP_OK) {
    return ret;
  }
  return lcd_cmd(0x2c);
}

static uint16_t status_color(pf2_status_t status) {
  switch (status) {
  case PF2_STATUS_WAITING_FOR_TOUCH:
    return 0xffe0;
  case PF2_STATUS_AUTH_SUCCESS:
    return 0x07e0;
  case PF2_STATUS_AUTH_FAILURE:
    return 0xf800;
  case PF2_STATUS_LOCKED:
    return 0x001f;
  case PF2_STATUS_BOOT:
  default:
    return 0x0000;
  }
}

static void lcd_fill(uint16_t rgb565) {
  if (!s_lcd_spi || !s_lcd) {
    return;
  }
  if (lcd_set_window(0, 0, s_lcd->width - 1, s_lcd->height - 1) != ESP_OK) {
    return;
  }

  uint16_t line[64];
  for (size_t i = 0; i < sizeof(line) / sizeof(line[0]); ++i) {
    line[i] = __builtin_bswap16(rgb565);
  }

  size_t remaining = (size_t)s_lcd->width * s_lcd->height;
  while (remaining > 0) {
    const size_t pixels = remaining > (sizeof(line) / sizeof(line[0]))
                              ? (sizeof(line) / sizeof(line[0]))
                              : remaining;
    if (lcd_data(line, pixels * sizeof(line[0])) != ESP_OK) {
      return;
    }
    remaining -= pixels;
  }
}

static esp_err_t lcd_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                          uint16_t rgb565) {
  if (w == 0 || h == 0 || !s_lcd || x >= s_lcd->width || y >= s_lcd->height) {
    return ESP_OK;
  }
  if (x + w > s_lcd->width) {
    w = s_lcd->width - x;
  }
  if (y + h > s_lcd->height) {
    h = s_lcd->height - y;
  }
  esp_err_t ret = lcd_set_window(x, y, x + w - 1, y + h - 1);
  if (ret != ESP_OK) {
    return ret;
  }
  const uint16_t color = __builtin_bswap16(rgb565);
  for (uint32_t i = 0; i < (uint32_t)w * h; ++i) {
    ret = lcd_data(&color, sizeof(color));
    if (ret != ESP_OK) {
      return ret;
    }
  }
  return ESP_OK;
}

static const uint8_t *glyph_for(char c) {
  static const uint8_t space[5] = {0x00, 0x00, 0x00, 0x00, 0x00};
  static const uint8_t unknown[5] = {0x7f, 0x41, 0x5d, 0x41, 0x7f};
  static const uint8_t digits[10][5] = {
      {0x3e, 0x51, 0x49, 0x45, 0x3e}, {0x00, 0x42, 0x7f, 0x40, 0x00},
      {0x42, 0x61, 0x51, 0x49, 0x46}, {0x21, 0x41, 0x45, 0x4b, 0x31},
      {0x18, 0x14, 0x12, 0x7f, 0x10}, {0x27, 0x45, 0x45, 0x45, 0x39},
      {0x3c, 0x4a, 0x49, 0x49, 0x30}, {0x01, 0x71, 0x09, 0x05, 0x03},
      {0x36, 0x49, 0x49, 0x49, 0x36}, {0x06, 0x49, 0x49, 0x29, 0x1e},
  };
  static const uint8_t letters[26][5] = {
      {0x7e, 0x11, 0x11, 0x11, 0x7e}, {0x7f, 0x49, 0x49, 0x49, 0x36},
      {0x3e, 0x41, 0x41, 0x41, 0x22}, {0x7f, 0x41, 0x41, 0x22, 0x1c},
      {0x7f, 0x49, 0x49, 0x49, 0x41}, {0x7f, 0x09, 0x09, 0x09, 0x01},
      {0x3e, 0x41, 0x49, 0x49, 0x7a}, {0x7f, 0x08, 0x08, 0x08, 0x7f},
      {0x00, 0x41, 0x7f, 0x41, 0x00}, {0x20, 0x40, 0x41, 0x3f, 0x01},
      {0x7f, 0x08, 0x14, 0x22, 0x41}, {0x7f, 0x40, 0x40, 0x40, 0x40},
      {0x7f, 0x02, 0x0c, 0x02, 0x7f}, {0x7f, 0x04, 0x08, 0x10, 0x7f},
      {0x3e, 0x41, 0x41, 0x41, 0x3e}, {0x7f, 0x09, 0x09, 0x09, 0x06},
      {0x3e, 0x41, 0x51, 0x21, 0x5e}, {0x7f, 0x09, 0x19, 0x29, 0x46},
      {0x46, 0x49, 0x49, 0x49, 0x31}, {0x01, 0x01, 0x7f, 0x01, 0x01},
      {0x3f, 0x40, 0x40, 0x40, 0x3f}, {0x1f, 0x20, 0x40, 0x20, 0x1f},
      {0x3f, 0x40, 0x38, 0x40, 0x3f}, {0x63, 0x14, 0x08, 0x14, 0x63},
      {0x07, 0x08, 0x70, 0x08, 0x07}, {0x61, 0x51, 0x49, 0x45, 0x43},
  };
  static const uint8_t dash[5] = {0x08, 0x08, 0x08, 0x08, 0x08};
  static const uint8_t colon[5] = {0x00, 0x36, 0x36, 0x00, 0x00};
  static const uint8_t dot[5] = {0x00, 0x60, 0x60, 0x00, 0x00};
  static const uint8_t slash[5] = {0x20, 0x10, 0x08, 0x04, 0x02};
  if (c >= 'a' && c <= 'z') {
    c = (char)(c - 'a' + 'A');
  }
  if (c >= '0' && c <= '9') {
    return digits[c - '0'];
  }
  if (c >= 'A' && c <= 'Z') {
    return letters[c - 'A'];
  }
  if (c == ' ') {
    return space;
  }
  if (c == '-' || c == '_') {
    return dash;
  }
  if (c == ':') {
    return colon;
  }
  if (c == '.') {
    return dot;
  }
  if (c == '/') {
    return slash;
  }
  return unknown;
}

static void lcd_draw_char(uint16_t x, uint16_t y, char c, uint16_t color,
                          uint8_t scale) {
  const uint8_t *glyph = glyph_for(c);
  for (uint8_t col = 0; col < 5; ++col) {
    for (uint8_t row = 0; row < 7; ++row) {
      if (glyph[col] & BIT(row)) {
        (void)lcd_rect(x + col * scale, y + row * scale, scale, scale, color);
      }
    }
  }
}

static void lcd_draw_text(uint16_t x, uint16_t y, const char *text,
                          uint16_t color, uint8_t scale) {
  if (!text) {
    return;
  }
  const uint16_t advance = 6 * scale;
  for (; *text && x + advance <= s_lcd->width; ++text, x += advance) {
    lcd_draw_char(x, y, *text, color, scale);
  }
}

static void display_task(void *arg) {
  (void)arg;
  pf2_display_state_t state;
  while (true) {
    if (xQueueReceive(s_status_queue, &state, portMAX_DELAY) == pdTRUE) {
      lcd_fill(status_color(state.status));
      if (state.line1[0] || state.line2[0]) {
        lcd_draw_text(8, 12, state.line1, 0xffff, 2);
        lcd_draw_text(8, 34, state.line2, 0xffff, 2);
      }
      ESP_LOGI("pf2_display", "status=%d %s / %s", (int)state.status,
               state.line1, state.line2);
    }
  }
}

static void display_queue_state(void) {
  if (s_status_queue) {
    (void)xQueueOverwrite(s_status_queue, &s_display_state);
  }
}
#endif

esp_err_t pf2_display_init(const pf2_lcd_desc_t *lcd) {
  s_lcd = lcd;
  s_status = PF2_STATUS_BOOT;
#if CONFIG_PICO_FIDO2_M5_DISPLAY_STATUS
  if (!lcd) {
    return ESP_ERR_INVALID_ARG;
  }

  gpio_config_t gpio_cfg = {
      .pin_bit_mask = BIT64(lcd->dc) | BIT64(lcd->rst) | BIT64(lcd->bl),
      .mode = GPIO_MODE_OUTPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
  };
  ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&gpio_cfg));

  spi_bus_config_t bus_cfg = {
      .mosi_io_num = lcd->mosi,
      .miso_io_num = lcd->miso,
      .sclk_io_num = lcd->sclk,
      .quadwp_io_num = GPIO_NUM_NC,
      .quadhd_io_num = GPIO_NUM_NC,
      .max_transfer_sz = lcd->width * 64 * sizeof(uint16_t),
  };
  esp_err_t ret = spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
  if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
    return ret;
  }

  spi_device_interface_config_t dev_cfg = {
      .clock_speed_hz = 40 * 1000 * 1000,
      .mode = 0,
      .spics_io_num = lcd->cs,
      .queue_size = 1,
  };
  ret = spi_bus_add_device(SPI2_HOST, &dev_cfg, &s_lcd_spi);
  if (ret != ESP_OK) {
    return ret;
  }

  gpio_set_level(lcd->rst, 0);
  vTaskDelay(pdMS_TO_TICKS(20));
  gpio_set_level(lcd->rst, 1);
  vTaskDelay(pdMS_TO_TICKS(120));

  uint8_t colmod = 0x55;
  ret = lcd_cmd(0x01);
  vTaskDelay(pdMS_TO_TICKS(150));
  if (ret == ESP_OK) {
    ret = lcd_cmd(0x11);
  }
  vTaskDelay(pdMS_TO_TICKS(120));
  if (ret == ESP_OK) {
    ret = lcd_cmd_data(0x3a, &colmod, sizeof(colmod));
  }
  if (ret == ESP_OK && lcd->invert_colors) {
    ret = lcd_cmd(0x21);
  }
  if (ret == ESP_OK) {
    ret = lcd_cmd(0x13);
  }
  if (ret == ESP_OK) {
    ret = lcd_cmd(0x29);
  }
  gpio_set_level(lcd->bl, 1);
  s_status_queue = xQueueCreate(1, sizeof(pf2_display_state_t));
  if (!s_status_queue) {
    return ESP_ERR_NO_MEM;
  }
  ret = xTaskCreate(display_task, "pf2_display", 3072, NULL,
                    tskIDLE_PRIORITY + 1, &s_display_task) == pdPASS
            ? ESP_OK
            : ESP_ERR_NO_MEM;
  if (ret != ESP_OK) {
    vQueueDelete(s_status_queue);
    s_status_queue = NULL;
    return ret;
  }
  s_display_state.status = s_status;
  display_queue_state();
  ESP_LOGI("pf2_display", "status display initialized for %ux%u LCD",
           lcd->width, lcd->height);
  return ret;
#else
  (void)lcd;
#endif
  return ESP_OK;
}

void pf2_display_set_status(pf2_status_t status) {
  s_status = status;
#if CONFIG_PICO_FIDO2_M5_DISPLAY_STATUS
  s_display_state.status = status;
  display_queue_state();
#endif
}

void pf2_display_set_message(const char *line1, const char *line2) {
  s_line1 = line1;
  s_line2 = line2;
#if CONFIG_PICO_FIDO2_M5_DISPLAY_STATUS
  snprintf(s_display_state.line1, sizeof(s_display_state.line1), "%s",
           line1 ? line1 : "");
  snprintf(s_display_state.line2, sizeof(s_display_state.line2), "%s",
           line2 ? line2 : "");
  display_queue_state();
#endif
  (void)s_lcd;
  (void)s_status;
  (void)s_line1;
  (void)s_line2;
}
