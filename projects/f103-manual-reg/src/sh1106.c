/**
 * @file    sh1106.c
 * @brief   SH1106 I2C 初始化、1024 B 帧缓冲、按页刷新
 *
 * 控制字节：0x00 命令，0x40 GDDRAM 数据。省略则黑屏。
 * 电荷泵 0x8D,0x14 必须发（3.3 V 升到面板高压；不是地址、不是分页）。
 * 页 = 8 行横带；每页：0xB0+page、列低 0x02、列高 0x10，再 0x40+128 列字节。
 * 一列字节 bit0=该页顶。Clear/Draw* 只改 RAM，Refresh 才发像素。
 * 4 针模块无 RES，用忙等代替复位脚。调用前须已 I2C1_Init。
 *
 * @see     doc/reference/sh1106/README.md
 */

#include "sh1106.h"
#include "i2c.h"
#include "sh1106_font.h"

#define SH1106_CTRL_CMD  0x00U
#define SH1106_CTRL_DATA 0x40U

unsigned char sh1106_buf[SH1106_PAGES][SH1106_WIDTH];

/**
 * @brief  约 100 ms 忙等（72 MHz 经验计数；4 针无 RES）
 */
static void sh1106_delay_100ms(void)
{
    volatile unsigned int n;
    unsigned int k;

    for (k = 0U; k < 6U; k++) {
        n = 0xFFFFFU;
        while (n != 0U) {
            n--;
        }
    }
}

static unsigned char sh1106_write_cmd(unsigned char cmd)
{
    unsigned char pkt[2];

    pkt[0] = SH1106_CTRL_CMD;
    pkt[1] = cmd;
    return I2C1_Write(SH1106_ADDR_WR, pkt, 2U);
}

/** 一页 128 列：控制字节 0x40 后从左到右各一字节（管内 8 个竖点） */
static unsigned char sh1106_write_page(const unsigned char *data)
{
    unsigned char pkt[1U + SH1106_WIDTH];
    unsigned int i;

    pkt[0] = SH1106_CTRL_DATA;
    for (i = 0U; i < SH1106_WIDTH; i++) {
        pkt[1U + i] = data[i];
    }
    return I2C1_Write(SH1106_ADDR_WR, pkt, 1U + SH1106_WIDTH);
}

static unsigned char sh1106_set_page_col(unsigned char page)
{
    unsigned char col_lo;
    unsigned char col_hi;

    col_lo = (unsigned char)(0x00U | (SH1106_COL_OFFSET & 0x0FU));
    col_hi = (unsigned char)(0x10U | ((SH1106_COL_OFFSET >> 4) & 0x0FU));

    if (sh1106_write_cmd((unsigned char)(0xB0U + page)) == 0U) {
        return 0U;
    }
    if (sh1106_write_cmd(col_lo) == 0U) {
        return 0U;
    }
    return sh1106_write_cmd(col_hi);
}

void SH1106_Clear(void)
{
    unsigned int page;
    unsigned int x;

    for (page = 0U; page < SH1106_PAGES; page++) {
        for (x = 0U; x < SH1106_WIDTH; x++) {
            sh1106_buf[page][x] = 0x00U;
        }
    }
}

/** page=y/8，该列 buf[page][x] 的 bit=y%8（bit0=页顶） */
void SH1106_DrawPixel(unsigned int x, unsigned int y, unsigned char set)
{
    unsigned int page;
    unsigned int bit;

    if ((x >= SH1106_WIDTH) || (y >= SH1106_HEIGHT)) {
        return;
    }

    page = y / 8U;
    bit = y % 8U;
    if (set != 0U) {
        sh1106_buf[page][x] |= (unsigned char)(1U << bit);
    } else {
        sh1106_buf[page][x] &= (unsigned char)~(1U << bit);
    }
}

#define SH1106_CLOCK_SCALE    2U
#define SH1106_CLOCK_GLYPH_W  (SH1106_FONT_WIDTH * SH1106_CLOCK_SCALE)
#define SH1106_CLOCK_GLYPH_H  (SH1106_FONT_HEIGHT * SH1106_CLOCK_SCALE)
#define SH1106_CLOCK_CHARS    8U

static void sh1106_draw_char_2x(unsigned int x0, unsigned int y0, unsigned char ch)
{
    const unsigned char *glyph;
    unsigned int row;
    unsigned int col;
    unsigned int dx;
    unsigned int dy;
    unsigned char bits;

    glyph = SH1106_Font8x16(ch);
    for (row = 0U; row < SH1106_FONT_HEIGHT; row++) {
        bits = glyph[row];
        for (col = 0U; col < SH1106_FONT_WIDTH; col++) {
            if ((bits & (unsigned char)(0x80U >> col)) != 0U) {
                for (dy = 0U; dy < SH1106_CLOCK_SCALE; dy++) {
                    for (dx = 0U; dx < SH1106_CLOCK_SCALE; dx++) {
                        SH1106_DrawPixel(x0 + (col * SH1106_CLOCK_SCALE) + dx,
                                         y0 + (row * SH1106_CLOCK_SCALE) + dy,
                                         1U);
                    }
                }
            }
        }
    }
}

/**
 * @brief  只改 RAM：把 HH:MM:SS 画到缓冲中央。不上 I2C，须再 Refresh。
 *
 * 字库 8×16，放大 2 倍 → 每字 16×32。8 字刚好铺满 128 列：
 *   x = (128 - 8*16) / 2 = 0
 *   y = (64 - 32) / 2 = 16  → 占 page 2–5（y=16..47）
 * 每个字库像素变成 2×2，由 sh1106_draw_char_2x → DrawPixel。
 * 本函数不管计时；时基在 main / SysTick。
 */
void SH1106_DrawClock(unsigned int hour, unsigned int minute, unsigned int second)
{
    unsigned char text[SH1106_CLOCK_CHARS];
    unsigned int i;
    unsigned int x;
    unsigned int y;

    hour %= 24U;
    minute %= 60U;
    second %= 60U;

    /* 8 个 ASCII：十位时、个位时、冒号、分、分、冒号、秒、秒 */
    text[0] = (unsigned char)('0' + (hour / 10U));
    text[1] = (unsigned char)('0' + (hour % 10U));
    text[2] = (unsigned char)':';
    text[3] = (unsigned char)('0' + (minute / 10U));
    text[4] = (unsigned char)('0' + (minute % 10U));
    text[5] = (unsigned char)':';
    text[6] = (unsigned char)('0' + (second / 10U));
    text[7] = (unsigned char)('0' + (second % 10U));

    /* 水平 / 垂直居中；当前常量下 x=0、y=16 */
    x = (SH1106_WIDTH - (SH1106_CLOCK_CHARS * SH1106_CLOCK_GLYPH_W)) / 2U;
    y = (SH1106_HEIGHT - SH1106_CLOCK_GLYPH_H) / 2U;

    /* 从左到右每个字占 16 列，同一 y */
    for (i = 0U; i < SH1106_CLOCK_CHARS; i++) {
        sh1106_draw_char_2x(x + (i * SH1106_CLOCK_GLYPH_W), y, text[i]);
    }
}

/** 对 8 页各发设页/列 + 128 字节；此时才上 I2C */
void SH1106_Refresh(void)
{
    unsigned char page;

    for (page = 0U; page < SH1106_PAGES; page++) {
        if (sh1106_set_page_col(page) == 0U) {
            return;
        }
        if (sh1106_write_page(sh1106_buf[page]) == 0U) {
            return;
        }
    }
}

void SH1106_Init(void)
{
    sh1106_delay_100ms();

    (void)sh1106_write_cmd(0xAEU); /* 关显示 */
    (void)sh1106_write_cmd(0xD5U);
    (void)sh1106_write_cmd(0x80U); /* 时钟分频 */
    (void)sh1106_write_cmd(0xA8U);
    (void)sh1106_write_cmd(0x3FU); /* MUX 1/64 */
    (void)sh1106_write_cmd(0xD3U);
    (void)sh1106_write_cmd(0x00U); /* display offset */
    (void)sh1106_write_cmd(0x40U); /* start line 0 */
    (void)sh1106_write_cmd(0x8DU);
    (void)sh1106_write_cmd(0x14U); /* 内部电荷泵，必须开 */
    (void)sh1106_write_cmd(0xA1U); /* SEG remap */
    (void)sh1106_write_cmd(0xC8U); /* COM 扫向 */
    (void)sh1106_write_cmd(0xDAU);
    (void)sh1106_write_cmd(0x12U); /* COM 硬件 */
    (void)sh1106_write_cmd(0x81U);
    (void)sh1106_write_cmd(0xCFU); /* 对比度 */
    (void)sh1106_write_cmd(0xD9U);
    (void)sh1106_write_cmd(0xF1U); /* 预充电 */
    (void)sh1106_write_cmd(0xDBU);
    (void)sh1106_write_cmd(0x30U); /* VCOMH */
    (void)sh1106_write_cmd(0xA4U); /* 跟 RAM */
    (void)sh1106_write_cmd(0xA6U); /* 正常极性 */

    SH1106_Clear();
    SH1106_Refresh();

    (void)sh1106_write_cmd(0xAFU);
    sh1106_delay_100ms();
}
