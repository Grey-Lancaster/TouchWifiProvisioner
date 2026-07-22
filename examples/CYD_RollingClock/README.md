# CYD_RollingClock

A full app example, not just a connect demo: an odometer-style rolling-digit
clock on the Cheap Yellow Display (ESP32-2432S028R, ST7789 panel, resistive
XPT2046 touch). `TouchWifiProvisioner` handles first-run setup exactly like
the other examples, but here `onConnected()` starts NTP sync via
[ezTime](https://github.com/ropg/ezTime) and builds a whole clock face
instead of a static "Connected" screen - showing what it looks like to hand
off into a real app that keeps running afterward.

A gear icon on the clock face opens a settings panel (12/24-hour toggle,
timezone dropdown) that persists via `Preferences`, independent of
TouchWifiProvisioner's own saved-credentials storage.

## Requirements

- A CYD board (ESP32-2432S028R), ST7789 panel variant (see
  [`CYD_BasicConnect`](../CYD_BasicConnect)'s README if yours turns out to
  be ILI9341-based).
- Libraries: `TFT_eSPI`, `XPT2046_Touchscreen`, `lvgl` (`^8.3.11`),
  `ropg/ezTime` (`^0.8.3`).

## Setup

### lv_conf.h

**Arduino IDE:** the repo-root [`lv_conf.h`](../../lv_conf.h) already covers
everything below - copy it into your sketchbook's `libraries/` folder, next
to (not inside) the `lvgl` folder, and skip the rest of this section.
**PlatformIO:** nothing to do, this example ships its own
`include/lv_conf.h`.

Same base requirements as the [main README](../../README.md#configuring-lvgl)
(`LV_USE_FLEX`, `LV_USE_LIST`, `LV_USE_LABEL`, `LV_USE_TEXTAREA`,
`LV_USE_KEYBOARD`, `LV_USE_BTNMATRIX`, `LV_USE_CHECKBOX`, `LV_USE_BTN`),
**plus** two things the other examples don't need:

- `LV_USE_SWITCH` and `LV_USE_DROPDOWN` (the settings panel's 12/24-hour
  toggle and timezone list)
- `LV_FONT_MONTSERRAT_16` enabled alongside the `_24`/`_48` the other
  examples already use (AM/PM label)

**`LV_MEM_CUSTOM` matters here in a way it doesn't for the other
examples.** The clock face's fonts/animations need more LVGL heap than a
static `LV_MEM_SIZE` comfortably provides on this chip - a modest bump past
~48K overflows the firmware's DRAM segment at link time, since static LVGL
memory is baked into DRAM at build time. Route LVGL through the runtime
heap instead:

```c
#define LV_MEM_CUSTOM 1
#if LV_MEM_CUSTOM
    #define LV_MEM_CUSTOM_INCLUDE <stdlib.h>
    #define LV_MEM_CUSTOM_ALLOC   malloc
    #define LV_MEM_CUSTOM_FREE    free
    #define LV_MEM_CUSTOM_REALLOC realloc
#endif
```

### ezTime build flag - depends on your toolchain

Whether you need a build flag for ezTime depends on which C library your
PlatformIO platform resolves:

- **Official `espressif32` from the PlatformIO registry** (what a fresh
  install gets, and what this repo's CI uses): newlib-based - ezTime
  compiles as-is. **Do not add any flag.**
- **Picolibc-based platforms** (some pioarduino/community platform
  builds): picolibc already defines `time_t`, which collides with ezTime's
  own typedef guard. There - and only there - add
  `-D__time_t_defined` to `build_flags` to satisfy the guard.

Adding the flag on the wrong (newlib) toolchain breaks the build with
`unknown type name 'time_t'` errors throughout the system headers, so
if you're unsure, start without it. Pinning `platform = espressif32@7.0.1`
matches this repo's CI exactly.

### TFT_eSPI panel configuration

Same as [`CYD_BasicConnect`](../CYD_BasicConnect) - see that example's
README for the full `build_flags` block (pins, `ST7789_DRIVER`, SPI
frequencies, fonts).

## What it does

1. Runs the same Wi-Fi picker as the other examples.
2. On connect, syncs time via NTP (ezTime) and builds the clock face.
3. Shows rolling-digit time (12 or 24-hour, user-editable), weekday, and
   date. Tap the gear icon to change hour format or timezone - changes
   apply live and persist across reboots.
