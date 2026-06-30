/**
 * @file    usart.c
 * @brief   USART1 纯寄存器初始化与阻塞发送
 *
 * @target  STM32F103C8T6，USART1 默认引脚 PA9(TX) / PA10(RX)，挂 APB2 总线
 *
 * 时钟前提（见 system_stm32f1xx.c）：
 *   HSE 8 MHz × PLL×9 → SYSCLK 72 MHz
 *   APB2 不分频 → PCLK2 = 72 MHz（USART1 波特率分频时钟源）
 *
 * 波特率（RM0008 §27.3.1）：
 *   USARTDIV = PCLK2 / (16 × BaudRate)
 *            = 72_000_000 / (16 × 1_500_000) = 3（整除，无舍入误差）
 *   BRR（Baud Rate Register，波特率寄存器）[15:4]=3（整数部分），[3:0]=0（小数部分）→ 写入 0x0030
 *
 * 硬件接线（CH341 USB-TTL）：
 *   模块 RX ← PA9（MCU 发）
 *   模块 TX → PA10（MCU 收，本 demo 仅 printf 发送）
 *   GND 共地
 *
 * printf 路径：syscalls.c 中 _write() → USART1_Write()；`\n` 在 _write 内补 `\r`
 *
 * @see     doc/learn/stm32f103-mmio-basics.md
 */

#include "usart.h"

/* -------------------------------------------------------------------------- */
/* 外设基地址与寄存器映射（RM0008）
 * MMIO：volatile 指针解引用即总线读写，编译器不得优化掉重复访问。              */
/* -------------------------------------------------------------------------- */

/** @brief RCC 复位与时钟控制寄存器块基地址 */
#define RCC_BASE     0x40021000U
/** @brief APB2 外设时钟使能（USART1、GPIOA 均在此域） */
#define RCC_APB2ENR  (*(volatile unsigned int *)(RCC_BASE + 0x18U))

/** @brief GPIOA 端口寄存器基地址 */
#define GPIOA_BASE   0x40010800U
/** @brief GPIOA 端口配置高寄存器（PA8–PA15；PA9/PA10 在此配置） */
#define GPIOA_CRH    (*(volatile unsigned int *)(GPIOA_BASE + 0x04U))

/** @brief USART1 寄存器基地址（APB2） */
#define USART1_BASE  0x40013800U
/** @brief USART 状态寄存器 SR */
#define USART1_SR    (*(volatile unsigned int *)(USART1_BASE + 0x00U))
/** @brief USART 数据寄存器 DR（写发送、读接收） */
#define USART1_DR    (*(volatile unsigned int *)(USART1_BASE + 0x04U))
/** @brief BRR（Baud Rate Register，波特率寄存器）；决定 TX/RX 位速率 */
#define USART1_BRR   (*(volatile unsigned int *)(USART1_BASE + 0x08U))
/** @brief USART 控制寄存器 1 CR1（含 UE（USART Enable，CR1[13]）、TE/RE、字长等） */
#define USART1_CR1   (*(volatile unsigned int *)(USART1_BASE + 0x0CU))
/** @brief USART 控制寄存器 2 CR2（停止位等；8N1 时保持 0） */
#define USART1_CR2   (*(volatile unsigned int *)(USART1_BASE + 0x10U))

/* -------------------------------------------------------------------------- */
/* RCC_APB2ENR 位定义                                                           */
/* -------------------------------------------------------------------------- */

#define RCC_APB2ENR_IOPAEN   (1U << 2)   /**< 使能 GPIOA 端口时钟 */
#define RCC_APB2ENR_USART1EN (1U << 14)  /**< 使能 USART1 模块时钟 */

/* -------------------------------------------------------------------------- */
/* GPIOA_CRH 引脚配置（每 pin 占 4 bit：CNF[1:0] + MODE[1:0]）                  */
/* PA9/PA10 属于 CRH 管辖范围（pin ≥ 8）                                      */
/* -------------------------------------------------------------------------- */

/**
 * PA9 = USART1_TX
 * CNF=10（复用推挽），MODE=11（50 MHz 输出）→ 半字节 0b1011 = 0xB
 * 位于 CRH[7:4]
 */
#define GPIOA_CRH_PA9_MASK   (0xFU << 4)
#define GPIOA_CRH_PA9_AF_PP  (0xBU << 4)

/**
 * PA10 = USART1_RX
 * CNF=01（浮空输入），MODE=00（输入模式）→ 半字节 0b0100 = 0x4
 * 位于 CRH[11:8]
 */
#define GPIOA_CRH_PA10_MASK      (0xFU << 8)
#define GPIOA_CRH_PA10_IN_FLOAT  (0x4U << 8)

/* -------------------------------------------------------------------------- */
/* USART SR / CR1 位定义                                                        */
/* -------------------------------------------------------------------------- */

#define USART_SR_TXE (1U << 7)   /**< 发送数据寄存器空：可写入 DR */
#define USART_CR1_UE  (1U << 13) /**< UE（USART Enable，CR1[13]）；改 BRR 前须清 0 */
#define USART_CR1_TE  (1U << 3)  /**< 发送使能 */
#define USART_CR1_RE  (1U << 2)  /**< 接收使能（预留，便于后续扩展串口读） */

/** 写入 BRR（Baud Rate Register，波特率寄存器）：USARTDIV=3 → 1.5 Mbps */
#define USART1_BRR_1500000 0x0030U

/**
 * @brief  初始化 USART1：时钟 → GPIO 复用 → 波特率 → 8N1 发送
 *
 * 初始化顺序说明：
 *   1. 开 GPIOA / USART1 时钟（否则后续寄存器写无效）
 *   2. 配置 PA9 为复用推挽、PA10 为浮空输入（F103 默认映射，无需 AFIO 重映射）
 *   3. 清 UE（USART Enable，CR1[13]），写 BRR（Baud Rate Register，波特率寄存器）、CR2，再置 TE|RE|UE
 *
 * 帧格式：8 数据位、无校验、1 停止位（CR1.M=0，CR2.STOP=00）
 */
void USART1_Init(void)
{
    /* --- 步骤 1：外设时钟 --- */
    RCC_APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN;

    /* --- 步骤 2：PA9 TX / PA10 RX 引脚模式 --- */
    GPIOA_CRH &= ~GPIOA_CRH_PA9_MASK;
    GPIOA_CRH |= GPIOA_CRH_PA9_AF_PP;
    GPIOA_CRH &= ~GPIOA_CRH_PA10_MASK;
    GPIOA_CRH |= GPIOA_CRH_PA10_IN_FLOAT;

    /*
     * --- 步骤 3：USART 帧格式与波特率 ---
     * UE（USART Enable，CR1[13]）：模块总使能；修改 BRR 等参数前须清 0（RM0008）。
     * BRR（Baud Rate Register，波特率寄存器）：BaudRate = PCLK2 / (16 × USARTDIV)；
     *   本工程 PCLK2=72 MHz、1.5 Mbps → USARTDIV=3 → 写 0x0030。
     */
    USART1_CR1 = 0U;                      /* 清 UE（USART Enable，CR1[13]）后再改 BRR */
    USART1_BRR = USART1_BRR_1500000;      /* BRR（Baud Rate Register，波特率寄存器） */
    USART1_CR2 = 0U;                      /* STOP=00 → 1 停止位 */
    USART1_CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE; /* 置 UE（USART Enable，CR1[13]）启动 */
}

/**
 * @brief  阻塞发送 len 字节（轮询 SR.TXE）
 * @param  buf  待发送缓冲区；允许 NULL（直接返回）
 * @param  len  字节数；≤0 时不发送
 *
 * 发送机制（RM0008）：
 *   写 DR 即把 1 字节载入 USART 发送数据寄存器；置 UE+TE 后，硬件自动按波特率
 *   在 PA9 上串行发出（起始位 + 8 数据位 + 停止位），无需软件再操作 GPIO。
 *   字节从 DR 进入发送移位寄存器后，由移位器逐 bit 驱动 TX 引脚，CPU 不参与位时序。
 *
 * 为何每次只写 1 字节：
 *   DR 仅 8 位有效，同一时刻只能接纳一个新字节；若上一字节尚未从 DR 移入移位器
 *   （SR.TXE=0）就再次写入，会覆盖未发完的数据。故循环内：等 TXE=1 → 写 1 字节 → 重复。
 *   printf/_write 虽可能传入多字节缓冲区，本函数仍按字节拆分发送（简单可靠，无 DMA）。
 *
 * 由 syscalls.c 的 _write() 调用；`\r\n` 转换在 _write 层完成。
 */
void USART1_Write(const char *buf, int len)
{
    int i;

    if (buf == 0 || len <= 0) {
        return;
    }

    for (i = 0; i < len; i++) {
        while ((USART1_SR & USART_SR_TXE) == 0U) {
            /* TXE=0：DR 仍被占用，上一字节尚未移入移位器，不可写 */
        }
        /*
         * 写 DR[7:0]：硬件自动在 PA9 发出该字节；无需再操作 GPIO。
         * 强制转为 unsigned char 再写，避免 char 符号扩展污染 DR 高位。
         * 每次循环只写 1 字节，因 DR 单字节宽且须等 TXE 后再写下一字节。
         */
        USART1_DR = (unsigned int)(unsigned char)buf[i];
    }
}
