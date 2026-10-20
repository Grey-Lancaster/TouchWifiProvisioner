# CLAUDE.md

TouchWifiProvisioner: an Arduino/PlatformIO library (ESP32, LVGL 8.x) that gives a touchscreen device its own Wi-Fi setup UI: scan, pick a network, type the password on an on-screen keyboard. No AP/captive-portal fallback by design. English and Spanish screens, up to 5 remembered networks, hidden-network entry. MIT licensed, public repo. `README.md` is the user guide and API description.

## Layout
- `src/TouchWifiProvisioner.{h,cpp}` - the whole library (`begin`, `loop`, `reset`, `openPicker`, `setOnDisconnected`, `setReconnectTimeout`, `setMaxSavedNetworks`, `setLanguage`, ...). `src/twp_font_latin1_14.c` is a small bundled font for accented characters (`TWP_NO_LATIN1_FONT` drops it).
- `library.json`, `library.properties` - library metadata; both carry the version.
- `lv_conf.h`, `User_Setup.h` (repo root) - ready-made LVGL and TFT_eSPI configs for Arduino IDE users of the CYD examples. PlatformIO examples ship their own `include/lv_conf.h`.
- `examples/` - `CYD_BasicConnect`, `CYD_RollingClock` (Cheap Yellow Display) and `CrowPanel7_BasicConnect`, `CrowPanel7_RollingClock` (Elecrow CrowPanel Advance 7", PlatformIO only). Each has its own `platformio.ini`; build with `cd examples/<name> && pio run` (they find the library through `lib_extra_dirs = ../../..`).
- `docs/` - the GitHub Pages site: `index.html`, `flash.html` (browser flasher) and prebuilt demo firmware under `docs/firmware/<example>/` (bootloader, partitions, firmware, manifest). Committed binaries; regenerate them rather than hand-editing.
- `.github/ci/` - `lv_conf` files used by CI. `.github/workflows/build-examples.yml` builds every example (push to main, PRs, weekly cron). `release.yml` publishes releases.

## Gotchas
- LVGL must be 8.x. LVGL 9 removed the driver structs this library and all examples use, so it will not compile.
- The library only touches LVGL objects and the WiFi/Preferences APIs, never the display bus or touch controller; keep it that way so it stays portable. Layout uses flex/percent sizing, not pixel coordinates.
- `lv_conf.h` must enable the widgets listed in the README (flex, list, label, textarea, keyboard, btnmatrix, checkbox, btn).
- Credentials are stored unencrypted in NVS under the `wifiprov` namespace (documented in the README).
- The CrowPanel7 CI jobs download a release asset and upstream packages at build time, so the weekly run can fail without any commit here.
- Downstream projects (CYD-PhotoFrame, CYD_Weather) pin this library by commit hash in `platformio.ini`; changing public behaviour needs a coordinated bump there.

## Release conventions
- Tags are bare versions with no `v` (`1.6.4`, `1.6.0`, `1.5.0`). Pushing a tag runs `release.yml`, which copies the matching `## [<tag>]` section of `CHANGELOG.md` into the GitHub Release body, so the tag must match a CHANGELOG heading exactly.
- Version rule (from `CHANGELOG.md`): patch goes up by 4 per release and carries into minor odometer-style once it would reach two digits. Update `library.json`, `library.properties` and `CHANGELOG.md` together.
- Do not tag or publish a release unless asked.
