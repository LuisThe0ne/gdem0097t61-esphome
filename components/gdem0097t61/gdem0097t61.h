#pragma once

/**
 * ESPHome custom component for the Good Display GDEM0097T61
 * 0.97" B/W e-paper, 184x88 pixels, SSD1680 driver IC
 *
 * Datasheet: https://www.good-display.com/companyfile/1333.html
 * SSD1680 datasheet: https://v4.cecdn.yun300.cn/100001_1909185148/SSD1680.pdf
 *
 * Wiring (18-pin FPC connector — use the DESPI-C097 breakout or similar):
 *   VCC  → 3.3V
 *   GND  → GND
 *   BUSY → busy_pin  (output from display, HIGH = busy)
 *   RST  → reset_pin (active LOW)
 *   DC   → dc_pin    (LOW = command, HIGH = data)
 *   CS   → cs_pin    (active LOW)
 *   CLK  → SPI CLK
 *   DIN  → SPI MOSI
 */

#include "esphome/core/component.h"
#include "esphome/components/spi/spi.h"
#include "esphome/components/display/display_buffer.h"

namespace esphome {
namespace gdem0097t61 {

// SSD1680 command registers
static const uint8_t SSD1680_DRIVER_CONTROL      = 0x01;
static const uint8_t SSD1680_GATE_VOLTAGE        = 0x03;
static const uint8_t SSD1680_SOURCE_VOLTAGE      = 0x04;
static const uint8_t SSD1680_DEEP_SLEEP          = 0x10;
static const uint8_t SSD1680_DATA_ENTRY_MODE     = 0x11;
static const uint8_t SSD1680_SW_RESET            = 0x12;
static const uint8_t SSD1680_TEMP_SENSOR_CONTROL = 0x18;
static const uint8_t SSD1680_MASTER_ACTIVATION   = 0x20;
static const uint8_t SSD1680_DISP_CTRL1          = 0x21;
static const uint8_t SSD1680_DISP_CTRL2          = 0x22;
static const uint8_t SSD1680_WRITE_RAM_BW        = 0x24;
static const uint8_t SSD1680_WRITE_VCOM          = 0x2C;
static const uint8_t SSD1680_WRITE_LUT           = 0x32;
static const uint8_t SSD1680_WRITE_BORDER        = 0x3C;
static const uint8_t SSD1680_SET_RAMX_ADDR       = 0x44;
static const uint8_t SSD1680_SET_RAMY_ADDR       = 0x45;
static const uint8_t SSD1680_SET_RAMX_COUNTER    = 0x4E;
static const uint8_t SSD1680_SET_RAMY_COUNTER    = 0x4F;

// Display dimensions
static const uint16_t GDEM0097T61_WIDTH  = 184;
static const uint16_t GDEM0097T61_HEIGHT = 88;

// RAM width must be a multiple of 8 (bytes per row)
// 184 pixels = 23 bytes per row
static const uint16_t GDEM0097T61_RAM_WIDTH_BYTES = (GDEM0097T61_WIDTH + 7) / 8;  // = 23

class GDEM0097T61 : public display::DisplayBuffer,
                    public spi::SPIDevice<spi::BIT_ORDER_MSB_FIRST,
                                         spi::CLOCK_POLARITY_LOW,
                                         spi::CLOCK_PHASE_LEADING,
                                         spi::DATA_RATE_2MHZ> {
 public:
  void set_dc_pin(GPIOPin *dc_pin) { dc_pin_ = dc_pin; }
  void set_reset_pin(GPIOPin *reset_pin) { reset_pin_ = reset_pin; }
  void set_busy_pin(GPIOPin *busy_pin) { busy_pin_ = busy_pin; }

  void setup() override;
  void update() override;
  void dump_config() override;

  float get_setup_priority() const override { return setup_priority::PROCESSOR; }

  display::DisplayType get_display_type() override { return display::DisplayType::DISPLAY_TYPE_BINARY; }

  int get_width_internal() override { return GDEM0097T61_WIDTH; }
  int get_height_internal() override { return GDEM0097T61_HEIGHT; }

 protected:
  void draw_absolute_pixel_internal(int x, int y, Color color) override;
  size_t get_buffer_length_();

  void initialize_display_();
  void send_to_display_();
  void reset_display_();
  void wait_until_idle_();

  void cmd_(uint8_t command);
  void data_(uint8_t data);
  void cmd_data_(uint8_t command, const uint8_t *data, size_t len);

  GPIOPin *dc_pin_{nullptr};
  GPIOPin *reset_pin_{nullptr};
  GPIOPin *busy_pin_{nullptr};
};

}  // namespace gdem0097t61
}  // namespace esphome
