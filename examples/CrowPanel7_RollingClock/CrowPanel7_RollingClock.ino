// TouchWifiProvisioner example: a full app, not just a connect demo - an
// odometer-style rolling-digit clock on the Elecrow CrowPanel Advance 7.0"
// HMI, with the on-device Wi-Fi picker handling setup and ezTime handling
// NTP/timezone once connected.
//
// Code by Grey and his buddy Claude.
//
// Unlike CrowPanel7_BasicConnect (which just shows a green "Connected"
// screen), this shows what it looks like to hand off from
// TouchWifiProvisioner into an app that keeps running afterward: the same
// TouchWifiProvisioner::begin()/loop() calls, but onWifiConnected() here
// starts NTP sync and builds a whole clock face instead of a static screen.
// The clock UI itself (ClockFace/RollingDigit/SettingsPanel/ClockSettings)
// is identical to CYD_RollingClock's - it's plain LVGL widgets laid out with
// flex containers, so it doesn't care which board or resolution it's on.
//
// Before this compiles, see README.md next to this file for:
//   - the PlatformIO platform/lib_deps this board needs (PSRAM speed fix
//     for RGB panel jitter, same as CrowPanel7_BasicConnect)
//   - lv_conf.h requirements - notably LV_MEM_CUSTOM, which this example
//     needs but CrowPanel7_BasicConnect doesn't
//   - the ezTime/picolibc build flag workaround

#include "pins_config.h"
#include "LovyanGFX_Driver.h"

#include <Arduino.h>
#include <lvgl.h>  // version 8.3.11
#include <Wire.h>
#include <WiFi.h>
#include <ezTime.h>
#include <TouchWifiProvisioner.h>
#include "ClockFace.h"
#include "ClockSettings.h"

// Defaults for first boot - after that, 24-hour format and timezone are
// user-editable on-device via the gear icon's settings panel and persist
// across reboots (see ClockSettings). US-style date has no on-device toggle.
#define CLOCK_TIMEZONE "America/New_York"
#define CLOCK_24_HOUR false
#define CLOCK_US_DATE true

LGFX gfx;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf1;
static lv_color_t *buf2;
static uint16_t touch_x, touch_y;
Timezone myTZ;

static bool clockActive = false;

static void disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  if (gfx.getStartCount() > 0) {
    gfx.endWrite();
  }
  gfx.pushImageDMA(area->x1, area->y1, area->x2 - area->x1 + 1, area->y2 - area->y1 + 1, (lgfx::rgb565_t *)&color_p->full);
  lv_disp_flush_ready(disp);
}

static void touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data) {
  data->state = LV_INDEV_STATE_REL;
  if (gfx.getTouch(&touch_x, &touch_y)) {
    data->state = LV_INDEV_STATE_PR;
    data->point.x = touch_x;
    data->point.y = touch_y;
  }
}

// The CrowPanel's onboard STC8H1K28 microcontroller gates the backlight and
// arms the GT911 touch controller over I2C - without this, the panel stays
// dark and touch never responds, regardless of how LovyanGFX/LVGL are set up.
static bool i2cScanForAddress(uint8_t address) {
  Wire.beginTransmission(address);
  return (Wire.endTransmission() == 0);
}

static void sendI2CCommand(uint8_t command) {
  Wire.beginTransmission(0x30);
  Wire.write(command);
  Wire.endTransmission();
}

// Common phone-style RSSI breakpoints (dBm, higher/less-negative = stronger),
// same thresholds CrowPanel7_BasicConnect uses for its on-screen bars.
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

  Wire.begin(15, 16);
  delay(50);
  while (!(i2cScanForAddress(0x30) && i2cScanForAddress(0x5D))) {
    Serial.println("Waiting for backlight/touch controller...");
    sendI2CCommand(250); // activate touch screen
    pinMode(1, OUTPUT);
    digitalWrite(1, LOW);
    delay(120);
    pinMode(1, INPUT);
    delay(100);
  }
  sendI2CCommand(0); // brightest backlight (0-245, 245 = off)

  gfx.init();
  gfx.initDMA();
  gfx.startWrite();
  gfx.fillScreen(TFT_BLACK);

  lv_init();

  size_t buffer_size = sizeof(lv_color_t) * LCD_H_RES * LCD_V_RES;
  buf1 = (lv_color_t *)heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM);
  buf2 = (lv_color_t *)heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM);
  lv_disp_draw_buf_init(&draw_buf, buf1, buf2, LCD_H_RES * LCD_V_RES);

  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = LCD_H_RES;
  disp_drv.ver_res = LCD_V_RES;
  disp_drv.flush_cb = disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = touchpad_read;
  lv_indev_drv_register(&indev_drv);

  TouchWifiProvisioner::begin(lv_scr_act(), "CrowPanel7-RollingClock", onWifiConnected);
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
