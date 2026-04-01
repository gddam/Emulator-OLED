#pragma once

#include <SDL_timer.h>
#include <algorithm>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <string>

#include "Pins.h"
#include "Print.h"
#include "U8g2lib.h"
#include "WouoFix.h"
#include "EmulatorLog.h"

#define PROGMEM
#define F(x) x

using byte = uint8_t;

class String
{
public:
    String() = default;
    String(const char *value) : value_(value ? value : "") {}
    String(const std::string &value) : value_(value) {}
    String(char value) : value_(1, value) {}
    String(int value) : value_(std::to_string(value)) {}
    String(unsigned value) : value_(std::to_string(value)) {}
    String(long value) : value_(std::to_string(value)) {}
    String(unsigned long value) : value_(std::to_string(value)) {}

    const char *c_str() const { return value_.c_str(); }
    size_t length() const { return value_.size(); }
    bool isEmpty() const { return value_.empty(); }

    void toCharArray(char *buffer, size_t len) const
    {
        if (!buffer || len == 0)
            return;
        std::strncpy(buffer, value_.c_str(), len - 1);
        buffer[len - 1] = 0;
    }

    String &operator+=(const char *rhs)
    {
        value_ += rhs ? rhs : "";
        return *this;
    }

    String &operator+=(const String &rhs)
    {
        value_ += rhs.value_;
        return *this;
    }

    String &operator=(const char *rhs)
    {
        value_ = rhs ? rhs : "";
        return *this;
    }

private:
    std::string value_;
};

template <typename T>
inline T min(T a, T b)
{
    return std::min(a, b);
}

template <typename T>
inline T max(T a, T b)
{
    return std::max(a, b);
}

template <typename T>
inline T constrain(T x, T low, T high)
{
    return std::max(low, std::min(x, high));
}

class U8G2_SDL_128X64 : public U8G2, public Print
{
public:
    U8G2_SDL_128X64(const u8g2_cb_t *pStruct, int i, int i1, int i2) : U8G2()
    {
        emulator_log("[ctor] U8G2_SDL_128X64 4-arg");
        u8g2_SetupBuffer_SDL_128x64(&u8g2, &u8g2_cb_r0);
        emulator_log("[ctor] U8G2_SDL_128X64 4-arg setup done");
    }

    U8G2_SDL_128X64(const u8g2_cb_t *pStruct, int i) : U8G2()
    {
        emulator_log("[ctor] U8G2_SDL_128X64 2-arg");
        u8g2_SetupBuffer_SDL_128x64(&u8g2, &u8g2_cb_r0);
        emulator_log("[ctor] U8G2_SDL_128X64 2-arg setup done");
    }

    size_t write(uint8_t v) override
    {
        return U8G2::write(v);
    }

    size_t write(const uint8_t *buffer, size_t size) override
    {
        return U8G2::write(buffer, size);
    }
};

class U8G2_SDL_128X128 : public U8G2, public Print
{
public:
    U8G2_SDL_128X128(const u8g2_cb_t *pStruct, int i, int i1, int i2) : U8G2()
    {
        emulator_log("[ctor] U8G2_SDL_128X128 4-arg");
        u8g2_SetupBuffer_SDL_128x128(&u8g2, &u8g2_cb_r0);
        emulator_log("[ctor] U8G2_SDL_128X128 4-arg setup done");
    }

    size_t write(uint8_t v) override
    {
        return U8G2::write(v);
    }

    size_t write(const uint8_t *buffer, size_t size) override
    {
        return U8G2::write(buffer, size);
    }
};

class SerialClass : public Print
{
public:
    size_t write(uint8_t value) override
    {
        std::cout << static_cast<char>(value);
        return 1;
    }

    size_t write(const uint8_t *buffer, size_t size) override
    {
        std::cout.write(reinterpret_cast<const char *>(buffer), static_cast<std::streamsize>(size));
        return size;
    }

    void begin(int rate)
    {
        emulator_log("[serial] begin rate=%d", rate);
        std::cout << "set fake Serial baud rate " << rate << std::endl;
    }

    int printf(const char *fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        const int written = std::vfprintf(stdout, fmt, args);
        va_end(args);
        return written;
    }
};

extern SerialClass Serial;

class EspClass
{
public:
    uint32_t getFreeHeap() const { return 512u * 1024u; }
};

extern EspClass ESP;

#define delay(ms) SDL_Delay(ms)

long map(long value, long fromLow, long fromHigh, long toLow, long toHigh);

uint32_t millis();
void yield();
void randomSeed(uint32_t seed);
long random(long max);
long random(long min, long max);
void analogReadResolution(int bits);
void analogSetAttenuation(int attenuation);
void analogSetPinAttenuation(uint8_t pin, int attenuation);
uint16_t analogReadMilliVolts(int pin);
uint32_t esp_random();
void tone(uint8_t pin, unsigned int frequency, unsigned long duration = 0);
void noTone(uint8_t pin);
void noInterrupts();
void interrupts();
int analogRead(int pin);

#define LOW 0
#define HIGH 1
#define ADC_11db 0
