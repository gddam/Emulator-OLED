#define KEY_ESC 0
#define KEY_F1 0
#define KEY_F2 0
#define KEY_F3 0
#define KEY_F4 0
#define KEY_F5 0
#define KEY_F6 0
#define KEY_F7 0
#define KEY_F8 0
#define KEY_F9 0
#define KEY_F10 0
#define KEY_F11 0
#define KEY_F12 0
#define KEY_LEFT_CTRL 0
#define KEY_LEFT_SHIFT 0
#define KEY_LEFT_ALT 0
#define KEY_LEFT_GUI 0
#define KEY_RIGHT_CTRL 0
#define KEY_RIGHT_SHIFT 0
#define KEY_RIGHT_ALT 0
#define KEY_RIGHT_GUI 0
#define KEY_CAPS_LOCK 0
#define KEY_BACKSPACE 0
#define KEY_RETURN 0
#define KEY_INSERT 0
#define KEY_DELETE 0
#define KEY_TAB 0
#define KEY_HOME 0
#define KEY_END 0
#define KEY_PAGE_UP 0
#define KEY_PAGE_DOWN 0
#define KEY_UP_ARROW 0
#define KEY_DOWN_ARROW 0
#define KEY_LEFT_ARROW 0
#define KEY_RIGHT_ARROW 0

#ifndef CFG_THUMB_PUSH_PIN
#define CFG_THUMB_PUSH_PIN 3
#endif
#ifndef CFG_THUMB_CW_PIN
#define CFG_THUMB_CW_PIN 4
#endif
#ifndef CFG_THUMB_CCW_PIN
#define CFG_THUMB_CCW_PIN 2
#endif
#ifndef CFG_BTN_PIN
#define CFG_BTN_PIN CFG_THUMB_PUSH_PIN
#endif

#define PA0 0
#define PA1 0
#define PA2 0
#define PA3 0
#define PA4 0
#define PA5 0
#define PA6 0
#define PA7 0
#define PB0 0
#define PB1 0

#include <SDL_keycode.h>

#define PB12 SDLK_q
#define PB13 SDLK_w
#define PB14 SDLK_e

#define PB6 0
#define PB7 0
#define U8X8_PIN_NONE (-1)

#ifndef WOUOUI_EMULATOR
#define WOUOUI_EMULATOR 1
#endif

#define U8G2_SH1107_SEEED_128X128_F_4W_HW_SPI U8G2_SDL_128X128
#define U8G2_SH1106_128X64_NONAME_F_HW_I2C U8G2_SDL_128X64

#ifndef UI_U8G2_TYPE
#define UI_U8G2_TYPE U8G2_SDL_128X64
#endif

void ui_param_init();
