# CrowPanel7_RollingClock

A full app example, not just a connect demo: an odometer-style rolling-digit
clock on the Elecrow CrowPanel Advance 7.0" HMI (ESP32-S3, 800x480 RGB IPS
panel, GT911 capacitive touch). `TouchWifiProvisioner` handles first-run
setup exactly like [`CrowPanel7_BasicConnect`](../CrowPanel7_BasicConnect),
but here `onWifiConnected()` starts NTP sync via
[ezTime](https://github.com/ropg/ezTime) and builds a whole clock face
instead of a static "Connected" screen.

The clock UI itself (`ClockFace`/`RollingDigit`/`SettingsPanel`/
`ClockSettings`) is copied verbatim from
[`CYD_RollingClock`](../CYD_RollingClock) - it's plain LVGL widgets laid out
with flex containers, so it doesn't care which board or resolution it runs
on. Only the display/touch/backlight bring-up section is board-specific.

A gear icon on the clock face opens a settings panel (12/24-hour toggle,
timezone dropdown) that persists via `Preferences`, independent of
TouchWifiProvisioner's own saved-credentials storage.

Like `CrowPanel7_BasicConnect`, **this example is PlatformIO-only** - the
PSRAM speed fix relies on pinning a specific framework package and vendoring
prebuilt libs, which has no Arduino IDE equivalent.

## Requirements

- Elecrow CrowPanel Advance 7.0" HMI (ESP32-S3, 16MB flash / 8MB PSRAM).
- Libraries: `lvgl` (`8.3.11`), `LovyanGFX` (`1.2.19`), `ropg/ezTime`
  (`^0.8.3`).

## Setup

### lv_conf.h

Same requirements as [`CYD_RollingClock`](../CYD_RollingClock#lv_confh) -
notably `LV_MEM_CUSTOM` (malloc-based LVGL heap instead of a static
`LV_MEM_SIZE`), `LV_USE_SWITCH`, `LV_USE_DROPDOWN`, and
`LV_FONT_MONTSERRAT_16` in addition to the base widget set the main
[README](../../README.md#configuring-lvgl) lists.

### ezTime build flag - this board does NOT need it

`CYD_RollingClock`'s README documents a `-D__time_t_defined` build flag to
work around a conflict between ezTime's `time_t` typedef guard and
arduino-esp32's picolibc. **Do not add that flag here.** This board's
pinned toolchain (`framework-arduinoespressif32@3.0.2`, the same one
`CrowPanel7_BasicConnect` uses for its PSRAM fix) is old-newlib-based, not
picolibc-based - ezTime's own `time_t` typedef is the only one available on
this toolchain, and suppressing it with that flag breaks every system
header that needs `time_t` (`unknown type name 'time_t'` errors throughout
`sys/_timeval.h`, `sys/_timespec.h`, etc., confirmed by testing both ways).
The two examples need opposite settings for this flag because they resolve
to genuinely different C libraries, not because of anything in the sketch
itself.

### platformio.ini

```ini
[env:crowpanel7]
; Same PSRAM-at-120MHz fix as CrowPanel7_BasicConnect - see that example's
; README for why.
platform = https://github.com/pioarduino/platform-espressif32.git#backup_Ard_30
platform_packages =
	platformio/framework-arduinoespressif32@https://github.com/espressif/arduino-esp32/releases/download/3.0.2/esp32-3.0.2.zip
	platformio/framework-arduinoespressif32-libs@file://elecrow_120m_libs
board = ESP32-S3-WROOM-1-N16R8
framework = arduino
monitor_speed = 115200
upload_speed = 460800
board_build.arduino.partitions = partitions.csv
board_upload.partitions = partitions.csv
build_flags =
	-DCONFIG_SPIRAM_SPEED_120M=1
	-DLV_CONF_INCLUDE_SIMPLE
	-Iinclude
lib_deps =
	lvgl/lvgl@8.3.11
	lovyan03/LovyanGFX@1.2.19
	ropg/ezTime@^0.8.3
```

`board = ESP32-S3-WROOM-1-N16R8` refers to the custom board definition
copied alongside this example in [`boards/`](boards) - PlatformIO picks up
any `.json` in a project's `boards/` folder automatically.

[`partitions.csv`](partitions.csv), [`sdkconfig.defaults`](sdkconfig.defaults)
and [`sdkconfig.defaults.esp32s3`](sdkconfig.defaults.esp32s3) are copied
here unmodified from `CrowPanel7_BasicConnect` - same board, same partition
layout.

**`elecrow_120m_libs/`** (the vendored prebuilt libs referenced above) is
not included here - see
[`CrowPanel7_BasicConnect`'s README](../CrowPanel7_BasicConnect#platformioini)
for where to get it and how to place it.

## What it does

1. Runs the same Wi-Fi picker as the other examples, plus the
   backlight/touch-controller I2C handshake `CrowPanel7_BasicConnect` needs.
2. On connect, syncs time via NTP (ezTime) and builds the clock face.
3. Shows rolling-digit time (12 or 24-hour, user-editable), weekday, and
   date. Tap the gear icon to change hour format or timezone - changes
   apply live and persist across reboots.
