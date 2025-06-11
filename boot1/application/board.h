#ifndef __BOARD_H__
#define __BOARD_H__

#define read32(addr)         (*((volatile unsigned int *)(addr)))
#define write32(addr, value) (*((volatile unsigned int *)(addr)) = (value))

#define read16(addr)         (*((volatile unsigned short *)(addr)))
#define write16(addr, value) (*((volatile unsigned short *)(addr)) = (value))

#define read8(addr)          (*((volatile unsigned char *)(addr)))
#define write8(addr, value)  (*((volatile unsigned char *)(addr)) = (value))

#define U_NULL    0

extern int __memheap_start;
extern int __memheap_end;
#define HEAP_BEGIN          (&__memheap_start)
#define HEAP_END            (&__memheap_end)
#define HEAP_SIZE           ((unsigned int)HEAP_END - (unsigned int)HEAP_BEGIN)

unsigned long long get_gtimer_count(void);
unsigned long long get_count_ms(void);
unsigned long long get_count_us(void);
void us_delay(unsigned long long us);
void ms_delay(unsigned long long ms);

#endif /* __BOARD_H__ */
