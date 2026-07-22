# Changelog

All notable changes to this project are documented here. Versions follow
`major.minor.patch`; patch is bumped by 4 on each release, carrying into
minor odometer-style once patch would reach double digits (e.g.
`0.4.9 + 0.0.4 = 0.5.3`, not `0.4.13`).

## [0.9.7] - 2026-07-22

### Added
- Ready-to-use [`User_Setup.h`](User_Setup.h) at the repo root for Arduino
  IDE users on CYD hardware - TFT_eSPI has the same "no build flags in
  Arduino IDE" problem `lv_conf.h` had, needing its config baked into an
  actual file inside the TFT_eSPI library folder instead. Replace
  `TFT_eSPI/User_Setup.h` in your sketchbook with it and both CYD examples
  just work.

## [0.9.3] - 2026-07-22

### Added
- Ready-to-use [`lv_conf.h`](lv_conf.h) at the repo root for Arduino IDE
  users - copy it into your sketchbook's `libraries/` folder (next to
  the `lvgl` folder) and every example here just works, no manual
  widget/font/memory-mode configuration needed. Merges the two slightly
  different configs the examples were built with in CI into one: uses
  the malloc-based LVGL heap (`LV_MEM_CUSTOM`) everywhere, since it's
  strictly safer and CYD_RollingClock needs it anyway.
- README's Arduino IDE install and LVGL-config sections now point to
  this file directly instead of just linking to LVGL's own generic
  template.

## [0.8.9] - 2026-07-22

### Fixed
- Arduino IDE's Library Manager doesn't enforce a version when a
  dependency has none pinned, so it was installing LVGL 9.x by default -
  which completely restructured the display/input driver API
  (`lv_disp_drv_t`/`lv_indev_drv_t` don't exist anymore), breaking every
  example. `library.properties` now pins `lvgl (^8.3.11)`, and the
  README calls out explicitly to pick an 8.x version from the Library
  Manager's version dropdown, not the latest.

## [0.8.5] - 2026-07-22

### Added
- Every example now ships its own `platformio.ini` (and `include/lv_conf.h`
  where needed) - `git clone` this repo, `cd examples/<name>`, `pio run`
  works with no manual file-copying, closing the gap where PlatformIO
  users previously had to hand-assemble a project per each example's
  README instructions. Verified against a genuinely fresh clone (no
  cached local state) for three of the four examples; the fourth uses
  the identical mechanism.
- Uses PlatformIO's `lib_extra_dirs` to pull in the library itself from
  three folders up, and `src_dir = .` since the `.ino` has to stay in the
  example's own folder (not a `src/` subfolder) for Arduino IDE's
  File > Examples convention to keep working.

## [0.8.1] - 2026-07-22

### Fixed
- Hardware testing found `WiFi.scanNetworks()` could complete with zero
  results on the very first scan after boot, on some toolchain/core
  builds - even a real cold reboot didn't self-heal it, and it
  reproduced on both CYD_BasicConnect and CYD_RollingClock (confirming
  it's in the shared library, not example-specific). `begin()` now does
  the same `WiFi.mode(WIFI_OFF)` / `WiFi.mode(WIFI_STA)` reset cycle
  added in v0.7.7 for the post-failed-connect case, before the very
  first scan too. Also added a few silent scan retries before actually
  reporting "no networks found," as a second line of defense.
- CrowPanel7 examples now rebuilt with this fix too - all four hosted
  firmwares on flash.html are current with v0.8.1.

## [0.7.7] - 2026-07-22

### Fixed
- After a failed connect (wrong password especially), the picker's rescan
  could come back permanently empty until a power cycle - a plain
  `WiFi.disconnect()` wasn't enough to reset the ESP32 WiFi driver, so
  `scanNetworks()` would "complete" instantly with zero results instead
  of actually failing, meaning the built-in scan-retry logic never
  triggered. Now does a full `WiFi.mode(WIFI_OFF)` / `WiFi.mode(WIFI_STA)`
  cycle before rescanning. Found by hardware testing right after v0.7.3
  shipped the fail-fast connect-error handling this exposed.

## [0.7.3] - 2026-07-20

### Added
- `setOnDisconnected()` - optional callback that fires once when an
  established connection drops. The library then waits for the network to
  come back (nudging `WiFi.reconnect()` every 15s) without touching the
  screen, and fires `onConnected` again on recovery. Previously a drop
  after connect was silently ignored.
- Open (passwordless) networks are now tagged "open" in the picker list.
  (LVGL's built-in symbol font has no padlock glyph, so the rare open
  networks get marked rather than decorating every secured one.)

### Fixed
- The password screen now has its own status line, so tapping Connect
  shows "Connecting..." (and any failure) instead of leaving the screen
  frozen for the whole attempt
- The on-screen keyboard's checkmark key now connects and its close key
  now goes back to the picker - previously both did nothing
- Wrong passwords and vanished networks now fail fast on
  `WL_CONNECT_FAILED` / `WL_NO_SSID_AVAIL` with a specific message
  ("wrong password?" / "Network not found"), instead of always burning
  the full 15-second timeout with a generic error
- A failed connect now calls `WiFi.disconnect()` before rescanning, so
  the picker's scan doesn't race a radio still stuck mid-connect
- Password entry is capped at 63 characters (the WPA2 passphrase limit)

## [0.6.9] - 2026-07-18

### Fixed
- `CYD_RollingClock` and `CrowPanel7_RollingClock` were only logging the IP
  address on connect, unlike the BasicConnect examples which dump the full
  connection info (SSID, gateway, subnet, DNS, RSSI with signal bars,
  channel, BSSID, MAC). Both clock examples now log the same full block.

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
