#include "soft_oled.h"
#include "delay_us.h"  // 包含你上传的高精度延时
#include <stdarg.h>
#include <stdio.h>

/* --- I2C 底层宏操作 (开漏输出模式) --- */
/* * 硬件老王注：
 * 只要 GPIO 初始化为 Open-Drain (开漏) + 上拉，
 * 写 1 就是释放总线(高电平)，写 0 就是拉低。
 * 读数据时不需要切换输入/输出模式，直接读 IDR 即可！
 */
#define OLED_SCL_H()    HAL_GPIO_WritePin(OLED_SCL_PORT, OLED_SCL_PIN, GPIO_PIN_SET)
#define OLED_SCL_L()    HAL_GPIO_WritePin(OLED_SCL_PORT, OLED_SCL_PIN, GPIO_PIN_RESET)

#define OLED_SDA_H()    HAL_GPIO_WritePin(OLED_SDA_PORT, OLED_SDA_PIN, GPIO_PIN_SET)
#define OLED_SDA_L()    HAL_GPIO_WritePin(OLED_SDA_PORT, OLED_SDA_PIN, GPIO_PIN_RESET)

// I2C 延时控制：1us 大约对应 400kHz-500kHz 的 I2C 速率
// 得益于 DWT，这个 1us 在任何主频下都是精准的 1us
#define I2C_DELAY()     delay_us(1) 

/* ================= 软件 I2C 驱动层 ================= */

/**
 * @brief I2C 起始信号
 */
static void I2C_Start(void)
{
    OLED_SDA_H();
    OLED_SCL_H();
    I2C_DELAY();
    OLED_SDA_L(); // SCL高期间，SDA拉低 -> START
    I2C_DELAY();
    OLED_SCL_L(); // 钳住总线
}

/**
 * @brief I2C 停止信号
 */
static void I2C_Stop(void)
{
    OLED_SDA_L();
    OLED_SCL_H();
    I2C_DELAY();
    OLED_SDA_H(); // SCL高期间，SDA拉高 -> STOP
    I2C_DELAY();
}

/**
 * @brief I2C 等待应答
 * @note  OLED 通常不回传数据，我们只发个时钟脉冲假装读ACK，不处理返回值以提高速度
 */
static void I2C_WaitAck(void)
{
    OLED_SDA_H(); // 释放SDA
    I2C_DELAY();
    OLED_SCL_H();
    I2C_DELAY();
    // 此时可以读取 SDA 状态，但 SSD1306 只有写操作，这里忽略 ACK 检查
    OLED_SCL_L();
}

/**
 * @brief I2C 发送一个字节
 */
static void I2C_SendByte(uint8_t byte)
{
    uint8_t i;
    for (i = 0; i < 8; i++)
    {
        if (byte & 0x80) {
            OLED_SDA_H();
        } else {
            OLED_SDA_L();
        }
        byte <<= 1;
        I2C_DELAY();
        OLED_SCL_H(); // 拉高 SCL 通知从机采样
        I2C_DELAY();
        OLED_SCL_L(); // 拉低 SCL 准备下一位
    }
    I2C_WaitAck();
}



/**
 * @brief 写命令
 */
static void SoftOLED_WriteCmd(uint8_t cmd)
{
    I2C_Start();
    I2C_SendByte(OLED_ADDR);
    I2C_SendByte(OLED_CMD_MODE);
    I2C_SendByte(cmd);
    I2C_Stop();
}

/**
 * @brief 写数据
 */
static void SoftOLED_WriteData(uint8_t data)
{
    I2C_Start();
    I2C_SendByte(OLED_ADDR);
    I2C_SendByte(OLED_DATA_MODE);
    I2C_SendByte(data);
    I2C_Stop();
}

/**
 * @brief 批量写数据 (优化版，只发送一次地址头)
 * @note  大幅提升刷屏和汉字显示速度
 */
static void SoftOLED_WriteDataBlock(const uint8_t *data, uint16_t len)
{
    I2C_Start();
    I2C_SendByte(OLED_ADDR);
    I2C_SendByte(OLED_DATA_MODE); // 开启连续数据传输模式
    for(uint16_t i=0; i<len; i++) {
        I2C_SendByte(data[i]);
    }
    I2C_Stop();
}

/* ================= OLED 业务逻辑层 ================= */

void SoftOLED_Init(void)
{
    // 1. 初始化 DWT 延时 (这一步至关重要！)
    delay_init();

    // 2. 初始化 GPIO
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // 开启时钟 (如果不确定总线，建议开启所有相关的)
    // 这里假设用户使用的是 GPIOB，如果改了宏定义，这里记得改时钟
    __HAL_RCC_GPIOB_CLK_ENABLE(); 

    GPIO_InitStruct.Pin = OLED_SCL_PIN | OLED_SDA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD; // [老师傅敲黑板] 必须开漏输出！
    GPIO_InitStruct.Pull = GPIO_PULLUP;         // 必须上拉
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH; // 高速模式
    HAL_GPIO_Init(OLED_SCL_PORT, &GPIO_InitStruct);

    // 3. 初始电平
    OLED_SCL_H();
    OLED_SDA_H();
    delay_us(200); // 上电稳定等待

    // 4. 发送初始化序列 (标准 SSD1306 初始化)
    SoftOLED_WriteCmd(0xAE); // Display Off
    SoftOLED_WriteCmd(0xD5); SoftOLED_WriteCmd(0x80);
    SoftOLED_WriteCmd(0xA8); SoftOLED_WriteCmd(0x3F);
    SoftOLED_WriteCmd(0xD3); SoftOLED_WriteCmd(0x00);
    SoftOLED_WriteCmd(0x40);
    SoftOLED_WriteCmd(0x8D); SoftOLED_WriteCmd(0x14); // Charge Pump Enable
    SoftOLED_WriteCmd(0x20); SoftOLED_WriteCmd(0x02); // Page Addressing Mode
    SoftOLED_WriteCmd(0xA1); // Segment Remap
    SoftOLED_WriteCmd(0xC8); // COM Scan Direction
    SoftOLED_WriteCmd(0xDA); SoftOLED_WriteCmd(0x12);
    SoftOLED_WriteCmd(0x81); SoftOLED_WriteCmd(0xCF);
    SoftOLED_WriteCmd(0xD9); SoftOLED_WriteCmd(0xF1);
    SoftOLED_WriteCmd(0xDB); SoftOLED_WriteCmd(0x40);
    SoftOLED_WriteCmd(0xA4);
    SoftOLED_WriteCmd(0xA6);
    SoftOLED_WriteCmd(0xAF); // Display On

    SoftOLED_Clear();
}

void SoftOLED_SetCursor(uint8_t x, uint8_t page)
{
    if (page > 7) page = 7;
    if (x > 127) x = 127;

    SoftOLED_WriteCmd(0xB0 | page);
    SoftOLED_WriteCmd(0x00 | (x & 0x0F));
    SoftOLED_WriteCmd(0x10 | ((x >> 4) & 0x0F));
}

void SoftOLED_Clear(void)
{
    uint8_t zero_buf[128] = {0}; // 全0缓冲区
    for (uint8_t i = 0; i < 8; i++) {
        SoftOLED_SetCursor(0, i);
        SoftOLED_WriteDataBlock(zero_buf, 128); // 极速清屏
    }
}

void SoftOLED_ShowChar(uint8_t x, uint8_t page, char c, OLED_FontSize font)
{
    const uint8_t *glyph = NULL;
    uint8_t width = 0, pages = 0;
    
    // 字符偏移计算
    uint8_t idx = c - ' ';
    if (c < ' ' || c > '~') idx = 0; // 简单保护

    // 根据 font.h 选择字库
    switch (font) {
        case OLED_FONT_6X8: // asc2_0806
            glyph = asc2_0806[idx]; width = 6; pages = 1;
            break;
        case OLED_FONT_6X12: // asc2_1206 (12字节, 6宽x2页)
            glyph = asc2_1206[idx]; width = 6; pages = 2;
            break;
        case OLED_FONT_8X16: // asc2_1608 (16字节, 8宽x2页)
            glyph = asc2_1608[idx]; width = 8; pages = 2;
            break;
        case OLED_FONT_12X24: // asc2_2412 (36字节, 12宽x3页)
            glyph = asc2_2412[idx]; width = 12; pages = 3;
            break;
        default:
            return;
    }

    if (x + width > 128) return;
    if (page + pages > 8) return;

    // 绘制
    for (uint8_t p = 0; p < pages; p++) {
        SoftOLED_SetCursor(x, page + p);
        // data指针偏移：当前页 * 宽度
        SoftOLED_WriteDataBlock(glyph + (p * width), width);
    }
}

void SoftOLED_ShowString(uint8_t x, uint8_t page, const char *str, OLED_FontSize font)
{
    // 预计算字体参数
    uint8_t width = 0, h_pages = 0;
    switch (font) {
        case OLED_FONT_6X8:  width = 6; h_pages = 1; break;
        case OLED_FONT_6X12: width = 6; h_pages = 2; break;
        case OLED_FONT_8X16: width = 8; h_pages = 2; break;
        case OLED_FONT_12X24: width = 12; h_pages = 3; break;
        default: return;
    }

    while (*str) {
        if (*str == '\n') { // 支持换行符
            x = 0;
            page += h_pages;
            str++;
            continue;
        }

        if (x + width > 128) { // 自动换行
            x = 0;
            page += h_pages;
        }

        if (page + h_pages > 8) break; // 底部越界停止

        SoftOLED_ShowChar(x, page, *str, font);
        x += width;
        str++;
    }
}

void SoftOLED_Printf(uint8_t x, uint8_t page, OLED_FontSize font, const char *format, ...)
{
    char str_buf[128];
    va_list args;
    va_start(args, format);
    vsnprintf(str_buf, sizeof(str_buf), format, args);
    va_end(args);
    SoftOLED_ShowString(x, page, str_buf, font);
}