#include <rtthread.h>

#ifdef RT_USING_DFS
#include <dfs_fs.h>
#endif

#ifdef RT_USING_DFS_ROMFS
#include <dfs_romfs.h>

static const struct romfs_dirent _romfs_root[] =
{
    {ROMFS_DIRENT_DIR, "sdcard",  RT_NULL, 0},
};

const struct romfs_dirent romfs_root = {
    ROMFS_DIRENT_DIR, "/", (rt_uint8_t *)_romfs_root, sizeof(_romfs_root)/sizeof(_romfs_root[0]),
};

int romfs_init(void)
{
    if (dfs_mount(RT_NULL, "/", "rom", 0, &(romfs_root)) == 0)
    {
        rt_kprintf("ROM file system initializated!\n");
    }
    else
    {
        rt_kprintf("ROM file system initializate failed!\n");
    }

    return 0;
}
INIT_ENV_EXPORT(romfs_init);

#define SD_NAME "sd0"
#define PATH_NAME "/sdcard"

static void sd_mount_thread_entry(void *param)
{
    static rt_uint8_t is_mount = 0;
    static rt_device_t dev = RT_NULL;

    while(1)
    {
        rt_thread_mdelay(1000);
        dev = rt_device_find(SD_NAME);
        if(dev == RT_NULL)
        {
            is_mount = 0;
            continue;
        }

        if(is_mount == 0)
        {
            is_mount = 1;
            if(dfs_mount(SD_NAME, PATH_NAME, "elm", 0, 0) != RT_EOK)
            {
                rt_kprintf("elm file system initializate failed;\n");
            }
            else
            {
                rt_kprintf("elm file system initializated;\n");
            }
        }
    }
}

static int sd_mount_elm(void)
{
    rt_thread_t tid = RT_NULL;

    tid = rt_thread_create("sd_mount", sd_mount_thread_entry, RT_NULL, 4096, 25, 10);
    if (tid)
    {
        rt_thread_startup(tid);
    }
    else
    {
        rt_kprintf("sd_mount thread create err.\n");
        return -RT_ERROR;
    }

    return RT_EOK;
}
INIT_APP_EXPORT(sd_mount_elm);

#endif
