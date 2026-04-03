#pragma once

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include <Arduino.h>
#include <ctime>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <deque>

extern uint8_t ui_param[];
extern const uint8_t *FONT_PAGE2;
extern const uint8_t *FONT_PAGE3;
extern UiU8g2 u8g2;
extern bool pageDirty;
extern uint8_t pageNow;
static inline void useFont(const uint8_t *f);
static inline void draw_settings_common();
static inline void sl_recalc_targets();
static inline void chronos_mark_dirty();

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
    uint32_t iconCRC = 0;
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

static constexpr uint16_t BT_BRIDGE_UDP_PORT = 9233;
static constexpr uint32_t BT_BRIDGE_CONN_TIMEOUT_MS = 7000;
static uint32_t g_btBridgeLastRxMs = 0;
static std::deque<Notification> g_btNotifQueue;
static uint8_t g_navIconPending[288] = {0};
static uint32_t g_navIconAssembleCrc = 0;
static uint8_t g_navIconAssembleMask = 0;

#if defined(_WIN32)
namespace bt_bridge
{
static bool g_wsaStarted = false;
static bool g_socketReady = false;
static SOCKET g_socket = INVALID_SOCKET;

static inline void close_socket()
{
    if (g_socket != INVALID_SOCKET)
    {
        closesocket(g_socket);
        g_socket = INVALID_SOCKET;
    }
    g_socketReady = false;
}

static inline bool init_socket()
{
    if (g_socketReady)
        return true;

    if (!g_wsaStarted)
    {
        WSADATA wsaData{};
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
            return false;
        g_wsaStarted = true;
    }

    g_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (g_socket == INVALID_SOCKET)
        return false;

    u_long nonBlocking = 1;
    if (ioctlsocket(g_socket, FIONBIO, &nonBlocking) != 0)
    {
        close_socket();
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(BT_BRIDGE_UDP_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (bind(g_socket, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) != 0)
    {
        close_socket();
        return false;
    }

    g_socketReady = true;
    Serial.printf("[BT-EMU] UDP bridge ready on 127.0.0.1:%u\n", (unsigned)BT_BRIDGE_UDP_PORT);
    return true;
}
} // namespace bt_bridge
#endif

static inline int bt_split_fields(char *line, char **out, int maxFields)
{
    if (!line || !out || maxFields <= 0)
        return 0;

    int count = 0;
    char *p = line;
    out[count++] = p;

    while (*p && count < maxFields)
    {
        if (*p == '|')
        {
            *p = 0;
            out[count++] = p + 1;
        }
        p++;
    }

    return count;
}

static inline void bt_trim_line(char *s)
{
    if (!s)
        return;

    size_t n = std::strlen(s);
    while (n > 0 && (s[n - 1] == '\r' || s[n - 1] == '\n' || s[n - 1] == ' ' || s[n - 1] == '\t'))
    {
        s[n - 1] = 0;
        n--;
    }
}

static inline void bt_push_notification(int icon, const char *app, const char *title, const char *msg, const char *time)
{
    Notification n{};
    n.icon = icon;
    n.app = app ? app : "";
    n.title = title ? title : "";
    n.message = msg ? msg : "";
    n.time = time ? time : "";

    g_btNotifQueue.push_back(n);
    while (g_btNotifQueue.size() > 12)
        g_btNotifQueue.pop_front();
}

static inline int bt_hex_nibble(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F')
        return 10 + (c - 'A');
    return -1;
}

static inline bool bt_parse_hex_bytes(const char *hex, uint8_t *out, size_t outLen)
{
    if (!hex || !out)
        return false;
    const size_t hexLen = std::strlen(hex);
    if (hexLen != outLen * 2)
        return false;

    for (size_t i = 0; i < outLen; i++)
    {
        const int hi = bt_hex_nibble(hex[i * 2]);
        const int lo = bt_hex_nibble(hex[i * 2 + 1]);
        if (hi < 0 || lo < 0)
            return false;
        out[i] = static_cast<uint8_t>((hi << 4) | lo);
    }
    return true;
}

static inline void bt_apply_bridge_line(char *line)
{
    if (!line || !line[0])
        return;

    bt_trim_line(line);
    if (!line[0])
        return;

    char *f[8] = {nullptr};
    int n = bt_split_fields(line, f, 8);
    if (n <= 0 || !f[0])
        return;

    if (std::strcmp(f[0], "CONN") == 0 && n >= 2)
    {
        g_connected = (std::atoi(f[1]) != 0);
        if (!g_connected)
        {
            g_navActive = false;
            g_nav.active = false;
            g_nav.iconCRC = 0;
            memset(g_nav.icon, 0, sizeof(g_nav.icon));
            memset(g_navIconPending, 0, sizeof(g_navIconPending));
            g_navIconAssembleMask = 0;
        }
        chronos_mark_dirty();
        return;
    }

    if (std::strcmp(f[0], "TIME") == 0 && n >= 2)
    {
        g_timeSynced = (std::atoi(f[1]) != 0);
        chronos_mark_dirty();
        return;
    }

    if (std::strcmp(f[0], "PBAT") == 0 && n >= 3)
    {
        int level = std::atoi(f[1]);
        if (level < 0)
            level = 0;
        if (level > 100)
            level = 100;
        g_phoneBat = static_cast<uint8_t>(level);
        g_phoneCharging = (std::atoi(f[2]) != 0);
        g_phoneBatValid = true;
        chronos_mark_dirty();
        return;
    }

    if (std::strcmp(f[0], "NAV") == 0 && n >= 7)
    {
        g_navActive = (std::atoi(f[1]) != 0);
        g_nav.active = g_navActive;
        g_nav.title = f[2] ? f[2] : "";
        g_nav.directions = f[3] ? f[3] : "";
        g_nav.distance = f[4] ? f[4] : "";
        g_nav.eta = f[5] ? f[5] : "";
        g_nav.duration = f[6] ? f[6] : "";
        chronos_mark_dirty();
        return;
    }

    if (std::strcmp(f[0], "NAVICON") == 0 && n >= 4)
    {
        const int pos = std::atoi(f[1] ? f[1] : "0");
        if (pos < 0 || pos > 2)
            return;

        uint8_t chunk[96];
        if (!bt_parse_hex_bytes(f[3], chunk, sizeof(chunk)))
            return;

        const uint32_t crc = static_cast<uint32_t>(std::strtoul(f[2] ? f[2] : "0", nullptr, 16));

        // Saat CRC baru datang, mulai icon baru dari buffer kosong agar tidak
        // tercampur sisa chunk icon sebelumnya.
        if (crc != g_navIconAssembleCrc)
        {
            g_navIconAssembleCrc = crc;
            g_navIconAssembleMask = 0;
            memset(g_navIconPending, 0, sizeof(g_navIconPending));
        }

        const size_t offset = static_cast<size_t>(pos) * sizeof(chunk);
        memcpy(g_navIconPending + offset, chunk, sizeof(chunk));

        // Satu icon nav dikirim dalam 3 potongan (pos 0..2).
        // Terapkan CRC/icon baru hanya saat semua potongan untuk CRC ini sudah lengkap,
        // supaya render tidak "setengah icon".
        g_navIconAssembleMask |= static_cast<uint8_t>(1u << static_cast<unsigned>(pos));

        if (g_navIconAssembleMask == 0x07u)
        {
            memcpy(g_nav.icon, g_navIconPending, sizeof(g_nav.icon));
            g_navCrc = crc;
            g_nav.iconCRC = crc;
            g_navIconAssembleMask = 0;
            chronos_mark_dirty();
        }
        return;
    }

    if (std::strcmp(f[0], "NAVICONFULL") == 0 && n >= 3)
    {
        uint8_t fullIcon[288];
        if (!bt_parse_hex_bytes(f[2], fullIcon, sizeof(fullIcon)))
            return;

        const uint32_t crc = static_cast<uint32_t>(std::strtoul(f[1] ? f[1] : "0", nullptr, 16));
        memcpy(g_nav.icon, fullIcon, sizeof(fullIcon));
        memcpy(g_navIconPending, fullIcon, sizeof(fullIcon));
        g_navCrc = crc;
        g_nav.iconCRC = crc;
        g_navIconAssembleMask = 0;
        g_navIconAssembleCrc = crc;
        chronos_mark_dirty();
        return;
    }

    if (std::strcmp(f[0], "NOTIF") == 0 && n >= 6)
    {
        const int icon = std::atoi(f[1] ? f[1] : "0");
        bt_push_notification(icon, f[2], f[3], f[4], f[5]);
        chronos_mark_dirty();
        return;
    }
}

static inline void bt_bridge_poll_udp()
{
#if defined(_WIN32)
    if (!bt_bridge::init_socket())
        return;

    char buf[2048];
    for (;;)
    {
        sockaddr_in from{};
        int fromLen = sizeof(from);
        const int rc = recvfrom(bt_bridge::g_socket, buf, sizeof(buf) - 1, 0, reinterpret_cast<sockaddr *>(&from), &fromLen);
        if (rc <= 0)
        {
            const int err = WSAGetLastError();
            if (err == WSAEWOULDBLOCK)
                break;
            break;
        }

        buf[rc] = 0;
        g_btBridgeLastRxMs = millis();

        char *line = buf;
        while (line && *line)
        {
            char *next = std::strchr(line, '\n');
            if (next)
            {
                *next = 0;
                next++;
            }
            bt_apply_bridge_line(line);
            line = next;
        }
    }
#endif
}

MENU setting_menu_bt[] = {
    {"[ Bluetooth ]"},
    {"+ Bluetooth ON/OFF"},
    {"[ Bluetooth PIN ]"},
};
const uint8_t SET_BT_N = sizeof(setting_menu_bt) / sizeof(setting_menu_bt[0]);

extern char bt_pass[BT_PASS_LEN + 1];
char BT_PIN_LINE[24] = "BT PIN : OFF";
char NUMERIC_PIN[7] = "------";

MENU setting_menu_btpass[] = {
    {"[ Bluetooth PIN ]"},
    {BT_PIN_LINE},
    {NUMERIC_PIN},
};
const uint8_t SET_BTPASS_N = sizeof(setting_menu_btpass) / sizeof(setting_menu_btpass[0]);

static inline void btpass_sync_menu_text()
{
    bool hasSpace = false;
    for (uint8_t i = 0; i < BT_PASS_LEN; i++)
    {
        char c = bt_pass[i];
        if (c < '0' || c > '9')
            c = ' ';
        if (c == ' ')
            hasSpace = true;
        NUMERIC_PIN[i] = (c == ' ') ? '-' : c;
    }
    NUMERIC_PIN[BT_PASS_LEN] = 0;

    if (hasSpace)
    {
        std::snprintf(BT_PIN_LINE, sizeof(BT_PIN_LINE), "BT PIN : OFF");
    }
    else
    {
        std::snprintf(BT_PIN_LINE, sizeof(BT_PIN_LINE), "BT PIN : %s", bt_pass);
    }
}

static inline void chronos_mark_dirty()
{
    if (pageNow == PAGE_NAVIGATION || pageNow == PAGE_DASAI || pageNow == PAGE_TIME)
        pageDirty = true;
}

static inline void bt_preinit_if_needed()
{
}

static inline void bt_apply_power()
{
    const bool wantOn = (ui_param[BT_ENABLED] != 0);
    if (wantOn == btRuntimeOn)
        return;

    btRuntimeOn = wantOn;
    if (!btRuntimeOn)
    {
        g_connected = false;
        g_navActive = false;
        g_phoneBatValid = false;
        g_btNotifQueue.clear();
    }
    else
    {
        g_btBridgeLastRxMs = millis();
        bt_bridge_poll_udp();
    }
}

static inline void chronos_loop()
{
    if (!btRuntimeOn)
        return;

    bt_bridge_poll_udp();
    const uint32_t now = millis();
    if ((uint32_t)(now - g_btBridgeLastRxMs) > BT_BRIDGE_CONN_TIMEOUT_MS)
    {
        g_connected = false;
        g_navActive = false;
        g_nav.iconCRC = 0;
    }
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
    if (g_btNotifQueue.empty())
        return false;

    out = g_btNotifQueue.front();
    g_btNotifQueue.pop_front();
    return true;
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
    btpass_sync_menu_text();
    draw_settings_common();
}
