_STACK_SIZE = DEFINED(_STACK_SIZE) ? _STACK_SIZE : 0x100000;
_VECTORS_SIZE = DEFINED(_VECTORS_SIZE) ? _VECTORS_SIZE : 0x100;
_IRQSTACK_SIZE = DEFINED(_IRQSTACK_SIZE) ? _IRQSTACK_SIZE : 0x200;
_FIQSTACK_SIZE = DEFINED(_FIQSTACK_SIZE) ? _FIQSTACK_SIZE : 0x200;
_ABTSTACK_SIZE = DEFINED(_ABTSTACK_SIZE) ? _ABTSTACK_SIZE : 0x200;
_UNDSTACK_SIZE = DEFINED(_UNDSTACK_SIZE) ? _UNDSTACK_SIZE : 0x200;
_HEAP_SIZE = DEFINED(_HEAP_SIZE) ? _HEAP_SIZE : 0xA00000;

MEMORY
{
   DRAM : ORIGIN = 0x40100000, LENGTH = 0x01000000
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
   } > DRAM

   .init : {
      KEEP (*(.init))
   } > DRAM

   .fini : {
      KEEP (*(.fini))
   } > DRAM

   .got : {
      *(.got)
   } > DRAM

   .ARM.attributes : {
      __ARM.attributes_start = .;
      *(.ARM.attributes)
      __ARM.attributes_end = .;
   } > DRAM

   .rodata : {
      __rodata_start = .;
      *(.rodata)
      *(.rodata.*)
      __rodata_end = .;
   } > DRAM

   .data : {
      __data_start = .;
      *(.data)
      *(.data.*)
      __data_end = .;
   } > DRAM

   .ctors : {
      . = ALIGN(64);
       __CTOR_LIST__ = .;
      ___CTORS_LIST___ = .;
      __ctors_start__ = .;
      KEEP (*crtbegin.o(.ctors))
      KEEP (*(EXCLUDE_FILE(*crtend.o) .ctors))
      KEEP (*(SORT(.ctors.*)))
      KEEP (*(.ctors))
      __CTOR_END__ = .;
      ___CTORS_END___ = .;
      __ctors_end__ = .;
   } > DRAM

   .sdata : {
      __sdata_start = .;
      *(.sdata)
      *(.sdata.*)
      __sdata_end = .;
   } > DRAM

   .sbss (NOLOAD) : {
      __sbss_start = .;
      *(.sbss)
      *(.sbss.*)
      __sbss_end = .;
   } > DRAM

   .tdata : {
      __tdata_start = .;
      *(.tdata)
      *(.tdata.*)
      __tdata_end = .;
   } > DRAM

   .tbss : {
      __tbss_start = .;
      *(.tbss)
      *(.tbss.*)
      __tbss_end = .;
   } > DRAM

   .bss (NOLOAD) : ALIGN(0x4){
      __bss_start = .;
      *(.bss)
      *(.bss.*)
      *(.gnu.linkonce.b.*)
      *(COMMON)
      __bss_end = .;
   } > DRAM

   .stack (NOLOAD) : ALIGN(0x10){
      __stack_start__ = .;
      . += _STACK_SIZE;
      _estack = .;
      . = ALIGN(0x10);
      _irq_stack_end = .;
      . += _IRQSTACK_SIZE;
      . = ALIGN(0x10);
      __irq_stack = .;
      _fiq_stack_end = .;
      . += _FIQSTACK_SIZE;
      . = ALIGN(0x10);
      __fiq_stack = .;
      _abort_stack_end = .;
      . += _ABTSTACK_SIZE;
      . = ALIGN(0x10);
      __abort_stack = .;
      _undef_stack_end = .;
      . += _UNDSTACK_SIZE;
      . = ALIGN(0x10);
      __undef_stack = .;
   } > DRAM

   .heap (NOLOAD) : ALIGN(0x10){
      _heap = .;
      HeapBase = .;
      _heap_start = .;
      . += _HEAP_SIZE;
      _heap_end = .;
      HeapLimit = .;
   } > DRAM

   _end = .;
}
