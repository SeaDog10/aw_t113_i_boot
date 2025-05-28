#ifndef __BOARD_H__
#define __BOARD_H__

#define read32(addr) (*((volatile unsigned int *)(addr)))
#define write32(addr, value) (*((volatile unsigned int *)(addr)) = (value))

#endif /* __BOARD_H__ */
