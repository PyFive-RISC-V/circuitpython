MEMORY
{
    BRAM  (rwx) : ORIGIN = 0x00000400, LENGTH = 0x0400
    FLASH (rx)  : ORIGIN = 0x40100000, LENGTH = 0x100000    /* 1024k */
    PSRAM (rwx) : ORIGIN = 0x41000000, LENGTH = 0x800000    /* 8192k */	/* FIXME */
}
ENTRY(_start)
SECTIONS {
    _estack = ORIGIN(PSRAM) + LENGTH(PSRAM);
    .ramtext : AT ( _siramtext )
    {
        . = ALIGN(4);
	_sramtext = .;
	*(.ramtext)
	*(.ramtext*)
        . = ALIGN(4);
	_eramtext = .;
    } >BRAM
    .data : AT ( _sidata )
    {
        . = ALIGN(4);
        _sdata = .;
        _ram_start = .;
        . = ALIGN(4);
        *(.data)
        *(.data*)
        *(.sdata)
        *(.sdata*)
        . = ALIGN(4);
        _edata = .;
    } >PSRAM
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        *(.bss*)
        *(.sbss)
        *(.sbss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } >PSRAM
    .heap :
    {
        . = ALIGN(4);
        _heap_start = .;
    } >PSRAM
    .text :
    {
        . = ALIGN(4);
        *(.text.start)
        *(.text)
        *(.text*)
        *(.rodata)
        *(.rodata*)
        *(.srodata)
        *(.srodata*)
        . = ALIGN(4);
        _etext = .;
        _siramtext = _etext;
	_sidata = _siramtext + SIZEOF(.ramtext);
    } >FLASH
}
