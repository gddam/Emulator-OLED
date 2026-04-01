#pragma once

#include <Arduino.h>

enum wl_status_t
{
    WL_IDLE_STATUS = 0,
    WL_CONNECTED = 3,
    WL_DISCONNECTED = 6,
};

class IPAddress
{
public:
    IPAddress(uint8_t a = 127, uint8_t b = 0, uint8_t c = 0, uint8_t d = 1)
        : a_(a), b_(b), c_(c), d_(d) {}

    String toString() const
    {
        char buf[20];
        std::snprintf(buf, sizeof(buf), "%u.%u.%u.%u", a_, b_, c_, d_);
        return String(buf);
    }

private:
    uint8_t a_;
    uint8_t b_;
    uint8_t c_;
    uint8_t d_;
};

class WiFiClass
{
public:
    wl_status_t status() const { return WL_DISCONNECTED; }
    IPAddress localIP() const { return IPAddress(); }
    IPAddress softAPIP() const { return IPAddress(); }
    String SSID() const { return String("Emulator"); }
};

inline WiFiClass WiFi;
