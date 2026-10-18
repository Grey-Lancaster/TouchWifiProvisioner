// Code by Papa Lanc and his Buddy Claude.

#include "TouchWifiProvisioner.h"

#include <WiFi.h>
#include <Preferences.h>
#include <vector>
#include <algorithm>

extern "C" {
LV_FONT_DECLARE(twp_font_latin1_14);
}

namespace {
const char *PREFS_NAMESPACE = "wifiprov";
const uint8_t MAX_SAVED_HARD = 10;
const char *UI_NAMESPACE = "wifiprov_ui"; // separate so reset() keeps the language
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

enum Str : uint8_t {
  S_SELECT_NETWORK, S_SCANNING, S_SCANNING_NETWORKS, S_RESCAN, S_HIDDEN_NETWORK,
  S_TAP_CONNECT, S_TAP_CONNECT_HOLD, S_NO_NETWORKS, S_PASSWORD, S_PASSWORD_FOR,
  S_SHOW_PASSWORD, S_BACK, S_CONNECT, S_NEXT, S_HIDDEN_TITLE, S_SSID_PLACEHOLDER,
  S_TAG_OPEN, S_TAG_SAVED, S_TAG_OPEN_SAVED, S_CONNECTING_TO, S_CONNECTED,
  S_RECONNECTED, S_LOST, S_LOOKING_SAVED, S_SCAN_FAILED, S_NOT_FOUND,
  S_NO_SAVED_WORKED, S_COULDNT_CONNECT, S_WRONG_PASSWORD, S_HOLD_FORGET_HINT,
  S_COULDNT_RECONNECT, S_FORGOT, S_LANGUAGE_TITLE, S_OTHER_LANGUAGE, S_CANCEL,
  S_STR_COUNT
};

// [English, Spanish]. Messages that end in a network name are prefixes.
const char *const STRINGS[S_STR_COUNT][2] = {
  /* S_SELECT_NETWORK   */ {"Select a Wi-Fi Network", "Elige una red Wi-Fi"},
  /* S_SCANNING         */ {"Scanning...", "Buscando..."},
  /* S_SCANNING_NETWORKS*/ {"Scanning for networks...", "Buscando redes..."},
  /* S_RESCAN           */ {"Rescan", "Buscar de nuevo"},
  /* S_HIDDEN_NETWORK   */ {"Hidden network...", "Red oculta..."},
  /* S_TAP_CONNECT      */ {"Tap a network to connect", "Toca una red para conectar"},
  /* S_TAP_CONNECT_HOLD */ {"Tap a network to connect - hold a saved one to forget it",
                            "Toca una red para conectar - mantén pulsada una guardada para olvidarla"},
  /* S_NO_NETWORKS      */ {"No networks found - tap Rescan",
                            "No se encontraron redes - toca Buscar de nuevo"},
  /* S_PASSWORD         */ {"Password", "Contraseña"},
  /* S_PASSWORD_FOR     */ {"Password for ", "Contraseña de "},
  /* S_SHOW_PASSWORD    */ {"Show password", "Mostrar contraseña"},
  /* S_BACK             */ {"Back", "Atrás"},
  /* S_CONNECT          */ {"Connect", "Conectar"},
  /* S_NEXT             */ {"Next", "Siguiente"},
  /* S_HIDDEN_TITLE     */ {"Hidden network name", "Nombre de la red oculta"},
  /* S_SSID_PLACEHOLDER */ {"Network name (SSID)", "Nombre de red (SSID)"},
  /* S_TAG_OPEN         */ {"open", "abierta"},
  /* S_TAG_SAVED        */ {"saved", "guardada"},
  /* S_TAG_OPEN_SAVED   */ {"open, saved", "abierta, guardada"},
  /* S_CONNECTING_TO    */ {"Connecting to ", "Conectando a "},
  /* S_CONNECTED        */ {"Connected: ", "Conectado: "},
  /* S_RECONNECTED      */ {"Reconnected: ", "Reconectado: "},
  /* S_LOST             */ {"Wi-Fi connection lost - reconnecting...",
                            "Conexión Wi-Fi perdida - reconectando..."},
  /* S_LOOKING_SAVED    */ {"Looking for saved networks...", "Buscando redes guardadas..."},
  /* S_SCAN_FAILED      */ {"Scan failed, retrying...", "Falló la búsqueda, reintentando..."},
  /* S_NOT_FOUND        */ {"Network not found: ", "Red no encontrada: "},
  /* S_NO_SAVED_WORKED  */ {"Couldn't connect to any saved network",
                            "No se pudo conectar a ninguna red guardada"},
  /* S_COULDNT_CONNECT  */ {"Couldn't connect to ", "No se pudo conectar a "},
  /* S_WRONG_PASSWORD   */ {" - wrong password?", " - ¿contraseña incorrecta?"},
  /* S_HOLD_FORGET_HINT */ {" (hold it to forget, then re-add)",
                            " (mantén pulsada para olvidarla y vuelve a añadirla)"},
  /* S_COULDNT_RECONNECT*/ {"Couldn't reconnect to ", "No se pudo reconectar a "},
  /* S_FORGOT           */ {"Forgot ", "Olvidada: "},
  /* S_LANGUAGE_TITLE   */ {"Language / Idioma", "Language / Idioma"},
  /* S_OTHER_LANGUAGE   */ {"Español", "English"}, // names the language a tap switches TO
  /* S_CANCEL           */ {"Cancel", "Cancelar"},
};

const char *tr(Str s, TouchWifiProvisioner::Language lang) {
  return STRINGS[s][lang == TouchWifiProvisioner::Language::Spanish ? 1 : 0];
}

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
lv_obj_t *TouchWifiProvisioner::_ssidTa = nullptr;

String TouchWifiProvisioner::_hostname;
String TouchWifiProvisioner::_pendingSsid;
String TouchWifiProvisioner::_pendingPassword;
unsigned long TouchWifiProvisioner::_connectStartMs = 0;
unsigned long TouchWifiProvisioner::_outageStartMs = 0;
unsigned long TouchWifiProvisioner::_reconnectTimeoutMs = 0;
TouchWifiProvisioner::State TouchWifiProvisioner::_state = TouchWifiProvisioner::State::Idle;
int TouchWifiProvisioner::_emptyScanRetries = 0;
std::vector<TouchWifiProvisioner::SavedNet> TouchWifiProvisioner::_candidates;
size_t TouchWifiProvisioner::_candIdx = 0;
uint8_t TouchWifiProvisioner::_maxSaved = 5;
TouchWifiProvisioner::Language TouchWifiProvisioner::_lang = TouchWifiProvisioner::Language::English;
bool TouchWifiProvisioner::_langChosen = false;
bool TouchWifiProvisioner::_cancelable = false;
TouchWifiProvisioner::CancelCallback TouchWifiProvisioner::_onCancel;

TouchWifiProvisioner::ConnectedCallback TouchWifiProvisioner::_onConnected = nullptr;
TouchWifiProvisioner::StatusCallback TouchWifiProvisioner::_onStatus = nullptr;
TouchWifiProvisioner::DisconnectedCallback TouchWifiProvisioner::_onDisconnected = nullptr;

void TouchWifiProvisioner::setOnDisconnected(DisconnectedCallback cb) {
  _onDisconnected = cb;
}

#define TR(s) tr(s, _lang)

void TouchWifiProvisioner::loadLanguage() {
  prefs.begin(UI_NAMESPACE, true);
  if (prefs.isKey("lang")) {
    _lang = prefs.getUChar("lang", 0) == 1 ? Language::Spanish : Language::English;
    _langChosen = true;
  }
  prefs.end();
}

void TouchWifiProvisioner::setLanguage(Language lang) {
  _lang = lang;
  _langChosen = true;
  prefs.begin(UI_NAMESPACE, false);
  prefs.putUChar("lang", lang == Language::Spanish ? 1 : 0);
  prefs.end();
}

TouchWifiProvisioner::Language TouchWifiProvisioner::language() {
  return _lang;
}

// Gives a screen the bundled Latin-1 font (accents, n-tilde, inverted
// punctuation) with the host's own font as fallback for everything else, so
// ASCII text still looks like the host app. Children inherit it.
void TouchWifiProvisioner::applyFont(lv_obj_t *screen) {
#ifndef TWP_NO_LATIN1_FONT
  static lv_font_t font;
  font = twp_font_latin1_14;
  font.fallback = lv_obj_get_style_text_font(screen, LV_PART_MAIN);
  lv_obj_set_style_text_font(screen, &font, 0);
#endif
}

void TouchWifiProvisioner::setReconnectTimeout(unsigned long ms) {
  _reconnectTimeoutMs = ms;
}

std::vector<TouchWifiProvisioner::SavedNet> TouchWifiProvisioner::loadSaved() {
  std::vector<SavedNet> out;
  prefs.begin(PREFS_NAMESPACE, true);
  if (prefs.isKey("n")) {
    int n = prefs.getUChar("n", 0);
    for (int i = 0; i < n && i < MAX_SAVED_HARD; i++) {
      String s = prefs.getString(("s" + String(i)).c_str(), "");
      if (s.isEmpty()) continue;
      out.push_back({s, prefs.getString(("p" + String(i)).c_str(), "")});
    }
  } else {
    // Pre-1.5.0 layout: a single network under "ssid"/"password". It is
    // rewritten in the list format the next time anything is saved.
    String s = prefs.getString("ssid", "");
    if (!s.isEmpty()) out.push_back({s, prefs.getString("password", "")});
  }
  prefs.end();
  return out;
}

void TouchWifiProvisioner::saveSaved(const std::vector<SavedNet> &list) {
  prefs.begin(PREFS_NAMESPACE, false);
  prefs.clear();
  prefs.putUChar("n", (uint8_t)list.size());
  for (size_t i = 0; i < list.size(); i++) {
    prefs.putString(("s" + String((int)i)).c_str(), list[i].ssid);
    prefs.putString(("p" + String((int)i)).c_str(), list[i].password);
  }
  prefs.end();
}

// Moves (or adds) the network to the front of the list. A no-op - no flash
// write - when it is already first with the same password, which is the
// common case on every normal boot.
void TouchWifiProvisioner::rememberNetwork(const String &ssid, const String &password) {
  std::vector<SavedNet> list = loadSaved();
  if (!list.empty() && list[0].ssid == ssid && list[0].password == password) return;
  list.erase(std::remove_if(list.begin(), list.end(),
                            [&](const SavedNet &n) { return n.ssid == ssid; }),
             list.end());
  list.insert(list.begin(), SavedNet{ssid, password});
  while (list.size() > _maxSaved) list.pop_back();
  saveSaved(list);
  Serial.printf("[TouchWifiProvisioner] Saved \"%s\" (%u saved network(s))\n", ssid.c_str(),
                (unsigned)list.size());
}

std::vector<String> TouchWifiProvisioner::savedNetworks() {
  std::vector<String> out;
  for (const auto &n : loadSaved()) out.push_back(n.ssid);
  return out;
}

void TouchWifiProvisioner::forget(const String &ssid) {
  std::vector<SavedNet> list = loadSaved();
  size_t before = list.size();
  list.erase(std::remove_if(list.begin(), list.end(),
                            [&](const SavedNet &n) { return n.ssid == ssid; }),
             list.end());
  if (list.size() != before) {
    saveSaved(list);
    Serial.printf("[TouchWifiProvisioner] Forgot \"%s\"\n", ssid.c_str());
  }
}

void TouchWifiProvisioner::setMaxSavedNetworks(uint8_t n) {
  _maxSaved = n < 1 ? 1 : (n > MAX_SAVED_HARD ? MAX_SAVED_HARD : n);
}

void TouchWifiProvisioner::begin(lv_obj_t *parent, const char *hostname,
                             ConnectedCallback onConnected, StatusCallback onStatus) {
  _parent = parent;
  _hostname = hostname;
  _onConnected = onConnected;
  _onStatus = onStatus;
  loadLanguage();

  // A bare WiFi.mode(WIFI_STA) right at boot can leave the radio in a
  // state where the very first scanNetworks() completes with zero
  // results instead of actually finding anything nearby, on some
  // toolchain/core combos - a full stop/restart cycle reliably brings
  // the radio up scan-ready. Same fix as the post-failed-connect
  // rescan path in loop() below.
  WiFi.mode(WIFI_OFF);
  delay(100);
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(_hostname.c_str());
  Serial.printf("[TouchWifiProvisioner] hostname: %s (try http://%s.local if your network supports mDNS)\n",
                _hostname.c_str(), _hostname.c_str());

  tryStoredCredentials();
}

void TouchWifiProvisioner::tryStoredCredentials() {
  _candidates = loadSaved();
  _candIdx = 0;
  if (_candidates.empty()) {
    // Fresh board: ask for a language first. Boards that already have
    // networks (e.g. upgraded) just default to English - no surprise screen.
    if (_langChosen) showPicker(); else showLanguageSelect();
    return;
  }
  // One saved network: connect straight away like before. Several: scan
  // first so the strongest one in range is tried first.
  if (_candidates.size() == 1) {
    tryNextCandidate();
  } else {
    beginSavedScan();
  }
}

void TouchWifiProvisioner::beginSavedScan() {
  _state = State::ScanningSaved;
  _emptyScanRetries = 0;
  WiFi.scanNetworks(true);
  reportStatus(TR(S_LOOKING_SAVED));
  Serial.printf("[TouchWifiProvisioner] %u saved networks - scanning to pick one\n",
                (unsigned)_candidates.size());
}

// Reorders _candidates: saved networks seen in the scan, strongest first,
// then the ones not seen (possibly hidden) in most-recently-used order.
void TouchWifiProvisioner::orderCandidates(int scanCount) {
  std::vector<std::pair<SavedNet, int>> visible;
  std::vector<SavedNet> unseen;
  for (const auto &n : _candidates) {
    int best = -1000;
    bool seen = false;
    for (int i = 0; i < scanCount; i++) {
      if (WiFi.SSID(i) == n.ssid) {
        seen = true;
        best = std::max(best, (int)WiFi.RSSI(i));
      }
    }
    if (seen) visible.push_back({n, best}); else unseen.push_back(n);
  }
  std::stable_sort(visible.begin(), visible.end(),
                   [](const std::pair<SavedNet, int> &x, const std::pair<SavedNet, int> &y) {
                     return x.second > y.second;
                   });
  _candidates.clear();
  for (const auto &v : visible) _candidates.push_back(v.first);
  for (const auto &u : unseen) _candidates.push_back(u);
  _candIdx = 0;
}

void TouchWifiProvisioner::tryNextCandidate() {
  if (_candIdx >= _candidates.size()) return;
  SavedNet c = _candidates[_candIdx++];
  startConnect(c.ssid, c.password, false);
}

void TouchWifiProvisioner::startConnect(const String &ssid, const String &password, bool fromPicker) {
  _pendingSsid = ssid;
  _pendingPassword = password;
  if (fromPicker) {
    _candidates.clear();
    _candIdx = 0;
  }
  _state = fromPicker ? State::ConnectingFromPicker : State::ConnectingStored;
  _connectStartMs = millis();
  reportStatus(String(TR(S_CONNECTING_TO)) + ssid + "...");
  Serial.printf("[TouchWifiProvisioner] Connecting to \"%s\"%s\n", ssid.c_str(),
                fromPicker ? " (from picker)" : " (stored credentials)");
  WiFi.begin(ssid.c_str(), password.c_str());
}

void TouchWifiProvisioner::loop() {
  switch (_state) {
    case State::ConnectingStored:
    case State::ConnectingFromPicker: {
      wl_status_t st = WiFi.status();
      if (st == WL_CONNECTED) {
        rememberNetwork(_pendingSsid, _pendingPassword);
        _candidates.clear();
        _candIdx = 0;
        _cancelable = false;
        _onCancel = nullptr;
        teardownUi();
        _state = State::Connected;
        reportStatus(String(TR(S_CONNECTED)) + WiFi.localIP().toString());
        Serial.printf("[TouchWifiProvisioner] Connected: SSID=\"%s\" IP=%s RSSI=%d dBm\n",
                      WiFi.SSID().c_str(), WiFi.localIP().toString().c_str(), WiFi.RSSI());
        if (_onConnected) _onConnected(WiFi.localIP().toString());
      } else {
        unsigned long elapsed = millis() - _connectStartMs;
        bool definiteFailure = elapsed > FAIL_FAST_GRACE_MS &&
                               (st == WL_CONNECT_FAILED || st == WL_NO_SSID_AVAIL);
        if (definiteFailure || elapsed > CONNECT_TIMEOUT_MS) {
          resetRadio();
          if (_state == State::ConnectingStored && _candIdx < _candidates.size()) {
            Serial.printf("[TouchWifiProvisioner] \"%s\" failed - trying next saved network\n",
                          _pendingSsid.c_str());
            tryNextCandidate();
            break;
          }
          bool savedPw = false;
          if (st == WL_CONNECT_FAILED) {
            for (const auto &n : loadSaved()) {
              if (n.ssid == _pendingSsid) { savedPw = true; break; }
            }
          }
          bool anyMode = _state == State::ConnectingStored && _candidates.size() > 1;
          // Built per language so Serial stays English while the screen and
          // the onStatus callback follow the UI language.
          auto makeWhy = [&](Language lg) {
            String w;
            if (anyMode) {
              w = tr(S_NO_SAVED_WORKED, lg);
            } else if (st == WL_NO_SSID_AVAIL) {
              w = String(tr(S_NOT_FOUND, lg)) + _pendingSsid;
            } else if (st == WL_CONNECT_FAILED) {
              w = String(tr(S_COULDNT_CONNECT, lg)) + _pendingSsid + tr(S_WRONG_PASSWORD, lg);
              if (savedPw) w += tr(S_HOLD_FORGET_HINT, lg);
            } else {
              w = String(tr(S_COULDNT_CONNECT, lg)) + _pendingSsid;
            }
            return w;
          };
          String why = makeWhy(_lang);
          showPicker();
          // After showPicker so it lands on the fresh status label (and
          // stays up during the scan) instead of being overwritten by it.
          reportStatus(why);
          Serial.println("[TouchWifiProvisioner] " + makeWhy(Language::English));
        }
      }
      break;
    }
    case State::Connected: {
      if (WiFi.status() != WL_CONNECTED) {
        _state = State::WaitingReconnect;
        _connectStartMs = millis();
        _outageStartMs = _connectStartMs;
        reportStatus(TR(S_LOST));
        Serial.println("[TouchWifiProvisioner] Wi-Fi connection lost - reconnecting...");
        if (_onDisconnected) _onDisconnected();
      }
      break;
    }
    case State::WaitingReconnect: {
      // Deliberately UI-less: a transient outage must not tear down the
      // host app's screen. Auto-reconnect does the work; we just nudge it
      // if it seems to have given up.
      if (WiFi.status() == WL_CONNECTED) {
        _state = State::Connected;
        reportStatus(String(TR(S_RECONNECTED)) + WiFi.localIP().toString());
        Serial.printf("[TouchWifiProvisioner] Reconnected: SSID=\"%s\" IP=%s RSSI=%d dBm\n",
                      WiFi.SSID().c_str(), WiFi.localIP().toString().c_str(), WiFi.RSSI());
        if (_onConnected) _onConnected(WiFi.localIP().toString());
      } else if (_reconnectTimeoutMs > 0 &&
                 millis() - _outageStartMs > _reconnectTimeoutMs) {
        // Opt-in give-up (setReconnectTimeout): try every saved network
        // (the board may have moved), then hand control back to the user.
        // Saved credentials stay until forgotten.
        Serial.println("[TouchWifiProvisioner] Reconnect timed out - trying saved networks");
        resetRadio();
        _candidates = loadSaved();
        _candIdx = 0;
        if (_candidates.empty()) {
          showPicker();
          reportStatus(String(TR(S_COULDNT_RECONNECT)) + _pendingSsid);
        } else if (_candidates.size() == 1) {
          tryNextCandidate();
        } else {
          beginSavedScan();
        }
      } else if (millis() - _connectStartMs > CONNECT_TIMEOUT_MS) {
        Serial.println("[TouchWifiProvisioner] Still not reconnected, nudging WiFi.reconnect()...");
        WiFi.reconnect();
        _connectStartMs = millis();
      }
      break;
    }
    case State::ScanningSaved: {
      int found = WiFi.scanComplete();
      if (found >= 0 && found == 0 && _emptyScanRetries < MAX_EMPTY_SCAN_RETRIES) {
        _emptyScanRetries++;
        WiFi.scanDelete();
        WiFi.scanNetworks(true);
      } else if (found >= 0) {
        orderCandidates(found);
        WiFi.scanDelete();
        tryNextCandidate();
      } else if (found == WIFI_SCAN_FAILED) {
        if (_emptyScanRetries < MAX_EMPTY_SCAN_RETRIES) {
          _emptyScanRetries++;
          WiFi.scanNetworks(true);
        } else {
          // Can't scan at all - just probe the saved networks in order.
          orderCandidates(0);
          tryNextCandidate();
        }
      }
      break;
    }
    case State::Scanning: {
      int found = WiFi.scanComplete();
      if (found >= 0) {
        populateNetworkList();
      } else if (found == WIFI_SCAN_FAILED) {
        reportStatus(TR(S_SCAN_FAILED));
        WiFi.scanNetworks(true);
      }
      break;
    }
    default:
      break;
  }
}

// A plain WiFi.disconnect() isn't enough after a failed WiFi.begin() (wrong
// password especially) - the ESP32 WiFi driver can be left in a state where
// scanNetworks() "completes" instantly with zero results instead of actually
// failing, so the retry-on-WIFI_SCAN_FAILED logic in the Scanning state never
// kicks in and the picker looks permanently empty until reboot. A full
// stop/restart of the radio is the reliable fix. The short delay is a
// deliberate one-time exception to this library's non-blocking design - it
// only runs once, right after a failed connection, not on any hot path.
void TouchWifiProvisioner::resetRadio() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(100);
  WiFi.mode(WIFI_STA);
}

void TouchWifiProvisioner::reset() {
  prefs.begin(PREFS_NAMESPACE, false);
  prefs.clear();
  prefs.end();
  WiFi.disconnect(true);
  showPicker();
}

void TouchWifiProvisioner::openPicker(CancelCallback onCancel) {
  _onCancel = onCancel;
  _cancelable = true;
  showPicker();
}

void TouchWifiProvisioner::addCancelRow() {
  if (!_cancelable || !_list) return;
  lv_obj_t *cancel = lv_list_add_btn(_list, LV_SYMBOL_CLOSE, TR(S_CANCEL));
  styleListRow(cancel, /*accent=*/true);
  lv_obj_add_event_cb(cancel, onCancelClicked, LV_EVENT_CLICKED, nullptr);
}

void TouchWifiProvisioner::onCancelClicked(lv_event_t *e) {
  WiFi.scanDelete();
  teardownUi();
  _cancelable = false;
  // Back to whatever the connection was doing before the picker opened.
  _state = (WiFi.status() == WL_CONNECTED) ? State::Connected : State::WaitingReconnect;
  _connectStartMs = millis();
  _outageStartMs = _connectStartMs;
  Serial.println("[TouchWifiProvisioner] Picker cancelled");
  CancelCallback cb = _onCancel;
  _onCancel = nullptr;
  if (cb) cb();
}

bool TouchWifiProvisioner::isConnected() {
  return _state == State::Connected && WiFi.status() == WL_CONNECTED;
}

void TouchWifiProvisioner::showPicker() {
  teardownUi();
  _state = State::Scanning;
  _emptyScanRetries = 0;

  _screen = lv_obj_create(_parent);
  lv_obj_set_size(_screen, lv_pct(100), lv_pct(100));
  lv_obj_center(_screen);
  lv_obj_set_flex_flow(_screen, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(_screen, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_all(_screen, 12, 0);
  styleScreen(_screen);
  applyFont(_screen);

  lv_obj_t *title = lv_label_create(_screen);
  lv_label_set_text(title, (String(LV_SYMBOL_WIFI "  ") + TR(S_SELECT_NETWORK)).c_str());
  lv_obj_set_style_text_color(title, COLOR_TEXT, 0);

  _statusLabel = lv_label_create(_screen);
  lv_label_set_text(_statusLabel, TR(S_SCANNING));
  lv_obj_set_style_text_color(_statusLabel, COLOR_TEXT_MUTED, 0);

  _list = lv_list_create(_screen);
  lv_obj_set_size(_list, lv_pct(90), lv_pct(75));
  lv_obj_set_flex_grow(_list, 1);
  lv_obj_set_style_bg_color(_list, COLOR_CARD, 0);
  lv_obj_set_style_bg_opa(_list, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(_list, 0, 0);
  lv_obj_set_style_radius(_list, 14, 0);
  lv_obj_set_style_pad_all(_list, 4, 0);

  addCancelRow();
  lv_obj_t *rescan = lv_list_add_btn(_list, LV_SYMBOL_REFRESH, TR(S_RESCAN));
  styleListRow(rescan, /*accent=*/true);
  lv_obj_add_event_cb(rescan, onRescanClicked, LV_EVENT_CLICKED, nullptr);

  WiFi.scanNetworks(true);
  reportStatus(TR(S_SCANNING_NETWORKS));
  Serial.println("[TouchWifiProvisioner] Scanning for networks...");
}

void TouchWifiProvisioner::populateNetworkList() {
  int count = WiFi.scanComplete();

  // A scan can "complete" with zero results even though real networks are
  // in range, if the radio wasn't fully settled when it started (see the
  // WiFi.mode(WIFI_OFF)/WIFI_STA reset in begin()). Silently retry a few
  // times before actually telling the user nothing was found.
  if (count == 0 && _emptyScanRetries < MAX_EMPTY_SCAN_RETRIES) {
    _emptyScanRetries++;
    Serial.printf("[TouchWifiProvisioner] Scan returned 0 networks, retrying (%d/%d)...\n",
                  _emptyScanRetries, MAX_EMPTY_SCAN_RETRIES);
    WiFi.scanDelete();
    WiFi.scanNetworks(true);
    return;
  }

  Serial.printf("[TouchWifiProvisioner] Scan complete: %d network(s) found\n", count);
  lv_obj_clean(_list);

  addCancelRow();
  lv_obj_t *rescan = lv_list_add_btn(_list, LV_SYMBOL_REFRESH, TR(S_RESCAN));
  styleListRow(rescan, /*accent=*/true);
  lv_obj_add_event_cb(rescan, onRescanClicked, LV_EVENT_CLICKED, nullptr);

  lv_obj_t *hidden = lv_list_add_btn(_list, LV_SYMBOL_EDIT, TR(S_HIDDEN_NETWORK));
  styleListRow(hidden, /*accent=*/true);
  lv_obj_add_event_cb(hidden, onHiddenClicked, LV_EVENT_CLICKED, nullptr);

  lv_obj_t *langRow = lv_list_add_btn(_list, LV_SYMBOL_SETTINGS, TR(S_OTHER_LANGUAGE));
  styleListRow(langRow, /*accent=*/true);
  lv_obj_add_event_cb(langRow, onLanguageToggleClicked, LV_EVENT_CLICKED, nullptr);

  std::vector<SavedNet> saved = loadSaved();
  bool anySaved = false;
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
    bool isSaved = false;
    for (const auto &n : saved) {
      if (n.ssid == ssid) { isSaved = true; break; }
    }
    anySaved = anySaved || isSaved;
    Serial.printf("[TouchWifiProvisioner]   \"%s\" RSSI=%d dBm%s%s\n", ssid.c_str(),
                  WiFi.RSSI(i), secure ? "" : " (open)", isSaved ? " (saved)" : "");
    lv_obj_t *btn = lv_list_add_btn(_list, LV_SYMBOL_WIFI, ssid.c_str());
    lv_obj_set_user_data(btn, (void *)(intptr_t)((secure ? 1 : 0) | (isSaved ? 2 : 0)));
    styleListRow(btn, /*accent=*/false);
    // SHORT_CLICKED (not CLICKED) so a long-press to forget doesn't also
    // count as a tap.
    lv_obj_add_event_cb(btn, onListButtonClicked, LV_EVENT_SHORT_CLICKED, nullptr);
    lv_obj_add_event_cb(btn, onListButtonLongPressed, LV_EVENT_LONG_PRESSED, nullptr);
    if (!secure || isSaved) {
      // LVGL's built-in symbol font has no padlock glyph, so tag the rare
      // open networks (and the saved ones) instead of decorating every
      // secured one. Added as a separate label - lv_list_get_btn_text()
      // returns the first label (the SSID), so this can't leak into the
      // SSID we connect to.
      lv_obj_t *tag = lv_label_create(btn);
      lv_label_set_text(tag, TR(!secure && isSaved ? S_TAG_OPEN_SAVED : (isSaved ? S_TAG_SAVED : S_TAG_OPEN)));
      lv_obj_set_style_text_color(tag, COLOR_TEXT_MUTED, 0);
    }
  }

  WiFi.scanDelete();
  _state = State::Picker;
  reportStatus(TR(seen.empty() ? S_NO_NETWORKS
                  : anySaved   ? S_TAP_CONNECT_HOLD
                               : S_TAP_CONNECT));
  if (seen.empty()) Serial.println("[TouchWifiProvisioner] No networks found");
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
  lv_label_set_text_fmt(title, LV_SYMBOL_WIFI "  %s%s", TR(S_PASSWORD_FOR), ssid.c_str());
  lv_obj_set_style_text_color(title, COLOR_TEXT, 0);

  _passwordTa = lv_textarea_create(_screen);
  lv_textarea_set_password_mode(_passwordTa, true);
  lv_textarea_set_one_line(_passwordTa, true);
  lv_textarea_set_max_length(_passwordTa, 63); // WPA2 passphrase limit
  lv_textarea_set_placeholder_text(_passwordTa, TR(S_PASSWORD));
  lv_obj_set_width(_passwordTa, lv_pct(90));
  lv_obj_set_style_bg_color(_passwordTa, COLOR_CARD, 0);
  lv_obj_set_style_bg_opa(_passwordTa, LV_OPA_COVER, 0);
  lv_obj_set_style_text_color(_passwordTa, COLOR_TEXT, 0);
  lv_obj_set_style_border_color(_passwordTa, COLOR_ACCENT, 0);
  lv_obj_set_style_border_width(_passwordTa, 1, 0);
  lv_obj_set_style_radius(_passwordTa, 8, 0);

  lv_obj_t *showPasswordCb = lv_checkbox_create(_screen);
  lv_checkbox_set_text(showPasswordCb, TR(S_SHOW_PASSWORD));
  lv_obj_set_style_text_color(showPasswordCb, COLOR_TEXT_MUTED, 0);
  lv_obj_set_style_bg_color(showPasswordCb, COLOR_ACCENT, LV_PART_INDICATOR | LV_STATE_CHECKED);
  lv_obj_add_event_cb(showPasswordCb, onShowPasswordToggled, LV_EVENT_VALUE_CHANGED, nullptr);

  // This screen gets its own status label so "Connecting..." (and any
  // failure that follows) is visible here too - without it, tapping
  // Connect left the user staring at a frozen password screen for the
  // whole connect attempt.
  _statusLabel = lv_label_create(_screen);
  lv_label_set_text(_statusLabel, "");
  lv_obj_set_style_text_color(_statusLabel, COLOR_TEXT_MUTED, 0);

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
  lv_label_set_text(backLabel, (String(LV_SYMBOL_LEFT " ") + TR(S_BACK)).c_str());

  lv_obj_t *connectBtn = lv_btn_create(btnRow);
  styleActionButton(connectBtn, /*primary=*/true);
  lv_obj_add_event_cb(connectBtn, onConnectClicked, LV_EVENT_CLICKED, nullptr);
  lv_obj_t *connectLabel = lv_label_create(connectBtn);
  lv_label_set_text(connectLabel, (String(TR(S_CONNECT)) + " " LV_SYMBOL_OK).c_str());

  _keyboard = lv_keyboard_create(_screen);
  lv_keyboard_set_textarea(_keyboard, _passwordTa);
  lv_obj_set_size(_keyboard, lv_pct(100), lv_pct(45));
  // The keyboard's own checkmark/close keys should behave like the Connect
  // and Back buttons - people press those expecting exactly that.
  lv_obj_add_event_cb(_keyboard, onConnectClicked, LV_EVENT_READY, nullptr);
  lv_obj_add_event_cb(_keyboard, onBackClicked, LV_EVENT_CANCEL, nullptr);
}

void TouchWifiProvisioner::teardownUi() {
  if (_screen) {
    lv_obj_del(_screen);
    _screen = nullptr;
  }
  _list = nullptr;
  _statusLabel = nullptr;
  _passwordTa = nullptr;
  _ssidTa = nullptr;
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

  intptr_t flags = (intptr_t)lv_obj_get_user_data(btn);
  bool secure = flags & 1;
  if (flags & 2) {
    // Saved network: reuse the stored password, no keyboard needed.
    String ssid(text);
    for (const auto &n : loadSaved()) {
      if (n.ssid == ssid) {
        startConnect(ssid, n.password, true);
        return;
      }
    }
  }
  if (secure) {
    showPasswordEntry(String(text));
  } else {
    // Open network - no password to collect, connect straight away.
    startConnect(String(text), "", true);
  }
}

void TouchWifiProvisioner::onListButtonLongPressed(lv_event_t *e) {
  lv_obj_t *btn = static_cast<lv_obj_t *>(lv_event_get_target(e));
  intptr_t flags = (intptr_t)lv_obj_get_user_data(btn);
  if (!(flags & 2)) return;
  const char *text = lv_list_get_btn_text(_list, btn);
  if (!text) return;

  String ssid(text);
  forget(ssid);
  lv_obj_set_user_data(btn, (void *)(intptr_t)(flags & ~2));
  if (lv_obj_get_child_cnt(btn) > 2) {
    lv_obj_t *tag = lv_obj_get_child(btn, 2);
    if (flags & 1) lv_obj_del(tag); else lv_label_set_text(tag, TR(S_TAG_OPEN));
  }
  reportStatus(String(TR(S_FORGOT)) + ssid);
}

void TouchWifiProvisioner::onRescanClicked(lv_event_t *e) {
  lv_obj_clean(_list);
  _emptyScanRetries = 0;
  WiFi.scanNetworks(true);
  _state = State::Scanning;
  reportStatus(TR(S_SCANNING_NETWORKS));
  Serial.println("[TouchWifiProvisioner] Rescan requested");
}

void TouchWifiProvisioner::onConnectClicked(lv_event_t *e) {
  String password = _passwordTa ? lv_textarea_get_text(_passwordTa) : "";
  startConnect(_pendingSsid, password, true);
}

void TouchWifiProvisioner::onBackClicked(lv_event_t *e) {
  showPicker();
}

void TouchWifiProvisioner::showLanguageSelect() {
  teardownUi();
  _state = State::Idle;

  _screen = lv_obj_create(_parent);
  lv_obj_set_size(_screen, lv_pct(100), lv_pct(100));
  lv_obj_center(_screen);
  lv_obj_set_flex_flow(_screen, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(_screen, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_all(_screen, 12, 0);
  styleScreen(_screen);
  applyFont(_screen);

  lv_obj_t *title = lv_label_create(_screen);
  lv_label_set_text(title, LV_SYMBOL_SETTINGS "  Language / Idioma");
  lv_obj_set_style_text_color(title, COLOR_TEXT, 0);

  const char *names[2] = {"English", "Español"};
  for (int i = 0; i < 2; i++) {
    lv_obj_t *btn = lv_btn_create(_screen);
    styleActionButton(btn, /*primary=*/true);
    lv_obj_set_size(btn, lv_pct(70), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_ver(btn, 14, 0);
    lv_obj_set_user_data(btn, (void *)(intptr_t)i);
    lv_obj_add_event_cb(btn, onLanguageChosen, LV_EVENT_CLICKED, nullptr);
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, names[i]);
    lv_obj_center(label);
  }
  Serial.println("[TouchWifiProvisioner] Asking for language (English / Espanol)");
}

void TouchWifiProvisioner::onLanguageChosen(lv_event_t *e) {
  lv_obj_t *btn = static_cast<lv_obj_t *>(lv_event_get_target(e));
  setLanguage((intptr_t)lv_obj_get_user_data(btn) == 1 ? Language::Spanish : Language::English);
  Serial.printf("[TouchWifiProvisioner] Language: %s\n",
                _lang == Language::Spanish ? "Spanish" : "English");
  showPicker();
}

void TouchWifiProvisioner::onLanguageToggleClicked(lv_event_t *e) {
  setLanguage(_lang == Language::Spanish ? Language::English : Language::Spanish);
  Serial.printf("[TouchWifiProvisioner] Language: %s\n",
                _lang == Language::Spanish ? "Spanish" : "English");
  showPicker();
}

void TouchWifiProvisioner::showHiddenSsidEntry() {
  lv_obj_clean(_screen);
  _statusLabel = nullptr;
  _list = nullptr;
  _passwordTa = nullptr;

  lv_obj_t *title = lv_label_create(_screen);
  lv_label_set_text(title, (String(LV_SYMBOL_WIFI "  ") + TR(S_HIDDEN_TITLE)).c_str());
  lv_obj_set_style_text_color(title, COLOR_TEXT, 0);

  _ssidTa = lv_textarea_create(_screen);
  lv_textarea_set_one_line(_ssidTa, true);
  lv_textarea_set_max_length(_ssidTa, 32); // SSID length limit
  lv_textarea_set_placeholder_text(_ssidTa, TR(S_SSID_PLACEHOLDER));
  lv_obj_set_width(_ssidTa, lv_pct(90));
  lv_obj_set_style_bg_color(_ssidTa, COLOR_CARD, 0);
  lv_obj_set_style_bg_opa(_ssidTa, LV_OPA_COVER, 0);
  lv_obj_set_style_text_color(_ssidTa, COLOR_TEXT, 0);
  lv_obj_set_style_border_color(_ssidTa, COLOR_ACCENT, 0);
  lv_obj_set_style_border_width(_ssidTa, 1, 0);
  lv_obj_set_style_radius(_ssidTa, 8, 0);

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
  lv_label_set_text(backLabel, (String(LV_SYMBOL_LEFT " ") + TR(S_BACK)).c_str());

  lv_obj_t *nextBtn = lv_btn_create(btnRow);
  styleActionButton(nextBtn, /*primary=*/true);
  lv_obj_add_event_cb(nextBtn, onHiddenNextClicked, LV_EVENT_CLICKED, nullptr);
  lv_obj_t *nextLabel = lv_label_create(nextBtn);
  lv_label_set_text(nextLabel, (String(TR(S_NEXT)) + " " LV_SYMBOL_RIGHT).c_str());

  _keyboard = lv_keyboard_create(_screen);
  lv_keyboard_set_textarea(_keyboard, _ssidTa);
  lv_obj_set_size(_keyboard, lv_pct(100), lv_pct(45));
  lv_obj_add_event_cb(_keyboard, onHiddenNextClicked, LV_EVENT_READY, nullptr);
  lv_obj_add_event_cb(_keyboard, onBackClicked, LV_EVENT_CANCEL, nullptr);
}

void TouchWifiProvisioner::onHiddenClicked(lv_event_t *e) {
  showHiddenSsidEntry();
}

void TouchWifiProvisioner::onHiddenNextClicked(lv_event_t *e) {
  String ssid = _ssidTa ? lv_textarea_get_text(_ssidTa) : "";
  if (ssid.isEmpty()) return;
  // A hidden network's security type is unknown (it isn't in the scan), so
  // always go through the password screen; leaving it blank connects as open.
  showPasswordEntry(ssid);
}

void TouchWifiProvisioner::onShowPasswordToggled(lv_event_t *e) {
  lv_obj_t *cb = static_cast<lv_obj_t *>(lv_event_get_target(e));
  bool show = lv_obj_has_state(cb, LV_STATE_CHECKED);
  if (_passwordTa) lv_textarea_set_password_mode(_passwordTa, !show);
}
