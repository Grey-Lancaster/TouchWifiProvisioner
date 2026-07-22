// TouchWifiProvisioner example: a full app, not just a connect demo - an
// odometer-style rolling-digit clock on the Cheap Yellow Display, with the
// on-device Wi-Fi picker handling setup and ezTime handling NTP/timezone
// once connected.
//
// Unlike CYD_BasicConnect/CrowPanel7_BasicConnect (which just show a green
// "Connected" screen), this shows what it looks like to hand off from
// TouchWifiProvisioner into an app that keeps running afterward: the same
// TouchWifiProvisioner::begin()/loop() calls, but onConnected() here starts
// NTP sync and builds a whole clock face instead of a static screen.
//
// Before this compiles, see README.md next to this file for:
//   - TFT_eSPI panel configuration (wiring + driver select)
//   - lv_conf.h requirements - notably LV_MEM_CUSTOM, which this example
//     needs but the other two don't
//   - the ezTime/picolibc build flag workaround

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include <lvgl.h>  // version 8.3.11
#include <WiFi.h>
#include <ezTime.h>
#include <TouchWifiProvisioner.h>
#include "ClockFace.h"
#include "ClockSettings.h"

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

// Defaults for first boot - after that, 24-hour format and timezone are
// user-editable on-device via the gear icon's settings panel and persist
// across reboots (see ClockSettings). US-style date has no on-device toggle.
#define CLOCK_TIMEZONE "America/New_York"
#define CLOCK_24_HOUR false
#define CLOCK_US_DATE true

static const uint16_t SCREEN_W = 320;
static const uint16_t SCREEN_H = 240;

TFT_eSPI tft = TFT_eSPI();
// TFT_eSPI is on HSPI (see README build flags), which leaves the default
// SPI (VSPI) free to dedicate to touch.
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);
Timezone myTZ;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[SCREEN_W * 40];

static bool clockActive = false;

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

// Common phone-style RSSI breakpoints (dBm, higher/less-negative = stronger),
// same thresholds CYD_BasicConnect uses for its on-screen bars.
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

  Serial.println("Waiting for NTP sync...");
  waitForSync();
  myTZ.setLocation(ClockSettings::timezone());
  Serial.println(myTZ.dateTime());

  ClockFace::setup(lv_scr_act(), ClockSettings::is24Hour(), CLOCK_US_DATE);
  clockActive = true;
}

void setup() {
  Serial.begin(115200);

  ClockSettings::begin(CLOCK_24_HOUR, CLOCK_TIMEZONE);

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

  TouchWifiProvisioner::begin(lv_scr_act(), "CYD-RollingClock", onWifiConnected);
}

void loop() {
  lv_timer_handler(); // must keep running after WiFi connects - it now drives the LVGL clock face, not just the picker UI
  TouchWifiProvisioner::loop(); // no-op once connected

  if (clockActive) {
    events(); // ezTime's NTP/event pump
    if (secondChanged()) {
      ClockFace::update();
    }
  }

  delay(5);
}
