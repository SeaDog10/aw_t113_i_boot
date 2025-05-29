#ifndef __BOARD_H__
#define __BOARD_H__

#define read32(addr) (*((volatile unsigned int *)(addr)))
#define write32(addr, value) (*((volatile unsigned int *)(addr)) = (value))

#define U_NULL    0

extern int __memheap_start;
extern int __memheap_end;
#define HEAP_BEGIN          (&__memheap_start)
#define HEAP_END            (&__memheap_end)
#define HEAP_SIZE           ((unsigned int)HEAP_END - (unsigned int)HEAP_BEGIN)

#endif /* __BOARD_H__ */
