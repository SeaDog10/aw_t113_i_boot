#ifndef __SHELL_H__
#define __SHELL_H__

#include "xformat.h"

#define SHELL_NULL 0

int shell_init(void (*outchar_func)(void *, char), void *arg);
void s_printf(const char *fmt, ...);

#endif /* __SHELL_H__ */
