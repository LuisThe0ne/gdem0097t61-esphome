# GDEM0097T61 ESPHome Custom Component

ESPHome display driver for the **Good Display GDEM0097T61** —
a 0.97" B/W e-paper display (184×88 px) driven by the SSD1680 IC.

## Files

```
components/
  gdem0097t61/
    __init__.py        — ESPHome Python config schema
    gdem0097t61.h      — C++ class declaration
    gdem0097t61.cpp    — C++ implementation (init, draw, update)
example.yaml           — Example ESPHome configuration
```

## Wiring

The GDEM0097T61 uses an 18-pin FPC connector.
Use the Good Display **DESPI-C097** breakout board or a custom adapter.

| Display pin | Signal  | Connect to          |
|-------------|---------|---------------------|
| VCC         | Power   | 3.3V                |
| GND         | Ground  | GND                 |
| BUSY        | Output  | `busy_pin` GPIO     |
| RST         | Input   | `reset_pin` GPIO    |
| DC          | Input   | `dc_pin` GPIO       |
| CS          | Input   | `cs_pin` GPIO (SPI) |
| CLK         | Input   | SPI CLK             |
| DIN         | Input   | SPI MOSI            |

## Usage

1. Copy the `components/` folder next to your `.yaml` file.
2. Add the `external_components` block (see `example.yaml`).
3. Configure the `display` platform:

```yaml
external_components:
  - source:
      type: local
      path: components

spi:
  clk_pin: GPIO18
  mosi_pin: GPIO23

display:
  - platform: gdem0097t61
    cs_pin: GPIO5
    dc_pin: GPIO17
    reset_pin: GPIO16
    busy_pin: GPIO4
    update_interval: 60s
    lambda: |-
      it.print(0, 0, id(my_font), "Hello!");
```

## Notes

- **update_interval**: e-paper has a limited refresh cycle lifetime.
  Don't set this below ~10s for regular use; 60s or longer is recommended.
- **Full update only**: This driver performs a full (flicker) refresh on every
  update. Partial update support can be added by loading `PARTIAL_UPDATE_LUT`
  and using display update command `0xFC` instead of `0xF7`.
- **BUSY pin**: The SSD1680 holds BUSY HIGH while the panel is refreshing.
  Always connect this pin; without it the driver falls back to a 200ms delay.
- **Color convention**: ESPHome's `Color::BLACK` → black pixel,
  `Color::WHITE` (or background) → white pixel.
- **Waveform LUT**: The LUT in `gdem0097t61.cpp` bypasses the OTP waveform
  stored on the SSD1680 chip. If you see ghosting or poor contrast, the OTP
  waveform can be used instead by issuing display update command `0xF4`
  (which triggers OTP LUT loading) rather than uploading a custom LUT.
  To use OTP: remove the `SSD1680_WRITE_LUT` call in `initialize_display_()`
  and change the `DISP_CTRL2` byte from `0xF7` to `0xF4`.
