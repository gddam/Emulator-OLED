#include <SDL_events.h>
#include <unordered_map>

#include "Arduino.h"
#include "WouoFix.h"

namespace
{
constexpr int kReleased = HIGH;
constexpr int kPressed = LOW;

std::unordered_map<int, int> &pinStates()
{
    static std::unordered_map<int, int> states;
    return states;
}

void setPinState(int pin, bool pressed)
{
    pinStates()[pin] = pressed ? kPressed : kReleased;
}

void applyInput(SDL_Keycode key, bool pressed)
{
    switch (key)
    {
    case SDLK_UP:
    case SDLK_LEFT:
        setPinState(CFG_THUMB_CW_PIN, pressed);
        break;
    case SDLK_DOWN:
    case SDLK_RIGHT:
        setPinState(CFG_THUMB_CCW_PIN, pressed);
        break;
    case SDLK_RETURN:
    case SDLK_SPACE:
        setPinState(CFG_BTN_PIN, pressed);
        break;
    default:
        break;
    }
}

void pumpInput()
{
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0)
    {
        switch (event.type)
        {
        case SDL_QUIT:
            exit(0);
            break;
        case SDL_KEYDOWN:
            if (event.key.repeat == 0)
                applyInput(event.key.keysym.sym, true);
            break;
        case SDL_KEYUP:
            applyInput(event.key.keysym.sym, false);
            break;
        default:
            break;
        }
    }
}
} // namespace

extern "C" void postLoop()
{
    pumpInput();
    delay(10);
}

int digitalRead(int pin)
{
    pumpInput();

    auto &states = pinStates();
    auto it = states.find(pin);
    if (it == states.end())
        return kReleased;

    return it->second;
}
