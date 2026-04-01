#pragma once

#include <Arduino.h>

extern MENU setting_menu_root[];
extern const uint8_t SET_ROOT_N;
extern UiU8g2 u8g2;

char about_line_version[48] = "Version: emulator";
char about_line_board[48] = "Board: Desktop";
char about_line_ram_static[56] = "RamS: emulator";
char about_line_ram_heap[56] = "Heap: emulator";
char about_line_flash[56] = "Flash: emulator";
char about_line_freq[32] = "Freq: desktop";
char about_line_creator[32] = "Creator: emulator";

bool aboutRefreshPending = false;
uint32_t aboutRefreshArmMs = 0;

MENU about_menu[] = {
    {"[ About ]"},
    {about_line_board},
    {about_line_version},
    {about_line_ram_static},
    {about_line_ram_heap},
    {about_line_flash},
    {about_line_freq},
    {about_line_creator},
    {"[ Firmware ]"},
    {"[ QR ]"},
};
const uint8_t ABOUT_N = sizeof(about_menu) / sizeof(about_menu[0]);
const uint8_t ABOUT_FW_ROW = 8;
const uint8_t ABOUT_QR_ROW = 9;

MENU *curMenu = setting_menu_root;
uint8_t curMenuN = SET_ROOT_N;

static inline void about_rebuild_lines_light()
{
    std::snprintf(about_line_version, sizeof(about_line_version), "Version: emulator");
    std::snprintf(about_line_board, sizeof(about_line_board), "Board: desktop");
    std::snprintf(about_line_ram_static, sizeof(about_line_ram_static), "Static RAM: n/a");
    std::snprintf(about_line_ram_heap, sizeof(about_line_ram_heap), "Heap: %u KB", (unsigned)(ESP.getFreeHeap() / 1024));
    std::snprintf(about_line_flash, sizeof(about_line_flash), "Flash: desktop");
    std::snprintf(about_line_freq, sizeof(about_line_freq), "Freq: host");
    std::snprintf(about_line_creator, sizeof(about_line_creator), "Creator: emulator");
}

static inline void about_tick_refresh(uint32_t nowMs)
{
    if (!aboutRefreshPending)
        return;

    about_rebuild_lines_light();
    aboutRefreshPending = false;
    aboutRefreshArmMs = nowMs;
}

static inline void displayQRCode(const char *text)
{
    u8g2.drawFrame(16, 4, 96, 56);
    u8g2.drawStr(24, 20, "QR preview");
    if (text && text[0])
        u8g2.drawStr(24, 36, "Desktop stub");
}
