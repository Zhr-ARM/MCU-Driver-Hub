#ifndef __SOFT_OLED_H
#define __SOFT_OLED_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h" // 确保能包含 GPIO 定义
#include "font.h" // 包含字体定义
/* ================= 用户配置区 ================= */
/* 修改这里的宏定义来适配你的硬件引脚 */

// SCL 引脚定义 (例如: PB6)
#define OLED_SCL_PORT   GPIOB
#define OLED_SCL_PIN    GPIO_PIN_6

// SDA 引脚定义 (例如: PB7)
#define OLED_SDA_PORT   GPIOB
#define OLED_SDA_PIN    GPIO_PIN_7

/* ================= OLED 协议层 ================= */

#define OLED_ADDR       0x78 // I2C地址 (0x3C << 1)
#define OLED_CMD_MODE   0x00
#define OLED_DATA_MODE  0x40

/* API 函数声明 */
void SoftOLED_Init(void);
void SoftOLED_Clear(void);
void SoftOLED_SetCursor(uint8_t x, uint8_t page);
void SoftOLED_ShowChar(uint8_t x, uint8_t page, char c, OLED_FontSize font);
void SoftOLED_ShowString(uint8_t x, uint8_t page, const char *str, OLED_FontSize font);
// 格式化打印 (类似于 printf)
void SoftOLED_Printf(uint8_t x, uint8_t page, OLED_FontSize font, const char *format, ...);
#ifdef __cplusplus
}
#endif

#endif /* __SOFT_OLED_H */