#pragma once

#include <stdint.h>

#include "portmacro.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef int BaseType_t;
typedef unsigned int UBaseType_t;
typedef uint32_t TickType_t;
typedef void *TaskHandle_t;
typedef void (*TaskFunction_t)(void *);

#define pdFALSE 0
#define pdTRUE 1
#define pdPASS 1

#define pdMS_TO_TICKS(ms) ((TickType_t)(ms))
#define portMAX_DELAY ((TickType_t)0xFFFFFFFFu)

#ifdef __cplusplus
}
#endif
