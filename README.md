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

## Usage

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

## Storage

Credentials are persisted via `Preferences` under the `wifiprov` namespace, so
they don't collide with the host app's own settings namespace.
