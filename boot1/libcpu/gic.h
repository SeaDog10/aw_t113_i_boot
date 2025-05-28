#ifndef __GIC_H__
#define __GIC_H__

#define MAX_HANDLERS        223
#define GIC_ACK_INTID_MASK  0x000003ff
#define GIC_IRQ_START       0
#define ARM_GIC_NR_IRQS     MAX_HANDLERS
#define ARM_GIC_MAX_NR      1

#define __REG32(x)          (*((volatile unsigned int *)(x)))
#define __REG16(x)          (*((volatile unsigned short *)(x)))

int arm_gic_get_active_irq(unsigned int index);
void arm_gic_ack(unsigned int index, int irq);

void arm_gic_mask(unsigned int index, int irq);
void arm_gic_umask(unsigned int index, int irq);

unsigned int arm_gic_get_pending_irq(unsigned int index, int irq);
void arm_gic_set_pending_irq(unsigned int index, int irq);
void arm_gic_clear_pending_irq(unsigned int index, int irq);

void arm_gic_set_configuration(unsigned int index, int irq, unsigned int config);
unsigned int arm_gic_get_configuration(unsigned int index, int irq);

void arm_gic_clear_active(unsigned int index, int irq);

void arm_gic_set_cpu(unsigned int index, int irq, unsigned int cpumask);
unsigned int arm_gic_get_target_cpu(unsigned int index, int irq);

void arm_gic_set_priority(unsigned int index, int irq, unsigned int priority);
unsigned int arm_gic_get_priority(unsigned int index, int irq);

void arm_gic_set_interface_prior_mask(unsigned int index, unsigned int priority);
unsigned int arm_gic_get_interface_prior_mask(unsigned int index);

void arm_gic_set_binary_point(unsigned int index, unsigned int binary_point);
unsigned int arm_gic_get_binary_point(unsigned int index);

unsigned int arm_gic_get_irq_status(unsigned int index, int irq);

void arm_gic_send_sgi(unsigned int index, int irq, unsigned int target_list, unsigned int filter_list);

unsigned int arm_gic_get_high_pending_irq(unsigned int index);

unsigned int arm_gic_get_interface_id(unsigned int index);

void arm_gic_set_group(unsigned int index, int irq, unsigned int group);
unsigned int arm_gic_get_group(unsigned int index, int irq);

int arm_gic_dist_init(unsigned int index, unsigned int dist_base, int irq_start);
int arm_gic_cpu_init(unsigned int index, unsigned int cpu_base);

void arm_gic_dump_type(unsigned int index);
void arm_gic_dump(unsigned int index);

#endif
