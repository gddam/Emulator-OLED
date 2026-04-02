#pragma once

#include <Arduino.h>

#include "FreeRTOS.h"

#ifdef __cplusplus
extern "C"
{
#endif

static inline BaseType_t xTaskCreatePinnedToCore(TaskFunction_t taskFn,
                                                 const char *name,
                                                 uint32_t stackDepth,
                                                 void *params,
                                                 UBaseType_t priority,
                                                 TaskHandle_t *taskHandle,
                                                 BaseType_t coreId)
{
    (void)name;
    (void)stackDepth;
    (void)priority;
    (void)coreId;

    if (taskHandle)
        *taskHandle = (TaskHandle_t)1;

    if (taskFn)
        taskFn(params);

    return pdPASS;
}

static inline void vTaskDelete(TaskHandle_t taskHandle)
{
    (void)taskHandle;
}

static inline void vTaskDelay(TickType_t ticks)
{
    delay((uint32_t)ticks);
}

#ifdef __cplusplus
}
#endif
