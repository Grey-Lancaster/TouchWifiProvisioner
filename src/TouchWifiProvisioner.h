#pragma once

// Code by Papa Lanc and his Buddy Claude.

#include <Arduino.h>
#include <lvgl.h>
#include <functional>
#include <vector>

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
  using CancelCallback = std::function<void()>;

  enum class Language : uint8_t { English = 0, Spanish = 1 };

  // Loads the saved networks and tries to connect; if there are none, or
  // none of them can be reached, builds a scan/pick/keyboard UI inside
  // `parent`. With several saved networks it first scans and tries the
  // strongest one in range, then the rest. `onConnected` fires once per
  // successful connection (including reconnects after reset()).
  static void begin(lv_obj_t *parent, const char *hostname,
                     ConnectedCallback onConnected,
                     StatusCallback onStatus = nullptr);

  // Call every loop() iteration - drives scanning/connecting without
  // blocking lv_timer_handler().
  static void loop();

  // Forgets the saved network and re-shows the picker UI.
  static void reset();

  // Shows the picker over the host app's UI WITHOUT forgetting the saved
  // networks, e.g. from a "Wi-Fi" button in the app. It never times out and
  // has a Cancel row at the top; Cancel closes the picker, keeps the current
  // connection (or keeps trying the saved networks) and calls `onCancel`.
  // Picking and joining a network saves it, closes the picker and fires the
  // normal onConnected callback again. Call it from the same loop that calls
  // loop(); the host app should keep calling lv_timer_handler() and loop().
  static void openPicker(CancelCallback onCancel = nullptr);

  static bool isConnected();

  // Optional: fires once when an established connection drops. The library
  // then waits for the connection to come back (nudging WiFi.reconnect()
  // periodically) without touching the screen - it never tears down the
  // host app's UI on a transient outage - and fires onConnected again once
  // the network returns.
  static void setOnDisconnected(DisconnectedCallback cb);

  // Optional: if an established connection stays down for longer than
  // `ms`, give up waiting and re-show the on-device picker (over the host
  // app's UI) so the user can choose a different network or fix the
  // password. Before showing the picker it tries every saved network (so a
  // board carried to another known SSID roams on its own). The saved
  // credentials are kept until you forget them. 0 (the default) means wait
  // forever, the original behavior.
  static void setReconnectTimeout(unsigned long ms);

  // Networks the board remembers, most recently used first. Every
  // successful connect from the picker saves (or updates) its network.
  // Default capacity is 5 (clamped to 1..10); the oldest is dropped when
  // full. Upgrading from a single-network version keeps that network.
  // UI language for the library's own screens and status messages (the
  // `onStatus` callback gets them too; Serial logging stays English). On a
  // fresh board - nothing saved and no language chosen yet - the first
  // screen asks "English / Espanol" and remembers the answer, so this is
  // only needed to preset a language or to build your own settings screen.
  // Takes effect on the next screen that is built. Persisted separately
  // from the networks, so reset() keeps it. Spanish accents are drawn with
  // a small bundled Latin-1 font (define TWP_NO_LATIN1_FONT to drop it).
  static void setLanguage(Language lang);
  static Language language();

  static std::vector<String> savedNetworks();
  static void forget(const String &ssid);
  static void setMaxSavedNetworks(uint8_t n);

private:
  enum class State {
    Idle,
    ConnectingStored,
    ConnectingFromPicker,
    Connected,
    WaitingReconnect,
    Scanning,
    Picker,
    ScanningSaved,
  };

  struct SavedNet {
    String ssid;
    String password;
  };

  static std::vector<SavedNet> loadSaved();
  static void saveSaved(const std::vector<SavedNet> &list);
  static void rememberNetwork(const String &ssid, const String &password);
  static void beginSavedScan();
  static void orderCandidates(int scanCount);
  static void tryNextCandidate();
  static void onListButtonLongPressed(lv_event_t *e);

  static void tryStoredCredentials();
  static void startConnect(const String &ssid, const String &password, bool fromPicker);
  static void showPicker();
  static void populateNetworkList();
  static void showPasswordEntry(const String &ssid);
  static void showHiddenSsidEntry();
  static void showLanguageSelect();
  static void applyFont(lv_obj_t *screen);
  static void loadLanguage();
  static void resetRadio();
  static void teardownUi();
  static void reportStatus(const String &message);

  static void onListButtonClicked(lv_event_t *e);
  static void onRescanClicked(lv_event_t *e);
  static void onConnectClicked(lv_event_t *e);
  static void onBackClicked(lv_event_t *e);
  static void onCancelClicked(lv_event_t *e);
  static void addCancelRow();
  static void onHiddenClicked(lv_event_t *e);
  static void onLanguageChosen(lv_event_t *e);
  static void onLanguageToggleClicked(lv_event_t *e);
  static void onHiddenNextClicked(lv_event_t *e);
  static void onShowPasswordToggled(lv_event_t *e);

  static lv_obj_t *_parent;
  static lv_obj_t *_screen;
  static lv_obj_t *_list;
  static lv_obj_t *_statusLabel;
  static lv_obj_t *_passwordTa;
  static lv_obj_t *_keyboard;
  static lv_obj_t *_ssidTa;

  static String _hostname;
  static String _pendingSsid;
  static String _pendingPassword;
  static unsigned long _connectStartMs;
  static unsigned long _outageStartMs;
  static unsigned long _reconnectTimeoutMs;
  static State _state;
  static int _emptyScanRetries;
  static std::vector<SavedNet> _candidates;
  static size_t _candIdx;
  static uint8_t _maxSaved;
  static Language _lang;
  static bool _langChosen;
  static bool _cancelable;
  static CancelCallback _onCancel;

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
  // Arduino-ESP32 2.x gives an async scan a hidden timeout of 20 * max_ms_per_chan
  // (6 s at the 300 ms default) and scanComplete() reports WIFI_SCAN_FAILED once
  // it passes, even if the scan finished. A full scan takes 6-7 s on the S3, so
  // the default races that timeout; the retry then aborts the running scan and
  // the picker shows "0 networks". 600 ms gives a 12 s timeout.
  static const uint32_t SCAN_MS_PER_CHAN = 600;
};
