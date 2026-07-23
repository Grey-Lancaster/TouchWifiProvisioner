# CrowPanel7_BasicConnect

Minimal example: bring up the Elecrow CrowPanel Advance 7.0" HMI
(ESP32-S3-WROOM-1-N16R8, 800x480 RGB IPS panel, GT911 capacitive touch), run
the Wi-Fi provisioning picker, then show a plain green "Connected" screen
with the assigned IP address once online.

This bring-up (LovyanGFX RGB/touch config + LVGL glue) is board-specific and
lives in the example, not the library - TouchWifiProvisioner itself only
touches LVGL objects, so swap this section for whatever display/touch setup
your board uses and `TouchWifiProvisioner::begin()` still works unmodified.

Unlike [`CYD_BasicConnect`](../CYD_BasicConnect), **this example is
PlatformIO-only** - the PSRAM speed fix below relies on pinning a specific
framework package and vendoring prebuilt libs, which has no Arduino IDE
equivalent.

## Requirements

- Elecrow CrowPanel Advance 7.0" HMI (ESP32-S3, 16MB flash / 8MB PSRAM).
- Libraries: `lvgl` (`8.3.11`), `LovyanGFX` (`1.2.19`).

## Setup

### lv_conf.h

Required by TouchWifiProvisioner - see the
[main README](../../README.md#configuring-lvgl) for placement and required
widgets.

### platformio.ini

```ini
[env:crowpanel7]
; Elecrow's known-good combo for this board (see their PlatformIO example:
; https://github.com/Elecrow-RD/CrowPanel-Advance-7-HMI-ESP32-S3-AI-Powered-IPS-Touch-Screen-800x480).
; The officially-published esp32-arduino-libs bake PSRAM at 80MHz, which
; can't keep up with LVGL redraw + RGB DMA scanout + CPU instruction fetch
; all sharing the PSRAM bus at once - this shows up as constant
; left-to-right jitter on the RGB panel. Elecrow's platform_packages
; override rebuilds the six affected static libs with PSRAM at 120MHz.
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
```

`board = ESP32-S3-WROOM-1-N16R8` refers to the custom board definition
shipped alongside this example in [`boards/`](boards) - PlatformIO picks up
any `.json` in a project's `boards/` folder automatically, no extra config
needed. It's a stock N16R8 variant definition (qio_opi memory, PSRAM cache
flags) that isn't in PlatformIO's built-in board list under this name.

[`partitions.csv`](partitions.csv), [`sdkconfig.defaults`](sdkconfig.defaults)
and [`sdkconfig.defaults.esp32s3`](sdkconfig.defaults.esp32s3) are copied
here unmodified from a confirmed-working build on this hardware - the
120MHz PSRAM speed fix needs the sdkconfig-level flags, not just the
build_flags define, to actually take effect.

**`elecrow_120m_libs`** (the prebuilt libs referenced above, originally
Elecrow's
[`ESP32S3_120M`](https://github.com/Elecrow-RD/CrowPanel-Advance-7-HMI-ESP32-S3-AI-Powered-IPS-Touch-Screen-800x480/tree/master/ESP32S3_120M)
from their own example repo) is fetched automatically - `platform_packages`
above points at a mirrored copy hosted as a
[release asset](https://github.com/Grey-Lancaster/TouchWifiProvisioner/releases/tag/elecrow-libs-1)
on this repo (mirrored with permission from Elecrow, since it's a ~220MB
binary package that doesn't belong in git history). No manual fetch step
needed - `pio run` handles it like any other dependency.

## What it does

1. Loads any saved Wi-Fi credentials and tries to connect.
2. If none are saved (or the saved network can't be reached), shows the
   on-device scan/pick/keyboard UI.
3. On success, replaces the screen with a green "Connected" panel showing
   the IP address, and prints it to Serial.
