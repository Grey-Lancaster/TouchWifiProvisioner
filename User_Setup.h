// Ready-to-use TFT_eSPI User_Setup.h for the Cheap Yellow Display
// (ESP32-2432S028R, ST7789 panel) - covers CYD_BasicConnect and
// CYD_RollingClock, both examples use identical wiring.
//
// Where this goes: replace the User_Setup.h file inside your Arduino
// sketchbook's TFT_eSPI library folder with this one - e.g.
// Documents/Arduino/libraries/TFT_eSPI/User_Setup.h. This is TFT_eSPI's
// own configuration mechanism (see
// https://github.com/Bodmer/TFT_eSPI#user-content-section-2-setup), not
// something specific to this library - a one-time, global setup shared
// by every sketch that uses TFT_eSPI, not per-example.
//
// If your CYD unit turns out to be ILI9341-based instead of ST7789, swap
// the driver define below accordingly - see this example's own README.
//
// PlatformIO users don't need this file - each example's platformio.ini
// already sets the same options via build_flags.

#define USER_SETUP_INFO "TouchWifiProvisioner CYD examples"

#define ST7789_DRIVER
#define TFT_INVERSION_OFF

#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   15
#define TFT_DC    2
#define TFT_RST  -1
#define TFT_BL   21
#define TFT_BACKLIGHT_ON HIGH

#define USE_HSPI_PORT

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF

#define SPI_FREQUENCY       55000000
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY  2500000
