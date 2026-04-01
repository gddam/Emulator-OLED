#include <cmath>
#include <cstdlib>

#include "ArduinoCompatible.h"

SerialClass Serial;
EspClass ESP;

long map(long value, long fromLow, long fromHigh, long toLow, long toHigh)
{
    return (value - fromLow) * (toHigh - toLow) / (fromHigh - fromLow) + toLow;
}

uint32_t millis()
{
    return SDL_GetTicks();
}

void yield()
{
    SDL_Delay(0);
}

void randomSeed(uint32_t seed)
{
    std::srand(seed);
}

long random(long max)
{
    if (max <= 0)
        return 0;
    return std::rand() % max;
}

long random(long min, long max)
{
    if (max <= min)
        return min;
    return min + random(max - min);
}

void analogReadResolution(int bits)
{
}

void analogSetAttenuation(int attenuation)
{
}

void analogSetPinAttenuation(uint8_t pin, int attenuation)
{
}

uint16_t analogReadMilliVolts(int pin)
{
    const int raw = analogRead(pin);
    return static_cast<uint16_t>((raw * 3300) / 1023);
}

uint32_t esp_random()
{
    return static_cast<uint32_t>(std::rand());
}

void tone(uint8_t pin, unsigned int frequency, unsigned long duration)
{
}

void noTone(uint8_t pin)
{
}

void noInterrupts()
{
}

void interrupts()
{
}

int analogRead(int pin)
{
    static float phase = 0.0f;
    phase += 1.0f;
    return static_cast<int>(std::sin(0.1f * phase) * 511.0f + 512.0f);
}
