#pragma once

// Code by Papa Lanc and his Buddy Claude.

#include <Arduino.h>
#include <lvgl.h>
#include <functional>

// On-device Wi-Fi provisioning UI for LVGL touchscreens.
//
// This library only ever touches LVGL objects and the WiFi/Preferences APIs -
// never the display bus or touch controller directly. As long as the host app
// has already registered a display + touch indev with LVGL (RGB parallel
// panel, SPI panel, whatever), this works unmodified.
//
// There is deliberately no AP/captive-portal fallback: that pattern exists to
// work around headless devices with no input method. A device with its own
// touchscreen doesn't need to borrow a phone's screen to enter credentials -
// on connect failure this just re-shows the on-device picker.
//
// Usage:
//   void onWifiConnected(const String &ip) { ... }
//
//   void setup() {
//     // ...init display/touch/LVGL first...
//     TouchWifiProvisioner::begin(lv_scr_act(), "Gramps", onWifiConnected);
//   }
//
//   void loop() {
//     lv_timer_handler();
//     TouchWifiProvisioner::loop();
//   }
class TouchWifiProvisioner {
public:
  using ConnectedCallback = std::function<void(const String &ip)>;
  using StatusCallback = std::function<void(const String &message)>;
  using DisconnectedCallback = std::function<void()>;

  // Loads saved credentials and tries to connect; if there are none, or the
  // saved network can't be reached within the connect timeout, builds a
  // scan/pick/keyboard UI inside `parent`. `onConnected` fires once per
  // successful connection (including reconnects after reset()).
  static void begin(lv_obj_t *parent, const char *hostname,
                     ConnectedCallback onConnected,
                     StatusCallback onStatus = nullptr);

  // Call every loop() iteration - drives scanning/connecting without
  // blocking lv_timer_handler().
  static void loop();

  // Forgets the saved network and re-shows the picker UI.
  static void reset();

  static bool isConnected();

  // Optional: fires once when an established connection drops. The library
  // then waits for the connection to come back (nudging WiFi.reconnect()
  // periodically) without touching the screen - it never tears down the
  // host app's UI on a transient outage - and fires onConnected again once
  // the network returns.
  static void setOnDisconnected(DisconnectedCallback cb);

private:
  enum class State {
    Idle,
    ConnectingStored,
    ConnectingFromPicker,
    Connected,
    WaitingReconnect,
    Scanning,
    Picker,
  };

  static void tryStoredCredentials();
  static void startConnect(const String &ssid, const String &password, bool fromPicker);
  static void showPicker();
  static void populateNetworkList();
  static void showPasswordEntry(const String &ssid);
  static void teardownUi();
  static void reportStatus(const String &message);

  static void onListButtonClicked(lv_event_t *e);
  static void onRescanClicked(lv_event_t *e);
  static void onConnectClicked(lv_event_t *e);
  static void onBackClicked(lv_event_t *e);
  static void onShowPasswordToggled(lv_event_t *e);

  static lv_obj_t *_parent;
  static lv_obj_t *_screen;
  static lv_obj_t *_list;
  static lv_obj_t *_statusLabel;
  static lv_obj_t *_passwordTa;
  static lv_obj_t *_keyboard;

  static String _hostname;
  static String _pendingSsid;
  static String _pendingPassword;
  static unsigned long _connectStartMs;
  static State _state;
  static int _emptyScanRetries;

  static ConnectedCallback _onConnected;
  static StatusCallback _onStatus;
  static DisconnectedCallback _onDisconnected;

  static const unsigned long CONNECT_TIMEOUT_MS = 15000;
  // Definitive-failure statuses (wrong password, SSID not found) are honored
  // after this grace period instead of waiting out the full timeout. The
  // grace exists because WiFi.status() can briefly report a stale failure
  // state right after WiFi.begin().
  static const unsigned long FAIL_FAST_GRACE_MS = 1500;
  // A scan can "succeed" with zero results if the radio wasn't fully
  // settled yet (right after WiFi.mode(WIFI_STA), notably on first boot) -
  // that's not a real "no networks here" and deserves a few silent
  // retries before actually telling the user nothing was found.
  static const int MAX_EMPTY_SCAN_RETRIES = 3;
};
