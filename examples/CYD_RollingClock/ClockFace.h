// LVGL rolling-digit clock face. Replaces the old TFT_eSprite-based
// RollingClockLogic.h/Digit.h/cpp with real LVGL widgets (RollingDigit +
// lv_label), laid out with flex containers instead of hand-computed pixel
// offsets.
#pragma once

#include <lvgl.h>

namespace ClockFace {

// Builds the clock UI inside `parent` (typically lv_scr_act()) and draws the
// current time once, without animating. Call once after NTP sync completes.
void setup(lv_obj_t *parent, bool is24Hour, bool usDateFormat);

// Re-reads the current time and animates any digits that changed. Call once
// per second (e.g. from ezTime's secondChanged()).
void update();

// Switches between 12-hour (with AM/PM) and 24-hour display live, without
// rebuilding the digit widgets. Redraws immediately (no roll animation).
void setIs24Hour(bool is24Hour);

} // namespace ClockFace
