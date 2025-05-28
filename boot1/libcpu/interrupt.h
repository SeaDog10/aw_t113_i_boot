#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

#define INT_IRQ     0x00
#define INT_FIQ     0x01

#define IRQ_MODE_TRIG_LEVEL         (0x00) /* Trigger: level triggered interrupt */
#define IRQ_MODE_TRIG_EDGE          (0x01) /* Trigger: edge triggered interrupt */

typedef void (*isr_handler_t)(int vector, void *param);

struct irq_desc
{
    isr_handler_t handler;
    void            *param;
};

void vector_init(void);

void interrupt_init(void);
void interrupt_mask(int vector);
void interrupt_umask(int vector);

int interrupt_get_irq(void);
void interrupt_ack(int vector);

void interrupt_set_target_cpus(int vector, unsigned int cpu_mask);
unsigned int interrupt_get_target_cpus(int vector);

void interrupt_set_triger_mode(int vector, unsigned int mode);
unsigned int interrupt_get_triger_mode(int vector);

void interrupt_set_pending(int vector);
unsigned int interrupt_get_pending(int vector);
void interrupt_clear_pending(int vector);

void interrupt_set_priority(int vector, unsigned int priority);
unsigned int interrupt_get_priority(int vector);

void interrupt_set_priority_mask(unsigned int priority);
unsigned int interrupt_get_priority_mask(void);

int interrupt_set_prior_group_bits(unsigned int bits);
unsigned int interrupt_get_prior_group_bits(void);

isr_handler_t interrupt_install(int vector, isr_handler_t handler,
                                void *param, const char *name);

#endif
