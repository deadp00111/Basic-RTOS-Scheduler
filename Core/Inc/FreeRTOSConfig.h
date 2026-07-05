#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* ---- Hardware description ----
 * STM32F411CEU6, HSI/PLL configured to 96 MHz SYSCLK in system_stm32f4xx.c/SystemClock_Config
 * (see Core/Src/main.c). SysTick is the FreeRTOS tick source at configTICK_RATE_HZ.
 */
#define configCPU_CLOCK_HZ                      ( 96000000UL )
#define configTICK_RATE_HZ                       ( 1000 )
#define configUSE_PREEMPTION                     1
#define configUSE_TIME_SLICING                   1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION   1
#define configUSE_TICKLESS_IDLE                   0
#define configMAX_PRIORITIES                     ( 6 )
#define configMINIMAL_STACK_SIZE                 ( (unsigned short)128 )
#define configTOTAL_HEAP_SIZE                    ( (size_t)(24*1024) )
#define configMAX_TASK_NAME_LEN                  ( 12 )
#define configUSE_16_BIT_TICKS                   0
#define configIDLE_SHOULD_YIELD                  1
#define configUSE_MUTEXES                        1
#define configUSE_RECURSIVE_MUTEXES              1
#define configUSE_COUNTING_SEMAPHORES            1
#define configQUEUE_REGISTRY_SIZE                8
#define configUSE_QUEUE_SETS                     0
#define configCHECK_FOR_STACK_OVERFLOW           2
#define configUSE_MALLOC_FAILED_HOOK             1
#define configUSE_IDLE_HOOK                      0
#define configUSE_TICK_HOOK                      0
#define configGENERATE_RUN_TIME_STATS            0

/* Software timers -- used for the periodic sample trigger */
#define configUSE_TIMERS                         1
#define configTIMER_TASK_PRIORITY                ( 3 )
#define configTIMER_QUEUE_LENGTH                  10
#define configTIMER_TASK_STACK_DEPTH              configMINIMAL_STACK_SIZE

/* API inclusions */
#define INCLUDE_vTaskPrioritySet                 1
#define INCLUDE_uxTaskPriorityGet                1
#define INCLUDE_vTaskDelete                      1
#define INCLUDE_vTaskSuspend                     1
#define INCLUDE_vTaskDelayUntil                  1
#define INCLUDE_vTaskDelay                       1
#define INCLUDE_xTaskGetSchedulerState           1
#define INCLUDE_xTaskGetCurrentTaskHandle         1
#define INCLUDE_uxTaskGetStackHighWaterMark       1
#define INCLUDE_xTimerPendFunctionCall           1
#define INCLUDE_xQueueGetMutexHolder              1

/* Cortex-M4F interrupt priority config
 * STM32F411 implements 4 priority bits -> 16 levels (0=highest).
 * Interrupts that call FreeRTOS "FromISR" APIs must be numerically >=
 * configLIBRARY_LOWEST_INTERRUPT_PRIORITY - configLIBRARY_KERNEL_INTERRUPT_PRIORITY... i.e.
 * must be within configMAX_SYSCALL_INTERRUPT_PRIORITY and below.
 */
#define configPRIO_BITS                          4
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY   0x0f
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 0x05
#define configKERNEL_INTERRUPT_PRIORITY          ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY     ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )

#define vPortSVCHandler                          SVC_Handler
#define xPortPendSVHandler                       PendSV_Handler
#define xPortSysTickHandler                      SysTick_Handler

#define configASSERT( x ) if( ( x ) == 0 ) { taskDISABLE_INTERRUPTS(); for( ;; ); }

/* Application-defined priorities used across the task files */
#define TASK_PRIO_CLI          ( 2 )
#define TASK_PRIO_SENSOR       ( 4 )
#define TASK_PRIO_LOGGER       ( 3 )
#define TASK_PRIO_DISPLAY      ( 2 )
#define TASK_PRIO_MONITOR      ( 1 )

#endif /* FREERTOS_CONFIG_H */
