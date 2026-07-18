// TouchWifiProvisioner example: connect to Wi-Fi and show a plain green
// "Connected" screen with the assigned IP address.
//
// Tested on the Cheap Yellow Display (ESP32-2432S028R, ST7789 panel,
// resistive XPT2046 touch). TFT_eSPI drives the display and
// XPT2046_Touchscreen reads touch, glued into LVGL below -
// TouchWifiProvisioner itself never touches either directly, so this
// bring-up section can be swapped for any other LVGL display/touch setup.
//
// Before this compiles, see README.md next to this file for:
//   - TFT_eSPI panel configuration (wiring + driver select)
//   - lv_conf.h placement (required by TouchWifiProvisioner's parent
//     README, not shipped by the library itself)

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include <lvgl.h>
#include <WiFi.h>
#include <TouchWifiProvisioner.h>

// Touch controller pins - CYD's XPT2046 is on its own SPI bus, separate
// from the display's.
#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

// Touch calibration (raw ADC range -> screen pixels), for rotation 3.
// Resistive panels drift unit to unit - nudge these if taps land off-target.
#define TOUCH_X_MIN 200
#define TOUCH_X_MAX 3700
#define TOUCH_Y_MIN 240
#define TOUCH_Y_MAX 3800

static const uint16_t SCREEN_W = 320;
static const uint16_t SCREEN_H = 240;

TFT_eSPI tft = TFT_eSPI();
// TFT_eSPI is on HSPI (see README build flags), which leaves the default
// SPI (VSPI) free to dedicate to touch.
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[SCREEN_W * 40];

static void disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = area->x2 - area->x1 + 1;
  uint32_t h = area->y2 - area->y1 + 1;

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t *)&color_p->full, w * h, true);
  tft.endWrite();

  lv_disp_flush_ready(disp);
}

static void touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data) {
  if (!ts.touched()) {
    data->state = LV_INDEV_STATE_REL;
    return;
  }

  TS_Point p = ts.getPoint();

  data->state = LV_INDEV_STATE_PR;
  data->point.x = map(p.x, TOUCH_X_MIN, TOUCH_X_MAX, 0, SCREEN_W);
  data->point.y = map(p.y, TOUCH_Y_MIN, TOUCH_Y_MAX, 0, SCREEN_H);
}

static lv_obj_t *connectedScreen = nullptr;

// Common phone-style RSSI breakpoints (dBm, higher/less-negative = stronger),
// shared between the on-screen icon and the Serial dump below.
static int rssiToBars(int32_t rssi) {
  if (rssi >= -55) return 4;
  if (rssi >= -65) return 3;
  if (rssi >= -75) return 2;
  if (rssi >= -85) return 1;
  return 0;
}

static String rssiBarsAscii(int32_t rssi) {
  int lit = rssiToBars(rssi);
  String bars;
  for (int i = 0; i < 4; i++) bars += (i < lit) ? "#" : ".";
  return bars;
}

// Classic 4-bar signal-strength icon.
static lv_obj_t *createSignalBars(lv_obj_t *parent, int32_t rssi) {
  int lit = rssiToBars(rssi);

  lv_obj_t *cont = lv_obj_create(parent);
  lv_obj_set_size(cont, 44, 24);
  lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(cont, 0, 0);
  lv_obj_set_style_pad_all(cont, 0, 0);
  lv_obj_set_style_pad_column(cont, 3, 0);
  lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
  lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

  static const int heights[4] = {8, 13, 18, 23};
  for (int i = 0; i < 4; i++) {
    lv_obj_t *bar = lv_obj_create(cont);
    lv_obj_set_size(bar, 7, heights[i]);
    lv_obj_set_style_radius(bar, 2, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_bg_color(bar, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(bar, i < lit ? LV_OPA_COVER : LV_OPA_30, 0);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
  }
  return cont;
}

static void showConnectedScreen(const String &ip) {
  if (connectedScreen) lv_obj_del(connectedScreen);

  connectedScreen = lv_obj_create(lv_scr_act());
  lv_obj_set_size(connectedScreen, lv_pct(100), lv_pct(100));
  lv_obj_center(connectedScreen);
  lv_obj_set_style_bg_color(connectedScreen, lv_palette_main(LV_PALETTE_GREEN), 0);
  lv_obj_set_style_border_width(connectedScreen, 0, 0);
  lv_obj_set_style_radius(connectedScreen, 0, 0);
  lv_obj_set_flex_flow(connectedScreen, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(connectedScreen, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(connectedScreen, 6, 0);

  lv_obj_t *icon = lv_label_create(connectedScreen);
  lv_label_set_text(icon, LV_SYMBOL_OK);
  lv_obj_set_style_text_font(icon, &lv_font_montserrat_48, 0);
  lv_obj_set_style_text_color(icon, lv_color_white(), 0);

  lv_obj_t *title = lv_label_create(connectedScreen);
  lv_label_set_text(title, "Connected");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
  lv_obj_set_style_text_color(title, lv_color_white(), 0);

  lv_obj_t *ssidLabel = lv_label_create(connectedScreen);
  lv_label_set_text_fmt(ssidLabel, "SSID: %s", WiFi.SSID().c_str());
  lv_obj_set_style_text_color(ssidLabel, lv_color_white(), 0);

  createSignalBars(connectedScreen, WiFi.RSSI());

  lv_obj_t *ipLabel = lv_label_create(connectedScreen);
  lv_label_set_text_fmt(ipLabel, "%s", ip.c_str());
  lv_obj_set_style_text_color(ipLabel, lv_color_white(), 0);

  lv_obj_t *forgetBtn = lv_btn_create(connectedScreen);
  lv_obj_set_style_pad_hor(forgetBtn, 14, 0);
  lv_obj_set_style_pad_ver(forgetBtn, 8, 0);
  lv_obj_set_style_bg_opa(forgetBtn, LV_OPA_20, 0);
  lv_obj_set_style_bg_color(forgetBtn, lv_color_white(), 0);
  lv_obj_set_style_shadow_width(forgetBtn, 0, 0);
  lv_obj_add_event_cb(forgetBtn, onForgetClicked, LV_EVENT_CLICKED, nullptr);
  lv_obj_t *forgetLabel = lv_label_create(forgetBtn);
  lv_label_set_text(forgetLabel, LV_SYMBOL_CLOSE " Forget Network");
  lv_obj_set_style_text_color(forgetLabel, lv_color_white(), 0);
}

static void onWifiConnected(const String &ip) {
  Serial.println("Wifi connected");
  Serial.printf("  SSID:    %s\n", WiFi.SSID().c_str());
  Serial.printf("  IP:      %s\n", ip.c_str());
  Serial.printf("  Gateway: %s\n", WiFi.gatewayIP().toString().c_str());
  Serial.printf("  Subnet:  %s\n", WiFi.subnetMask().toString().c_str());
  Serial.printf("  DNS:     %s\n", WiFi.dnsIP(0).toString().c_str());
  Serial.printf("  RSSI:    %d dBm  [%s]\n", WiFi.RSSI(), rssiBarsAscii(WiFi.RSSI()).c_str());
  Serial.printf("  Channel: %d\n", WiFi.channel());
  Serial.printf("  BSSID:   %s\n", WiFi.BSSIDstr().c_str());
  Serial.printf("  MAC:     %s\n", WiFi.macAddress().c_str());
  showConnectedScreen(ip);
}

static void onForgetClicked(lv_event_t *e) {
  TouchWifiProvisioner::reset();
}

void setup() {
  Serial.begin(115200);

  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);

  SPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  ts.begin();
  ts.setRotation(3); // matches tft rotation; inverts raw x/y to stay aligned with our calibration map()

  lv_init();

  lv_disp_draw_buf_init(&draw_buf, buf1, nullptr, SCREEN_W * 40);

  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = SCREEN_W;
  disp_drv.ver_res = SCREEN_H;
  disp_drv.flush_cb = disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = touchpad_read;
  lv_indev_drv_register(&indev_drv);

  TouchWifiProvisioner::begin(lv_scr_act(), "TouchWifi", onWifiConnected);
}

void loop() {
  lv_timer_handler();
  TouchWifiProvisioner::loop();
  delay(5);
}
