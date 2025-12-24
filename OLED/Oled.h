#ifndef __OLED_H
#define __OLED_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "font.h"  // 包含字体定义
/* --- 配置区 --- */
// 定义使用的 I2C 句柄，外部引用
extern I2C_HandleTypeDef hi2c1;
#define OLED_I2C_HANDLE   &hi2c1
#define OLED_I2C_ADDR     0x78  // 已经左移过的 8-bit 地址 (0x3C << 1)

/* --- API --- */
void OLED_Init(void);
void OLED_Clear(void);
void OLED_SetCursor(uint8_t x, uint8_t page);
void OLED_ShowChar(uint8_t x, uint8_t page, char c, OLED_FontSize font);
void OLED_ShowString(uint8_t x, uint8_t page, const char *str, OLED_FontSize font);

// [老张赠送] 像 printf 一样打印调试信息
void OLED_Printf(uint8_t x, uint8_t page, OLED_FontSize font, const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif

