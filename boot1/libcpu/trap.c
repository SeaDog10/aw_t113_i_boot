#include "armv7.h"
#include "interrupt.h"
#include "gic.h"
#include "drv_uart.h"

extern struct uart_handle uart0;

void trap_undef(struct exp_stack *regs)
{
    uart_putc(&uart0, 'u');
    uart_putc(&uart0, 'd');
    uart_putc(&uart0, 'f');
    uart_putc(&uart0, '\r');
    uart_putc(&uart0, '\n');
    while (1);
}

void trap_swi(struct exp_stack *regs)
{
    uart_putc(&uart0, 's');
    uart_putc(&uart0, 'w');
    uart_putc(&uart0, 'i');
    uart_putc(&uart0, '\r');
    uart_putc(&uart0, '\n');
    while (1);
}

void trap_pabt(struct exp_stack *regs)
{
    uart_putc(&uart0, 'p');
    uart_putc(&uart0, 'a');
    uart_putc(&uart0, 'b');
    uart_putc(&uart0, '\r');
    uart_putc(&uart0, '\n');
    while (1);
}

void trap_dabt(struct exp_stack *regs)
{
    uart_putc(&uart0, 'd');
    uart_putc(&uart0, 'a');
    uart_putc(&uart0, 'b');
    uart_putc(&uart0, '\r');
    uart_putc(&uart0, '\n');
    while (1);
}

void trap_resv(struct exp_stack *regs)
{
    uart_putc(&uart0, 'r');
    uart_putc(&uart0, 'e');
    uart_putc(&uart0, 'v');
    uart_putc(&uart0, '\r');
    uart_putc(&uart0, '\n');
    while (1);
}

void trap_fiq(void)
{
    uart_putc(&uart0, 'f');
    uart_putc(&uart0, 'i');
    uart_putc(&uart0, 'q');
    uart_putc(&uart0, '\r');
    uart_putc(&uart0, '\n');
    while (1);
}


void trap_irq(void)
{
    void *param;
    int int_ack;
    int ir;
    isr_handler_t isr_func;
    extern struct irq_desc isr_table[MAX_HANDLERS];

    int_ack = interrupt_get_irq();

    ir = int_ack & GIC_ACK_INTID_MASK;
    if (ir == 1023)
    {
        /* Spurious interrupt */
        return;
    }

    /* get interrupt service routine */
    isr_func = isr_table[ir].handler;

    if (isr_func)
    {
        /* Interrupt for myself. */
        param = isr_table[ir].param;
        /* turn to interrupt service routine */
        isr_func(ir, param);
    }

    /* end of interrupt */
    interrupt_ack(int_ack);
}
