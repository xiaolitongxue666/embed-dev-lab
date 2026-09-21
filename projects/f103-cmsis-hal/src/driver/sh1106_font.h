/**
 * @file    sh1106_font.h
 * @brief   8×16 点阵：'0'–'9'、':'、'.'、'-'、'C'
 *
 * 每字 16 行 × 1 字节，bit7 为最左像素。
 */

#ifndef SH1106_FONT_H
#define SH1106_FONT_H

#define SH1106_FONT_WIDTH  8U
#define SH1106_FONT_HEIGHT 16U

const unsigned char *SH1106_Font8x16(unsigned char ch);

#endif /* SH1106_FONT_H */
