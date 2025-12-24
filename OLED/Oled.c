#include "Oled.h"
#include "font.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// SSD1306 Control Bytes
#define OLED_CMD_MODE  0x00
#define OLED_DATA_MODE 0x40

/**
 * @brief  内部使用的写命令函数 (优化版)
 * @note   利用 HAL_I2C_Mem_Write 直接发送，减少总线 Start/Stop 开销
 */
static void OLED_WriteCommand(uint8_t cmd)
{
    // MemAddress = 0x00，表示写入的是命令
    HAL_I2C_Mem_Write(OLED_I2C_HANDLE, OLED_I2C_ADDR, OLED_CMD_MODE, 
                      I2C_MEMADD_SIZE_8BIT, &cmd, 1, 10);
}

/**
 * @brief  内部使用的写数据函数 (大师级优化：零拷贝)
 * @note   直接发送指针指向的数据，无需在栈上开辟 buffer 搬运
 */
static void OLED_WriteData(const uint8_t *data, uint16_t len)
{
    // MemAddress = 0x40，表示写入的是数据 (GDDRAM)
    // 我们可以直接把 Flash 里的字模指针传进来，HAL 库会直接读 Flash 发送
    // 效率极大提升，且不会爆栈
    HAL_I2C_Mem_Write(OLED_I2C_HANDLE, OLED_I2C_ADDR, OLED_DATA_MODE, 
                      I2C_MEMADD_SIZE_8BIT, (uint8_t *)data, len, 100);
}

/**
 * @brief  获取字模信息的查表函数 (补全了 12x24)
 */
static void OLED_GetAsciiGlyph(char c, OLED_FontSize font, const uint8_t **glyph, uint8_t *width, uint8_t *pages)
{
    uint8_t idx = c - ' '; // 简单偏移，前提是已过滤不可见字符
    if (c < ' ' || c > '~') {
        idx = '?' - ' ';   // 非法字符替换为 '?'
    }

    switch (font) {
    case OLED_FONT_6X12:
        *glyph = asc2_1206[idx]; *width = 6; *pages = 2;
        break;
    case OLED_FONT_8X16:
        *glyph = asc2_1608[idx]; *width = 8; *pages = 2;
        break;
    case OLED_FONT_12X24:
        *glyph = asc2_2412[idx]; *width = 12; *pages = 3; // 24px 高需要 3 页
        break;
    case OLED_FONT_6X8:
    default:
        *glyph = asc2_0806[idx]; *width = 6; *pages = 1;
        break;
    }
}

/**
 * @brief  设置光标 (原子化操作)
 * @note   一次 I2C 传输发送 3 个指令，比分开发送快 3 倍
 */
void OLED_SetCursor(uint8_t x, uint8_t page)
{
    if (page > 7) page = 7;
    if (x > 127) x = 127;

    // 构造指令包：
    // [0] = Control Byte (0x00) -> 告诉 SSD1306 后面全是命令
    // [1] = Set Page Address
    // [2] = Set Lower Column
    // [3] = Set Higher Column
    uint8_t cmds[4];
    cmds[0] = 0x00; 
    cmds[1] = 0xB0 | page;
    cmds[2] = 0x00 | (x & 0x0F);
    cmds[3] = 0x10 | ((x >> 4) & 0x0F);

    // 使用 Master_Transmit 一次性发出去
    HAL_I2C_Master_Transmit(OLED_I2C_HANDLE, OLED_I2C_ADDR, cmds, 4, 10);
}

void OLED_Clear(void)
{
    uint8_t zero_buf[128] = {0}; // 栈上开128字节通常没问题，甚至可以更大
    // 某些低端单片机如果栈不够，可以改小分批刷，或者定义为 static

    for (uint8_t i = 0; i < 8; i++) {
        OLED_SetCursor(0, i);
        // 一次刷一整行 (128字节)，利用 I2C 连续写入特性
        OLED_WriteData(zero_buf, 128);
    }
}

void OLED_Init(void)
{
    // 初始化序列建议紧凑发送，这里为了可读性保留原样，
    // 实际生产中可以定义一个 const 数组一次性发完。
    HAL_Delay(100); // 上电延时

    OLED_WriteCommand(0xAE); // Display Off

    OLED_WriteCommand(0xD5); OLED_WriteCommand(0x80); // Clock Divide
    OLED_WriteCommand(0xA8); OLED_WriteCommand(0x3F); // Multiplex
    OLED_WriteCommand(0xD3); OLED_WriteCommand(0x00); // Offset
    OLED_WriteCommand(0x40);                          // Start Line

    OLED_WriteCommand(0x8D); OLED_WriteCommand(0x14); // Charge Pump (重要!)

    OLED_WriteCommand(0x20); OLED_WriteCommand(0x02); // Page Addressing Mode (0x02更稳健)
    
    OLED_WriteCommand(0xA1); // Segment Remap
    OLED_WriteCommand(0xC8); // COM Scan Direction

    OLED_WriteCommand(0xDA); OLED_WriteCommand(0x12); // COM Pins
    OLED_WriteCommand(0x81); OLED_WriteCommand(0xCF); // Contrast
    OLED_WriteCommand(0xD9); OLED_WriteCommand(0xF1); // Pre-charge
    OLED_WriteCommand(0xDB); OLED_WriteCommand(0x40); // VCOM Detect

    OLED_WriteCommand(0xA4); // Resume to RAM
    OLED_WriteCommand(0xA6); // Normal Display
    OLED_WriteCommand(0xAF); // Display On

    OLED_Clear();
}

/**
 * @brief 显示字符 (核心绘制函数)
 */
void OLED_ShowChar(uint8_t x, uint8_t page, char c, OLED_FontSize font)
{
    const uint8_t *glyph = NULL;
    uint8_t width = 0, pages = 0;

    // 1. 获取字模
    OLED_GetAsciiGlyph(c, font, &glyph, &width, &pages);
    if (!glyph) return;

    // 2. 越界保护
    if (x + width > 128) return;
    if (page + pages > 8) return;

    // 3. 分页绘制
    for (uint8_t p = 0; p < pages; p++) {
        OLED_SetCursor(x, page + p);
        // 这里的 glyph + p*width 是精髓
        // 因为你的字模是“分行式”取模 (Row-Major/Page-Major combination)
        // 第一页的数据都在前面，第二页的数据紧接在后
        OLED_WriteData(glyph + (p * width), width);
    }
}

/**
 * @brief 显示字符串 (支持自动换行和 \n)
 */
void OLED_ShowString(uint8_t x, uint8_t page, const char *str, OLED_FontSize font)
{
    const uint8_t *dummy_glyph = NULL;
    uint8_t char_w = 0, char_h_pages = 0;
    
    // 获取当前字体的高度和宽度信息 (假设等宽)
    OLED_GetAsciiGlyph('A', font, &dummy_glyph, &char_w, &char_h_pages);

    while (*str) {
        // [牛点] 处理换行符
        if (*str == '\n') {
            x = 0;
            page += char_h_pages;
            str++;
            continue;
        }

        // [牛点] 自动换行
        if (x + char_w > 128) {
            x = 0;
            page += char_h_pages;
        }

        // 底部越界检查
        if (page + char_h_pages > 8) break;

        OLED_ShowChar(x, page, *str, font);
        x += char_w;
        str++;
    }
}

// 格式化打印函数
void OLED_Printf(uint8_t x, uint8_t page, OLED_FontSize font, const char *format, ...)
{
    char str_buf[64]; // 缓冲区
    va_list args;

    va_start(args, format);
    vsnprintf(str_buf, sizeof(str_buf), format, args);
    va_end(args);

    OLED_ShowString(x, page, str_buf, font);
}
