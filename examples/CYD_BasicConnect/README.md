# CYD_BasicConnect

Minimal example: bring up the Cheap Yellow Display (ESP32-2432S028R), run the
Wi-Fi provisioning picker, then show a plain green "Connected" screen with
the assigned IP address once online.

This bring-up (TFT_eSPI + XPT2046 + LVGL glue) is CYD-specific and lives in
the example, not the library - TouchWifiProvisioner itself only touches LVGL
objects, so swap this section for whatever display/touch setup your board
uses and `TouchWifiProvisioner::begin()` still works unmodified.

## Requirements

- A CYD board (ESP32-2432S028R). This example assumes the **ST7789** panel
  variant - if colors look wrong or inverted, your unit may have the older
  ILI9341 controller instead; adjust the TFT_eSPI driver define accordingly.
- Libraries: `TFT_eSPI`, `XPT2046_Touchscreen`, `lvgl` (`^8.3.11`).

## Setup

### lv_conf.h

Required by TouchWifiProvisioner - see the
[main README](../../README.md#configuring-lvgl) for placement and required
widgets (`LV_USE_FLEX`, `LV_USE_LIST`, `LV_USE_LABEL`, `LV_USE_TEXTAREA`,
`LV_USE_KEYBOARD`, `LV_USE_BTNMATRIX`, `LV_USE_CHECKBOX`, `LV_USE_BTN`).

### TFT_eSPI panel configuration

**PlatformIO:** add these to `build_flags` in `platformio.ini`:

```ini
build_flags =
	-DUSER_SETUP_LOADED
	-DST7789_DRIVER
	-DTFT_INVERSION_OFF
	-DTFT_MISO=12
	-DTFT_MOSI=13
	-DTFT_SCLK=14
	-DTFT_CS=15
	-DUSE_HSPI_PORT
	-DTFT_DC=2
	-DTFT_RST=-1
	-DTFT_BL=21
	-DTFT_BACKLIGHT_ON=HIGH
	-DSPI_FREQUENCY=55000000
	-DSPI_READ_FREQUENCY=20000000
	-DSPI_TOUCH_FREQUENCY=2500000
	-DLOAD_GLCD
	-DLOAD_FONT2
	-DLOAD_FONT4
	-DLOAD_FONT6
	-DLOAD_FONT7
	-DLOAD_FONT8
	-DLOAD_GFXFF
	-DLV_CONF_INCLUDE_SIMPLE
```

**Arduino IDE:** TFT_eSPI reads its config from `User_Setup.h` inside the
library folder instead of build flags - copy the same defines above into
that file (or select/adapt one of TFT_eSPI's bundled setups for a 320x240
ST7789 panel on ESP32).

Touch (XPT2046) pins are set directly in `CYD_BasicConnect.ino` via
`#define`, so no extra config is needed for touch on either toolchain.

## What it does

1. Loads any saved Wi-Fi credentials and tries to connect.
2. If none are saved (or the saved network can't be reached), shows the
   on-device scan/pick/keyboard UI.
3. On success, replaces the screen with a green "Connected" panel showing
   the IP address, and prints it to Serial.
