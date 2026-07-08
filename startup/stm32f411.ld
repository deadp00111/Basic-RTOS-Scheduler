/*
 * STM32F411CEUx_FLASH.ld
 * -----------------------
 * Linker script for STM32F411CEU6: 512KB flash @ 0x08000000,
 * 128KB SRAM @ 0x20000000. Defines where each section (vector table,
 * code, constants, initialised/zero data) physically lives, and
 * exports the symbols (_sdata, _edata, _sidata, _sbss, _ebss, _estack)
 * that startup_stm32f411ceu6.s uses to copy .data and zero .bss.
 */

ENTRY(Reset_Handler)

/* End of RAM = top of stack (stack grows downward) */
_estack = ORIGIN(RAM) + LENGTH(RAM);

/* Minimum sizes to reserve for stack/heap; the linker will error if
 * .bss + .data + these don't fit in RAM, catching overflow at build time
 * rather than as a mysterious crash on the board. */
_Min_Heap_Size  = 0x200;   /* 512 B */
_Min_Stack_Size = 0x400;   /* 1 KB  */

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}

SECTIONS
{
    /* Vector table must be the very first thing in flash */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    /* Program code and read-only data */
    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    /* ARM unwind tables (needed by some libc builds) */
    .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } > FLASH
    .ARM.exidx :
    {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } > FLASH

    _sidata = LOADADDR(.data);   /* flash address .data is copied from */

    /* Initialised globals: stored in flash, copied to RAM at startup */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT> FLASH

    /* Zero-initialised globals: live only in RAM, cleared at startup */
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM

    /* Reserve space so heap+stack are guaranteed to fit */
    ._user_heap_stack :
    {
        . = ALIGN(8);
        . = . + _Min_Heap_Size;
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
    } > RAM

    /DISCARD/ :
    {
        libc.a ( * )
        libm.a ( * )
        libgcc.a ( * )
    }
}
