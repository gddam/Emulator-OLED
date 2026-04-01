#pragma once

#include <Arduino.h>
#include <ctime>

extern uint8_t ui_param[];
extern const uint8_t *FONT_PAGE2;
extern UiU8g2 u8g2;
static inline void useFont(const uint8_t *f);

struct SettingList;
SettingList sl;

struct Weather
{
    int day = 0;
};

struct Notification
{
    int icon = 0;
    String app;
    String title;
    String message;
    String time;
};

struct RemoteTouch
{
    bool state = false;
    int16_t x = 0;
    int16_t y = 0;
};

struct Navigation
{
    bool active = false;
    String directions = "Turn right";
    String title = "Desktop preview";
    String distance = "120 m";
    String eta = "12:34";
    String duration = "02 min";
    uint8_t icon[288] = {0};
};

class ChronosESP32
{
public:
    explicit ChronosESP32(const char *name = "DASAI") : name_(name ? name : "DASAI") {}

    bool isRunning() const { return false; }
    bool isConnected() const { return false; }
    RemoteTouch getTouch() const { return RemoteTouch(); }

    bool is24Hour() const { return is24Hour_; }
    void set24Hour(bool value) { is24Hour_ = value; }

    String getHourZ() const
    {
        std::time_t now = std::time(nullptr);
        std::tm localTm{};
        localtime_s(&localTm, &now);

        char buf[8];
        int hour = localTm.tm_hour;
        if (!is24Hour_)
        {
            hour %= 12;
            if (hour == 0)
                hour = 12;
        }
        std::snprintf(buf, sizeof(buf), "%02d", hour);
        return String(buf);
    }

    String getTime(const char *format) const
    {
        std::time_t now = std::time(nullptr);
        std::tm localTm{};
        localtime_s(&localTm, &now);

        char buf[32];
        std::strftime(buf, sizeof(buf), format ? format : "%H:%M", &localTm);
        return String(buf);
    }

    String getAmPmC(bool caps) const
    {
        if (is24Hour_)
            return String("");

        std::time_t now = std::time(nullptr);
        std::tm localTm{};
        localtime_s(&localTm, &now);
        const bool pm = localTm.tm_hour >= 12;
        return String(pm ? (caps ? "PM" : "pm") : (caps ? "AM" : "am"));
    }

    int getWeatherCount() const { return 1; }

    Weather getWeatherAt(int index) const
    {
        Weather weather;
        weather.day = getDayofWeek();
        return weather;
    }

    int getDayofWeek() const
    {
        std::time_t now = std::time(nullptr);
        std::tm localTm{};
        localtime_s(&localTm, &now);
        return localTm.tm_wday;
    }

private:
    String name_;
    bool is24Hour_ = true;
};

static constexpr const char *BT_DEVICE_NAME = "DASAI";
static constexpr uint8_t BT_PASS_LEN = 6;
static constexpr int16_t BT_ICON_Y_TARGET = 1;
static constexpr int16_t BT_ICON_Y_FROM_TOP = -10;
static constexpr float BT_EASE_POW = 2.0f;
static constexpr uint16_t BT_ICON_ANIM_MS = 500;

#define UI_NAV_LOCK()
#define UI_NAV_UNLOCK()

ChronosESP32 watch(BT_DEVICE_NAME);
bool btRuntimeOn = false;
bool g_connected = false;
bool g_timeSynced = true;
bool g_navActive = false;
uint32_t g_navCrc = 0;
uint8_t g_phoneBat = 87;
bool g_phoneCharging = false;
bool g_phoneBatValid = true;
Navigation g_nav;

MENU setting_menu_bt[] = {
    {"[ Bluetooth ]"},
    {"+ Enable"},
    {"[ Pairing ]"},
};
const uint8_t SET_BT_N = sizeof(setting_menu_bt) / sizeof(setting_menu_bt[0]);

MENU setting_menu_btpass[] = {
    {"[ Pairing ]"},
    {"Press Enter"},
    {"Desktop stub"},
};
const uint8_t SET_BTPASS_N = sizeof(setting_menu_btpass) / sizeof(setting_menu_btpass[0]);

static inline void chronos_mark_dirty()
{
}

static inline void bt_preinit_if_needed()
{
}

static inline void bt_apply_power()
{
    btRuntimeOn = (ui_param[BT_ENABLED] != 0);
    g_connected = btRuntimeOn;
    g_phoneBatValid = btRuntimeOn;
}

static inline void chronos_loop()
{
}

static inline void bt_streaming_hint(bool streaming)
{
}

static inline void bt_security_request_pairing(uint32_t nowMs)
{
}

static inline void bt_security_guard_tick(uint32_t nowMs)
{
}

static inline bool bt_notification_take_snapshot(Notification &out)
{
    return false;
}

static inline void bt_nav_text_snapshot(String &dir, String &title, String &distance, String &ETA, String &duration)
{
    dir = g_nav.directions;
    title = g_nav.title;
    distance = g_nav.distance;
    ETA = g_nav.eta;
    duration = g_nav.duration;
}

static inline void bt_schedule_sync_request(uint32_t nowMs)
{
}

static inline void bt_post_connect_tick(uint32_t nowMs)
{
}

static inline bool btToast_is_active()
{
    return false;
}

static inline void btToast_prepare_for_slide(uint32_t nowMs)
{
}

static inline bool btToast_tick(uint32_t nowMs)
{
    return false;
}

static inline void btToast_draw(uint32_t nowMs)
{
}

static inline bool bt_status_icon_tick(uint32_t nowMs)
{
    return false;
}

static inline void bt_status_icon_draw(uint32_t nowMs, bool forceDraw, int16_t yOffset = 0)
{
}

static inline void btpass_press()
{
}

static inline void btpass_prepare_open_from_list()
{
}

static inline bool btpass_tick(uint32_t nowMs)
{
    return false;
}

static inline void btpass_draw()
{
    useFont(FONT_PAGE2);
    u8g2.setFontPosTop();
    u8g2.drawStr(8, 16, "Desktop pairing");
    u8g2.drawStr(8, 30, "Press Enter");
}
