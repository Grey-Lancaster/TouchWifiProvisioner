// TouchWifiProvisioner example: connect to Wi-Fi and show a plain green
// "Connected" screen with the assigned IP address.
//
// Tested on the Elecrow CrowPanel Advance 7.0" HMI (ESP32-S3, 800x480 RGB
// IPS panel, GT911 capacitive touch). LovyanGFX drives the display and
// touch via the RGB/GT911 config in LovyanGFX_Driver.h, glued into LVGL
// below - TouchWifiProvisioner itself never touches either directly, so
// this bring-up section can be swapped for any other LVGL display/touch
// setup.
//
// Before this compiles, see README.md next to this file for:
//   - the PlatformIO platform/lib_deps pins this board needs (PSRAM speed
//     fix for RGB panel jitter)
//   - lv_conf.h placement (required by TouchWifiProvisioner's parent
//     README, not shipped by the library itself)

#include "pins_config.h"
#include "LovyanGFX_Driver.h"

#include <Arduino.h>
#include <lvgl.h>
#include <Wire.h>
#include <WiFi.h>
#include <TouchWifiProvisioner.h>

LGFX gfx;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf1;
static lv_color_t *buf2;
static uint16_t touch_x, touch_y;

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

void setup() {
  Serial.begin(115200);

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

  TouchWifiProvisioner::begin(lv_scr_act(), "CrowPanel7", onWifiConnected);
}

void loop() {
  lv_timer_handler();
  TouchWifiProvisioner::loop();
  delay(5);
}
