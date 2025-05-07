// Copyright (c) Konstantin Belyalov. All rights reserved.
// Licensed under the MIT license.
// https://github.com/belyalov/stm32-hal-libraries

#ifndef __DEBUG_H
#define __DEBUG_H

#include "main.h"
#include <string.h>
#include <errno.h>
#include <stdint.h>

#define DEBUG_UART huart1

#ifndef DEBUG_UART_TIMEOUT
#define DEBUG_UART_TIMEOUT 100
#endif

#define DEBUG_MUTEX uart_mutex

#ifdef __cplusplus
#define EXPORT extern "C"
#else
#define EXPORT
#endif

#ifndef DEBUG_BUFFER_SIZE
#define DEBUG_BUFFER_SIZE 17
#endif

// Convenient macros to use uart defined in DEBUG_UART to send debug messages to
#ifdef DEBUG_UART
extern UART_HandleTypeDef DEBUG_UART;
// Optional mutex when using with FreeRTOS
#ifdef DEBUG_MUTEX

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

extern SemaphoreHandle_t DEBUG_MUTEX;
#define DEBUG_UART_LOCK    xSemaphoreTake(DEBUG_MUTEX, pdMS_TO_TICKS(256))
#define DEBUG_UART_UNLOCK  xSemaphoreGive(DEBUG_MUTEX)
#else
#define DEBUG_UART_LOCK
#define DEBUG_UART_UNLOCK
#endif
#define DEBUGP(s)                   do { DEBUG_UART_LOCK; debug_print_str(&DEBUG_UART, s); DEBUG_UART_UNLOCK; } while(0);
#define DEBUGLN(s)                 do { DEBUG_UART_LOCK; debug_print_strln(&DEBUG_UART, s); DEBUG_UART_UNLOCK; } while(0);
#define DEBUG_STR_LN(s1, s2)       do { DEBUG_UART_LOCK; debug_print_strstrln(&DEBUG_UART, s1, s2); DEBUG_UART_UNLOCK; } while(0);
#define DEBUG_UINT(s, v)           do { DEBUG_UART_LOCK; debug_print_uint64(&DEBUG_UART, s, v); DEBUG_UART_UNLOCK; } while(0);
#define DEBUG_UINT_LN(s, v)        do { DEBUG_UART_LOCK; debug_print_uint64ln(&DEBUG_UART, s, v); DEBUG_UART_UNLOCK; } while(0);
#define DEBUG_INT(s, v)            do { DEBUG_UART_LOCK; debug_print_int64(&DEBUG_UART, s, v); DEBUG_UART_UNLOCK; } while(0);
#define DEBUG_INT_LN(s, v)         do { DEBUG_UART_LOCK; debug_print_int64ln(&DEBUG_UART, s, v); DEBUG_UART_UNLOCK; } while(0);
#define DEBUG_UINT_HEX(s, v)       do { DEBUG_UART_LOCK; debug_print_hex64(&DEBUG_UART, s, v); DEBUG_UART_UNLOCK; } while(0);
#define DEBUG_UINT_HEXLN(s, v)     do { DEBUG_UART_LOCK; debug_print_hex64ln(&DEBUG_UART, s, v); DEBUG_UART_UNLOCK; } while(0);
#endif

EXPORT void debug_print_str(UART_HandleTypeDef *uart, const char *msg);
EXPORT void debug_print_strln(UART_HandleTypeDef *uart, const char *msg);
EXPORT void debug_print_strstrln(UART_HandleTypeDef *uart, const char *msg1, const char *msg2);
EXPORT void debug_print_uint64(UART_HandleTypeDef *uart, const char *msg, uint64_t val);
EXPORT void debug_print_uint64ln(UART_HandleTypeDef *uart, const char *msg, uint64_t val);
EXPORT void debug_print_int64(UART_HandleTypeDef *uart, const char *msg, int64_t val);
EXPORT void debug_print_int64ln(UART_HandleTypeDef *uart, const char *msg, int64_t val);
EXPORT void debug_print_hex64(UART_HandleTypeDef *uart, const char *msg, uint64_t val);
EXPORT void debug_print_hex64ln(UART_HandleTypeDef *uart, const char *msg, uint64_t val);

// Converts (un)signed int64 into string.
// Returns pointer to first symbol within this buffer.
EXPORT uint8_t *debug_uint64_to_string(uint64_t value, uint8_t *buf, uint8_t len);
EXPORT uint8_t *debug_int64_to_string(int64_t value, uint8_t *buf, uint8_t len);
EXPORT uint8_t *debug_uint64_to_hexstring(uint64_t value, uint8_t *buf, uint8_t len);

#define PRINT_TASK_INFO()                                                           \
    do {                                                                            \
        char *taskListBuffer;                                                       \
        const int bufferSize = 512;                                                 \
        taskListBuffer = (char*)pvPortMalloc(bufferSize);                                  \
        if (taskListBuffer != NULL) {                                               \
            vTaskList(taskListBuffer);                                              \
            DEBUGLN("Task Name\tState\tPrio\tStack");                          \
            DEBUGLN("-------------------------------------------------");           \
            DEBUGLN(taskListBuffer);                                                \
            vPortFree(taskListBuffer);                                              \
        } else {                                                                    \
            DEBUGLN("Failed to allocate memory for task info.");                    \
        }                                                                           \
    } while (0)


#endif
