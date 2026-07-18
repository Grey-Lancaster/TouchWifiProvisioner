// Full-screen settings overlay: 24-hour toggle + timezone picker. Opened by
// the gear button on the clock face, applies changes live via ClockFace and
// ezTime, and persists them via ClockSettings.
#pragma once

#include <lvgl.h>

namespace SettingsPanel {

// Builds the overlay as a child of `parent` (drawn on top since created
// last) and shows it. Deletes itself when the user taps Save or Cancel.
void open(lv_obj_t *parent);

} // namespace SettingsPanel
