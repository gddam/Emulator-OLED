#pragma once

#include "FreeRTOS.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef void *SemaphoreHandle_t;

static inline SemaphoreHandle_t xSemaphoreCreateMutex()
{
    return (SemaphoreHandle_t)1;
}

static inline BaseType_t xSemaphoreTake(SemaphoreHandle_t sem, TickType_t ticksToWait)
{
    (void)sem;
    (void)ticksToWait;
    return pdTRUE;
}

static inline BaseType_t xSemaphoreGive(SemaphoreHandle_t sem)
{
    (void)sem;
    return pdTRUE;
}

#ifdef __cplusplus
}
#endif
