/*
 * Copyright (c) 2024 wtcat
 *
 * stm32f334r8
 */


MEMORY
{
  flash (rx)  : ORIGIN = 0x08000000, LENGTH =  64K
  ccm   (rwx) : ORIGIN = 0x10000000, LENGTH =  4K
  sram  (rwx) : ORIGIN = 0x20000000, LENGTH =  12K
}

OUTPUT_ARCH(arm)
EXTERN(_stm32_reset)
ENTRY(_stm32_reset)
SECTIONS
{
    .vector :
    {
        . = ALIGN(256);
        KEEP(*(.vectors*))
    } > flash

    .text :
    {
        _stext = .;
        *(.text .text.*)
        *(.fixup)
        *(.gnu.warning)
        *(.rodata .rodata.*)
        KEEP(*(SORT(.basework.roset*)))
        *(.gnu.linkonce.t.*)
        *(.glue_7)
        *(.glue_7t)
        *(.got)
        *(.gcc_except_table)
        *(.gnu.linkonce.r.*)
        _etext = ABSOLUTE(.);
    } > flash

    .init_section :
    {
        _sinit = ABSOLUTE(.);
        *(.init_array .init_array.*)
        _einit = ABSOLUTE(.);
    } > flash

    .ARM.extab :
    {
        *(.ARM.extab*)
    } > flash

    __exidx_start = ABSOLUTE(.);
    .ARM.exidx :
    {
        *(.ARM.exidx*)
    } > flash
    __exidx_end = ABSOLUTE(.);

    .data :
    {
        _sdata = ABSOLUTE(.);
        *(.data .data.*)
        KEEP(*(SORT(.basework.rwset*)))
        *(.gnu.linkonce.d.*)
        CONSTRUCTORS
        . = ALIGN(4);
        _edata = ABSOLUTE(.);
    } > sram AT > flash

    _eronly = LOADADDR(.data);

    .bss :
    {
        _sbss = ABSOLUTE(.);
        *(.bss .bss.*)
        *(.gnu.linkonce.b.*)
        *(COMMON)
        . = ALIGN(32);
        _ebss = ABSOLUTE(.);
    } > sram

    .fastbss (NOLOAD) :
    {
        _fsbss = ABSOLUTE(.);
        *(.fastbss*)
        rtos/threadx/libthreadx.a:(.bss*)
        _febss = ABSOLUTE(.);
    } > ccm
    
    .fastdata :
    {
        _fsdata = ABSOLUTE(.);
        *(.fastdata*)
        _fedata = ABSOLUTE(.);
    } > ccm AT > flash
    _fdataload = LOADADDR(.fastdata);

    /* _kernel_byte_pool_start = _ebss;
    _kernel_byte_pool_size = ORIGIN(sram) + LENGTH(sram) - _ebss;

    _app_pool_byte_start = _heap_start;
    _app_pool_byte_size  = _heap_end - _heap_start; */

    /* Stabs debugging sections. */

    .stab 0 : { *(.stab) }
    .stabstr 0 : { *(.stabstr) }
    .stab.excl 0 : { *(.stab.excl) }
    .stab.exclstr 0 : { *(.stab.exclstr) }
    .stab.index 0 : { *(.stab.index) }
    .stab.indexstr 0 : { *(.stab.indexstr) }
    .comment 0 : { *(.comment) }
    .debug_abbrev 0 : { *(.debug_abbrev) }
    .debug_info 0 : { *(.debug_info) }
    .debug_line 0 : { *(.debug_line) }
    .debug_pubnames 0 : { *(.debug_pubnames) }
    .debug_aranges 0 : { *(.debug_aranges) }
}
