#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#include "esphome/components/display/display_buffer.h"

namespace esphome {
namespace waveshare37_epaper {

static const char *const TAG = "waveshare37_epaper";

static const int PANEL_WIDTH  = 280;
static const int PANEL_HEIGHT = 480;
static const int BUF_SIZE     = (PANEL_WIDTH * PANEL_HEIGHT) / 8;

static const uint8_t LUT_GC[] = {
    0x2A,0x05,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x05,0x2A,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x2A,0x15,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x05,0x0A,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x02,0x03,0x0A,0x00,0x02,0x06,0x0A,0x05,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x22,0x22,0x22,0x22,0x22
};

static const uint8_t LUT_A2[] = {
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x0A,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x05,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x03,0x05,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x22,0x22,0x22,0x22,0x22
};

class Waveshare37EPaperDisplay : public display::DisplayBuffer {
 public:
  void set_pins(GPIOPin *clk, GPIOPin *mosi, GPIOPin *cs,
                GPIOPin *dc, GPIOPin *reset, GPIOPin *busy) {
    clk_pin_ = clk; mosi_pin_ = mosi; cs_pin_ = cs;
    dc_pin_ = dc; reset_pin_ = reset; busy_pin_ = busy;
  }

  void set_full_update_every(uint32_t count) { full_update_every_ = count; }

  void setup() override {
    this->init_internal_(BUF_SIZE);
    memset(this->buffer_, 0xFF, BUF_SIZE);

    clk_pin_->setup();  mosi_pin_->setup(); cs_pin_->setup();
    dc_pin_->setup();   reset_pin_->setup(); busy_pin_->setup();

    cs_pin_->digital_write(true);
    clk_pin_->digital_write(false);

    epd_init_();
    epd_clear_();
    ESP_LOGI(TAG, "Setup complete");
  }

  void update() override {
    memset(this->buffer_, 0xFF, BUF_SIZE);
    this->do_update_();

    bool full = (update_count_ == 0) ||
                (full_update_every_ > 0 && update_count_ % full_update_every_ == 0);

    ESP_LOGD(TAG, "Update %d (%s)", update_count_, full ? "full" : "partial");
    full ? epd_update_full_() : epd_update_partial_();
    update_count_++;
  }

  display::DisplayType get_display_type() override { return display::DISPLAY_TYPE_BINARY; }
  float get_setup_priority() const override { return setup_priority::PROCESSOR; }

 protected:
  void draw_absolute_pixel_internal(int x, int y, Color color) override {
    if (x < 0 || x >= PANEL_WIDTH || y < 0 || y >= PANEL_HEIGHT) return;
    int byte_idx = (x + y * PANEL_WIDTH) / 8;
    int bit_idx  = 7 - ((x + y * PANEL_WIDTH) % 8);
    if (color.is_on()) {
      this->buffer_[byte_idx] &= ~(1 << bit_idx);
    } else {
      this->buffer_[byte_idx] |=  (1 << bit_idx);
    }
  }

  int get_width_internal() override  { return PANEL_WIDTH; }
  int get_height_internal() override { return PANEL_HEIGHT; }

  void spi_write_byte_(uint8_t data) {
    for (int i = 0; i < 8; i++) {
      mosi_pin_->digital_write(data & 0x80);
      data <<= 1;
      clk_pin_->digital_write(true);
      ets_delay_us(1);
      clk_pin_->digital_write(false);
      ets_delay_us(1);
    }
  }

  void send_command_(uint8_t cmd) {
    dc_pin_->digital_write(false);
    cs_pin_->digital_write(false);
    spi_write_byte_(cmd);
    cs_pin_->digital_write(true);
  }

  void send_data_(uint8_t data) {
    dc_pin_->digital_write(true);
    cs_pin_->digital_write(false);
    spi_write_byte_(data);
    cs_pin_->digital_write(true);
  }

  void reset_() {
    reset_pin_->digital_write(true);  delay(20);
    reset_pin_->digital_write(false); delay(2);
    reset_pin_->digital_write(true);  delay(20);
  }

  void wait_until_idle_(const char *ctx = "") {
    uint32_t start = millis();
    while (busy_pin_->digital_read()) {
      delay(10);
      if (millis() - start > 3000) {
        ESP_LOGW(TAG, "Timeout waiting for idle (%s)", ctx);
        break;
      }
    }
    delay(200);
  }

  void load_lut_(bool full) {
    send_command_(0x32);
    const uint8_t *lut = full ? LUT_GC : LUT_A2;
    for (int i = 0; i < 105; i++)
      send_data_(lut[i]);
  }

  void send_buffer_() {
    dc_pin_->digital_write(true);
    cs_pin_->digital_write(false);
    for (int i = 0; i < BUF_SIZE; i++)
      spi_write_byte_(this->buffer_[i]);
    cs_pin_->digital_write(true);
  }

  void epd_init_() {
    reset_();
    send_command_(0x12); delay(300);

    send_command_(0x46); send_data_(0xF7); wait_until_idle_("init1");
    send_command_(0x47); send_data_(0xF7); wait_until_idle_("init2");

    send_command_(0x01); send_data_(0xDF); send_data_(0x01); send_data_(0x00);
    send_command_(0x03); send_data_(0x00);
    send_command_(0x04); send_data_(0x41); send_data_(0xA8); send_data_(0x32);
    send_command_(0x11); send_data_(0x03);
    send_command_(0x3C); send_data_(0x00);
    send_command_(0x0C); send_data_(0xAE); send_data_(0xC7); send_data_(0xC3); send_data_(0xC0); send_data_(0xC0);
    send_command_(0x18); send_data_(0x80);
    send_command_(0x2C); send_data_(0x44);
    send_command_(0x37);
    send_data_(0x00); send_data_(0xFF); send_data_(0xFF); send_data_(0xFF); send_data_(0xFF);
    send_data_(0x4F); send_data_(0xFF); send_data_(0xFF); send_data_(0xFF); send_data_(0xFF);
    send_command_(0x44); send_data_(0x00); send_data_(0x00); send_data_(0x17); send_data_(0x01);
    send_command_(0x45); send_data_(0x00); send_data_(0x00); send_data_(0xDF); send_data_(0x01);
    send_command_(0x22); send_data_(0xCF);

    ESP_LOGI(TAG, "Init done");
  }

  void epd_clear_() {
    send_command_(0x4E); send_data_(0x00); send_data_(0x00);
    send_command_(0x4F); send_data_(0x00); send_data_(0x00);
    send_command_(0x24);
    for (int i = 0; i < BUF_SIZE; i++)
      send_data_(0xFF);
    load_lut_(true);
    send_command_(0x20);
    wait_until_idle_("clear");
  }

  void epd_set_cursor_() {
    send_command_(0x4E); send_data_(0x00); send_data_(0x00);
    send_command_(0x4F); send_data_(0x00); send_data_(0x00);
  }

  void epd_update_full_() {
    epd_set_cursor_();
    send_command_(0x24); send_buffer_();
    send_command_(0x26); send_buffer_();
    load_lut_(true);
    send_command_(0x20);
    wait_until_idle_("full");
  }

  void epd_update_partial_() {
    epd_set_cursor_();
    send_command_(0x24); send_buffer_();
    load_lut_(false);
    send_command_(0x20);
    wait_until_idle_("partial");
  }

  GPIOPin *clk_pin_{nullptr};
  GPIOPin *mosi_pin_{nullptr};
  GPIOPin *cs_pin_{nullptr};
  GPIOPin *dc_pin_{nullptr};
  GPIOPin *reset_pin_{nullptr};
  GPIOPin *busy_pin_{nullptr};

  uint32_t update_count_{0};
  uint32_t full_update_every_{10};
};

}  // namespace waveshare37_epaper
}  // namespace esphome
