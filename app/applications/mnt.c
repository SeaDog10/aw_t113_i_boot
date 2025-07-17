#include <rtthread.h>

#ifdef RT_USING_DFS
#include <dfs_fs.h>
#endif

#ifdef RT_USING_DFS_ROMFS
#include <dfs_romfs.h>

static const struct romfs_dirent _romfs_root[] =
{
    {ROMFS_DIRENT_DIR, "usb", RT_NULL, 0},
    {ROMFS_DIRENT_DIR, "sd",  RT_NULL, 0},
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
#endif
