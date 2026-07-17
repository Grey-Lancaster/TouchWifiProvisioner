# TouchWifiProvisioner

On-device Wi-Fi provisioning UI for LVGL touchscreens - scan, pick a network,
and enter a password with the on-screen keyboard.

No AP/captive-portal fallback (a la WiFiManager). That pattern exists to work
around headless devices that have no input method of their own - the device
opens a hotspot and borrows a phone's screen via a web form. A device with its
own touchscreen doesn't need that: on a failed connection (or no saved
credentials yet), this library just re-shows the on-device picker directly.

## Portability

This library only ever touches **LVGL objects** and the **WiFi/Preferences
APIs** - never the display bus or touch controller. As long as the host app
has already set up LVGL with a display + touch indev, it doesn't matter
whether that's an RGB parallel panel, an SPI panel, or anything else LVGL can
drive. Layout uses flex sizing (`lv_pct`) rather than hardcoded pixel
coordinates so it adapts to whatever screen resolution the host provides.

## Installation

**Arduino IDE:** not in the Library Manager index yet, so install manually -
either clone this repo into your sketchbook's `libraries/` folder, or
download it as a ZIP and use Sketch > Include Library > Add .ZIP Library.
Requires the `lvgl` library (search "lvgl" in the Library Manager) to also
be installed.

**PlatformIO:** add it to `lib_deps` in `platformio.ini`:

```ini
lib_deps =
    https://github.com/Grey-Lancaster/TouchWifiProvisioner.git
    lvgl/lvgl@^8.3.11
```

### Configuring LVGL

This library doesn't ship its own `lv_conf.h` - LVGL only supports one
config per build, and it's inherently host-specific (color depth, memory
budget, tick source), so it has to come from the project that owns the
display, not from a library. Your `lv_conf.h` needs these widgets enabled
(all on by default in LVGL's stock template, so only a concern if you've
trimmed it down): `LV_USE_FLEX`, `LV_USE_LIST`, `LV_USE_LABEL`,
`LV_USE_TEXTAREA`, `LV_USE_KEYBOARD`, `LV_USE_BTNMATRIX`, `LV_USE_CHECKBOX`,
`LV_USE_BTN`.

Setting up `lv_conf.h` itself is a one-time, per-project LVGL step, not
specific to this library:
- **Arduino IDE:** per [LVGL's Arduino docs](https://docs.lvgl.io/8.3/get-started/platforms/arduino.html),
  copy `lvgl/lv_conf_template.h` to `lv_conf.h` and place it in your
  sketchbook's `libraries/` folder *next to* (not inside) the `lvgl` folder.
- **PlatformIO:** place `lv_conf.h` in your project's `include/` folder and
  add `-DLV_CONF_INCLUDE_SIMPLE` to `build_flags`.

## Usage

Identical for Arduino IDE and PlatformIO - PlatformIO's `framework = arduino`
compiles against the same arduino-esp32 core, so this code is unchanged
either way (drop it into a `.ino` file or `src/main.cpp`, same result).

```cpp
#include <TouchWifiProvisioner.h>

void onWifiConnected(const String &ip) {
  Serial.printf("Connected, IP: %s\n", ip.c_str());
}

void setup() {
  // ...init display/touch/LVGL first...
  TouchWifiProvisioner::begin(lv_scr_act(), "Gramps", onWifiConnected);
}

void loop() {
  lv_timer_handler();
  TouchWifiProvisioner::loop();
}
```

Pass an optional `StatusCallback` as the 4th argument to `begin()` if the host
app wants to mirror connection status text onto its own UI (e.g. a status bar
outside the provisioning screen).

Call `TouchWifiProvisioner::reset()` (e.g. from a "Forget network" settings button)
to clear the saved credentials and show the picker again.

## Examples

- [`examples/CYD_BasicConnect`](examples/CYD_BasicConnect) - full bring-up on
  the Cheap Yellow Display (TFT_eSPI + XPT2046 + LVGL glue), showing a green
  "Connected" screen with the IP address once online.

## Storage

Credentials are persisted via `Preferences` under the `wifiprov` namespace, so
they don't collide with the host app's own settings namespace.

## License

[MIT](LICENSE)
