# TouchWifiProvisioner

On-device Wi-Fi provisioning UI for LVGL touchscreens - scan, pick a network,
and enter a password with the on-screen keyboard.

Code by Grey and his buddy Claude.

No AP/captive-portal fallback (a la WiFiManager). That pattern exists to work
around headless devices that have no input method of their own - the device
opens a hotspot and borrows a phone's screen via a web form. A device with its
own touchscreen doesn't need that: on a failed connection (or no saved
credentials yet), this library just re-shows the on-device picker directly.

## An easy way to see it

Don't want to build anything yet? [Try it in your browser](https://grey-lancaster.github.io/TouchWifiProvisioner/flash.html)
(right-click and open in a new tab so you don't lose this page) - it flashes
a real demo straight onto a CYD or CrowPanel Advance 7" over USB from Chrome
or Edge, no toolchain or IDE needed. Good for seeing the picker/keyboard flow
work on real hardware before deciding whether to wire this into your own
project.

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
be installed - **pick version 8.3.11 or any other 8.x from the version
dropdown, not the latest**. LVGL 9 restructured the display/input driver
API entirely (`lv_disp_drv_t`/`lv_indev_drv_t` don't exist anymore), so
this library and every example here won't compile against it. You'll also
need an `lv_conf.h` in place before anything compiles - see
[Configuring LVGL](#configuring-lvgl) below, this repo ships a ready-made
one. If you're trying the CYD examples, TFT_eSPI needs its own one-time
config too - this repo ships a ready-made [`User_Setup.h`](User_Setup.h)
for that as well (see the [`CYD_BasicConnect` example's
README](examples/CYD_BasicConnect#tft_espi-panel-configuration)).

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
- **Arduino IDE:** this repo includes a ready-to-use [`lv_conf.h`](lv_conf.h)
  at its root with everything every example here needs already enabled -
  copy *that* file (not LVGL's own template) into your sketchbook's
  `libraries/` folder, *next to* (not inside) the `lvgl` folder, per
  [LVGL's Arduino docs](https://docs.lvgl.io/8.3/get-started/platforms/arduino.html).
  This is a one-time, global setup shared by every sketch that uses lvgl,
  not per-example - Arduino IDE has no way to place this automatically
  when you open an example, which is why it isn't done for you.
- **PlatformIO:** each example already ships its own `include/lv_conf.h` -
  nothing to set up (see [Examples](#examples) below).

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

Optionally call `TouchWifiProvisioner::setOnDisconnected(cb)` to be told when an
established connection drops. The library then waits for the network to come
back on its own - it never tears down your app's UI over a transient outage -
and fires `onConnected` again once it recovers.

## Examples

Each example is a full, board-specific sketch: display/touch/LVGL bring-up
plus a call into `TouchWifiProvisioner::begin()`, ending in a green
"Connected" screen with the SSID and IP address once online. The bring-up section is
what changes per board - swap it for your own display/touch setup and
`TouchWifiProvisioner::begin()`/`loop()` still work unmodified, per
[Portability](#portability) above.

**Examples need the library installed alongside them** - they only contain
the sketch, not `TouchWifiProvisioner.h`/`.cpp` itself. That's normal for
Arduino libraries: install this repo via one of the methods in
[Installation](#installation) (which pulls in `src/` too), don't just
download an `examples/<name>` subfolder on its own.

- **Arduino IDE:** after installing, open via File > Examples >
  TouchWifiProvisioner > *(example name)*.
- **PlatformIO:** each example folder already has its own `platformio.ini` -
  `git clone` this repo, `cd examples/<name>`, `pio run`. No copying files
  around, no manual fetch steps, for any example.

Available examples:

- [`examples/CYD_BasicConnect`](examples/CYD_BasicConnect) - Cheap Yellow
  Display (TFT_eSPI + XPT2046 + LVGL glue). Works with Arduino IDE or
  PlatformIO.
- [`examples/CrowPanel7_BasicConnect`](examples/CrowPanel7_BasicConnect) -
  [Elecrow CrowPanel Advance 7.0" HMI](https://github.com/Elecrow-RD/CrowPanel-Advance-7-HMI-ESP32-S3-AI-Powered-IPS-Touch-Screen-800x480)
  (ESP32-S3, RGB panel + GT911 touch via LovyanGFX). PlatformIO-only - see
  that example's README for why.
- [`examples/CYD_RollingClock`](examples/CYD_RollingClock) - a full app
  example on the Cheap Yellow Display: an odometer-style clock, showing
  what it looks like to hand off from the provisioning flow into an app
  that keeps running afterward, instead of just a static "Connected"
  screen.
- [`examples/CrowPanel7_RollingClock`](examples/CrowPanel7_RollingClock) -
  the same rolling-digit clock app, ported to the CrowPanel Advance 7.0".
  PlatformIO-only, same reason as `CrowPanel7_BasicConnect`.

## Appearance

The picker and password screens use a small built-in dark theme - card-style
list with rounded corners, a blue accent for the primary action (Rescan,
Connect), muted grey for status text. This is colors/spacing/radius only,
never custom font sizes: `lv_conf.h` is host-specific, so the library can't
assume any font beyond whatever default the host project already has
enabled. The theme isn't currently configurable - if you need to match a
specific host app's palette, the styling lives in `styleScreen()`,
`styleListRow()`, and `styleActionButton()` near the top of
`TouchWifiProvisioner.cpp`.

## Storage

Credentials are persisted via `Preferences` under the `wifiprov` namespace, so
they don't collide with the host app's own settings namespace.

## License

[MIT](LICENSE)
