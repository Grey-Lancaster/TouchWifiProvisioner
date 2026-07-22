/* CI-only lv_conf.h for the CYD_RollingClock example - not shipped to end
   users. Differs from lv_conf.h (used by the other two examples) mainly
   in LV_MEM_CUSTOM: this example needs more LVGL heap than a static
   LV_MEM_SIZE comfortably provides. See that example's README. */

#if 1 /* Set to "1" to enable content */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
   COLOR SETTINGS
 *====================*/
#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 0 /* byte swap is done at flush time via tft.pushColors(..., true) */
#define LV_COLOR_SCREEN_TRANSP 0
#define LV_COLOR_MIX_ROUND_OFS 0
#define LV_COLOR_CHROMA_KEY lv_color_hex(0x00ff00)

/*=========================
   MEMORY SETTINGS
 *=========================*/
/* Static LV_MEM_SIZE (LV_MEM_CUSTOM 0) is baked into the firmware's DRAM
   segment at link time, which has very little spare room on this chip -
   even a modest bump past the original 48K overflowed dram0_0_seg. Using
   the ESP32's runtime heap instead avoids that link-time ceiling and gives
   LVGL much more room to actually grow into. */
#define LV_MEM_CUSTOM 1
#if LV_MEM_CUSTOM
    #define LV_MEM_CUSTOM_INCLUDE <stdlib.h>
    #define LV_MEM_CUSTOM_ALLOC   malloc
    #define LV_MEM_CUSTOM_FREE    free
    #define LV_MEM_CUSTOM_REALLOC realloc
#else
    #define LV_MEM_SIZE (48U * 1024U)
    #define LV_MEM_ADR 0
    #define LV_MEM_AUTO_DEFRAG 1
#endif

/*====================
   HAL SETTINGS
 *====================*/
#define LV_DISP_DEF_REFR_PERIOD 30
#define LV_INDEV_DEF_READ_PERIOD 30

#define LV_TICK_CUSTOM 1
#if LV_TICK_CUSTOM
    #define LV_TICK_CUSTOM_INCLUDE "Arduino.h"
    #define LV_TICK_CUSTOM_SYS_TIME_EXPR (millis())
#endif

#define LV_DPI_DEF 130

/*=======================
 * FEATURE CONFIGURATION
 *=======================*/

#define LV_DRAW_COMPLEX 1
#define LV_SHADOW_CACHE_SIZE 0
#define LV_CIRCLE_CACHE_SIZE 4

#define LV_LAYER_SIMPLE_BUF_SIZE (24 * 1024)
#define LV_LAYER_SIMPLE_FALLBACK_BUF_SIZE (3 * 1024)

#define LV_IMG_CACHE_DEF_SIZE 0
#define LV_GRADIENT_MAX_STOPS 2
#define LV_GRAD_CACHE_DEF_SIZE 0
#define LV_DITHER_GRADIENT 0

#define LV_DISP_ROT_MAX_BUF (10*1024)

#define LV_USE_USER_DATA 1

#define LV_ENABLE_GC 0

/*=====================
 *  COMPILER SETTINGS
 *====================*/
#define LV_BIG_ENDIAN_SYSTEM 0
#define LV_ATTRIBUTE_TICK_INC
#define LV_ATTRIBUTE_TIMER_HANDLER
#define LV_ATTRIBUTE_FLUSH_READY
#define LV_ATTRIBUTE_MEM_ALIGN_SIZE 1
#define LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_LARGE_CONST
#define LV_ATTRIBUTE_LARGE_RAM_ARRAY
#define LV_ATTRIBUTE_FAST_MEM
#define LV_ATTRIBUTE_DMA
#define LV_EXPORT_CONST_INT(int_value) struct _silence_gcc_warning
#define LV_USE_LARGE_COORD 0

/*==================
 *   FONT USAGE
 *===================*/
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_48 1
#define LV_FONT_DEFAULT &lv_font_montserrat_14

#define LV_FONT_FMT_TXT_LARGE 0
#define LV_USE_FONT_COMPRESSED 0
#define LV_USE_FONT_SUBPX 0

/*=================
 *  TEXT SETTINGS
 *=================*/
#define LV_TXT_ENC LV_TXT_ENC_UTF8
#define LV_TXT_BREAK_CHARS " ,.;:-_"
#define LV_TXT_LINE_BREAK_LONG_LEN 0
#define LV_TXT_LINE_BREAK_LONG_PRE_MIN_LEN 3
#define LV_TXT_LINE_BREAK_LONG_POST_MIN_LEN 3
#define LV_TXT_COLOR_CMD "#"
#define LV_USE_BIDI 0
#define LV_USE_ARABIC_PERSIAN_CHARS 0

/*==================
 *  WIDGET USAGE
 *================*/
#define LV_USE_ARC 1
#define LV_USE_BAR 1
#define LV_USE_BTN 1
#define LV_USE_BTNMATRIX 1
#define LV_USE_CANVAS 1
#define LV_USE_CHECKBOX 1
#define LV_USE_DROPDOWN 1
#define LV_USE_IMG 1
#define LV_USE_LABEL 1
#define LV_LABEL_TEXT_SELECTION 1
#define LV_LABEL_LONG_TXT_HINT 1
#define LV_USE_LINE 1
#define LV_USE_LIST 1
#define LV_USE_ROLLER 1
#define LV_USE_SLIDER 1
#define LV_USE_SWITCH 1
#define LV_USE_TEXTAREA 1
#define LV_TEXTAREA_DEF_PWD_SHOW_TIME 1500
#define LV_USE_TABLE 1

/*==================
 * EXTRA COMPONENTS
 *==================*/
#define LV_USE_ANIMIMG 1
#define LV_USE_CALENDAR 0
#define LV_USE_CHART 0
#define LV_USE_COLORWHEEL 0
#define LV_USE_IMGBTN 1
#define LV_USE_KEYBOARD 1
#define LV_USE_LED 0
#define LV_USE_MENU 0
#define LV_USE_METER 0
#define LV_USE_MSGBOX 1
#define LV_USE_SPAN 1
#define LV_SPAN_SNIPPET_STACK_SIZE 64
#define LV_USE_SPINBOX 0
#define LV_USE_SPINNER 1
#define LV_USE_TABVIEW 0
#define LV_USE_TILEVIEW 0
#define LV_USE_WIN 0

/*-----------
 * Themes
 *----------*/
#define LV_USE_THEME_DEFAULT 1
#if LV_USE_THEME_DEFAULT
    #define LV_THEME_DEFAULT_DARK 0
    #define LV_THEME_DEFAULT_GROW 1
    #define LV_THEME_DEFAULT_TRANSITION_TIME 80
#endif

#define LV_USE_THEME_BASIC 1
#define LV_USE_THEME_MONO 0

/*-----------
 * Layouts
 *----------*/
#define LV_USE_FLEX 1
#define LV_USE_GRID 1

/*---------------------
 * 3rd party libraries
 *--------------------*/
#define LV_USE_FS_STDIO 0
#define LV_USE_FS_POSIX 0
#define LV_USE_FS_WIN32 0
#define LV_USE_FS_FATFS 0
#define LV_USE_PNG 0
#define LV_USE_BMP 0
#define LV_USE_SJPG 0
#define LV_USE_GIF 0
#define LV_USE_QRCODE 0
#define LV_USE_FREETYPE 0
#define LV_USE_RLOTTIE 0
#define LV_USE_FFMPEG 0

/*-----------
 * Others
 *----------*/
#define LV_USE_SNAPSHOT 0
#define LV_USE_MONKEY 0
#define LV_USE_GRIDNAV 0
#define LV_USE_FRAGMENT 0
#define LV_USE_IMGFONT 0
#define LV_USE_MSG 0
#define LV_USE_IME_PINYIN 0

/*==================
 * EXAMPLES
 *==================*/
#define LV_BUILD_EXAMPLES 0

/*===================
 * DEMO USAGE
 ====================*/
#define LV_USE_DEMO_WIDGETS 0
#define LV_USE_DEMO_KEYPAD_AND_ENCODER 0
#define LV_USE_DEMO_BENCHMARK 0
#define LV_USE_DEMO_STRESS 0
#define LV_USE_DEMO_MUSIC 0

#endif /*LV_CONF_H*/

#endif /*Content enable*/
