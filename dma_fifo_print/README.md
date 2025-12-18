# STM32 DMA 串口环形缓冲区打印库 (Non-blocking Printf)

这是一个轻量级、通用的 STM32 串口打印库。它利用 **DMA (直接存储器访问)** 和 **环形缓冲区 (Ring Buffer)** 技术，实现了高效、非阻塞的 `printf` 打印功能。

从此告别 `HAL_UART_Transmit` 的阻塞等待，让你的 CPU 腾出手来处理关键任务！

## ✨ 特性 (Features)

- **🚀 非阻塞 (Non-blocking)**: `printf` 函数瞬间返回，数据在后台由 DMA 搬运，不占用 CPU 宝贵时间。
- **🧩 零依赖 (Zero Coupling)**: 设计上与具体的 `main.h` 解耦，移植性极强，适用于 F1/F4/H7 等所有 STM32 系列。
- **🔌 双平台兼容**: 同时支持 **Keil MDK** (需开启 MicroLIB) 和 **GCC** (STM32CubeIDE) 环境。
- **🔄 环形缓冲区**: 自带缓冲区管理，自动处理数据回绕和连续发送逻辑。

## 📂 文件说明

将以下两个文件添加到你的工程中：

- `dma_fifo_print.h`: 头文件，包含配置和接口声明。
- `dma_fifo_print.c`: 源文件，包含核心实现和 `printf` 重定向。

## 🛠️ CubeMX 配置指南 (Prerequisites)

在使用本库之前，请确保在 STM32CubeMX 中正确配置了 UART：

1. **UART Mode**: Asynchronous (异步模式)。
2. **DMA Settings**:
   - Add **USARTx_TX** (例如 USART1_TX)。
   - Priority: **Low** / **Medium**。
   - Mode: **Normal** (⚠️注意：这里选 Normal，不要选 Circular，因为我们是按需发送)。
   - Data Width: Byte。
3. **NVIC Settings**:
   - **Enable** `USARTx global interrupt` (✅ 必须开启中断，否则 DMA 发送完成后无法触发回调，导致死锁)。
4. **Project Manager**:
   - 生成代码 (Generate Code)。

## 🚀 快速上手 (Getting Started)

### 1. 引入文件

将 `.c` 和 `.h` 文件放入工程目录（例如 `Core/Src` 和 `Core/Inc`），并在 IDE 中添加路径。

### 2. 初始化 (main.c)

在 `main()` 函数中引入头文件，并初始化库。

```
/* USER CODE BEGIN Includes */
#include "dma_fifo_print.h"
/* USER CODE END Includes */

int main(void) {
  /* ... HAL_Init(); SystemClock_Config(); MX_USART1_UART_Init(); ... */
  
  /* USER CODE BEGIN 2 */
  // 初始化打印库，绑定你的串口句柄 (如 &huart1)
  DMA_Printf_Init(&g_dma_print_handle, &huart1);
  
  printf("System Init OK! DMA Printf is ready.\r\n");
  /* USER CODE END 2 */

  while (1) {
    // 这里的 printf 极快，不会阻塞
    printf("Tick: %d\r\n", HAL_GetTick());
    HAL_Delay(500);
  }
}
```

### 3. 配置中断回调 (main.c 或 stm32xx_it.c)

这是最关键的一步！DMA 发送完成后，需要通知库去检查缓冲区是否还有剩余数据。

找到或添加 HAL_UART_TxCpltCallback，并调用库的处理函数。

```
/* USER CODE BEGIN 4 */

// 记得引入头文件
#include "dma_fifo_print.h"

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    // 判断是否是打印用的那个串口 (例如 USART1)
    if (huart->Instance == USART1) {
        // 调用库的回调函数，驱动后续数据发送
        DMA_Printf_TxCpltCallback(&g_dma_print_handle);
    }
}

/* USER CODE END 4 */
```

## ⚠️ Keil MDK 特别注意

如果你使用 Keil 开发，必须在工程选项中开启 MicroLIB，否则 `printf` 无法工作。

- 点击魔术棒 (Options for Target) -> **Target** 选项卡。
- 勾选 **Use MicroLIB**。

## ⚙️ 参数调整

在 `dma_fifo_print.h` 中，你可以根据 MCU 的 RAM 大小调整缓冲区：

```
// 建议设置为 2 的幂次，例如 512, 1024, 2048
#define TX_RING_BUFFER_SIZE 1024 
```

## 📝 许可证

MIT License. 既然是开源，就大胆拿去用吧！
