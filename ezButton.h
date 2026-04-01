#pragma once

#include <Arduino.h>

class ezButton
{
public:
    explicit ezButton(int pin)
        : pin_(pin), state_(digitalRead(pin)), lastReading_(state_) {}

    void setDebounceTime(uint32_t debounceMs)
    {
        debounceMs_ = debounceMs;
    }

    void loop()
    {
        const uint32_t now = millis();
        const int raw = digitalRead(pin_);

        if (raw != lastReading_)
        {
            lastDebounceMs_ = now;
            lastReading_ = raw;
        }

        pressedEvent_ = false;
        releasedEvent_ = false;

        if ((now - lastDebounceMs_) >= debounceMs_ && raw != state_)
        {
            const int previous = state_;
            state_ = raw;
            pressedEvent_ = (previous == HIGH && state_ == LOW);
            releasedEvent_ = (previous == LOW && state_ == HIGH);
        }
    }

    uint8_t getState() const
    {
        return static_cast<uint8_t>(state_);
    }

    bool isPressed()
    {
        const bool value = pressedEvent_;
        pressedEvent_ = false;
        return value;
    }

    bool isReleased()
    {
        const bool value = releasedEvent_;
        releasedEvent_ = false;
        return value;
    }

private:
    int pin_;
    uint32_t debounceMs_ = 30;
    uint32_t lastDebounceMs_ = 0;
    int state_;
    int lastReading_;
    bool pressedEvent_ = false;
    bool releasedEvent_ = false;
};
