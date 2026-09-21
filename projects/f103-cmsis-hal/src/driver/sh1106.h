/**
 * @file    sh1106.h
 * @brief   SH1106 1.3″ 4 针 I2C OLED 应用接口
 *
 * 分层：main → 本头文件 → sh1106.c → 命令 I2C1_Write / 页 I2C1_WriteDma → i2c.c
 * （HAL 路径：I2C1_WriteDma 名保留，实为 HAL_I2C_Master_Transmit 轮询）
 *
 * 总线 / 地址：
 *   PB6=SCL，PB7=SDA（I2C1 默认映射，复用开漏）。模块板载上拉。
 *   SH1106_ADDR_WR=0x78 为 **8 位写地址**，原样进 I2C DR，禁止再 << 1。
 *   读地址 0x79 本驱动不用。
 *
 * I2C 控制字节（每帧数据前必须先发）：
 *   0x00  Co=0 D/C#=0  后续为命令
 *   0x40  Co=0 D/C#=1  后续为 GDDRAM
 *
 * 显存：片内 132×64，可视 128×64。页 = 8 行高的横带，共 8 条（page0=y0..7）。
 * 缓冲 buf[page][x]：一字节一列，bit0=该页最上一行，bit7=最下一行。
 * 每页写前发 0xB0+page、列 0x02、0x10（轮询）；0x40 后 128 字节走 I2C1_WriteDma（HAL 轮询）。
 *
 * 调用顺序：I2C1_Init → I2C1_Probe(0x78) → SH1106_Init
 *   → 改缓冲（Clear / DrawPixel / DrawClock / DrawTemp）→ SH1106_Refresh。
 * Init 内会清 RAM 并开显示。4 针无 RES，Init 用忙等代替复位脚。
 *
 * @see     sh1106.c
 * @see     doc/reference/sh1106/README.md
 */

#ifndef SH1106_H
#define SH1106_H

/** 8 位写地址；写入 I2C DR 时不再左移 */
#define SH1106_ADDR_WR  0x78U

#define SH1106_WIDTH    128U
#define SH1106_HEIGHT   64U
/** 页数：64 行 / 8 = 8 条横带 */
#define SH1106_PAGES    8U

/** GDDRAM 可见区从列 2 起（132−128=4，两侧各约 2） */
#define SH1106_COL_OFFSET 2U

/** 发 init 序列、清屏、开显示；调用前须 I2C1_Init 且 Probe 成功 */
void SH1106_Init(void);

/** 只清 RAM 帧缓冲，不写屏；须再 Refresh */
void SH1106_Clear(void);

/** 8 页缓冲按列偏移 2 写入 GDDRAM；像素只在此时上 I2C */
void SH1106_Refresh(void);

/** 写缓冲一点；page=y/8，该列字节 bit=y%8（bit0=页顶）。须再 Refresh */
void SH1106_DrawPixel(unsigned int x, unsigned int y, unsigned char set);

/**
 * @brief  在缓冲中央画 00:00:00:00（8×16，时:分:秒:百分秒）
 * @note   不 Refresh；时基在 main / TimerEvent，本函数不管计时
 */
void SH1106_DrawClock(unsigned int hour, unsigned int minute,
                      unsigned int second, unsigned int centi);

/**
 * @brief  右下角画补偿温度（8×16，y=48），单位 0.01℃
 * @note   不 Refresh；不改 DrawClock
 */
void SH1106_DrawTemp(int temp_centi);

#endif /* SH1106_H */
