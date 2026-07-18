// A single odometer-style digit built from LVGL objects: two stacked labels
// inside a clipping container, animated with lv_anim so the old digit slides
// out upward while the new one slides in from below - the LVGL equivalent of
// the old TFT_eSprite roll-blit, but done with real widgets instead of a
// hand-rolled sprite loop.
#pragma once

#include <lvgl.h>

class RollingDigit {
public:
  RollingDigit(lv_obj_t *parent, const lv_font_t *font, lv_color_t color);

  // Updates the displayed value. Animates the roll transition unless
  // animate is false (used for the very first draw after boot).
  void setValue(int value, bool animate = true);

  int value() const { return m_value; }
  lv_obj_t *container() const { return m_container; }

private:
  static void onRollFinished(lv_anim_t *a);

  lv_obj_t *m_container;
  lv_obj_t *m_labelCur;
  lv_obj_t *m_labelNext;
  const lv_font_t *m_font;
  int m_value;
  int m_digitHeight;
};
