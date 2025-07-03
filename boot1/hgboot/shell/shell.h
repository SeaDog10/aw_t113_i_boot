#ifndef __SHELL_H__
#define __SHELL_H__

#include "shell/xformat.h"
#include "shell/xstring.h"

#define SHELL_NULL          0                           /* Null pointer definition for shell */
#define SHELL_PROMPT        "shell />"                  /* Default shell prompt string */
#define SHEEL_PROMPT_SIZE   (sizeof(SHELL_PROMPT)-1)    /* Size of the shell prompt string */

#define CMD_HISTORY_DEPTH   5                   /* Depth of command history */
#define CMD_BUF_SIZE        128                 /* Size of the command buffer */
#define CMD_MAX_ARGV        10                  /* Maximum number of command arguments */

/* shell_cmd_func_t: Function pointer type for shell command handlers. */
typedef int (*shell_cmd_func_t)(int argc, char **argv); /* shell_cmd_func_t: Function pointer for shell command. */

/* shell_port_t: Shell port interface for low-level input/output. */
typedef struct shell_port
{
    void (*shell_putchar)(char ch);             /* Output a character */
    char (*shell_getchar)(void);                /* Input a character */
} shell_port_t; /* shell_port_t: Shell port interface for low-level input/output. */

/* shell_command: Shell command structure. */
struct shell_command
{
    const char *name;                           /* Command name */
    const char *desc;                           /* Command description */
    shell_cmd_func_t func;                      /* Command handler function */
    struct shell_command *next;                 /* Pointer to next command in list */
}; /* shell_command: Shell command structure. */

int shell_init(shell_port_t *port, const char *sheel_headtag);
shell_port_t *shell_deinit(void);
void shell_servise(void);
void shell_register_command(struct shell_command *cmd);
void s_printf(const char *fmt, ...);

#endif /* __SHELL_H__ */
