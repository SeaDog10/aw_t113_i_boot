#include "shell.h"

struct shell_input
{
    char buffer[CMD_BUF_SIZE];
    unsigned short pos;
    unsigned short length;
};

struct shell_history
{
    char history[CMD_HISTORY_DEPTH][CMD_BUF_SIZE];
    int count;
    int current;
    int max_len;
};

enum input_state
{
    STATE_NORMAL           = 0,
    STATE_ESC_RECEIVED     = 1,
    STATE_BRACKET_RECEIVED = 2,
};

static struct shell_command shell_cmd_list =
{
    .name = SHELL_NULL,
    .desc = SHELL_NULL,
    .func = SHELL_NULL,
    .next = SHELL_NULL,
};

static struct shell_input   shell_in  = {0};
static struct shell_history shell_his = {0};

static void (*outputchar)(void *, char) = SHELL_NULL;
static char (*inputchar)(void *) = SHELL_NULL;
static void *outputchar_arg = SHELL_NULL;
static void *inputchar_arg = SHELL_NULL;

/* >>>>>>>>>>>>>>>>>cmds<<<<<<<<<<<<<<<<<<<<<<<<< */
static int echo(int argc, char **argv);
static int help(int argc, char **argv);
static int hexdump(int argc, char **argv);
/* >>>>>>>>>>>>>>>>>cmds<<<<<<<<<<<<<<<<<<<<<<<<< */

static void show_prompt(void)
{
    outputchar(outputchar_arg, '>');
}

static void show_unknown_cmd(char *arg)
{
    char *str = "Unknown command: ";

    for ( ;(str != SHELL_NULL) && (*str != '\0'); str++)
    {
        outputchar(outputchar_arg, *str);
    }

    for ( ;(arg != SHELL_NULL) && (*arg != '\0'); arg++)
    {
        outputchar(outputchar_arg, *arg);
    }

    outputchar(outputchar_arg, '\r');
    outputchar(outputchar_arg, '\n');
}

static void redraw_input(struct shell_input *input)
{
    int i = 0;
    int back = 0;

    outputchar(outputchar_arg, '\r');
    outputchar(outputchar_arg, '>');

    for (i = 0; i < input->length; i++)
    {
        outputchar(outputchar_arg, input->buffer[i]);
    }

    outputchar(outputchar_arg, ' ');
    outputchar(outputchar_arg, '\b');

    back = input->length - input->pos;
    while (back--)
    {
        outputchar(outputchar_arg, 0x1b);
        outputchar(outputchar_arg, '[');
        outputchar(outputchar_arg, 'D');
    }
}

static void save_to_history(struct shell_history *history, const char *buffer)
{
    int i = 0;
    int update = 1;

    for (i = 0; i < history->count; i++)
    {
        if (xstrcmp(history->history[i], buffer) == 0)
        {
            update = 0;
            break;
        }
    }

    if (update == 1)
    {
        if (history->count == CMD_HISTORY_DEPTH)
        {
            for (int i = 1; i < CMD_HISTORY_DEPTH; ++i)
            {
                xstrcpy(history->history[i - 1], history->history[i]);
            }
            history->count--;
        }

        xstrcpy(history->history[history->count++], buffer);
        history->current = history->count;
    }
}

static void browse_history_up(struct shell_history *history, struct shell_input *input)
{
    int i = 0;

    if (history->count == 0 || history->current == 0)
    {
        return;
    }

    history->current--;
    xstrcpy(input->buffer, history->history[history->current]);
    input->length = xstrlen(input->buffer);
    input->pos = input->length;
    if (input->length < history->max_len)
    {
        outputchar(outputchar_arg, '\r');
        for (i = 0; i < history->max_len + 1; i++)
        {
            outputchar(outputchar_arg, ' ');
        }
    }
    redraw_input(input);
}

static void browse_history_down(struct shell_history *history, struct shell_input *input)
{
    int i = 0;

    if (history->current + 1 >= history->count)
    {
        input->length = input->pos = 0;
        input->buffer[0] = '\0';
    }
    else
    {
        history->current++;
        xstrcpy(input->buffer, history->history[history->current]);
        input->length = xstrlen(input->buffer);
        input->pos = input->length;
    }

    if (input->length < history->max_len)
    {
        outputchar(outputchar_arg, '\r');
        for (i = 0; i < history->max_len + 1; i++)
        {
            outputchar(outputchar_arg, ' ');
        }
    }

    redraw_input(input);
}

static int shell_parse_args(char *cmdline, char *argv[], int max_args)
{
    int argc = 0;

    while (*cmdline && argc < max_args)
    {
        while (*cmdline == ' ')
        {
            cmdline++;
        }

        if (*cmdline == '\0')
        {
            break;
        }

        argv[argc++] = cmdline;

        while (*cmdline && *cmdline != ' ')
        {
            cmdline++;
        }

        if (*cmdline)
        {
            *cmdline++ = '\0';
        }
    }

    return argc;
}

static void shell_handle_command(const char *input_buffer)
{
    char buf[CMD_BUF_SIZE] = {0};
    char *argv[CMD_MAX_ARGV] = {0};
    int argc = 0;
    struct shell_command *cmd = &shell_cmd_list;

    if (input_buffer == SHELL_NULL || *input_buffer == '\0')
    {
        return;
    }

    xstrncpy(buf, input_buffer, CMD_BUF_SIZE);
    buf[CMD_BUF_SIZE - 1] = '\0';

    argc = shell_parse_args(buf, argv, CMD_MAX_ARGV);
    if (argc == 0)
    {
        return;
    }

    if (cmd->next)
    {
        cmd = cmd->next;
        while (cmd)
        {
            if (xstrcmp(argv[0], cmd->name) == 0)
            {
                cmd->func(argc, argv);
                return;
            }
            cmd = cmd->next;
        }
    }

    show_unknown_cmd(argv[0]);
}

static void console_input_char(struct shell_input *input, struct shell_history *history, char ch)
{
    static enum input_state state = STATE_NORMAL;
    struct shell_command *cmd_list = SHELL_NULL;
    struct shell_command *matched_cmd = SHELL_NULL;
    char *prefix = SHELL_NULL;
    int prefix_len = 0;
    int match_count = 0;
    int remaining = 0;
    int i = 0;

    if (state == STATE_NORMAL)
    {
        if (ch == 0x1b)
        {
            state = STATE_ESC_RECEIVED;
            return;
        }

        if (ch == '\r' || ch == '\n')
        {
            outputchar(outputchar_arg, '\r');
            outputchar(outputchar_arg, '\n');
            input->buffer[input->length] = '\0';
            if (input->length > 0)
            {
                if (input->length > history->max_len)
                {
                    history->max_len = input->length;
                }
                save_to_history(history, input->buffer);
                shell_handle_command(input->buffer);
            }
            input->pos    = 0;
            input->length = 0;
            history->current = history->count;
            show_prompt();
            return;
        }

        if (ch == '\t')
        {
            prefix = input->buffer;
            prefix_len = input->length;
            cmd_list = &shell_cmd_list;
            match_count = 0;

            input->buffer[input->length] = '\0';

            if (prefix_len == 0)
            {
                help(0, SHELL_NULL);
                show_prompt();
                redraw_input(input);
                return;
            }

            if (cmd_list)
            {
                cmd_list = cmd_list->next;

                while (cmd_list)
                {
                    if (xstrncmp(cmd_list->name, prefix, prefix_len) == 0)
                    {
                        match_count++;
                        if (match_count == 1)
                        {
                            matched_cmd = cmd_list;
                        }
                    }
                    cmd_list = cmd_list->next;
                }
            }

            if (match_count == 1)
            {
                remaining = xstrlen(matched_cmd->name) - prefix_len;
                if (input->length + remaining >= CMD_BUF_SIZE)
                {
                    return;
                }
                for (i = 0; i < remaining; i++)
                {
                    input->buffer[input->pos + i] = matched_cmd->name[prefix_len + i];
                }
                input->length += remaining;
                input->pos += remaining;
                input->buffer[input->length] = '\0';
                redraw_input(input);
            }
            else if (match_count > 1)
            {
                s_printf("\r\n");
                cmd_list = shell_cmd_list.next;

                while (cmd_list)
                {
                    if (xstrncmp(cmd_list->name, prefix, prefix_len) == 0)
                    {
                        s_printf("%s\r\n", cmd_list->name);
                    }
                    cmd_list = cmd_list->next;
                }

                s_printf("\r\n");
                show_prompt();
                redraw_input(input);
            }

            return;
        }

        if (ch == '\b' || ch == 127)
        {
            if (input->pos > 0)
            {
                xmemmove(&input->buffer[input->pos - 1], &input->buffer[input->pos], input->length - input->pos);
                input->pos--;
                input->length--;
                redraw_input(input);
            }
            return;
        }

        if ((ch >= 0x20 && ch <= 0x7E) && (input->length < CMD_BUF_SIZE - 1))
        {
            xmemmove(&input->buffer[input->pos + 1], &input->buffer[input->pos],
                    input->length - input->pos);

            input->buffer[input->pos++] = ch;
            input->length++;
            redraw_input(input);
        }
    }
    else if (state == STATE_ESC_RECEIVED)
    {
        if (ch == '[')
        {
            state = STATE_BRACKET_RECEIVED;
        }
        else
        {
            state = STATE_NORMAL;
        }
    }
    else if (state == STATE_BRACKET_RECEIVED)
    {
        switch (ch)
        {
            case 'A': // Up arrow key
                browse_history_up(history, input);
                break;
            case 'B': // Down arrow key
                browse_history_down(history, input);
                break;
            case 'C': // Right arrow key
                if (input->pos < input->length)
                {
                    outputchar(outputchar_arg, 0x1b);
                    outputchar(outputchar_arg, '[');
                    outputchar(outputchar_arg, 'C');
                    input->pos++;
                }
                break;
            case 'D': // Left arrow key
                if (input->pos > 0)
                {
                    outputchar(outputchar_arg, 0x1b);
                    outputchar(outputchar_arg, '[');
                    outputchar(outputchar_arg, 'D');
                    input->pos--;
                }
                break;
            default:
                break;
        }
        state = STATE_NORMAL;
    }
}

/* >>>>>>>>>>>>>>>>>cmds<<<<<<<<<<<<<<<<<<<<<<<<< */
static int echo(int argc, char **argv)
{
    for (int i = 1; i < argc; i++)
    {
        s_printf("%s ", argv[i]);
    }
    s_printf("\r\n");
    return 0;
}
static struct shell_command echo_cmd =
{
    .name = "echo",
    .desc = "show parameters",
    .func = echo,
    .next = SHELL_NULL,
};

static int help(int argc, char **argv)
{
    struct shell_command *cmd = &shell_cmd_list;

    if (cmd->next)
    {
        cmd = cmd->next;
        s_printf("shell commads:\r\n");
        while (cmd)
        {
            s_printf("%-12s : %s\r\n", cmd->name, cmd->desc);
            cmd = cmd->next;
        }
    }
    else
    {
        s_printf("no shell commads\r\n");
    }

    return 0;
}
static struct shell_command help_cmd =
{
    .name = "help",
    .desc = "show help",
    .func = help,
    .next = SHELL_NULL,
};

static int hexdump(int argc, char **argv)
{
    unsigned int start_base = 0;
    unsigned int len = 0;
    unsigned int i = 0, j=  0;
    unsigned char val = 0;
    char ascii_buf[17] = {0};

    if (argc < 3)
    {
        s_printf("Usage: hexdump <addr> <len>\r\n");
        return -1;
    }

    if (xstrtol(argv[1], (int *)&start_base) != 0)
    {
        s_printf("Invalid address: %s\r\n", argv[1]);
        return -2;
    }

    if (xstrtol(argv[2], (int *)&len) != 0)
    {
        s_printf("Invalid length: %s\r\n", argv[2]);
        return -3;
    }

    s_printf("Dump 0x%p %dBytes\r\n", start_base, len);
    s_printf("Offset      00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\r\n");

    for (i = 0; i < len; i += 16)
    {
        s_printf("0x%08X  ", start_base + i);

        for (j = 0; j < 16; j++)
        {
            if (i + j < len)
            {
                val = *((unsigned char *)(start_base + i + j));
                s_printf("%02X ", val);
                ascii_buf[j] = (val >= 32 && val <= 126) ? val : '.';
            }
            else
            {
                s_printf("   ");
                ascii_buf[j] = ' ';
            }
        }

        s_printf(" %s\r\n", ascii_buf);
    }

    return 0;
}
static struct shell_command hexdump_cmd =
{
    .name = "hexdump",
    .desc = "show mem info",
    .func = hexdump,
    .next = SHELL_NULL,
};
/* >>>>>>>>>>>>>>>>>cmds<<<<<<<<<<<<<<<<<<<<<<<<< */

void shell_register_command(struct shell_command *cmd)
{
    struct shell_command *node = &shell_cmd_list;

    while (node->next)
    {
        node = node->next;
    }

    /* append the node to the tail */
    node->next = cmd;
    cmd->next = SHELL_NULL;
}

void s_printf(const char *fmt, ...)
{
    if (outputchar == SHELL_NULL)
    {
        return;
    }

    va_list args;
    va_start(args, fmt);
    xvformat(outputchar, outputchar_arg, fmt, args);
    va_end(args);
}

int shell_init(void (*outchar_func)(void *, char), void *outarg,
    char (*inputchar_func)(void *), void *inarg)
{
    if (outchar_func == SHELL_NULL || inputchar_func == SHELL_NULL)
    {
        return -1;
    }

    outputchar = outchar_func;
    outputchar_arg = outarg;
    inputchar = inputchar_func;
    inputchar_arg = inarg;

    show_prompt();

/* >>>>>>>>>>>>>>>>>cmds<<<<<<<<<<<<<<<<<<<<<<<<< */
    shell_register_command(&help_cmd);
    shell_register_command(&echo_cmd);
    shell_register_command(&hexdump_cmd);
/* >>>>>>>>>>>>>>>>>cmds<<<<<<<<<<<<<<<<<<<<<<<<< */

    return 0;
}

void shell_servise(void)
{
    char ch;

    if (inputchar_arg == SHELL_NULL || outputchar_arg == SHELL_NULL)
    {
        return;
    }

    ch = inputchar(inputchar_arg);
    if (ch >= 0)
    {
        console_input_char(&shell_in, &shell_his, ch);
    }
}

