#include "armv7.h"
#include "interrupt.h"
#include "gic.h"
#include "drv_uart.h"

extern sunxi_usart_t usart0_dbg;

void trap_undef(struct exp_stack *regs)
{
    sunxi_usart_putc(&usart0_dbg, 'u');
    sunxi_usart_putc(&usart0_dbg, 'd');
    sunxi_usart_putc(&usart0_dbg, 'f');
    sunxi_usart_putc(&usart0_dbg, '\r');
    sunxi_usart_putc(&usart0_dbg, '\n');
    while (1);
}

void trap_swi(struct exp_stack *regs)
{
    sunxi_usart_putc(&usart0_dbg, 's');
    sunxi_usart_putc(&usart0_dbg, 'w');
    sunxi_usart_putc(&usart0_dbg, 'i');
    sunxi_usart_putc(&usart0_dbg, '\r');
    sunxi_usart_putc(&usart0_dbg, '\n');
    while (1);
}

void trap_pabt(struct exp_stack *regs)
{
    sunxi_usart_putc(&usart0_dbg, 'p');
    sunxi_usart_putc(&usart0_dbg, 'a');
    sunxi_usart_putc(&usart0_dbg, 'b');
    sunxi_usart_putc(&usart0_dbg, '\r');
    sunxi_usart_putc(&usart0_dbg, '\n');
    while (1);
}

void trap_dabt(struct exp_stack *regs)
{
    sunxi_usart_putc(&usart0_dbg, 'd');
    sunxi_usart_putc(&usart0_dbg, 'a');
    sunxi_usart_putc(&usart0_dbg, 'b');
    sunxi_usart_putc(&usart0_dbg, '\r');
    sunxi_usart_putc(&usart0_dbg, '\n');
    while (1);
}

void trap_resv(struct exp_stack *regs)
{
    sunxi_usart_putc(&usart0_dbg, 'r');
    sunxi_usart_putc(&usart0_dbg, 'e');
    sunxi_usart_putc(&usart0_dbg, 'v');
    sunxi_usart_putc(&usart0_dbg, '\r');
    sunxi_usart_putc(&usart0_dbg, '\n');
    while (1);
}

void trap_fiq(void)
{
    sunxi_usart_putc(&usart0_dbg, 'f');
    sunxi_usart_putc(&usart0_dbg, 'i');
    sunxi_usart_putc(&usart0_dbg, 'q');
    sunxi_usart_putc(&usart0_dbg, '\r');
    sunxi_usart_putc(&usart0_dbg, '\n');
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
