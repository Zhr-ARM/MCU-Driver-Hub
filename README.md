# 🏎️ MCU-Driver-Hub | 嵌入式驱动军火库

![License](https://img.shields.io/badge/license-MIT-green) ![Platform](https://img.shields.io/badge/platform-STM32%20%7C%20ARM-blue) ![Language](https://img.shields.io/badge/language-C-orange)

> **"工欲善其事，必先利其器。"**
> 
> 这是一个专为 **大学生嵌入式竞赛**、**电子设计大赛 (NUEDC)** 及 **工程项目** 打造的高质量驱动库集合。我们致力于提供 **非阻塞、高效率、易移植** 的底层驱动，助你在赛场上专注于算法与逻辑，不再为底层 BUG 掉头发。

## 📚 驱动列表 (Driver List)

| 驱动名称 (Name) | 核心功能 (Features) | 关键技术 (Tech Stack) | 适用场景 (Use Case) | 状态 |
| :--- | :--- | :--- | :--- | :--- |
| **[🚀 dma_fifo_print](./dma_fifo_print/)** | **高性能串口打印** <br> 告别阻塞，释放 CPU 算力 | `DMA` `Ring Buffer` `Non-blocking` | 调试日志、高频数据回传、多任务环境 | ✅ Stable |
| **[⏱️ Delay_us](./Delay_us/)** | **高精度微秒延时** <br> 纳秒级精度，RTOS 友好 | `Cortex-M DWT` `SystemCoreClock` | 单总线协议 (DHT11/DS18B20)、软件 I2C/SPI | ✅ Stable |
| **[📺 OLED](./OLED/)** | **极限性能显示驱动** <br> 硬件 DMA 零拷贝 + 软件 DWT 模拟 | `DMA` `I2C` `DWT` `Zero-Copy` | UI 交互、波形显示、双屏异显、调试副屏 | ✅ Stable |

---

## 🌟 亮点介绍 (Highlights)

### 1. 🚀 DMA 环形缓冲区打印 (dma_fifo_print)
你还在用 `HAL_UART_Transmit` 傻傻地等待发送完成吗？在中断里调用 `printf` 导致死机？
- **零阻塞**：调用即返回，数据由 DMA 后台搬运。
- **安全可靠**：内置环形缓冲区 (FIFO)，自动处理数据积压。
- **全平台兼容**：支持 Keil (MicroLIB) 和 GCC (STM32CubeIDE)，适配 F1/F4/H7 等全系列。

### 2. ⏱️ DWT 高精度延时 (Delay_us)
不要再浪费宝贵的硬件定时器 (TIM) 去做延时了！
- **硬件级精度**：利用 Cortex-M 内核自带的 DWT 计数器，精度高达 `1/主频` 秒。
- **智能调度**：独创 `delay_smart_us`，短延时死等保时序，长延时自动挂起 (Yield) 释放 CPU 给 RTOS。
- **极简集成**：无需占用中断，无需复杂的 TIM 配置。

### 3. 📺 STM32 OLED 极致驱动 (OLED)
抛弃官方低效例程，我们要榨干单片机的每一滴性能。
- **⚡ 硬件 I2C (零拷贝)**：利用 DMA 直接搬运 Flash 字模到 I2C 外设，不占用 RAM，CPU 占用率接近 0%。
- **🛡️ 软件 I2C (DWT)**：基于内核 DWT 实现纳秒级同步，无论主频是 72M 还是 480M，波形都标准如一。
- **📦 统一字库**：中央集权式字库管理，支持 6x8 ~ 12x24 多种字体，支持双屏异显（硬件屏+软件屏同时驱动）。

---

## 📂 目录结构 (Directory Structure)

```text
MCU-Driver-Hub/
├── Delay_us/            # DWT 微秒延时库
│   ├── delay_us.c       # 核心实现
│   ├── delay_us.h       # 接口声明
│   └── README.md        # 使用文档
├── dma_fifo_print/      # DMA 串口打印库
│   ├── dma_fifo_print.c # 核心实现 & printf 重定向
│   ├── dma_fifo_print.h # 配置参数
│   └── README.md        # 使用文档
├── OLED/                # SSD1306 OLED 驱动库
│   ├── Oled.c           # 硬件 I2C 实现
│   ├── Oled.h           # 硬件配置宏
│   ├── soft_oled.c      # 软件 I2C 实现
│   ├── soft_oled.h      # 软件引脚配置
│   ├── font.h           # 统一字库文件
│   └── Readme.md        # 使用文档
├── LICENSE              # MIT 开源协议
└── README.md            # 项目主页
