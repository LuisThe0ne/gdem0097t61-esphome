#include "gdem0097t61.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"

namespace esphome {
namespace gdem0097t61 {

static const char *const TAG = "gdem0097t61";

// ─────────────────────────────────────────────────────────────────────────────
// Full-update waveform LUT for the SSD1680 (OTP bypass)
//
// Sourced from Good Display's own Arduino sample code for the GDEM0097T61
// and cross-checked against the 2.9" SSD1680 implementation already in
// ESPHome (waveshare_epaper.cpp, GDEY029T94 / 2.9in GDEW029T5 drivers).
//
// The SSD1680 LUT register (0x32) accepts 153 bytes:
//   - 60 bytes  VS (voltage source) waveform: 5 phases × 12 bytes each
//   - 84 bytes  TP/SR/RP groups: 12 groups × 7 bytes each
//   - 9 bytes   Frame Rate (FR) + XON settings
// ─────────────────────────────────────────────────────────────────────────────
static const uint8_t FULL_UPDATE_LUT[] = {
  // VS (voltage source) waveform — 5 × 12 bytes
  0x80, 0x48, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // L0 (BB)
  0x40, 0x48, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // L1 (BW)
  0x80, 0x48, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // L2 (WB)
  0x40, 0x48, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // L3 (WW)
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // L4 (VCOM)
  // TP / SR / RP — 12 groups × 7 bytes
  0x43, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  // FR + XON — 9 bytes
  0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x00, 0x00, 0x00,
};
static_assert(sizeof(FULL_UPDATE_LUT) == 153, "LUT must be 153 bytes");

// ─────────────────────────────────────────────────────────────────────────────
// Partial-update waveform LUT (fast, flicker-free, for incremental changes)
// Based on the Good Display / Waveshare 2.9" SSD1680 partial-update sequence.
// ─────────────────────────────────────────────────────────────────────────────
static const uint8_t PARTIAL_UPDATE_LUT[] = {
  // VS
  0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x40, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  // TP / SR / RP
  0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  // FR + XON
  0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x00, 0x00, 0x00,
};
static_assert(sizeof(PARTIAL_UPDATE_LUT) == 153, "LUT must be 153 bytes");

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

void GDEM0097T61::cmd_(uint8_t command) {
  this->dc_pin_->digital_write(false);  // DC LOW = command
  this->enable();
  this->write_byte(command);
  this->disable();
}

void GDEM0097T61::data_(uint8_t value) {
  this->dc_pin_->digital_write(true);  // DC HIGH = data
  this->enable();
  this->write_byte(value);
  this->disable();
}

void GDEM0097T61::cmd_data_(uint8_t command, const uint8_t *data, size_t len) {
  this->cmd_(command);
  this->dc_pin_->digital_write(true);
  this->enable();
  for (size_t i = 0; i < len; i++) {
    this->write_byte(data[i]);
  }
  this->disable();
}

void GDEM0097T61::wait_until_idle_() {
  if (this->busy_pin_ == nullptr) {
    delay(200);
    return;
  }
  // SSD1680 BUSY is HIGH while busy
  uint32_t start = millis();
  while (this->busy_pin_->digital_read()) {
    if (millis() - start > 5000) {
      ESP_LOGW(TAG, "Timeout waiting for BUSY pin to go LOW");
      break;
    }
    delay(10);
  }
}

void GDEM0097T61::reset_display_() {
  if (this->reset_pin_ != nullptr) {
    this->reset_pin_->digital_write(true);
    delay(10);
    this->reset_pin_->digital_write(false);
    delay(10);
    this->reset_pin_->digital_write(true);
    delay(10);
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// Buffer management
// ─────────────────────────────────────────────────────────────────────────────

size_t GDEM0097T61::get_buffer_length_() {
  // 1 bit per pixel, rows padded to full bytes
  return (size_t) GDEM0097T61_RAM_WIDTH_BYTES * GDEM0097T61_HEIGHT;
}

void GDEM0097T61::draw_absolute_pixel_internal(int x, int y, Color color) {
  if (x < 0 || x >= GDEM0097T61_WIDTH || y < 0 || y >= GDEM0097T61_HEIGHT)
    return;

  // Buffer layout: row-major, MSB first (pixel 0 is bit 7 of byte 0)
  size_t idx = (size_t) y * GDEM0097T61_RAM_WIDTH_BYTES + (x / 8);
  uint8_t bit = 0x80 >> (x % 8);

  // SSD1680 RAM convention: 1 = white, 0 = black
  if (color.is_on()) {
    this->buffer_[idx] &= ~bit;  // black pixel → clear bit
  } else {
    this->buffer_[idx] |= bit;   // white pixel → set bit
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// Initialization
// ─────────────────────────────────────────────────────────────────────────────

void GDEM0097T61::initialize_display_() {
  this->reset_display_();

  // Software reset — wait for it to complete
  this->cmd_(SSD1680_SW_RESET);
  this->wait_until_idle_();

  // Driver output control:
  //   MUX = height - 1 = 87 = 0x057
  //   GD=0, SM=0, TB=0
  uint8_t driver_ctrl[] = {
    (uint8_t)((GDEM0097T61_HEIGHT - 1) & 0xFF),
    (uint8_t)(((GDEM0097T61_HEIGHT - 1) >> 8) & 0x01),
    0x00
  };
  this->cmd_data_(SSD1680_DRIVER_CONTROL, driver_ctrl, sizeof(driver_ctrl));

  // Data entry mode: X increment, Y increment, update address in X direction
  this->cmd_(SSD1680_DATA_ENTRY_MODE);
  this->data_(0x03);

  // Set RAM X address window: 0 to (RAM_WIDTH_BYTES - 1)
  uint8_t ramx[] = {0x00, (uint8_t)(GDEM0097T61_RAM_WIDTH_BYTES - 1)};
  this->cmd_data_(SSD1680_SET_RAMX_ADDR, ramx, sizeof(ramx));

  // Set RAM Y address window: 0 to (height - 1)
  uint8_t ramy[] = {
    0x00, 0x00,
    (uint8_t)((GDEM0097T61_HEIGHT - 1) & 0xFF),
    (uint8_t)(((GDEM0097T61_HEIGHT - 1) >> 8) & 0x01)
  };
  this->cmd_data_(SSD1680_SET_RAMY_ADDR, ramy, sizeof(ramy));

  // Use internal temperature sensor
  this->cmd_(SSD1680_TEMP_SENSOR_CONTROL);
  this->data_(0x80);

  // VCOM voltage: -1.5V (0x36 ≈ -1.5V per SSD1680 datasheet)
  this->cmd_(SSD1680_WRITE_VCOM);
  this->data_(0x36);

  // Gate voltage: VGH=20V, VGL=-20V
  this->cmd_(SSD1680_GATE_VOLTAGE);
  this->data_(0x17);

  // Source voltage: VSH1=15V, VSH2=5V, VSL=-15V
  uint8_t src_voltage[] = {0x41, 0xAE, 0x32};
  this->cmd_data_(SSD1680_SOURCE_VOLTAGE, src_voltage, sizeof(src_voltage));

  // Border waveform: follow LUT
  this->cmd_(SSD1680_WRITE_BORDER);
  this->data_(0x03);

  // Load full-update LUT
  this->cmd_data_(SSD1680_WRITE_LUT, FULL_UPDATE_LUT, sizeof(FULL_UPDATE_LUT));

  this->wait_until_idle_();
  ESP_LOGD(TAG, "GDEM0097T61 initialized");
}

// ─────────────────────────────────────────────────────────────────────────────
// Sending framebuffer to the display
// ─────────────────────────────────────────────────────────────────────────────

void GDEM0097T61::send_to_display_() {
  // Reset RAM counters to (0, 0)
  this->cmd_(SSD1680_SET_RAMX_COUNTER);
  this->data_(0x00);
  this->cmd_(SSD1680_SET_RAMY_COUNTER);
  this->data_(0x00);
  this->data_(0x00);

  // Write pixel data
  this->cmd_(SSD1680_WRITE_RAM_BW);
  this->dc_pin_->digital_write(true);
  this->enable();
  for (size_t i = 0; i < this->get_buffer_length_(); i++) {
    this->write_byte(this->buffer_[i]);
  }
  this->disable();

  // Display update sequence:
  //   0xF7 = Enable clock, enable analog, load LUT, display pattern, disable analog, disable OSC
  this->cmd_(SSD1680_DISP_CTRL2);
  this->data_(0xF7);

  this->cmd_(SSD1680_MASTER_ACTIVATION);
  this->wait_until_idle_();
}

// ─────────────────────────────────────────────────────────────────────────────
// ESPHome lifecycle
// ─────────────────────────────────────────────────────────────────────────────

void GDEM0097T61::setup() {
  ESP_LOGCONFIG(TAG, "Setting up GDEM0097T61...");

  // Allocate framebuffer (all white = 0xFF)
  this->init_internal_(this->get_buffer_length_());
  memset(this->buffer_, 0xFF, this->get_buffer_length_());

  // Set up GPIO pins
  this->dc_pin_->setup();
  this->dc_pin_->digital_write(false);

  if (this->reset_pin_ != nullptr) {
    this->reset_pin_->setup();
    this->reset_pin_->digital_write(true);
  }

  if (this->busy_pin_ != nullptr) {
    this->busy_pin_->setup();
  }

  // Set up SPI
  this->spi_setup();

  this->initialize_display_();
}

void GDEM0097T61::update() {
  // Clear buffer to white before redrawing
  memset(this->buffer_, 0xFF, this->get_buffer_length_());

  // Run the user lambda to draw into the buffer
  this->do_update_();

  // Push buffer to display
  this->send_to_display_();
}

void GDEM0097T61::dump_config() {
  LOG_DISPLAY("", "GDEM0097T61 e-Paper Display", this);
  ESP_LOGCONFIG(TAG, "  Model: GDEM0097T61");
  ESP_LOGCONFIG(TAG, "  Resolution: %dx%d", GDEM0097T61_WIDTH, GDEM0097T61_HEIGHT);
  LOG_PIN("  DC Pin: ", this->dc_pin_);
  LOG_PIN("  Reset Pin: ", this->reset_pin_);
  LOG_PIN("  Busy Pin: ", this->busy_pin_);
  LOG_UPDATE_INTERVAL(this);
}

}  // namespace gdem0097t61
}  // namespace esphome
