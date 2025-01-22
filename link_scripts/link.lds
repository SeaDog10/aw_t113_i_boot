OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
SEARCH_DIR(.)

/* Memory Spaces Definitions */
MEMORY
{
  ram   (rwx) : ORIGIN = 0x00020000, LENGTH = 0x14000
  heap  (rw)  : ORIGIN = 0x00034000, LENGTH = 0x4000
/*  ram   (rwx) : ORIGIN = 0x40000000, LENGTH = 0x14000 */
/*  heap  (rw)  : ORIGIN = 0x40014000, LENGTH = 0x4000  */
}

STACK_SIZE = 0x1000; /* 4KB */

ENTRY(reset)

SECTIONS
{
   .text :
   {
      . = ALIGN(4);
      PROVIDE(__spl_start = .);
      *start*(.text*)
      *(.text .text.*)
      . = ALIGN(4);
   } > ram

   . = ALIGN(4);

   .ARM.exidx :
   {
      __exidx_start = .;
      *(.ARM.exidx* .gnu.linkonce.armexidx.*)
      __exidx_end = .;
   } > ram

   PROVIDE(__spl_end = .);
   PROVIDE(__spl_size = __spl_end - __spl_start);

   /* .bss section which is used for uninitialized data */
   .bss :
   {
      . = ALIGN(4);
      _sbss = . ;
      *(.bss .bss.*)
      *(COMMON)
      . = ALIGN(4);
      _ebss = . ;
   } > ram

   .stack :
   {
      . = ALIGN(8);
      /* SRV stack section */
      __stack_srv_start = .;
      . += STACK_SIZE;
      __stack_srv_end = .;
   } > ram

   .memheap :
   {
      __memheap_start = .;
      . = ALIGN(8);
   } > heap

   PROVIDE(__memheap_end = ORIGIN(heap) + LENGTH(heap));

   . = ALIGN(4);
   _end = . ;
}