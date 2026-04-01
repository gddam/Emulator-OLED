#pragma once

#include <Arduino.h>

extern uint8_t ui_param[];
extern const uint8_t *FONT_PAGE2;
extern UiU8g2 u8g2;
static inline void useFont(const uint8_t *f);

static const char *BOOT_PATH = "/bootsplash.bin";

MENU setting_menu_wifi[] = {
    {"[ WiFi ]"},
    {"+ Enable"},
    {"[ QR ]"},
};

static inline uint8_t wifi_menu_count_now()
{
    return (uint8_t)(sizeof(setting_menu_wifi) / sizeof(setting_menu_wifi[0]));
}

static inline void wifi_menu_sync()
{
}

MENU firmware_menu[] = {
    {"[ Firmware ]"},
    {"Desktop stub"},
};
uint8_t firmware_menu_count = sizeof(firmware_menu) / sizeof(firmware_menu[0]);

MENU firmware_sd_menu[] = {
    {"[ SD Firmware ]"},
    {"No SD in emulator"},
};
uint8_t firmware_sd_menu_count = sizeof(firmware_sd_menu) / sizeof(firmware_sd_menu[0]);

MENU firmware_sd_version_menu[] = {
    {"[ SD Versions ]"},
    {"No versions"},
};
uint8_t firmware_sd_version_menu_count = sizeof(firmware_sd_version_menu) / sizeof(firmware_sd_version_menu[0]);

bool g_playRequested = false;
bool g_wifiUiDirty = false;

enum OtaState : uint8_t
{
    OTA_IDLE = 0,
    OTA_FAIL = 1,
};

uint8_t ota_state = OTA_IDLE;
bool ota_wait_win_close = false;

enum FwNotifyCode : uint8_t
{
    FW_WIFI_ERR = 1,
    FW_ERR_GH_CFG = 2,
};

static inline void draw_stub_page(const char *title, const char *body)
{
    useFont(FONT_PAGE2);
    u8g2.setFontPosTop();
    u8g2.drawStr(4, 8, title);
    u8g2.drawStr(4, 24, body);
}

static inline void page_wifi()
{
    draw_stub_page("WiFi", "Desktop emulator");
}

static inline void page_firmware()
{
    draw_stub_page("Firmware", "Desktop emulator");
}

static inline void page_firmware_sdcard()
{
    draw_stub_page("SD Firmware", "Desktop emulator");
}

static inline void page_firmware_sdcard_ver()
{
    draw_stub_page("SD Versions", "Desktop emulator");
}

static inline void page_ota_progress()
{
    draw_stub_page("OTA", "Not available");
}

static inline void page_firmware_wait()
{
    draw_stub_page("Firmware", "Preparing...");
}

static inline bool wifi_handle_menu_select(uint8_t sel)
{
    return false;
}

static inline bool firmware_handle_menu_select(uint8_t sel)
{
    return false;
}

static inline bool firmware_sd_handle_menu_select(uint8_t sel)
{
    return false;
}

static inline void firmware_request_refresh()
{
    g_fwUiDirty = true;
}

static inline void firmware_tick(uint32_t nowMs)
{
}

static inline void firmware_menu_sync()
{
}

static inline void firmware_sd_menu_sync()
{
}

static inline void firmware_sd_version_menu_sync()
{
}

static inline void firmware_sd_scan_boards()
{
}

static inline const char *wifi_root_tag(char *buf, size_t bufSize)
{
    if (buf && bufSize > 0)
    {
        std::snprintf(buf, bufSize, "%s", ui_param[WIFI_ENABLED] ? "ON" : "OFF");
        return buf;
    }
    return "OFF";
}

static inline const char *wifi_qr_text_now()
{
    return QR_DEFAULT_TEXT;
}

static inline bool wifi_sta_connected()
{
    return false;
}

static inline void wifi_apply_power()
{
}

static inline void wifi_streaming_hint(bool streaming)
{
}

static inline bool gh_has_repo()
{
    return true;
}

static inline void fw_notify(uint8_t code, bool sticky)
{
}

static inline bool ota_animating_now()
{
    return false;
}

static inline void screenbrodcast_tick(uint32_t nowMs)
{
}

static inline bool screenbrodcast_take_cmd(char *buffer, size_t len)
{
    return false;
}

static inline void screenbrodcast_set_streaming(bool streaming)
{
}

static inline bool screenbrodcast_has_clients()
{
    return false;
}

static inline void wifiPortalProcess()
{
}
