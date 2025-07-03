#ifndef __SHELL_H__
#define __SHELL_H__

#include "shell/xformat.h"
#include "shell/xstring.h"

#define SHELL_NULL          0
#define SHELL_PROMPT        "shell />"
#define SHEEL_PROMPT_SIZE   (sizeof(SHELL_PROMPT)-1)

#define CMD_HISTORY_DEPTH   5
#define CMD_BUF_SIZE        128
#define CMD_MAX_ARGV        10

typedef int (*shell_cmd_func_t)(int argc, char **argv);

typedef struct shell_port
{
    void (*shell_putchar)(char ch);
    char (*shell_getchar)(void);
} shell_port_t;

struct shell_command
{
    const char *name;
    const char *desc;
    shell_cmd_func_t func;
    struct shell_command *next;
};

int shell_init(shell_port_t *port, const char *sheel_headtag);
shell_port_t *shell_deinit(void);

void shell_servise(void);
void shell_register_command(struct shell_command *cmd);

void s_printf(const char *fmt, ...);

#endif /* __SHELL_H__ */
