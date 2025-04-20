/*
 * Copyright (c) 2024 wtcat
 *
 * stm32h750
 */


MEMORY
{
  null  (rwx) : ORIGIN = 0x00000000, LENGTH =   0x00000100
  itcm  (rwx) : ORIGIN = 0x00000100, LENGTH =   0x0000ff00
  flash (rx)  : ORIGIN = 0x08000000, LENGTH =  128K
  dtcm  (rwx) : ORIGIN = 0x20000000, LENGTH =  128K
  /*dtcm2 (rwx) : ORIGIN = 0x20010000, LENGTH =   64K*/
  sram  (rwx) : ORIGIN = 0x24000000, LENGTH =  512K
  sram1 (rwx) : ORIGIN = 0x30000000, LENGTH =  256K
  /*sram2 (rwx) : ORIGIN = 0x30020000, LENGTH =  128K*/
  sram3 (rwx) : ORIGIN = 0x30040000, LENGTH =   32K
  sram4 (rwx) : ORIGIN = 0x38000000, LENGTH =   64K
  bbram (rwx) : ORIGIN = 0x38800000, LENGTH =    4K
}

OUTPUT_ARCH(arm)
EXTERN(_vectors)
EXTERN(_stm32_reset)
ENTRY(_stm32_reset)
SECTIONS
{
    .vector :
    {
        *(.vectors)
    } > flash

    .itcm :
    {
        _sramfuncs = .;
        /* optimize kernel */
        /* rtos/threadx/libthreadx.a:(.text*) */

        /* optimize libc */
        /* libc.a: *memset*.o(.text*)
        libc.a: *memcpy*.o(.text*)
        libc.a: *strlen*.o(.text*)
        libc.a: *strcmp*.o(.text*)
        libc.a: *_aeabi_uldivmod*.o(.text*)
        libc.a: *_udivmoddi4*.o(.text*) */

        *(.itcm*)
        . = ALIGN(4);
        _eramfuncs = .;
    } > itcm AT > flash
    _framfuncs = LOADADDR (.itcm);

    .ram_vectors (NOLOAD): 
    {
        *(.ram_vectors)
        _dtcm_data_start = ABSOLUTE(.);
    } > dtcm
    
    .fastbss (NOLOAD) :
    {
        _fsbss = ABSOLUTE(.);
        *(.fastbss*)
        rtos/threadx/libthreadx.a:(.bss*)
        _febss = ABSOLUTE(.);
    } > dtcm
    
    .fastdata :
    {
        _fsdata = ABSOLUTE(.);
        *(.fastdata*)
        _fedata = ABSOLUTE(.);
    } > dtcm AT > flash
    _fdataload = LOADADDR(.fastdata);

    .text :
    {
        . = ALIGN(32);
        _stext = .;
        *(.text .text.*)
        *(.fixup)
        *(.gnu.warning)
        *(.rodata .rodata.*)
        KEEP(*(SORT(.base.roset*)))
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
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array)) 
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
        KEEP(*(SORT(.base.rwset*)))
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

    .dtcm (NOLOAD) :
    {
        *(.dtcm*)
        *(.isr.stack*)
    } > dtcm

    .noinit (NOLOAD):
    {
        *(.noinit*)
    } > sram1

    /* Emit the the D3 power domain section for locating BDMA data
     *
     * Static data with locate_data(".sram4") will be located
     * at start of SRAM4; the rest of SRAM4 will be added to the heap.
     */
    .sram3_reserve (NOLOAD) :
    {
        end = ABSOLUTE(.);
    } > sram3

    .sram4_reserve (NOLOAD) :
    {
        *(.sram4)
        . = ALIGN(4);
        _sram4_heap_start = ABSOLUTE(.);
    } > sram4

    _kernel_byte_pool_start = _sram4_heap_start;
    _kernel_byte_pool_size = ORIGIN(sram4) + LENGTH(sram4) - _sram4_heap_start;

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
