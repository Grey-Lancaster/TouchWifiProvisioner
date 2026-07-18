# Changelog

All notable changes to this project are documented here. Versions follow
`major.minor.patch`; patch is bumped by 4 on each release, carrying into
minor odometer-style once patch would reach double digits (e.g.
`0.4.9 + 0.0.4 = 0.5.3`, not `0.4.13`).

## [0.6.5] - 2026-07-18

### Added
- `examples/CrowPanel7_RollingClock` - the rolling-digit clock app ported to
  the Elecrow CrowPanel Advance 7" HMI. Reuses the same ClockFace/
  RollingDigit/SettingsPanel/ClockSettings code as `CYD_RollingClock`
  verbatim (it's plain LVGL, doesn't care about screen size); only the
  display/touch/backlight bring-up is board-specific.
- Fourth board card on `flash.html` for this firmware
- Fourth CI job building this example

### Fixed
- Documented that this board's toolchain is old-newlib-based, not
  picolibc-based like the CYD's - it must NOT use the `-D__time_t_defined`
  build flag that `CYD_RollingClock` needs, or every system header needing
  `time_t` breaks. Confirmed by testing both ways locally.

## [0.6.1] - 2026-07-17

### Added
- `examples/CYD_RollingClock` - a full app example (not just a connect
  demo): an odometer-style rolling-digit clock on the CYD, showing
  TouchWifiProvisioner handing off into a real running app instead of a
  static "Connected" screen. Ported from the LVGL_Rolling_Clock project.
  Documents the `LV_MEM_CUSTOM` requirement and the ezTime/picolibc
  `time_t` build flag workaround.
- Third board card on `flash.html` for the Rolling Clock firmware
- Third CI job building this example

## [0.5.7] - 2026-07-17

### Added
- "Forget Network" button on both examples' connected screen, calling
  the existing `TouchWifiProvisioner::reset()` - the library itself
  doesn't render a connected screen at all, so this lives entirely in
  the examples, not the library

## [0.5.3] - 2026-07-17

### Added
- CI: GitHub Actions workflow builds both examples on every push/PR
- GitHub Releases are now created automatically on tag push, with notes
  pulled from this changelog

## [0.4.9] - 2026-07-17

### Added
- `examples/CYD_BasicConnect` - full bring-up on the Cheap Yellow Display
  (TFT_eSPI + XPT2046 + LVGL glue)
- `examples/CrowPanel7_BasicConnect` - full bring-up on the Elecrow
  CrowPanel Advance 7.0" HMI (ESP32-S3, RGB panel + GT911 touch via
  LovyanGFX), PlatformIO-only
- Dark card theme for the picker and password-entry screens (colors/
  spacing/radius only, no custom font sizes)
- GitHub Pages landing site (`docs/index.html`) with device mockups,
  feature highlights, install snippet, SEO/Open Graph tags, and Google
  Analytics
- Browser-based flash tool (`docs/flash.html`, ESP Web Tools) to install
  either example straight from Chrome/Edge over USB, no toolchain needed
- Both examples now show the connected SSID and a 4-bar signal-strength
  icon on-screen, and dump full connection info to Serial (SSID, IP,
  gateway, subnet, DNS, RSSI, channel, BSSID, MAC)

## [0.1.0] - 2026-07-17

### Added
- Initial release: on-device Wi-Fi provisioning UI for LVGL touchscreens -
  scan, pick a network, enter a password with the on-screen keyboard
- MIT license
- Arduino IDE and PlatformIO installation docs
