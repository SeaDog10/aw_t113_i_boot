#ifndef __SHELL_H__
#define __SHELL_H__

#include "xformat.h"
#include "xstring.h"

#define SHELL_NULL          0
#define SHEEL_PROMPT_SIZE   6
#define SHELL_PROMPT        "shell>"

#define CMD_HISTORY_DEPTH   5
#define CMD_BUF_SIZE        128
#define CMD_MAX_ARGV        10

typedef int (*shell_cmd_func_t)(int argc, char **argv);

struct shell_command
{
    const char *name;
    const char *desc;
    shell_cmd_func_t func;
    struct shell_command *next;
};

int shell_init(void (*outchar_func)(void *, char), void *outarg,
    char (*inputchar_func)(void *), void *inarg, const char *sheel_headtag);

void shell_servise(void);
void s_printf(const char *fmt, ...);
void shell_register_command(struct shell_command *cmd);

#endif /* __SHELL_H__ */
