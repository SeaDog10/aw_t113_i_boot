# Open-AMP 软件包

OpenAMP（Open Asymmetric Multi-Processing）项目为异构多核系统中的处理器间通信（IPC）提供标准化的软件框架。本软件包集成了 OpenAMP 及其依赖的 libmetal，包含丰富的示例、文档和平台适配代码，适用于裸机、RTOS 等多种环境。

## 目录结构

- `libmetal/`：提供共享内存管理。
- `openamp/`：OpenAMP 主体库，包含 RPMsg、VirtIO、RemoteProc 等模块及应用示例。
- `openamp_demo/`：OpenAMP 应用演示代码。
- `operation_interface/`：平台相关的 OpenAMP/virtio 设备适配层。
- `platform_port/`：平台移植相关代码（如 RT-Thread 适配）。

## 主要功能

- **弱化主仆关系**：app节点没有主仆关系，可自由创建和删除app节点。
- **多种核间通信方式**：支持IPI、邮箱进行通信。
- **极简适配流程**：三步即可适配新平台。
- **简洁的应用编写**：依托于丰富的openamp抽象接口，可编写出极简应用代码。
- **平台适配**：支持RTOS、裸机。
- **架构适配**：目前已适配armv8、loongarch。

## 使用流程

### 配置软件包参数

- 配置共享内存、从核启动信息、缓存区数量。

### 创建openamp设备

- 创建app_auto_node_register函数自动初始化app节点。
- 使用amp_sigiface_dev_create函数创建平台操作设备。
- 使用openamp_dev_create函数创建openamp操作设备。

### app节点管理与使用

- 使用openamp_app_node_register函数创建app节点，接收方cpu会自动创建app节点。
- 使用openamp_app_node_unregister函数删除app节点，接收方cpu就自动删除app节点。
- 使用openamp_app_send函数发送数据。

## 平台适配流程

### 创建核间通信设备

创建一个static struct amp_sigiface_ops结构体并实现函数sigiface_init、sigiface_send、amp_sigiface_dev_create。

- **sigiface_init函数功能**：核间通信中断安装与处理、从核启动。
- **sigiface_send函数功能**：核间通信中断触发。
- **amp_sigiface_dev_create函数功能**：核间通信设备创建。

### 对接从核启动函数

- **pull_up_cpu函数功能**：拉起从核，让从核在指定地址处开始执行。

### 对接平台基础功能

对接内存管理函数、延时管理、中断管理、互斥锁（裸机使用全局信号即可），以下是rtthread对接示例：

```c
#ifdef RT_AMP_MASTER
#define AMP_MASTER
#define VIRTIO_DRIVER_ONLY
#elif defined RT_AMP_SLAVE
#define AMP_SLAVE
#define VIRTIO_DEVICE_ONLY
#endif

#ifndef RPMSG_BUFFER_SIZE
#define RPMSG_BUFFER_SIZE (512)
#endif

#define OPENAMP_APP_NODE_NAME_MAXLEN 20

#define APP_NODE_MAXNUM              16

#define TIMESTAMP_GET                rt_tick_get()
#define SLEEP_USEC(x)                rt_thread_mdelay(x / 1000)
#define FREE                         rt_free
#define MALLOC                       rt_malloc
#define INTERRUPT_ENABLE(flags)      rt_hw_interrupt_enable(flags)
#define INTERRUPT_DISABLE            rt_hw_interrupt_disable()
#define MUTEEX_INIT(mutex)           rt_mutex_init(mutex, "m", RT_IPC_FLAG_PRIO)
#define MUTEEX_DEINIT(mutex)         rt_mutex_detach(mutex)
#define MUTEEX_TAKE(mutex, timeout)  rt_mutex_take(mutex, timeout)
#define MUTEEX_RELEASE(mutex)        rt_mutex_release(mutex)
#define MUTEEX_TRYTAKE(mutex)        rt_mutex_trytake(mutex)
#define MUTEEX_GET_OWNER(mutex)      rt_mutex_get_owner(mutex)
#define HW_CACHE_FLUSH(addr, len) \
    do                            \
    {                             \
    } while(0)
#define HW_CACHE_INVALIDATE(addr, len) \
    do                                 \
    {                                  \
    } while(0)
```

## 参考资料

- [OpenAMP 官方文档](https://openamp.readthedocs.io/en/latest/)
- [libmetal 官方文档](https://openamp-libmetal.readthedocs.io/en/latest/)
