#include "metal/irq.h"
#include "openamp_virtio_device.h"

unsigned int metal_irq_save_disable(void)
{
    return INTERRUPT_DISABLE;
}

void metal_irq_restore_enable(unsigned int flags)
{
    INTERRUPT_ENABLE(flags);
}
