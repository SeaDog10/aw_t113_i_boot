MEMORY
{
    /* DRAM : ORIGIN = 0x40200000, LENGTH = 0x01000000 */
    DRAM_CODE(R)    : ORIGIN = 0x40200000, LENGTH = 0x60000 /* 384K */
    DRAM_RODATA(R)  : ORIGIN = 0x40260000, LENGTH = 0x20000 /* 128K  */
    DRAM_DATA(RW)   : ORIGIN = 0x40280000, LENGTH = 0x20000 /* 128K  */
    DRAM_BSS(RW)    : ORIGIN = 0x402a0000, LENGTH = 0x20000 /* 128K  */
    DRAM_HEAP(RW)   : ORIGIN = 0x402c0000, LENGTH = 0x40000 /* 256K */
}

ENTRY(system_vectors)

SECTIONS
{
    .text :
    {
        __text_start = .;
        KEEP (*(.vectors))
        /* make the ISRs close to vectors may be more cache-friendly */
        *(.text.isr)

        /* section information for utest */
        . = ALIGN(4);
        __rt_utest_tc_tab_start = .;
        KEEP(*(UtestTcTab))
        __rt_utest_tc_tab_end = .;

        /* section information for finsh shell */
        . = ALIGN(4);
        __fsymtab_start = .;
        KEEP(*(FSymTab))
        __fsymtab_end = .;
        . = ALIGN(4);
        __vsymtab_start = .;
        KEEP(*(VSymTab))
        __vsymtab_end = .;

        /* section information for var export */
        . = ALIGN(4);
        __ve_table_start = .;
        KEEP(*(SORT(*.VarExpTab.*)))
        __ve_table_end = .;

        /* section information for modules */
        . = ALIGN(4);
        __rtmsymtab_start = .;
        KEEP(*(RTMSymTab))
        __rtmsymtab_end = .;

        /* section information for initialization */
        . = ALIGN(4);
        __rt_init_start = .;
        KEEP(*(SORT(.rti_fn*)))
        __rt_init_end = .;

        . = ALIGN(16);
        __usbh_class_info_start__ = .;
        KEEP(*(.usbh_class_info))
        __usbh_class_info_end__ = .;

        . = ALIGN(16);
        __ARM.attributes_start = .;
        *(.ARM.attributes)
        __ARM.attributes_end = .;
    } > DRAM_CODE

    .rodata :
    {
        __rodata_start = .;
        *(.rodata)
        *(.rodata.*)
        __rodata_end = .;
    } > DRAM_RODATA

    .data :
    {
        __data_start = .;
        *(.data)
        *(.data.*)
        __data_end = .;
    } > DRAM_DATA

    .bss (NOLOAD):
    {
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        __bss_end = .;
    } > DRAM_BSS

    .heap (NOLOAD):
    {
        _heap_start = .;
        . += 0x40000;
        _heap_end = .;
    } > DRAM_HEAP
}
