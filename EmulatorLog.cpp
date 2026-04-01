#include "EmulatorLog.h"

#include <cstdarg>
#include <cstdio>
#include <mutex>

namespace
{
std::mutex &log_mutex()
{
    static std::mutex m;
    return m;
}

FILE *log_file()
{
    static FILE *f = nullptr;
    static bool opened = false;

    if (!opened)
    {
        opened = true;
        f = std::fopen("WouoUI.log", "a");
    }

    return f;
}
} // namespace

extern "C" void emulator_log(const char *fmt, ...)
{
    std::lock_guard<std::mutex> lock(log_mutex());
    FILE *f = log_file();
    if (!f)
        return;

    va_list args;
    va_start(args, fmt);
    std::vfprintf(f, fmt, args);
    va_end(args);
    std::fputc('\n', f);
    std::fflush(f);
}
