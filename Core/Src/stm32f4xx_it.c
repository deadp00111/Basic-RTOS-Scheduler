#include "stm32f4xx.h"

/* SVC_Handler, PendSV_Handler and SysTick_Handler are provided by the FreeRTOS
 * ARM_CM4F port (port.c) via the vPortSVCHandler/xPortPendSVHandler/
 * xPortSysTickHandler #define aliases in FreeRTOSConfig.h - do not define them here.
 */

void NMI_Handler(void)
{
    for (;;) { }
}

void HardFault_Handler(void)
{
    for (;;) { }
}

void MemManage_Handler(void)
{
    for (;;) { }
}

void BusFault_Handler(void)
{
    for (;;) { }
}

void UsageFault_Handler(void)
{
    for (;;) { }
}

void DebugMon_Handler(void)
{
    /* leave running - useful for attaching a debugger */
}
