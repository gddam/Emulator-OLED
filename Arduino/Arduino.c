#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif
#include "u8g2.h"
#include <stdio.h>
#include "SDL.h"
#include "Arduino.h"
#include "EmulatorLog.h"


void postLoop();

int main(int argc, char *argv[])
{
    emulator_log("[main] enter argc=%d", argc);
    SDL_SetMainReady();
    emulator_log("[main] SDL_SetMainReady done");
    setup();
    emulator_log("[main] setup done");
    while (1){
        loop();
        postLoop();
    }
}

