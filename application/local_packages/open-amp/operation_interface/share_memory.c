#include "metal/device.h"
#include "metal/sys.h"
#include "openamp/open_amp.h"
#include "stdio.h"

#define SHM_BUS_NAME       "generic"
#define SHM_DEVICE_NAME    "opshm"

#define DEFAULT_PAGE_SHIFT (-1U)
#define DEFAULT_PAGE_MASK  (-1U)

static struct metal_io_region *shm_io;
static struct metal_device     shm_device = {.name = SHM_DEVICE_NAME, .bus = NULL, .num_regions = 1, .node = {NULL}, .irq_num = 0, .irq_info = NULL};

void log_handler(enum metal_log_level level, const char *format, ...)
{
#ifdef DEFAULT_LOGGER_ON
    char                     msg[1024];
    va_list                  args;
    static const char *const level_strs[] = {
        "metal: emergency: ", "metal: alert:     ", "metal: critical:  ", "metal: error:     ",
        "metal: warning:   ", "metal: notice:    ", "metal: info:      ", "metal: debug:     ",
    };

    va_start(args, format);
    vsnprintf(msg, sizeof(msg), format, args);
    va_end(args);

    if(level <= METAL_LOG_EMERGENCY || level > METAL_LOG_DEBUG)
        level = METAL_LOG_EMERGENCY;

    fprintf(stderr, "%s%s", level_strs[level], msg);
#else
    (void)level;
    (void)format;
#endif
}

struct metal_io_region *openamp_shmem_init(unsigned long long vaaddr, unsigned long long physaddr, unsigned int shm_size)
{
    int                      status = 0;
    struct metal_device     *device;
    struct metal_init_params metal_params = METAL_INIT_DEFAULTS;
    static metal_phys_addr_t shm_physmap;
    rt_kprintf("openamp_shmem_init begin\n");
    metal_params.log_handler = log_handler;
    metal_params.log_level   = METAL_LOG_DEBUG;

    if(shm_size <= 0)
    {
        return NULL;
    }

#if defined(AMP_MASTER)
    memset((void *)vaaddr, 0, shm_size);
#endif

    metal_init(&metal_params);

    status = metal_register_generic_device(&shm_device);
    if(status != 0)
    {
        return NULL;
    }

    status = metal_device_open(SHM_BUS_NAME, SHM_DEVICE_NAME, &device);
    if(status != 0)
    {
        return NULL;
    }

    shm_physmap = physaddr;
    metal_io_init(&device->regions[0], (void *)vaaddr, &shm_physmap, shm_size, DEFAULT_PAGE_SHIFT, 0, NULL);

    shm_io = metal_device_io_region(device, 0);
    if(shm_io == NULL)
    {
        return NULL;
    }
    rt_kprintf("openamp_shmem_init end\n");
    return shm_io;
}

void openamp_shmem_deinit(void)
{
    metal_finish();
}
