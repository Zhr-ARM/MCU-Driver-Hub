/**
 * @file dma_fifo_print.c
 * @brief 实现文件
 */

#include "dma_fifo_print.h"
#include <string.h> // memcpy

/* 定义全局实例，方便 fputc/_write 调用 */
DMA_Print_Handle_t g_dma_print_handle;

/**
 * @brief 初始化
 */
void DMA_Printf_Init(DMA_Print_Handle_t *hprint, UART_HandleTypeDef *huart) {
    hprint->huart = huart;
    hprint->head = 0;
    hprint->tail = 0;
    hprint->dma_is_busy = 0;
}

/**
 * @brief 内部函数：尝试启动 DMA 传输
 * @note 这是一个非阻塞函数，只计算长度并告诉 DMA 搬运工干活
 */
static void DMA_Try_Transmit(DMA_Print_Handle_t *hprint) {
    // 1. 如果 DMA 正在忙，直接退出，让它干完手里的活再说
    if (hprint->dma_is_busy) {
        return;
    }

    // 2. 如果头尾重合，说明没数据，收工
    if (hprint->head == hprint->tail) {
        return;
    }

    // 3. 计算这次能搬运多长的数据
    uint16_t length_to_send;
    
    if (hprint->head > hprint->tail) {
        // 情况A: 线性，未回绕 [---TxxxxH---]
        length_to_send = hprint->head - hprint->tail;
    } else {
        // 情况B: 回绕 [xxH-------Txx]
        // 先发 Tail 到 BufferEnd 的这一段
        length_to_send = TX_RING_BUFFER_SIZE - hprint->tail;
    }

    // 4. 标记忙碌，启动 DMA
    hprint->dma_is_busy = 1;
    
    // 注意：这里使用 HAL_UART_Transmit_DMA
    if (HAL_UART_Transmit_DMA(hprint->huart, 
                             (uint8_t *)&hprint->buffer[hprint->tail], 
                             length_to_send) != HAL_OK) {
        // 如果启动失败（极其罕见），清除忙碌标志，防止死锁
        hprint->dma_is_busy = 0;
    }
}

/**
 * @brief 将数据推入环形缓冲区
 */
void DMA_Printf_Push(DMA_Print_Handle_t *hprint, uint8_t *data, uint16_t len) {
    uint16_t i;
    
    for (i = 0; i < len; i++) {
        uint16_t next_head = (hprint->head + 1) % TX_RING_BUFFER_SIZE;
        
        // 检查缓冲区是否满了
        if (next_head != hprint->tail) {
            hprint->buffer[hprint->head] = data[i];
            hprint->head = next_head;
        } else {
            // 缓冲区溢出策略：丢弃数据
            break; 
        }
    }
    
    // 尝试触发发送
    DMA_Try_Transmit(hprint);
}

/**
 * @brief 用户需要在 HAL_UART_TxCpltCallback 中调用此函数
 */
void DMA_Printf_TxCpltCallback(DMA_Print_Handle_t *hprint) {
    if (hprint->dma_is_busy) {
        // 利用 HAL 库记录的本次传输长度来更新尾指针
        uint16_t sent_len = hprint->huart->TxXferSize; 
        
        // 更新 Tail
        hprint->tail = (hprint->tail + sent_len) % TX_RING_BUFFER_SIZE;
        
        // 标记空闲
        hprint->dma_is_busy = 0;
        
        // 看看还有没有剩下的数据需要发
        DMA_Try_Transmit(hprint);
    }
}

/* * ============================================================
 * printf 重定向接口
 * ============================================================
 */

/* Keil MDK (ARMCC 或 ARMCLANG) */
#if defined(__CC_ARM) || defined(__ARMCC_VERSION)

/* Keil 标准库需要的 fputc 函数 */
int fputc(int ch, FILE *f) {
    // Keil 的 printf 是一个字一个字调用的，虽然效率稍低，
    // 但因为我们写入的是内存缓冲区（极快），所以不会阻塞 CPU。
    DMA_Printf_Push(&g_dma_print_handle, (uint8_t *)&ch, 1);
    return ch;
}

#elif defined(__GNUC__)

/* GCC / STM32CubeIDE 使用 _write */
int _write(int file, char *ptr, int len) {
    DMA_Printf_Push(&g_dma_print_handle, (uint8_t *)ptr, len);
    return len;
}

#endif