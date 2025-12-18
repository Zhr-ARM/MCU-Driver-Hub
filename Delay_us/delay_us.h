#ifndef __DELAY_US_H__
#define __DELAY_US_H__

#ifdef __cplusplus
extern "C" {
#endif

/* 根据你的芯片系列包含正确的头文件 */
#include "main.h"

/* 如果使用了 FreeRTOS，请取消下面这行的注释，或者在全局宏定义中添加 USE_FREERTOS */
// #define USE_FREERTOS
#define USE_FREERTOS
#if defined(USE_FREERTOS)
    #include "FreeRTOS.h"
    #include "task.h"
#endif

/**
 * @brief 初始化 DWT 延时组件
 */
void delay_init(void);

/**
 * @brief 微秒延时 (DWT 忙等待)
 * @param us 微秒数
 */
void delay_us(uint32_t us);

/**
 * @brief 智能延时 (根据时长自动选择忙等待或操作系统挂起)
 * @param us 微秒数
 */
void delay_smart_us(uint32_t us);

#ifdef __cplusplus
}
#endif

#endif /* __DELAY_US_H__ */