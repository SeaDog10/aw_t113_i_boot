# HGBOOT 框架

## 概述

HGBOOT 是一个面向嵌入式系统的轻量级、模块化 Bootloader 框架，具备启动加载、OTA 升级、分区管理、YMODEM 文件传输和 Shell 命令行等功能。框架高度可移植，只需实现少量底层接口即可适配不同硬件平台。

---

## 主要功能

- **启动加载**：支持多固件分区启动、固件校验、回滚等机制，保障系统可靠启动。
- **OTA 升级**：支持通过 YMODEM 协议进行固件升级和回滚，具备 CRC 和 magic 校验。
- **分区管理**：灵活的分区注册与管理，支持多存储设备和多分区。
- **YMODEM 协议**：可靠的文件传输能力，适用于固件和参数的远程升级。
- **Shell 命令行**：交互式命令行接口，便于调试、诊断和维护。

---

## 组件说明

HGBOOT 框架由多个高度解耦的核心组件组成，各组件功能特性如下：

### 1. 启动加载（Boot ）

- 负责从分区加载固件，并进行完整性校验（包括 CRC、magic 校验）。
- 根据固件信息头搬运固件到加载地址。
- 支持校验失败自动回滚机制，需要依赖OTA组件。

**伪代码示例：**

```c
void *entry = boot_firmware();

if (entry != NULL)
{
    // 从启动地址运行固件
}
else
{
    // 启动失败处理
}
```

### 2. 固件下载/更新/回滚（OTA）

- 提供固件下载、固件升级、固件回滚等接口，便于上层调用。
- 固件下载通过 YMODEM 协议接收固件，需要依赖Ymodem组件。
- 固件升级主要校验下载的固件的完整性并将其复制到固件启动分区。
- 固件回滚通过切换启动分区实现。

**伪代码示例：**

```c
if (ota_download_firmware() == 0)
{
    if (ota_update_firmware() == 0)
    {
        // 升级成功
    }
    else
    {
        // 升级失败处理
    }
}
else
{
    // 下载失败处理
}

// 启动或校验当前启动分区固件损坏可回滚
ota_backup_firmware();
```

### 3. 分区管理（Partition）

- 支持多存储设备、多分区灵活配置。
- 提供分区注册、读写、擦除等接口。
- 通过设备操作结构体实现底层驱动适配，便于移植。

**伪代码示例：**

```c
partition_device_register("flash0", &flash_ops, FLASH_SIZE);
partition_register("app", "flash0", 0x10000, 0x20000);

char buf[256];
partition_read("app", buf, 0, sizeof(buf));
partition_write("app", buf, 0, sizeof(buf));
partition_erase("app", 0, 0x1000);
```

### 4. 文件接收（Ymodem）

- 实现可靠的串口文件传输，适用于固件升级场景。
- 通过端口结构体适配不同硬件串口。
- 使用事件驱动，注册回调函数以实现应用逻辑。

**伪代码示例：**

```c
ymodem_init(&ymodem_port);
ymodem_receive(&ymodem_callback);
```

### 5. 命令行工具（Shell）

- 提供交互式命令行，支持运行时调试。
- 可注册自定义命令，扩展性强。
- 依赖底层输入输出接口，适配灵活。

**伪代码示例：**

```c
shell_init(&shell_port, "hgboot>");
shell_register_command(&cmd_ota);
while (1)
{
    shell_servise();
}
```

---

## OTA 原理说明

HGBOOT 的 OTA 升级流程如下：

1. **固件打包**：固件需通过``pack.py``工具进行打包操作，包括加入固件crc校验码和固件大小等信息到固件头部。
2. **固件接收**：通过 YMODEM 协议将新固件从上位机发送到设备，Bootloader 端通过 `ota_download_firmware()` 接口接收固件并写入下载分区。
3. **固件校验**：接收完成后对固件进行 CRC 校验和 magic 校验，确保固件完整性和合法性。
4. **固件升级**：通过 `ota_update_firmware()` 接口将下载分区的固件复制到活动分区，替换当前运行固件。
5. **回滚机制**：若新固件异常，可通过 `ota_backup_firmware()` 接口回滚到上一个固件分区，保障系统可用性。
6. **分区管理**：所有固件操作均基于分区管理组件，支持多分区和多设备灵活配置。

OTA 依赖于 文件接收（Ymodem）和分区管理（Partition）组件。

---

## 组件依赖关系

HGBOOT 各核心组件之间的依赖关系如下：

* **Boot**
  * 依赖于 Partition （用于固件读取和切换）
  * 依赖于 OTA （用于固件回滚）

- **OTA **
  - 依赖于 Ymodem（用于固件接收）
  - 依赖于 Partition（用于固件存储与切换）
- **Ymodem**
  - 依赖于底层串口或通信端口（通过 `ymodem_putchar`/`ymodem_getchar` 实现）
- **Partition**
  - 依赖于存储设备驱动（通过 `partition_dev_ops_t` 结构体实现设备操作）
- **Shell**
  - 可注册命令以调用 OTA、Partition等组件接口，便于调试和维护
  - 依赖于底层输入输出接口（通过 `shell_putchar`/`shell_getchar` 实现）

各组件之间通过标准接口解耦，便于裁剪和移植。

---

## 核心 API

### Boot

加载并校验固件，返回固件入口地址:
```c
/**
 * @brief 加载并校验固件，返回固件入口地址
 * @return 固件入口地址，失败返回 NULL
 */
void *boot_firmware(void);
```

### OTA

通过 YMODEM 接收固件并写入下载分区:
```c
/**
 * @brief 通过 YMODEM 协议接收固件并写入下载分区
 * @return 0 表示成功，负值表示失败
 */
int ota_download_firmware(void);
```

将下载分区的固件升级到活动分区:
```c
/**
 * @brief 将下载分区的固件升级到活动分区
 * @return 0 表示成功，负值表示失败
 */
int ota_update_firmware(void);
```

回滚到上一个固件分区:
```c
/**
 * @brief 回滚到上一个固件分区
 * @return 0 表示成功，负值表示失败
 */
int ota_backup_firmware(void);
```

### Partition

注册存储设备:
```c
/**
 * @brief 注册存储设备
 * @param dev_name 设备名称
 * @param ops 设备操作函数指针集
 * @param size 设备总容量（字节）
 * @return 0 表示成功，负值表示失败
 */
int partition_device_register(const char *dev_name, struct partition_dev_ops *ops, unsigned int size);
```

注册分区:
```c
/**
 * @brief 注册分区
 * @param partition_name 分区名称
 * @param dev_name 设备名称
 * @param start 分区起始地址（字节）
 * @param size 分区大小（字节）
 * @return 0 表示成功，负值表示失败
 */
int partition_register(const char *partition_name, const char *dev_name, unsigned int start, unsigned int size);
```

读取分区数据:
```c
/**
 * @brief 读取分区数据
 * @param partition_name 分区名称
 * @param buf 数据缓冲区
 * @param offset 分区内偏移（字节）
 * @param len 读取长度（字节）
 * @return 实际读取字节数，负值表示失败
 */
int partition_read(const char *partition_name, void *buf, unsigned int offset, unsigned int len);
```

写入分区数据:
```c
/**
 * @brief 写入分区数据
 * @param partition_name 分区名称
 * @param buf 数据缓冲区
 * @param offset 分区内偏移（字节）
 * @param len 写入长度（字节）
 * @return 实际写入字节数，负值表示失败
 */
int partition_write(const char *partition_name, void *buf, unsigned int offset, unsigned int len);
```

擦除分区区域:
```c
/**
 * @brief 擦除分区区域
 * @param partition_name 分区名称
 * @param offset 分区内偏移（字节）
 * @param len 擦除长度（字节）
 * @return 0 表示成功，负值表示失败
 */
int partition_erase(const char *partition_name, unsigned int offset, unsigned int len);
```

### Ymodem

初始化 YMODEM 端口:
```c
/**
 * @brief 初始化 YMODEM 端口
 * @param port YMODEM 端口结构体指针
 * @return 0 表示成功，负值表示失败
 */
int ymodem_init(ymdoem_port_t *port);
```

使用 YMODEM 协议接收文件:
```c
/**
 * @brief 使用 YMODEM 协议接收文件
 * @param callback 文件接收回调结构体指针
 * @return 0 表示成功，负值表示失败
 */
int ymodem_receive(ymodem_callback_t *callback);
```

### Shell

初始化 shell 命令行:
```c
/**
 * @brief 初始化 shell 命令行
 * @param port shell 端口结构体指针
 * @param shell_headtag shell 命令行前缀
 * @return 0 表示成功，负值表示失败
 */
int shell_init(shell_port_t *port, const char *shell_headtag);
```

注册 shell 命令:
```c
/**
 * @brief 注册 shell 命令
 * @param cmd shell 命令结构体指针
 */
void shell_register_command(struct shell_command *cmd);
```

shell 服务循环，处理输入与命令:
```c
/**
 * @brief shell 服务循环，处理输入与命令
 */
void shell_servise(void);
```

## 对接说明

HGBOOT 适配到不同硬件平台时，只需实现各组件的底层接口：

### Shell 对接

实现如下接口用于 shell 输入输出：
```C
void shell_putchar(char ch);    // 输出字符（如串口发送）
char shell_getchar(void);       // 输入字符（如串口接收）
```

将实现绑定到 `shell_port_t` 结构体，传递给 `shell_init`接口。

### Ymodem 对接

实现如下接口用于 YMODEM 输入输出：
```C
void ymodem_putchar(char ch);                          // 输出字符
int ymodem_getchar(char *ch, unsigned int timeout);    // 输入字符，带超时检测机制
```

将实现绑定到 `ymdoem_port_t` 结构体，传递给 `ymodem_init`接口。

### Partiton 对接

实现如下分区设备操作接口：
```C
int init(void);                                                       // 设备初始化
int read(unsigned int addr, unsigned char *buf, unsigned int size);   // 设备读取
int write(unsigned int addr, unsigned char *buf, unsigned int size);  // 设备写入
int erase(unsigned int addr, unsigned int size);                      // 设备擦除
```

将实现绑定到 `partition_dev_ops_t` 结构体，通过 `partition_device_register` 接口注册。

---

## License

本项目采用 MIT 协议开源，欢迎自由使用和修改。

```
Copyright (c) 2025 HGBOOT Authors

This program and the accompanying materials are made available under the
terms of the MIT License which is available at
https://opensource.org/licenses/MIT.

SPDX-License-Identifier:
```