#include "ClockFace.h"

#include <Arduino.h>
#include <cstdio>
#include <ezTime.h>
#include "RollingDigit.h"
#include "SettingsPanel.h"

extern Timezone myTZ;

namespace ClockFace {
namespace {

bool show24Hour = false;
bool usDate = true;
int prevDay = -1;

lv_obj_t *screenParent = nullptr;
RollingDigit *digits[6] = {nullptr};
lv_obj_t *ampmLabel = nullptr;
lv_obj_t *weekdayLabel = nullptr;
lv_obj_t *dateLabel = nullptr;

const lv_font_t *TIME_FONT = &lv_font_montserrat_48;
const lv_font_t *SUB_FONT = &lv_font_montserrat_24;
const lv_font_t *AMPM_FONT = &lv_font_montserrat_16;

lv_obj_t *makeColon(lv_obj_t *parent, lv_color_t color) {
  lv_obj_t *label = lv_label_create(parent);
  lv_obj_set_style_text_font(label, TIME_FONT, 0);
  lv_obj_set_style_text_color(label, color, 0);
  lv_label_set_text(label, ":");
  return label;
}

void onGearClicked(lv_event_t *e) { SettingsPanel::open(screenParent); }

void refresh(bool animate) {
  time_t local = myTZ.now();

  int hh = show24Hour ? hour(local) : hourFormat12(local);
  digits[0]->setValue(hh / 10, animate);
  digits[1]->setValue(hh % 10, animate);
  digits[2]->setValue(minute(local) / 10, animate);
  digits[3]->setValue(minute(local) % 10, animate);
  digits[4]->setValue(second(local) / 10, animate);
  digits[5]->setValue(second(local) % 10, animate);

  if (!show24Hour) {
    lv_label_set_text(ampmLabel, isPM(local) ? "PM" : "AM");
  }

  int dd = day(local);
  if (dd != prevDay) {
    prevDay = dd;

    char buffer[16];
    if (usDate) {
      snprintf(buffer, sizeof(buffer), "%02d/%02d/%d", month(local), dd, year(local));
    } else {
      snprintf(buffer, sizeof(buffer), "%02d/%02d/%d", dd, month(local), year(local));
    }
    lv_label_set_text(dateLabel, buffer);

    static const char *dayNames[] = {"",        "Sunday",   "Monday", "Tuesday", "Wednesday",
                                      "Thursday", "Friday",  "Saturday"};
    lv_label_set_text(weekdayLabel, dayNames[weekday(local)]);
  }
}

} // namespace

void setup(lv_obj_t *parent, bool is24Hour, bool usDateFormat) {
  show24Hour = is24Hour;
  usDate = usDateFormat;
  prevDay = -1;
  screenParent = parent;

  lv_obj_set_style_bg_color(parent, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
  lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  lv_color_t timeColor = lv_palette_main(LV_PALETTE_YELLOW);

  lv_obj_t *timeRow = lv_obj_create(parent);
  lv_obj_remove_style_all(timeRow);
  lv_obj_set_size(timeRow, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_clear_flag(timeRow, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_flex_flow(timeRow, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(timeRow, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  digits[0] = new RollingDigit(timeRow, TIME_FONT, timeColor);
  digits[1] = new RollingDigit(timeRow, TIME_FONT, timeColor);
  makeColon(timeRow, timeColor);
  digits[2] = new RollingDigit(timeRow, TIME_FONT, timeColor);
  digits[3] = new RollingDigit(timeRow, TIME_FONT, timeColor);
  makeColon(timeRow, timeColor);
  digits[4] = new RollingDigit(timeRow, TIME_FONT, timeColor);
  digits[5] = new RollingDigit(timeRow, TIME_FONT, timeColor);

  // Always created (so setIs24Hour() can toggle it live) - just hidden when
  // starting in 24-hour mode.
  ampmLabel = lv_label_create(timeRow);
  lv_obj_set_style_text_font(ampmLabel, AMPM_FONT, 0);
  lv_obj_set_style_text_color(ampmLabel, timeColor, 0);
  lv_obj_set_style_pad_left(ampmLabel, 8, 0);
  lv_label_set_text(ampmLabel, "AM");
  if (show24Hour) {
    lv_obj_add_flag(ampmLabel, LV_OBJ_FLAG_HIDDEN);
  }

  lv_obj_t *gearBtn = lv_btn_create(parent);
  lv_obj_add_flag(gearBtn, LV_OBJ_FLAG_IGNORE_LAYOUT);
  lv_obj_set_size(gearBtn, 36, 36);
  lv_obj_align(gearBtn, LV_ALIGN_TOP_RIGHT, -4, 4);
  lv_obj_set_style_bg_opa(gearBtn, LV_OPA_30, 0);
  lv_obj_set_style_shadow_width(gearBtn, 0, 0);
  lv_obj_add_event_cb(gearBtn, onGearClicked, LV_EVENT_CLICKED, nullptr);
  lv_obj_t *gearLabel = lv_label_create(gearBtn);
  lv_label_set_text(gearLabel, LV_SYMBOL_SETTINGS);
  lv_obj_center(gearLabel);

  weekdayLabel = lv_label_create(parent);
  lv_obj_set_style_text_font(weekdayLabel, SUB_FONT, 0);
  lv_obj_set_style_text_color(weekdayLabel, lv_color_white(), 0);
  lv_obj_set_style_pad_top(weekdayLabel, 16, 0);
  lv_label_set_text(weekdayLabel, "");

  dateLabel = lv_label_create(parent);
  lv_obj_set_style_text_font(dateLabel, SUB_FONT, 0);
  lv_obj_set_style_text_color(dateLabel, lv_color_white(), 0);
  lv_label_set_text(dateLabel, "");

  refresh(false);
}

void update() { refresh(true); }

void setIs24Hour(bool is24Hour) {
  show24Hour = is24Hour;
  if (is24Hour) {
    lv_obj_add_flag(ampmLabel, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_clear_flag(ampmLabel, LV_OBJ_FLAG_HIDDEN);
  }
  refresh(false);
}

} // namespace ClockFace
