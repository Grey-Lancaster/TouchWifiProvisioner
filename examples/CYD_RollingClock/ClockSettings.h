// Persisted user-editable clock settings (12/24-hour format, timezone),
// backed by NVS via Preferences - same storage mechanism TouchWifiProvisioner
// already uses for saved WiFi credentials.
#pragma once

#include <Arduino.h>

namespace ClockSettings {

// Loads saved settings, falling back to the given defaults on first boot.
void begin(bool defaultIs24Hour, const String &defaultTimezone);

bool is24Hour();
void setIs24Hour(bool value);

// IANA location string (e.g. "America/New_York") passed to ezTime's
// Timezone::setLocation().
String timezone();
void setTimezone(const String &location);

} // namespace ClockSettings
