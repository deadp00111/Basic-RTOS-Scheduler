

    .syntax unified
    .cpu cortex-m4
    .fpu fpv4-sp-d16
    .thumb


    .section .isr_vector, "a", %progbits
    .type g_pfnVectors, %object
    .size g_pfnVectors, .-g_pfnVectors

g_pfnVectors:
    .word  _estack                  /* 0  : initial stack pointer      */
    .word  Reset_Handler             /* 1  : reset                      */
    .word  NMI_Handler                /* 2  : NMI                        */
    .word  HardFault_Handler          /* 3  : hard fault                 */
    .word  MemManage_Handler          /* 4  : MPU fault                  */
    .word  BusFault_Handler           /* 5  : bus fault                  */
    .word  UsageFault_Handler         /* 6  : usage fault                */
    .word  0                          /* 7  : reserved                   */
    .word  0                          /* 8  : reserved                   */
    .word  0                          /* 9  : reserved                   */
    .word  0                          /* 10 : reserved                   */
    .word  SVC_Handler                /* 11 : SVCall                     */
    .word  DebugMon_Handler           /* 12 : debug monitor              */
    .word  0                          /* 13 : reserved                   */
    .word  PendSV_Handler             /* 14 : PendSV                     */
    .word  SysTick_Handler            /* 15 : SysTick  <-- our scheduler */
    /* IRQ0..IRQx (device interrupts) - not used by this project, but
     * reserve a reasonable block so the linker/debugger vector table
     * size looks normal. Add real entries here (e.g. USART1_IRQHandler)
     * if you extend the project with peripheral interrupts. */
    .word  Default_Handler            /* 16: WWDG                       */
    .word  Default_Handler            /* 17: PVD                        */
    .word  Default_Handler            /* 18: TAMP_STAMP                 */
    .word  Default_Handler            /* 19: RTC_WKUP                   */
    .word  Default_Handler            /* 20: FLASH                      */
    .word  Default_Handler            /* 21: RCC                        */
    .word  Default_Handler            /* 22: EXTI0                      */
    .word  Default_Handler            /* 23: EXTI1                      */
    .word  Default_Handler            /* 24: EXTI2                      */
    .word  Default_Handler            /* 25: EXTI3                      */
    .word  Default_Handler            /* 26: EXTI4                      */
    .word  Default_Handler            /* 27: DMA1_Stream0                */


    .section .text.Reset_Handler
    .weak Reset_Handler
    .type Reset_Handler, %function
Reset_Handler:
    ldr   sp, =_estack           /* set stack pointer (redundant with   */
                                   /* hardware auto-load, but explicit)   */

    /* Copy .data section from flash (LMA) to RAM (VMA) */
    ldr   r0, =_sdata
    ldr   r1, =_edata
    ldr   r2, =_sidata
    movs  r3, #0
    b     LoopCopyDataInit

CopyDataInit:
    ldr   r4, [r2, r3]
    str   r4, [r0, r3]
    adds  r3, r3, #4

LoopCopyDataInit:
    adds  r4, r0, r3
    cmp   r4, r1
    bcc   CopyDataInit

    /* Zero-fill .bss */
    ldr   r2, =_sbss
    ldr   r4, =_ebss
    movs  r3, #0
    b     LoopFillZerobss

FillZerobss:
    str   r3, [r2]
    adds  r2, r2, #4

LoopFillZerobss:
    cmp   r2, r4
    bcc   FillZerobss

    bl    main                    /* jump to application entry point    */

Infinite_Loop:
    b     Infinite_Loop           /* main() should never return, but    */
                                   /* just in case                        */
    .size Reset_Handler, .-Reset_Handler


    .section .text.Default_Handler, "ax", %progbits
Default_Handler:
    b Default_Handler
    .size Default_Handler, .-Default_Handler


    .macro weak_alias name
    .weak \name
    .thumb_set \name, Default_Handler
    .endm

    weak_alias NMI_Handler
    weak_alias HardFault_Handler
    weak_alias MemManage_Handler
    weak_alias BusFault_Handler
    weak_alias UsageFault_Handler
    weak_alias SVC_Handler
    weak_alias DebugMon_Handler
    weak_alias PendSV_Handler