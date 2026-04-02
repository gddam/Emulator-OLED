#pragma once

#include <stdarg.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef int (*vprintf_like_t)(const char *fmt, va_list ap);

typedef enum
{
    ESP_LOG_NONE = 0,
    ESP_LOG_ERROR = 1,
    ESP_LOG_WARN = 2,
    ESP_LOG_INFO = 3,
    ESP_LOG_DEBUG = 4,
    ESP_LOG_VERBOSE = 5
} esp_log_level_t;

static inline vprintf_like_t esp_log_set_vprintf(vprintf_like_t func)
{
    static vprintf_like_t current = vprintf;
    vprintf_like_t previous = current;
    current = func ? func : vprintf;
    return previous;
}

static inline void esp_log_level_set(const char *tag, esp_log_level_t level)
{
    (void)tag;
    (void)level;
}

#ifdef __cplusplus
}
#endif
