#include "platform_port.h"

extern uint64_t __ebase_end;

int pull_up_cpu(rt_uint64_t entry)
{
    // rt_uint64_t offset = (uint64_t)&__ebase_end;

    // offset -= (uint64_t)CPU_ENTRY_ADDRESS;
    // offset += (uint64_t)entry;

    // rt_kprintf("cpu (%x) on startup address [0x%x]\n", AMP_REMOTE_CPU_ID, offset);

    // cpu_online(AMP_REMOTE_CPU_ID, offset);

    return RT_EOK;
}
