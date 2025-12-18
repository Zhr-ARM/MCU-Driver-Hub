/**
 * @file delay_us.c
 * @brief 基于 Cortex-M DWT (Data Watchpoint and Trace) 的高精度微秒延时库
 * @author 退休全栈嵌入式专家团
 * @note   兼容 STM32F1/F4/F7/H7 等 Cortex-M3/M4/M7 内核
 */

#include "delay_us.h"

/* 记录每微秒需要的 CPU 周期数 (Ticks) */
static volatile uint32_t us_ticks = 0;

/**
 * @brief  初始化 DWT 计数器
 * @note   在系统时钟配置完成后调用一次即可
 */
void delay_init(void)
{
    /* 1. 确保 SystemCoreClock 已更新为当前实际主频 */
    SystemCoreClockUpdate();

    /* 2. 计算 1us 需要多少个时钟周期
     * 例如：72MHz 主频 -> 1us = 72 个周期
     * 例如：400MHz 主频 -> 1us = 400 个周期
     */
    us_ticks = SystemCoreClock / 1000000;

    /* 3. 解锁 DWT (对于某些 STM32，如 H7，或者是被调试器锁住的情况)
     * CoreDebug->DEMCR 的 TRCENA 位必须置 1 才能使用 DWT
     */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    /* 4. 清零周期计数器 (CYCCNT) */
    DWT->CYCCNT = 0;

    /* 5. 开启周期计数器
     * DWT_CTRL 的 CYCCNTENA 位置 1
     */
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

/**
 * @brief  微秒级延时 (阻塞模式，但精度极高)
 * @param  us: 延时微秒数
 * @note   对于 < 100us 的延时，建议直接使用此函数。
 * DWT 是 32 位计数器，在 400MHz 下约 10 秒溢出，
 * 但下方的减法逻辑利用了无符号数回绕特性，
 * 只要延时不超过 2^32 个 tick (约 10秒) 都是安全的。
 */
void delay_us(uint32_t us)
{
    uint32_t start_tick = DWT->CYCCNT;
    uint32_t target_ticks = us * us_ticks;

    /* * 这里的精髓在于利用 uint32_t 的溢出回绕特性。
     * 即使 start_tick 接近 0xFFFFFFFF，
     * (current - start) 的计算结果依然是正确的 tick 差值。
     */
    while ((DWT->CYCCNT - start_tick) < target_ticks)
    {
        /* * 在 FreeRTOS 环境下，如果这是一个极短的延时，
         * 我们不希望调度器打断我们，导致延时变长（例如变成 1ms）。
         * 但为了系统的实时性，通常我们允许中断发生。
         * 如果需要绝对精确，可以在这里加 taskENTER_CRITICAL()，但慎用。
         */
        __NOP(); 
    }
}

/**
 * @brief  智能延时函数 (自动选择阻塞或 OS 挂起)
 * @param  us: 延时微秒数
 * @note   如果检测到运行了 FreeRTOS 且延时较长，则挂起任务；
 * 否则使用 DWT 死等。
 */
void delay_smart_us(uint32_t us)
{
    /* 阈值设定：比如 2000us (2ms) */
    /* 如果你在 FreeRTOS 下且 tick 是 1ms，那么小于 1ms 的延时也没法 yield */
    
#if defined(USE_FREERTOS)
    /* 假设 FreeRTOS 的 tick 是 1000Hz (1ms) */
    const uint32_t os_threshold_us = 2000; 

    if (us >= os_threshold_us)
    {
        /* 转换为毫秒并向上取整，调用 RTOS 延时 */
        vTaskDelay((us / 1000) + 1); 
    }
    else
    {
        delay_us(us);
    }
#else
    /* 裸机模式，只能死等 */
    delay_us(us);
#endif
}