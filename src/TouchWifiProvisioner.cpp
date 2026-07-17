// Code by Papa Lanc and his Buddy Claude.

#include "TouchWifiProvisioner.h"

#include <WiFi.h>
#include <Preferences.h>
#include <vector>

namespace {
const char *PREFS_NAMESPACE = "wifiprov";
Preferences prefs;

// Color/spacing-only theme - deliberately no custom font sizes. lv_conf.h
// is host-specific and this library can't assume any font beyond whatever
// default the host already has enabled, so "prettier" stops at colors,
// radius, and spacing rather than typography.
const lv_color_t COLOR_BG = lv_color_hex(0x14171f);
const lv_color_t COLOR_CARD = lv_color_hex(0x1f2430);
const lv_color_t COLOR_CARD_PRESSED = lv_color_hex(0x2a3040);
const lv_color_t COLOR_ACCENT = lv_palette_main(LV_PALETTE_BLUE);
const lv_color_t COLOR_ACCENT_PRESSED = lv_palette_darken(LV_PALETTE_BLUE, 2);
const lv_color_t COLOR_TEXT = lv_color_white();
const lv_color_t COLOR_TEXT_MUTED = lv_color_hex(0x9aa0ab);

void styleScreen(lv_obj_t *screen) {
  lv_obj_set_style_bg_color(screen, COLOR_BG, 0);
  lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
  lv_obj_set_style_pad_row(screen, 14, 0);
  lv_obj_set_style_pad_top(screen, 20, 0);
}

void styleListRow(lv_obj_t *btn, bool accent) {
  lv_obj_set_style_bg_color(btn, COLOR_CARD, 0);
  lv_obj_set_style_bg_color(btn, COLOR_CARD_PRESSED, LV_STATE_PRESSED);
  lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(btn, 0, 0);
  lv_obj_set_style_text_color(btn, accent ? COLOR_ACCENT : COLOR_TEXT, 0);
  lv_obj_set_style_pad_ver(btn, 12, 0);
}

void styleActionButton(lv_obj_t *btn, bool primary) {
  lv_obj_set_style_bg_color(btn, primary ? COLOR_ACCENT : COLOR_CARD, 0);
  lv_obj_set_style_bg_color(btn, primary ? COLOR_ACCENT_PRESSED : COLOR_CARD_PRESSED, LV_STATE_PRESSED);
  lv_obj_set_style_radius(btn, 8, 0);
  lv_obj_set_style_text_color(btn, COLOR_TEXT, 0);
}
}

lv_obj_t *TouchWifiProvisioner::_parent = nullptr;
lv_obj_t *TouchWifiProvisioner::_screen = nullptr;
lv_obj_t *TouchWifiProvisioner::_list = nullptr;
lv_obj_t *TouchWifiProvisioner::_statusLabel = nullptr;
lv_obj_t *TouchWifiProvisioner::_passwordTa = nullptr;
lv_obj_t *TouchWifiProvisioner::_keyboard = nullptr;

String TouchWifiProvisioner::_hostname;
String TouchWifiProvisioner::_pendingSsid;
String TouchWifiProvisioner::_pendingPassword;
unsigned long TouchWifiProvisioner::_connectStartMs = 0;
TouchWifiProvisioner::State TouchWifiProvisioner::_state = TouchWifiProvisioner::State::Idle;

TouchWifiProvisioner::ConnectedCallback TouchWifiProvisioner::_onConnected = nullptr;
TouchWifiProvisioner::StatusCallback TouchWifiProvisioner::_onStatus = nullptr;

void TouchWifiProvisioner::begin(lv_obj_t *parent, const char *hostname,
                             ConnectedCallback onConnected, StatusCallback onStatus) {
  _parent = parent;
  _hostname = hostname;
  _onConnected = onConnected;
  _onStatus = onStatus;

  WiFi.mode(WIFI_STA);
  WiFi.setHostname(_hostname.c_str());

  tryStoredCredentials();
}

void TouchWifiProvisioner::tryStoredCredentials() {
  prefs.begin(PREFS_NAMESPACE, true);
  String ssid = prefs.getString("ssid", "");
  String password = prefs.getString("password", "");
  prefs.end();

  if (ssid.isEmpty()) {
    showPicker();
    return;
  }

  startConnect(ssid, password, false);
}

void TouchWifiProvisioner::startConnect(const String &ssid, const String &password, bool fromPicker) {
  _pendingSsid = ssid;
  _pendingPassword = password;
  _state = fromPicker ? State::ConnectingFromPicker : State::ConnectingStored;
  _connectStartMs = millis();
  reportStatus("Connecting to " + ssid + "...");
  WiFi.begin(ssid.c_str(), password.c_str());
}

void TouchWifiProvisioner::loop() {
  switch (_state) {
    case State::ConnectingStored:
    case State::ConnectingFromPicker: {
      if (WiFi.status() == WL_CONNECTED) {
        if (_state == State::ConnectingFromPicker) {
          prefs.begin(PREFS_NAMESPACE, false);
          prefs.putString("ssid", _pendingSsid);
          prefs.putString("password", _pendingPassword);
          prefs.end();
        }
        teardownUi();
        _state = State::Connected;
        reportStatus("Connected: " + WiFi.localIP().toString());
        if (_onConnected) _onConnected(WiFi.localIP().toString());
      } else if (millis() - _connectStartMs > CONNECT_TIMEOUT_MS) {
        reportStatus("Couldn't connect to " + _pendingSsid);
        showPicker();
      }
      break;
    }
    case State::Scanning: {
      int found = WiFi.scanComplete();
      if (found >= 0) {
        populateNetworkList();
      } else if (found == WIFI_SCAN_FAILED) {
        reportStatus("Scan failed, retrying...");
        WiFi.scanNetworks(true);
      }
      break;
    }
    default:
      break;
  }
}

void TouchWifiProvisioner::reset() {
  prefs.begin(PREFS_NAMESPACE, false);
  prefs.clear();
  prefs.end();
  WiFi.disconnect(true);
  showPicker();
}

bool TouchWifiProvisioner::isConnected() {
  return _state == State::Connected && WiFi.status() == WL_CONNECTED;
}

void TouchWifiProvisioner::showPicker() {
  teardownUi();
  _state = State::Scanning;

  _screen = lv_obj_create(_parent);
  lv_obj_set_size(_screen, lv_pct(100), lv_pct(100));
  lv_obj_center(_screen);
  lv_obj_set_flex_flow(_screen, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(_screen, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_all(_screen, 12, 0);
  styleScreen(_screen);

  lv_obj_t *title = lv_label_create(_screen);
  lv_label_set_text(title, LV_SYMBOL_WIFI "  Select a Wi-Fi Network");
  lv_obj_set_style_text_color(title, COLOR_TEXT, 0);

  _statusLabel = lv_label_create(_screen);
  lv_label_set_text(_statusLabel, "Scanning...");
  lv_obj_set_style_text_color(_statusLabel, COLOR_TEXT_MUTED, 0);

  _list = lv_list_create(_screen);
  lv_obj_set_size(_list, lv_pct(90), lv_pct(75));
  lv_obj_set_flex_grow(_list, 1);
  lv_obj_set_style_bg_color(_list, COLOR_CARD, 0);
  lv_obj_set_style_bg_opa(_list, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(_list, 0, 0);
  lv_obj_set_style_radius(_list, 14, 0);
  lv_obj_set_style_pad_all(_list, 4, 0);

  lv_obj_t *rescan = lv_list_add_btn(_list, LV_SYMBOL_REFRESH, "Rescan");
  styleListRow(rescan, /*accent=*/true);
  lv_obj_add_event_cb(rescan, onRescanClicked, LV_EVENT_CLICKED, nullptr);

  WiFi.scanNetworks(true);
  reportStatus("Scanning for networks...");
}

void TouchWifiProvisioner::populateNetworkList() {
  int count = WiFi.scanComplete();
  lv_obj_clean(_list);

  lv_obj_t *rescan = lv_list_add_btn(_list, LV_SYMBOL_REFRESH, "Rescan");
  styleListRow(rescan, /*accent=*/true);
  lv_obj_add_event_cb(rescan, onRescanClicked, LV_EVENT_CLICKED, nullptr);

  std::vector<String> seen;
  for (int i = 0; i < count; i++) {
    String ssid = WiFi.SSID(i);
    if (ssid.isEmpty()) continue;

    bool dup = false;
    for (const auto &s : seen) {
      if (s == ssid) { dup = true; break; }
    }
    if (dup) continue;
    seen.push_back(ssid);

    bool secure = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
    lv_obj_t *btn = lv_list_add_btn(_list, LV_SYMBOL_WIFI, ssid.c_str());
    lv_obj_set_user_data(btn, (void *)(intptr_t)(secure ? 1 : 0));
    styleListRow(btn, /*accent=*/false);
    lv_obj_add_event_cb(btn, onListButtonClicked, LV_EVENT_CLICKED, nullptr);
  }

  WiFi.scanDelete();
  _state = State::Picker;
  reportStatus(seen.empty() ? "No networks found - tap Rescan" : "Tap a network to connect");
}

void TouchWifiProvisioner::showPasswordEntry(const String &ssid) {
  _pendingSsid = ssid;
  lv_obj_clean(_screen);
  // _statusLabel and _list were children of _screen - lv_obj_clean just
  // freed them, so drop the stale pointers before anything (e.g.
  // reportStatus() on Connect) can dereference them.
  _statusLabel = nullptr;
  _list = nullptr;

  lv_obj_t *title = lv_label_create(_screen);
  lv_label_set_text_fmt(title, LV_SYMBOL_WIFI "  Password for %s", ssid.c_str());
  lv_obj_set_style_text_color(title, COLOR_TEXT, 0);

  _passwordTa = lv_textarea_create(_screen);
  lv_textarea_set_password_mode(_passwordTa, true);
  lv_textarea_set_one_line(_passwordTa, true);
  lv_textarea_set_placeholder_text(_passwordTa, "Password");
  lv_obj_set_width(_passwordTa, lv_pct(90));
  lv_obj_set_style_bg_color(_passwordTa, COLOR_CARD, 0);
  lv_obj_set_style_bg_opa(_passwordTa, LV_OPA_COVER, 0);
  lv_obj_set_style_text_color(_passwordTa, COLOR_TEXT, 0);
  lv_obj_set_style_border_color(_passwordTa, COLOR_ACCENT, 0);
  lv_obj_set_style_border_width(_passwordTa, 1, 0);
  lv_obj_set_style_radius(_passwordTa, 8, 0);

  lv_obj_t *showPasswordCb = lv_checkbox_create(_screen);
  lv_checkbox_set_text(showPasswordCb, "Show password");
  lv_obj_set_style_text_color(showPasswordCb, COLOR_TEXT_MUTED, 0);
  lv_obj_set_style_bg_color(showPasswordCb, COLOR_ACCENT, LV_PART_INDICATOR | LV_STATE_CHECKED);
  lv_obj_add_event_cb(showPasswordCb, onShowPasswordToggled, LV_EVENT_VALUE_CHANGED, nullptr);

  lv_obj_t *btnRow = lv_obj_create(_screen);
  lv_obj_set_size(btnRow, lv_pct(90), LV_SIZE_CONTENT);
  lv_obj_set_style_border_width(btnRow, 0, 0);
  lv_obj_set_style_bg_opa(btnRow, LV_OPA_TRANSP, 0);
  lv_obj_set_style_pad_all(btnRow, 0, 0);
  lv_obj_set_flex_flow(btnRow, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(btnRow, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  lv_obj_t *backBtn = lv_btn_create(btnRow);
  styleActionButton(backBtn, /*primary=*/false);
  lv_obj_add_event_cb(backBtn, onBackClicked, LV_EVENT_CLICKED, nullptr);
  lv_obj_t *backLabel = lv_label_create(backBtn);
  lv_label_set_text(backLabel, LV_SYMBOL_LEFT " Back");

  lv_obj_t *connectBtn = lv_btn_create(btnRow);
  styleActionButton(connectBtn, /*primary=*/true);
  lv_obj_add_event_cb(connectBtn, onConnectClicked, LV_EVENT_CLICKED, nullptr);
  lv_obj_t *connectLabel = lv_label_create(connectBtn);
  lv_label_set_text(connectLabel, "Connect " LV_SYMBOL_OK);

  _keyboard = lv_keyboard_create(_screen);
  lv_keyboard_set_textarea(_keyboard, _passwordTa);
  lv_obj_set_size(_keyboard, lv_pct(100), lv_pct(45));
}

void TouchWifiProvisioner::teardownUi() {
  if (_screen) {
    lv_obj_del(_screen);
    _screen = nullptr;
  }
  _list = nullptr;
  _statusLabel = nullptr;
  _passwordTa = nullptr;
  _keyboard = nullptr;
}

void TouchWifiProvisioner::reportStatus(const String &message) {
  if (_statusLabel) lv_label_set_text(_statusLabel, message.c_str());
  if (_onStatus) _onStatus(message);
}

void TouchWifiProvisioner::onListButtonClicked(lv_event_t *e) {
  lv_obj_t *btn = static_cast<lv_obj_t *>(lv_event_get_target(e));
  const char *text = lv_list_get_btn_text(_list, btn);
  if (!text) return;

  bool secure = (intptr_t)lv_obj_get_user_data(btn) != 0;
  if (secure) {
    showPasswordEntry(String(text));
  } else {
    // Open network - no password to collect, connect straight away.
    startConnect(String(text), "", true);
  }
}

void TouchWifiProvisioner::onRescanClicked(lv_event_t *e) {
  lv_obj_clean(_list);
  WiFi.scanNetworks(true);
  _state = State::Scanning;
  reportStatus("Scanning for networks...");
}

void TouchWifiProvisioner::onConnectClicked(lv_event_t *e) {
  String password = _passwordTa ? lv_textarea_get_text(_passwordTa) : "";
  startConnect(_pendingSsid, password, true);
}

void TouchWifiProvisioner::onBackClicked(lv_event_t *e) {
  showPicker();
}

void TouchWifiProvisioner::onShowPasswordToggled(lv_event_t *e) {
  lv_obj_t *cb = static_cast<lv_obj_t *>(lv_event_get_target(e));
  bool show = lv_obj_has_state(cb, LV_STATE_CHECKED);
  if (_passwordTa) lv_textarea_set_password_mode(_passwordTa, !show);
}
