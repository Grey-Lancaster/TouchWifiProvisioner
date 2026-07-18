#include "SettingsPanel.h"

#include <Arduino.h>
#include <cstring>
#include <ezTime.h>
#include "ClockFace.h"
#include "ClockSettings.h"

extern Timezone myTZ;

namespace SettingsPanel {
namespace {

struct TzOption {
  const char *label;
  const char *location;
};

// A curated subset of IANA zones rather than the full ~400-entry tz
// database - keeps the dropdown usable on a 320x240 touchscreen while still
// covering every populated continent, not just the US.
const TzOption TZ_OPTIONS[] = {
    {"Hawaii (US)", "Pacific/Honolulu"},
    {"Alaska (US)", "America/Anchorage"},
    {"Pacific (US)", "America/Los_Angeles"},
    {"Mountain (US)", "America/Denver"},
    {"Central (US)", "America/Chicago"},
    {"Eastern (US)", "America/New_York"},
    {"Atlantic Canada", "America/Halifax"},
    {"Mexico City", "America/Mexico_City"},
    {"Bogota", "America/Bogota"},
    {"Sao Paulo (Brazil)", "America/Sao_Paulo"},
    {"Buenos Aires", "America/Argentina/Buenos_Aires"},
    {"UTC", "UTC"},
    {"London (UK)", "Europe/London"},
    {"Western Europe", "Europe/Madrid"},
    {"Central Europe", "Europe/Berlin"},
    {"Eastern Europe", "Europe/Athens"},
    {"Moscow", "Europe/Moscow"},
    {"Istanbul", "Europe/Istanbul"},
    {"Cairo", "Africa/Cairo"},
    {"Johannesburg", "Africa/Johannesburg"},
    {"Lagos", "Africa/Lagos"},
    {"Dubai", "Asia/Dubai"},
    {"Tel Aviv", "Asia/Jerusalem"},
    {"Karachi", "Asia/Karachi"},
    {"India", "Asia/Kolkata"},
    {"Dhaka", "Asia/Dhaka"},
    {"Bangkok", "Asia/Bangkok"},
    {"Singapore", "Asia/Singapore"},
    {"Hong Kong", "Asia/Hong_Kong"},
    {"China", "Asia/Shanghai"},
    {"Japan", "Asia/Tokyo"},
    {"Korea", "Asia/Seoul"},
    {"Sydney (Australia)", "Australia/Sydney"},
    {"Perth (Australia)", "Australia/Perth"},
    {"Auckland (NZ)", "Pacific/Auckland"},
};
constexpr int TZ_OPTION_COUNT = sizeof(TZ_OPTIONS) / sizeof(TZ_OPTIONS[0]);

lv_obj_t *panel = nullptr;
lv_obj_t *hourSwitch = nullptr;
lv_obj_t *tzDropdown = nullptr;

int indexForCurrentTimezone() {
  String current = ClockSettings::timezone();
  for (int i = 0; i < TZ_OPTION_COUNT; i++) {
    if (strcmp(current.c_str(), TZ_OPTIONS[i].location) == 0) return i;
  }
  return 0;
}

void close() {
  if (panel) {
    lv_obj_del(panel);
    panel = nullptr;
    hourSwitch = nullptr;
    tzDropdown = nullptr;
  }
}

void onSaveClicked(lv_event_t *e) {
  bool is24Hour = lv_obj_has_state(hourSwitch, LV_STATE_CHECKED);
  uint16_t idx = lv_dropdown_get_selected(tzDropdown);
  if (idx >= TZ_OPTION_COUNT) idx = 0;
  const char *location = TZ_OPTIONS[idx].location;

  ClockSettings::setIs24Hour(is24Hour);
  ClockSettings::setTimezone(location);
  myTZ.setLocation(location);
  ClockFace::setIs24Hour(is24Hour);

  close();
}

void onCancelClicked(lv_event_t *e) { close(); }

lv_obj_t *makeRow(lv_obj_t *parent, int topPad) {
  lv_obj_t *row = lv_obj_create(parent);
  lv_obj_remove_style_all(row);
  lv_obj_set_size(row, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_top(row, topPad, 0);
  return row;
}

} // namespace

void open(lv_obj_t *parent) {
  if (panel) return; // already open

  // Full screen (not a smaller centered card) so none of the clock face
  // behind it - weekday/date included - can peek through the edges and eat
  // into the limited 320x240 real estate.
  panel = lv_obj_create(parent);
  // parent has flex layout for the clock face's own children (timeRow,
  // weekday, date) - without this flag the panel becomes just another flex
  // item instead of an absolute full-screen overlay, which is what let the
  // weekday/date show through underneath and squeezed the button row off
  // the bottom.
  lv_obj_add_flag(panel, LV_OBJ_FLAG_IGNORE_LAYOUT);
  lv_obj_set_size(panel, lv_pct(100), lv_pct(100));
  lv_obj_set_pos(panel, 0, 0);
  lv_obj_set_style_bg_color(panel, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
  lv_obj_set_style_pad_all(panel, 10, 0);
  lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  lv_obj_t *title = lv_label_create(panel);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
  lv_obj_set_style_text_color(title, lv_color_white(), 0);
  lv_label_set_text(title, "Settings");

  lv_obj_t *hourRow = makeRow(panel, 20);
  lv_obj_t *hourLabel = lv_label_create(hourRow);
  lv_obj_set_style_text_color(hourLabel, lv_color_white(), 0);
  lv_label_set_text(hourLabel, "24-Hour Time");

  hourSwitch = lv_switch_create(hourRow);
  if (ClockSettings::is24Hour()) {
    lv_obj_add_state(hourSwitch, LV_STATE_CHECKED);
  }

  // Stacked (label above, dropdown full-width below) rather than a row with
  // the dropdown pinned to the right edge - some zone names are long, and a
  // right-pinned dropdown pushed its opened list toward/off the screen edge.
  lv_obj_t *tzCol = lv_obj_create(panel);
  lv_obj_remove_style_all(tzCol);
  lv_obj_set_size(tzCol, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_clear_flag(tzCol, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_flex_flow(tzCol, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(tzCol, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
  lv_obj_set_style_pad_top(tzCol, 16, 0);

  lv_obj_t *tzLabel = lv_label_create(tzCol);
  lv_obj_set_style_text_color(tzLabel, lv_color_white(), 0);
  lv_label_set_text(tzLabel, "Timezone");

  String options;
  for (int i = 0; i < TZ_OPTION_COUNT; i++) {
    if (i > 0) options += "\n";
    options += TZ_OPTIONS[i].label;
  }
  tzDropdown = lv_dropdown_create(tzCol);
  lv_obj_set_width(tzDropdown, lv_pct(100));
  lv_obj_set_style_pad_top(tzDropdown, 6, 0);
  lv_dropdown_set_options(tzDropdown, options.c_str());
  lv_dropdown_set_selected(tzDropdown, indexForCurrentTimezone());

  lv_obj_t *btnRow = lv_obj_create(panel);
  lv_obj_remove_style_all(btnRow);
  lv_obj_set_size(btnRow, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_clear_flag(btnRow, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_flex_flow(btnRow, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(btnRow, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_top(btnRow, 24, 0);

  lv_obj_t *cancelBtn = lv_btn_create(btnRow);
  lv_obj_add_event_cb(cancelBtn, onCancelClicked, LV_EVENT_CLICKED, nullptr);
  lv_obj_t *cancelLabel = lv_label_create(cancelBtn);
  lv_label_set_text(cancelLabel, "Cancel");

  lv_obj_t *saveBtn = lv_btn_create(btnRow);
  lv_obj_add_event_cb(saveBtn, onSaveClicked, LV_EVENT_CLICKED, nullptr);
  lv_obj_t *saveLabel = lv_label_create(saveBtn);
  lv_label_set_text(saveLabel, "Save");
}

} // namespace SettingsPanel
