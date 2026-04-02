#pragma once

#include <Arduino.h>

extern uint8_t ui_param[];
extern const uint8_t *FONT_PAGE2;
extern UiU8g2 u8g2;
static inline void useFont(const uint8_t *f);
struct SettingList;
extern MENU *curMenu;
extern uint8_t curMenuN;
extern SettingList sl;
extern uint8_t pageDraw;
static inline void sl_recalc_targets();
static inline void draw_settings_common();
static inline void qr_open(const char *text, uint8_t returnPage);

static const char *BOOT_PATH = "/bootsplash.bin";
static const char WIFI_UI_AP_PASS[] = "12345678";
static const char WIFI_QR_ITEM[] = "[ QR ]";
static const char RESET_WIFI_CRE[] = "Reset WiFi Credentials";

static char wifi_mode_line[24] = "[ WiFi (OFF) ]";
static char wifi_ssid_line[32] = "SSID: -";
static char wifi_ip_line[64] = "IP: -";
static char wifi_pass_line[64] = "PASS: -";
static char wifi_rssi_line[32] = "";
static char wifi_qr_text[64] = "";
static uint8_t wifi_menu_count = 5;
static uint8_t wifi_reset_row = 4;
static uint8_t wifi_qr_row = 255;

MENU setting_menu_wifi[] = {
    {"[ WiFi ]"},
    {"+ WiFi ON/OFF"},
    {wifi_mode_line},
    {wifi_ssid_line},
    {wifi_ip_line},
    {wifi_pass_line},
    {wifi_rssi_line},
    {"--------------------------"},
    {RESET_WIFI_CRE},
    {WIFI_QR_ITEM},
};
static constexpr uint8_t SET_WIFI_N_OFF = 5;
static constexpr uint8_t SET_WIFI_N_AP = 8;
static constexpr uint8_t SET_WIFI_N_AP_QR = 9;

static inline uint8_t wifi_menu_count_now()
{
    return wifi_menu_count;
}

static inline void wifi_menu_sync()
{
    const bool wifiOn = (ui_param[WIFI_ENABLED] != 0);
    if (!wifiOn)
    {
        std::snprintf(wifi_mode_line, sizeof(wifi_mode_line), "[ WiFi (OFF) ]");
        std::snprintf(wifi_ssid_line, sizeof(wifi_ssid_line), "SSID: -");
        std::snprintf(wifi_ip_line, sizeof(wifi_ip_line), "IP: -");
        std::snprintf(wifi_pass_line, sizeof(wifi_pass_line), "PASS: -");
        wifi_rssi_line[0] = 0;
        wifi_qr_text[0] = 0;

        setting_menu_wifi[2].m_select = wifi_mode_line;
        setting_menu_wifi[3].m_select = "--------------------------";
        setting_menu_wifi[4].m_select = RESET_WIFI_CRE;
        wifi_menu_count = SET_WIFI_N_OFF;
        wifi_reset_row = 4;
        wifi_qr_row = 255;
        return;
    }

    std::snprintf(wifi_mode_line, sizeof(wifi_mode_line), "[ WiFi (AP) ]");
    std::snprintf(wifi_ssid_line, sizeof(wifi_ssid_line), "SSID: ESP32-HOTSPOT");
    std::snprintf(wifi_ip_line, sizeof(wifi_ip_line), "IP: 192.168.4.1:8080");
    std::snprintf(wifi_pass_line, sizeof(wifi_pass_line), "PASS: %s", WIFI_UI_AP_PASS);
    wifi_rssi_line[0] = 0;
    std::snprintf(wifi_qr_text, sizeof(wifi_qr_text), "http://192.168.4.1:8080");

    setting_menu_wifi[2].m_select = wifi_mode_line;
    setting_menu_wifi[3].m_select = wifi_ssid_line;
    setting_menu_wifi[4].m_select = wifi_ip_line;
    setting_menu_wifi[5].m_select = wifi_pass_line;
    setting_menu_wifi[6].m_select = "--------------------------";
    setting_menu_wifi[7].m_select = RESET_WIFI_CRE;
    setting_menu_wifi[8].m_select = WIFI_QR_ITEM;
    wifi_menu_count = SET_WIFI_N_AP_QR;
    wifi_reset_row = 7;
    wifi_qr_row = 8;
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
    pageDraw = PAGE_WIFI;
    wifi_menu_sync();
    uint8_t n = wifi_menu_count_now();
    if (curMenu != setting_menu_wifi || curMenuN != n)
    {
        curMenu = setting_menu_wifi;
        curMenuN = n;
        if (sl.sel >= curMenuN)
            sl.sel = 0;
        if (sl.top >= curMenuN)
            sl.top = 0;
        sl_recalc_targets();
    }
    draw_settings_common();
}

static inline void page_firmware()
{
    pageDraw = PAGE_FIRMWARE;
    curMenu = firmware_menu;
    curMenuN = firmware_menu_count;
    draw_settings_common();
}

static inline void page_firmware_sdcard()
{
    pageDraw = PAGE_FW_SD;
    curMenu = firmware_sd_menu;
    curMenuN = firmware_sd_menu_count;
    draw_settings_common();
}

static inline void page_firmware_sdcard_ver()
{
    pageDraw = PAGE_FW_SD_VER;
    curMenu = firmware_sd_version_menu;
    curMenuN = firmware_sd_version_menu_count;
    draw_settings_common();
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
    if (sel == wifi_reset_row)
        return true;
    if (wifi_qr_row != 255 && sel == wifi_qr_row && wifi_qr_text[0] != 0)
    {
        qr_open(wifi_qr_text, PAGE_WIFI);
        return true;
    }
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
        std::snprintf(buf, bufSize, "%s", ui_param[WIFI_ENABLED] ? "AP" : "OFF");
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
    return (ui_param[WIFI_ENABLED] != 0);
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
