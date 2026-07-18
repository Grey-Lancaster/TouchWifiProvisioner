#include "RollingDigit.h"

namespace {
constexpr uint32_t ROLL_TIME_MS = 220;
}

RollingDigit::RollingDigit(lv_obj_t *parent, const lv_font_t *font, lv_color_t color)
    : m_font(font), m_value(0) {
  lv_point_t size;
  lv_txt_get_size(&size, "8", font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
  m_digitHeight = size.y;

  m_container = lv_obj_create(parent);
  lv_obj_remove_style_all(m_container);
  lv_obj_set_size(m_container, size.x, size.y);
  lv_obj_clear_flag(m_container, LV_OBJ_FLAG_SCROLLABLE);

  // Container width is sized for "8" (the widest digit), but narrower
  // glyphs like "1" would otherwise sit flush left within that box, leaving
  // a visible gap on their right - give each label the full box width and
  // center its text so every digit lands in the same visual spot.
  m_labelCur = lv_label_create(m_container);
  lv_obj_set_style_text_font(m_labelCur, font, 0);
  lv_obj_set_style_text_color(m_labelCur, color, 0);
  lv_obj_set_style_text_align(m_labelCur, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_width(m_labelCur, size.x);
  lv_obj_set_pos(m_labelCur, 0, 0);
  lv_label_set_text(m_labelCur, "0");

  m_labelNext = lv_label_create(m_container);
  lv_obj_set_style_text_font(m_labelNext, font, 0);
  lv_obj_set_style_text_color(m_labelNext, color, 0);
  lv_obj_set_style_text_align(m_labelNext, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_width(m_labelNext, size.x);
  lv_obj_set_pos(m_labelNext, 0, m_digitHeight); // parked below, hidden by parent clip
  lv_label_set_text(m_labelNext, "0");
}

void RollingDigit::setValue(int value, bool animate) {
  if (value == m_value) return;

  if (!animate) {
    lv_label_set_text_fmt(m_labelCur, "%d", value);
    lv_obj_set_pos(m_labelCur, 0, 0);
    lv_obj_set_pos(m_labelNext, 0, m_digitHeight);
    m_value = value;
    return;
  }

  // Cancel any roll still in flight so a rapid re-trigger (shouldn't happen
  // at 1 update/sec, but cheap to guard) can't leave labels mid-slide.
  lv_anim_del(m_labelCur, nullptr);
  lv_anim_del(m_labelNext, nullptr);

  lv_label_set_text_fmt(m_labelNext, "%d", value);
  lv_obj_set_pos(m_labelNext, 0, m_digitHeight);
  lv_obj_set_pos(m_labelCur, 0, 0);

  lv_anim_t outAnim;
  lv_anim_init(&outAnim);
  lv_anim_set_var(&outAnim, m_labelCur);
  lv_anim_set_values(&outAnim, 0, -m_digitHeight);
  lv_anim_set_time(&outAnim, ROLL_TIME_MS);
  lv_anim_set_exec_cb(&outAnim, (lv_anim_exec_xcb_t)lv_obj_set_y);
  // Both anims finish at the same instant; whichever one LVGL happens to
  // apply last in that tick must be the one that leaves the resting state
  // correct, so both get the same idempotent ready_cb rather than just inAnim.
  outAnim.user_data = this;
  lv_anim_set_ready_cb(&outAnim, onRollFinished);
  lv_anim_start(&outAnim);

  lv_anim_t inAnim;
  lv_anim_init(&inAnim);
  lv_anim_set_var(&inAnim, m_labelNext);
  lv_anim_set_values(&inAnim, m_digitHeight, 0);
  lv_anim_set_time(&inAnim, ROLL_TIME_MS);
  lv_anim_set_exec_cb(&inAnim, (lv_anim_exec_xcb_t)lv_obj_set_y);
  inAnim.user_data = this;
  lv_anim_set_ready_cb(&inAnim, onRollFinished);
  lv_anim_start(&inAnim);

  m_value = value;
}

void RollingDigit::onRollFinished(lv_anim_t *a) {
  RollingDigit *self = static_cast<RollingDigit *>(a->user_data);
  // labelNext has settled into place - fold it into labelCur and park
  // labelNext back below, ready for the next roll.
  lv_label_set_text(self->m_labelCur, lv_label_get_text(self->m_labelNext));
  lv_obj_set_pos(self->m_labelCur, 0, 0);
  lv_obj_set_pos(self->m_labelNext, 0, self->m_digitHeight);
}
