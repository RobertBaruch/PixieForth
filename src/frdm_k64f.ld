/* 
 * The entry point is the ResetHandler function. In the C file, we defined
 * this as being in the .startup input section.
 */
ENTRY(ResetHandler)

/* 
 * The memory allocatable by the user. If you define __heap_size__ when running
 * the linker (via -Xlinker -defsym=__heap_size__=somevalue), you can override this.
 * Note that you can also define, in C, an extern uint32_t _sheap which tells you 
 * where the start of the heap ended up, and _eheap is the end of your heap.
 */ 
HEAP_SIZE = DEFINED(__heap_size__) ? __heap_size__  : 0x0400;
 
/* 
 * How memory is laid out. Note that we don't differentiate 
 * between SRAM_L (the first 64K) and SRAM_U. 
 */
MEMORY
{
    INTERRUPTS   (rx) : ORIGIN = 0x00000000, LENGTH = 0x00000400
    FLASH_CONFIG (rx) : ORIGIN = 0x00000400, LENGTH = 0x00000010 
	FLASH        (rx) : ORIGIN = 0x00000410, LENGTH = 0x000FFBF0
	SRAM        (rwx) : ORIGIN = 0x1FFF0000, LENGTH = 256K
}

/* 
 * How input sections get mapped onto output sections, and where in memory 
 * they go. Input sections marked as KEEP are immune to being removed by
 * the link-time unused section detector (the -gc-sections option to linker). 
 */
SECTIONS
{
    .interrupts : {
        . = 0;
        KEEP(*(.vectors))
        . = ALIGN(4);
    } > INTERRUPTS
    
    .startup : {
        . = ALIGN(4);
        KEEP(*(.startup*))
        . = ALIGN(4);
        __STARTUP_END = .;
    } > INTERRUPTS
    
    ASSERT(__STARTUP_END <= ORIGIN(FLASH_CONFIG), "startup overflowed into flash config region")

    .flash_config : {
        KEEP(*(.flashconfig*))
    } > FLASH_CONFIG
    
    .text : {
        . = ALIGN(4);
        _stext = .; /* Beginning of text. */
        *(.text*)
    } > FLASH
    
    .rodata : {
        *(.rodata*)
    } > FLASH
    
    .init : {
        . = ALIGN(4);
        KEEP(*(.init))
        . = ALIGN(4);
        __preinit_array_start = .;
        KEEP (*(.preinit_array))
        __preinit_array_end = .;
        __init_array_start = .;
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array))
        __init_array_end = .;
    } > FLASH = 0xFF
    
    /* 
     * Section containing exception unwinding information.
     * See http://infocenter.arm.com/help/topic/com.arm.doc.ihi0044f/IHI0044F_aaelf.pdf
     */ 
    .ARM.extab : {
        __extab_start = .;
        *(.ARM.extab* .gnu.linkonce.armextab.*)
        __extab_end = .;
    } > FLASH

    /* 
     * Section containing index entries for section unwinding.
     * See http://infocenter.arm.com/help/topic/com.arm.doc.ihi0044f/IHI0044F_aaelf.pdf
     */ 
	.ARM.exidx : {
		__exidx_start = .;
		*(.ARM.exidx* .gnu.linkonce.armexidx.*)
		__exidx_end = .;
	} > FLASH
	
	_etext = .; /* End of text. */
	
    ASSERT(_etext <= ORIGIN(SRAM), "text overflowed into SRAM region")

    /*
     * The following sections are marked NOLOAD so that they are not loaded
     * into memory, meaning they are not initialized from the object file.
     */
	.usbdescriptortable (NOLOAD) : {
		. = ALIGN(512);
		*(.usbdescriptortable*)
	} > SRAM

	.dmabuffers (NOLOAD) : {
		. = ALIGN(4);
		*(.dmabuffers*)
	} > SRAM

	.usbbuffers (NOLOAD) : {
		. = ALIGN(4);
		*(.usbbuffers*)
	} > SRAM

    /*
     * The initialized data section. It is placed after _etext in the file. The program
     * must copy this data out of flash (at _etext) into memory (at _sdata).
     */
	.data : AT (_etext) {
		. = ALIGN(4);
		_sdata = .; /* Beginning of data. */
		*(.fastrun*)
		*(.data*)
		. = ALIGN(4);
		_edata = .;  /* End of data. */
	} > SRAM
	
	ASSERT(_etext + (_edata - _sdata) <= ORIGIN(SRAM), "initialized data section overflowed into SRAM region")

    /* Uninitialized data */
	.noinit (NOLOAD) : {
		*(.noinit*)
	} > SRAM

    /* More uninitialized data */
	.bss (NOLOAD) : {
		. = ALIGN(4);
		_sbss = .;
		__bss_start__ = .;
		*(.bss*)
		*(COMMON)
		. = ALIGN(4);
		_ebss = .;
		__bss_end = .;
		__bss_end__ = .;
	} > SRAM

    /* Allocatable user memory */
    .heap (NOLOAD) : {
        . = ALIGN(4);
        _sheap = .;
        . += HEAP_SIZE; /* Defined at top of this file. */
        _eheap = .;
    } > SRAM
    
    /* 
     * Stack starts at the end of SRAM and grows upwards. Because there's no
     * way to tell whether the stack grows beyond its bounds, your program will
     * likely crash when that happens. An easy way to crash is to have a function
     * recursively and infinitely call itself.
     */    
	_estack = ORIGIN(SRAM) + LENGTH(SRAM);

    ASSERT(_eheap <= _estack, "heap overflowed SRAM, leaving no room for stack")
}




