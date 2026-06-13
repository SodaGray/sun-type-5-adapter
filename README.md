## Sun Type 5 / 5c 键盘适配器
## 简介

这是为 Sun Type 5 / 5c 键盘设计的转换器，旨在让这一经典设计的键盘配合 STM32 硬件实现高级功能。所有操作都在键盘和转换器内部完成，因此不需要主机软件。

固件基于 STM32 NUCLEO-L476RG，TinyUSB 设备栈 + FreeRTOS（CMSIS-RTOS2）。

作为《智能网联电子系统综合设计》课程的结课作业发布。

## 解决的问题

Sun Type 5 键盘使用私有串行协议，具有特殊的接口，无法在现代主机上使用。这个适配器在中间充当翻译，目标是能在任何设备上使用，同时还能提供尽可能多的高级功能。

## 功能特性和设计特色

- **在任何主机上工作。** 设计了 basic+ 模式：单个 boot-capable HID 接口，在 6 键 boot 协议（BIOS / Open Firmware）和全键无冲 report 协议（高级操作系统）之间自动协商。已在 PowerMac G4 Cube 及其附带的 Open Firmware 和 Apple Silicon Mac 等平台上验证。
- **两种 USB 模式，支持运行中切换、断电保存：**
    - **basic+** — 仅上述的自协商键盘接口。
    - **full** — 复合设备：接口 0 仍为上述接口，接口 1 增加 Consumer Control（音量/静音）+ System Control（电源/睡眠/唤醒），跨平台支持媒体与电源键。
- **键盘上的设置模式**，不需要运行任何主机端程序。
- **按键重映射**，可以将某个按键重新映射为另一个按键或者最大不超过四个按键的和弦。
- **设置持久化**，存在外部 SPI NOR flash 上，掉电安全、带磨损均衡，开机自动恢复。存储部分甚至可以分立出来作为单独的存储支持库使用。
- **锁定灯**，Num / Caps / Scroll / Compose 的状态回显到键盘 LED。
- **声音反馈**，复用键盘自带蜂鸣器做按键点击声和设置提示音。
- **热插拔**，你甚至可以把那个 Mini-DIN 8 接口拔下来再插回去。
- **高可靠性**，自从设计之初就对器件的物理特性和临时掉电进行考虑，确保工作安全无虞。
- **分层抽象**，用抽象层将功能抽象出来，更易于管理。

## 逆向工程

使用示波器读取了键盘发送的所有数据，列出扫描码。但是我花了两个小时把扫描码全部测出来之后两周在网上找到了规范文件……

## 硬件

|      |                                     |
|------|-------------------------------------|
| 微控制器 | STM32 NUCLEO-L476RG                 |
| 与键盘  | USART1，Sun 串行协议（1200 8N1，电平/反相硬件处理） |
| 与主机  | USB OTG FS，设备模式                     |
| 持久化  | 外部 SPI NOR 闪存（W25Q 系列）              |
| 指示   | 板载 LD2 = USB 挂载指示;键盘四个 LED 兼作设置模式反馈 |

## 设置模式（使用说明）

**进入：**长按**空白键**。键盘四个 LED 全亮，表示已进入设置模式;此时按键不会发往主机。

**操作：**先按一个功能键选设置项，再按 `0` / `1` 设定值。

| 按键 | 作用                                |
|---|-----------------------------------|
| `F1` 然后 `0` / `1` | USB 模式：`0` = basic+，`1` = full    |
| `F2` 然后 `0` / `1` | 按键点击声：`0` = 关，`1` = 开             |
| `F3` | 进入的按键重映射设置模式。先按下需要的源按键，再按目标按键/和弦。 |
| **静音键** | 切换适配器所有声音。                        |
| **空白键** | 取消：放弃本次更改并退出                      |

**反馈：**
- 成功 → 四灯常亮，并按当前设置发确认音，然后退出。
- 出错（无效的按键）→ 四灯快闪三次 + 蜂鸣，留在原地重试。
- 以上提示音都受静音设置控制。

**退出：** 成功或取消后自动退出，并恢复主机的 LED 状态。

**附加内容：** 如果键盘由于不正确的启动或中途重启出现状态机异常，可以使用开发板上的用户按键来重启相关逻辑。

## 软件架构

分层设计，应用层仅与语义层通信，不处理扇区、CRC、GC、协议字节等，不越级处理。存储方面体现了这一设计特色。使用了层层抽象做出了一个对象存储。

| 模块                                         | 职责                                                                        |
|--------------------------------------------|---------------------------------------------------------------------------|
| `sun_io` + `ring_buf`                      | USART1 收发：中断填充的 RX 环形缓冲、TX 命令（LED / reset / bell / click）、用线程 flag 通知键盘任务 |
| `sun_protocol`                             | 将 Sun 的 make/break 字节流解码成事件（MAKE / BREAK / ALL_KEYS_UP / RESET）           |
| `sun_keymap`                               | Sun 扫描码 → 带类型的按键（keyboard / modifier / consumer / system / internal）      |
| `hid_keyboard`                             | 组装并发送 NKRO 键盘报告（接口 0），处理 boot/report 协议                                   |
| `hid_extra`                                | Consumer + System Control 报告（接口 1）                                        |
| `usb_descriptors`                          | 设备/配置/HID 报告描述符;按模式返回 basic 或 full 配置                                     |
| `usb_mode`                                 | 运行时 USB 模式：get/set、重枚举、从 registry 读取与写回                                   |
| `sector_io` + `storage` + `crc32` + `w25q` | SPI NOR 上的对象存储：写时复制、磨损均衡、CRC、GC                                           |
| `registry`                                 | RAM 缓存的设置结构，落在单个存储对象上;**只追加**式 schema                                     |
| `storage_types`                            | 存储 （type） id 命名空间                                                         |
| `click`                                    | 按键点击声设置：应用 + 持久化                                                          |
| `settings`                                 | 设置模式状态机（F 键选择、取值、静音、取消）                                                   |
| `freertos.c`                               | 两个任务 + 长按计时器 + 反馈帮手                                                       |
| `remap`                                    | 按键重映射                                                                     |                                                                    |

**任务：**
- **usbTask**（realtime 优先级）—— 运行 `tud_task（）`，驱动 TinyUSB。
- **sunKbd**（above-normal）—— 抽干环形缓冲、解码、分发：正常模式发 HID 报告，设置模式喂给状态机；并处理 LED 透传、reset、长按进入。
  **启动时序：**
```
storage_mount（） → registry_init（） → usb_mode_init（） → tusb_init（）
```
点击声在键盘每次 RESET（含上电自检）时重新下发。

## 持久化

设置存放在一个 `registry` 结构里，由 RAM 缓存 + 单个存储对象支撑，坐在存储层之上：

- 加载：先填默认值，再用存储里的内容按长度覆盖;读不到或损坏就保留默认。
- schema **只追加**：新设置项往结构体尾部加，绝不重排/改大小/改用途。老固件存的短记录加载时只覆盖前缀，新字段自动留默认——`usb_mode → click → mute` 三个设置就是这样一步步加上去、且向后兼容的。
  存储层本身是写时复制 + 磨损均衡 + CRC + GC，掉电任意时刻旧版本都活着。

## 状态 / 后续

**第一阶段已完成**：翻译、两种 USB 模式 + 运行时切换、三个带持久化的设置（模式 / 点击 / 静音）、按键重映射、声光反馈。

**已知问题**：
- Apple Fn 键不太能移植的过来。
- 不知道是物理层的电平还是什么的问题，有时候会出现漏键、错键的问题。
- 每次烧写以后都需要手动按一下 User 按键，不太优雅，但也无关紧要。

往后：
- 个人产品版（STM32U5A5ZJ）：无线协处理器、nRF24L01+、VFD 等。

## 版权信息

Sun, Sun Microsystems, and Sun Type 5 are trademarks or registered trademarks of Oracle and/or its affiliates.

ST, STM32, and STM32Cube are trademarks or registered trademarks of STMicroelectronics International NV or its affiliates in the EU and/or elsewhere.

FreeRTOS is a trademark of Amazon Web Services, Inc. or its affiliates.
