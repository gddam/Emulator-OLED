#pragma once

#include <Arduino.h>
#include <U8g2lib.h>

extern UiU8g2 u8g2;
extern const uint8_t *FONT_PAGE2;

static inline void useFont(const uint8_t *f);

static inline void page_gyro()
{
    useFont(FONT_PAGE2);
    u8g2.setFontPosTop();
    u8g2.drawStr(4, 8, "Gyroscope");
    u8g2.drawStr(4, 24, "Desktop emulator");
}

static inline void gyro_prewarm_on_boot()
{
}

static inline bool gyro_recalibrate_on_button_press()
{
    return false;
}

static inline void gyro_background_tick()
{
}
