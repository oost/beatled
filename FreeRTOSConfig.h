#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* SMP Configuration — RP2040 only */
#ifdef PICO_PORT
#define configNUMBER_OF_CORES 2
#define configUSE_CORE_AFFINITY 1
#else
#define configNUMBER_OF_CORES 1
#define configUSE_CORE_AFFINITY 0
#endif

/* Scheduler */
#define configUSE_PREEMPTION 1
#define configUSE_TIME_SLICING 1
#define configTICK_RATE_HZ ((TickType_t)1000)
#define configUSE_16_BIT_TICKS 0
#define configIDLE_SHOULD_YIELD 1

/* Memory */
#define configMINIMAL_STACK_SIZE ((configSTACK_DEPTH_TYPE)512)
#define configTOTAL_HEAP_SIZE (65 * 1024)
#define configSUPPORT_STATIC_ALLOCATION 0
#define configSUPPORT_DYNAMIC_ALLOCATION 1
#define configSTACK_DEPTH_TYPE uint32_t

/* Task features */
#define configMAX_PRIORITIES 8
#define configMAX_TASK_NAME_LEN 16
#define configUSE_MUTEXES 1
#define configUSE_RECURSIVE_MUTEXES 1
#define configUSE_COUNTING_SEMAPHORES 1
#define configUSE_TASK_NOTIFICATIONS 1
#define configQUEUE_REGISTRY_SIZE 8

/* Timer */
#define configUSE_TIMERS 1
#define configTIMER_TASK_PRIORITY (configMAX_PRIORITIES - 1)
#define configTIMER_QUEUE_LENGTH 10
#define configTIMER_TASK_STACK_DEPTH 1024

/* Hook functions */
#define configUSE_IDLE_HOOK 0
#define configUSE_PASSIVE_IDLE_HOOK 0
#define configUSE_TICK_HOOK 0
#define configUSE_MALLOC_FAILED_HOOK 0
#define configCHECK_FOR_STACK_OVERFLOW 2

/* Runtime stats */
#define configUSE_TRACE_FACILITY 1
#define configGENERATE_RUN_TIME_STATS 0

/* Co-routines (unused) */
#define configUSE_CO_ROUTINES 0

/* Optional function includes */
#define INCLUDE_vTaskPrioritySet 1
#define INCLUDE_uxTaskPriorityGet 1
#define INCLUDE_vTaskDelete 1
#define INCLUDE_vTaskSuspend 1
#define INCLUDE_vTaskDelay 1
#define INCLUDE_vTaskDelayUntil 1
#define INCLUDE_xTaskGetSchedulerState 1
#define INCLUDE_xTaskGetCurrentTaskHandle 1
#define INCLUDE_uxTaskGetStackHighWaterMark 1
#define INCLUDE_xSemaphoreGetMutexHolder 1
#define INCLUDE_xTimerPendFunctionCall 1

/* RP2040 specific */
#ifdef PICO_PORT
#define configSUPPORT_PICO_SYNC_INTEROP 1
#define configSUPPORT_PICO_TIME_INTEROP 1
#include "rp2040_config.h"
#endif

#endif /* FREERTOS_CONFIG_H */
