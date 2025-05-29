#include "shell.h"

static void (*outchar)(void *, char) = SHELL_NULL;
static void *outchar_arg = SHELL_NULL;

void s_printf(const char *fmt, ...)
{
    if (outchar == SHELL_NULL)
    {
        return;
    }

    va_list args;
    va_start(args, fmt);
    xvformat(outchar, outchar_arg, fmt, args);
    va_end(args);
}

int shell_init(void (*outchar_func)(void *, char), void *arg)
{
    if (outchar_func == SHELL_NULL)
    {
        return -1;
    }

    outchar = outchar_func;
    outchar_arg = arg;

    return 0;
}


