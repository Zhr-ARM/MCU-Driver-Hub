# 📺 STM32 OLED Driver | 高性能双模显示驱动库

> **"天下武功，唯快不破。"**
>
> 这是一个专为 STM32 打造的 **SSD1306 OLED** 极致驱动库。我们抛弃了低效的官方例程，引入了 **零拷贝 DMA 硬件 I2C** 和 **基于 DWT 的纳秒级软件 I2C**，旨在榨干单片机的每一滴性能。

## 📚 核心特性 (Key Features)

| **驱动模式 (Mode)** | **核心优势 (Highlights)**                     | **关键技术 (Tech Stack)**                 | **适用场景 (Use Case)**            |
| ------------------- | --------------------------------------------- | ----------------------------------------- | ---------------------------------- |
| **⚡ 硬件 I2C**      | **零拷贝 DMA 传输**   CPU 占用率接近 0%       | `HAL_I2C_Mem_Write` `DMA` `Atomic Cursor` | 主界面刷新、高帧率动画、多任务环境 |
| **🛡️ 软件 I2C**      | **DWT 精准时序**   无论主频多少，波形稳如泰山 | `Cortex-M DWT` `Open-Drain` `Block Write` | 调试副屏、引脚资源受限、跨平台移植 |
| **📦 字库管理**      | **中央集权制**   多屏共用字库，不浪费 Flash   | `Centralized Font` `Extern`               | 双屏异显、多字体需求 (6x8 ~ 12x24) |

## 🌟 亮点解析 (Deep Dive)

### 1. ⚡ 硬件 I2C：告别“总线抖动”

传统驱动发一个字节就要 Start/Stop 一次，总线全是垃圾信号。本库采用了 **原子化光标 (Atomic Cursor)** 和 **内存直通 (Zero-Copy)** 技术：

- **效率暴增**：将 3 字节命令打包发送，总线开销降低 66%。
- **DMA 就绪**：数据直接从 Flash 搬运到 I2C 外设，不经过 RAM 中转。

### 2. 🛡️ 软件 I2C：可能是全网最稳的模拟 I2C

不要再用 `for(i=0;i<100;i++)` 这种玄学延时了！

- **DWT 加持**：利用内核 DWT 计数器实现纳秒级同步。72MHz 的 F1 和 480MHz 的 H7 跑出来的波形一模一样（400kHz）。
- **开漏极速翻转**：初始化为开漏输出 (OD)，读写切换无需重新配置 GPIO 寄存器，速度提升 50%。

## 📂 目录结构 (Directory Structure)

建议将文件按照以下结构放入你的 `Drivers` 目录：

```
Drivers/Hardware/OLED/
├── Core/                # 核心资源
│   ├── font.c           # 字库数据真身 (所有数据都在这)
│   └── font.h           # 字库对外接口
├── Hardware_I2C/        # 硬件驱动
│   ├── Oled.c           # 硬件 I2C 实现
│   └── Oled.h           # 硬件配置宏
└── Software_I2C/        # 软件驱动
    ├── soft_oled.c      # 软件 I2C 实现
    ├── soft_oled.h      # 引脚配置宏
    ├── delay_us.c       # DWT 延时依赖
    └── delay_us.h       # DWT 接口
```

## 🛠️ 集成指南 (Integration)

### 第一步：配置 CubeMX

1. **硬件 I2C 模式**：
   - 开启 I2C1 (或 I2C2)，速率设为 `400000` (Fast Mode)。
   - *(可选)* 开启 DMA TX 通道以获得极致性能。
2. **软件 I2C 模式**：
   - 选择两个 GPIO (如 PB6, PB7)。
   - GPIO Mode: **Output Open Drain** (开漏输出，⚠️关键)。
   - Pull-up/Pull-down: **Pull-up** (上拉)。
   - Maximum output speed: **High** (高速)。

### 第二步：修改配置宏

- **硬件驱动** (`Oled.h`):

  ```c
  extern I2C_HandleTypeDef hi2c1; // 链接你的 HAL 句柄
  #define OLED_I2C_HANDLE   &hi2c1
  ```

- **软件驱动** (`soft_oled.h`):

  ```c
  #define OLED_SCL_PORT   GPIOB
  #define OLED_SCL_PIN    GPIO_PIN_6
  #define OLED_SDA_PORT   GPIOB
  #define OLED_SDA_PIN    GPIO_PIN_7
  ```

## 🚀 快速上手 (Quick Start)

### 场景 A：主力开发 (使用硬件 I2C)

```c
#include "Oled.h"

int main(void) {
    HAL_Init();
    /* ... SystemClock & MX_I2C1_Init ... */
    
    OLED_Init(); // 初始化
    
    // 显示字符串 (8x16 字体)
    OLED_ShowString(0, 0, "STM32 Hard-I2C", OLED_FONT_8X16);
    
    // 像 printf 一样打印变量
    OLED_Printf(0, 2, OLED_FONT_6X8, "System Core: %d MHz", SystemCoreClock/1000000);
    
    while(1) {}
}
```

### 场景 B：调试/副屏 (使用软件 I2C)

```c
#include "soft_oled.h"

int main(void) {
    /* ... SystemClock & GPIO Init ... */
    
    // DWT 会自动初始化，无需手动调用 delay_init
    SoftOLED_Init(); 
    
    // 使用大号字体 (12x24)
    SoftOLED_ShowString(0, 0, "Soft-I2C Running", OLED_FONT_12X24);
    
    int count = 0;
    while(1) {
        SoftOLED_Printf(0, 3, OLED_FONT_8X16, "Count: %d", count++);
        HAL_Delay(100);
    }
}
```

### 场景 C：双屏同显 (Dual Screen)

得益于完全解耦的设计，你可以同时驱动两个屏幕！

```c
OLED_Init();      // 屏幕 1 (硬件 I2C)
SoftOLED_Init();  // 屏幕 2 (软件 I2C)

OLED_ShowString(0, 0, "MAIN SCREEN", OLED_FONT_8X16);
SoftOLED_ShowString(0, 0, "DEBUG SCREEN", OLED_FONT_8X16);
```

## ⚠️ 避坑指南 (Troubleshooting)

1. **OLED 不亮**：
   - 检查 `OLED_Init` 函数中的电荷泵设置。大多数 SSD1306 模组需要开启电荷泵 (`0x8D, 0x14`)。
   - 如果是外接 5V 升压的特殊屏幕，请改为 (`0x8D, 0x10`)。
2. **软件 I2C 速度过快**：
   - 本库默认 I2C 时钟约 400kHz。如果你的杜邦线太长导致信号完整性差，可以在 `soft_oled.c` 中将 `I2C_DELAY()` 改为 `delay_us(2)` 或更高。
3. **DWT 无法运行**：
   - Cortex-M0 (F0/L0) 内核没有 DWT 单元，软件 I2C 驱动无法使用高精度延时，请手动替换为普通的 `for` 循环延时。

## 📜 许可证 (License)

MIT License. 开源万岁！