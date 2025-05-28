OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)

/* Memory Spaces Definitions */
MEMORY
{
   ram   (rwx) : ORIGIN = 0x40000000, LENGTH = 0x100000
}

SECTIONS
{
    .text :
    {
        . = ALIGN(4);
        __text_start = .;
        *(.vectors)
        *(.text)
        *(.text.*)
        __text_end = .;
    } > ram

    PROVIDE(__img_start = __text_start);
    PROVIDE(__entry_addr = _reset);

    . = ALIGN(4);

    .ARM.exidx :
    {
        __exidx_start = .;
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        __exidx_end = .;
    } > ram

    .rodata :
    {
        __rodata_start = .;
        *(.rodata)
        *(.rodata.*)
        __rodata_end = .;
    } > ram

    .data :
    {
        __data_start = .;
        *(.data)
        *(.data.*)
        __data_end = .;
    } > ram

    PROVIDE(__img_end = .);
    PROVIDE(__img_size = __img_end - __img_start);

    .bss       :
    {
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end = .;
    } > ram

    . = ALIGN(4);
    _end = .;
}
