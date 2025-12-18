/**
 * @file dma_fifo_print.h
 * @brief STM32 通用 DMA 环形缓冲区串口打印库
 * @author 退休嵌入式老工程师
 * @note 适用于 STM32 HAL 库，支持 F1/F4/H7 等全系列
 */

#ifndef __DMA_FIFO_PRINT_H__
#define __DMA_FIFO_PRINT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h" /* 引入 main.h 以获取具体的 HAL 库定义 (如 stm32f4xx_hal.h) */
#include <stdio.h>

/* 定义缓冲区大小，必须是 2 的幂次方便位运算，或者根据内存调整 */
#define TX_RING_BUFFER_SIZE 1024 

/**
 * @brief 环形缓冲区管理结构体
 */
typedef struct {
    UART_HandleTypeDef *huart;        // 关联的 UART 句柄
    uint8_t buffer[TX_RING_BUFFER_SIZE]; // 静态分配的缓冲区
    volatile uint16_t head;           // 写指针 (Head)
    volatile uint16_t tail;           // 读/DMA指针 (Tail)
    volatile uint8_t dma_is_busy;     // DMA 忙碌标志位
} DMA_Print_Handle_t;

/**
 * @brief 初始化打印服务
 * @param hprint 打印句柄指针
 * @param huart STM32 HAL UART 句柄指针
 */
void DMA_Printf_Init(DMA_Print_Handle_t *hprint, UART_HandleTypeDef *huart);

/**
 * @brief 核心处理函数，将数据写入缓冲区并尝试启动 DMA
 * @param hprint 打印句柄
 * @param data 要发送的数据指针
 * @param len 数据长度
 */
void DMA_Printf_Push(DMA_Print_Handle_t *hprint, uint8_t *data, uint16_t len);

/**
 * @brief DMA 发送完成回调
 * @note 必须在 main.c 或 stm32xx_it.c 的 HAL_UART_TxCpltCallback 中调用此函数
 * @param hprint 打印句柄
 */
void DMA_Printf_TxCpltCallback(DMA_Print_Handle_t *hprint);

/* * 全局单例句柄声明 
 * 为了方便 printf 重定向，我们需要一个全局的默认实例
 */
extern DMA_Print_Handle_t g_dma_print_handle;

#ifdef __cplusplus
}
#endif

#endif /* __DMA_FIFO_PRINT_H__ */