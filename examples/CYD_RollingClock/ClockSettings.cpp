#include "ClockSettings.h"

#include <Preferences.h>

namespace ClockSettings {
namespace {

const char *PREFS_NAMESPACE = "clocksettings";

Preferences prefs;
bool cachedIs24Hour = false;
String cachedTimezone;

} // namespace

void begin(bool defaultIs24Hour, const String &defaultTimezone) {
  prefs.begin(PREFS_NAMESPACE, false);
  cachedIs24Hour = prefs.getBool("is24h", defaultIs24Hour);
  cachedTimezone = prefs.getString("tz", defaultTimezone);
  prefs.end();
}

bool is24Hour() { return cachedIs24Hour; }

void setIs24Hour(bool value) {
  cachedIs24Hour = value;
  prefs.begin(PREFS_NAMESPACE, false);
  prefs.putBool("is24h", value);
  prefs.end();
}

String timezone() { return cachedTimezone; }

void setTimezone(const String &location) {
  cachedTimezone = location;
  prefs.begin(PREFS_NAMESPACE, false);
  prefs.putString("tz", location);
  prefs.end();
}

} // namespace ClockSettings
