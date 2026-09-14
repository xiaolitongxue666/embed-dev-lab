/**
 * @file    usart.c
 * @brief   USART1 纯寄存器初始化与中断收发（环形缓冲）
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
 * 中断（RM0008 §27）：
 *   RX：CR1.RXNEIE 常开；ISR 读 DR 入 RX ring（ORE 须先读 SR 再读 DR）
 *   TX：有待发数据时置 CR1.TXEIE；ISR 写 DR；ring 空则清 TXEIE
 *
 * 硬件接线（CH341 USB-TTL）：
 *   模块 RX ← PA9（MCU 发）
 *   模块 TX → PA10（MCU 收）
 *   GND 共地（与蓝板 / 面包板 / ST-Link 同一地）
 *   CH341 为 USB↔TTL 转换；MCU 脚仍是 3.3 V CMOS（习惯称 TTL），不是 RS232
 *
 * 引脚事实（DS5319 Table 5）：
 *   PA9/PA10 为 FT（5 V tolerant 输入）；输出仍为 3.3 V
 *   勿用 PA13/PA14 作串口（SWD 占用）
 *   勿置 USART1_REMAP（否则 TX/RX 改到 PB6/PB7，与计划 I2C1 冲突）
 *
 * printf 路径：syscalls.c 中 _write() → USART1_Write()；`\n` 在 _write 内补 `\r`
 *
 * @see     doc/hardware/stm32f103c8t6-pinout.md
 * @see     doc/reference/stm32f103/md/topics/lqfp48-pinout.md
 * @see     doc/learn/stm32f103-mmio-basics.md
 * @see     doc/learn/gpio-eight-modes.md — PA9 复用推挽、PA10 浮空输入（高阻）
 * @see     doc/learn/uart-ttl-rs232-rs485.md — UART 外设 vs TTL vs RS232/485
 * @see     doc/learn/interrupt-vector-table-and-nvic.md
 */

#include "nvic.h"
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
 * CNF=01（浮空输入=高阻），MODE=00（输入模式）→ 半字节 0b0100 = 0x4
 * 位于 CRH[11:8]
 */
#define GPIOA_CRH_PA10_MASK      (0xFU << 8)
#define GPIOA_CRH_PA10_IN_FLOAT  (0x4U << 8)

/* -------------------------------------------------------------------------- */
/* USART SR / CR1 位定义                                                        */
/* -------------------------------------------------------------------------- */

#define USART_SR_ORE  (1U << 3)  /**< 过载：须读 SR 再读 DR 才能清除 */
#define USART_SR_RXNE (1U << 5)  /**< 接收数据寄存器非空：可读 DR */
#define USART_SR_TXE  (1U << 7)  /**< 发送数据寄存器空：可写入 DR */
#define USART_CR1_UE  (1U << 13) /**< UE（USART Enable，CR1[13]）；改 BRR 前须清 0 */
#define USART_CR1_RXNEIE (1U << 5) /**< 接收非空中断使能 */
#define USART_CR1_TXEIE  (1U << 7) /**< 发送寄存器空中断使能 */
#define USART_CR1_TE  (1U << 3)  /**< 发送使能 */
#define USART_CR1_RE  (1U << 2)  /**< 接收使能 */

/** 写入 BRR（Baud Rate Register，波特率寄存器）：USARTDIV=3 → 1.5 Mbps */
#define USART1_BRR_1500000 0x0030U

/** USART1 NVIC 抢占优先级（0–15，越小越高） */
#define USART1_NVIC_PRIO 5U

#define USART1_TX_BUF_SIZE 256U
#define USART1_RX_BUF_SIZE 128U

unsigned char g_usart1_tx_buf[USART1_TX_BUF_SIZE];
unsigned char g_usart1_rx_buf[USART1_RX_BUF_SIZE];
volatile unsigned int g_usart1_tx_head;
volatile unsigned int g_usart1_tx_tail;
volatile unsigned int g_usart1_rx_head;
volatile unsigned int g_usart1_rx_tail;

static USART1_RxHandle_t g_usart1_rx_handle;

static unsigned int usart1_ring_next(unsigned int index, unsigned int size)
{
    return (index + 1U) % size;
}

/**
 * @brief  初始化 USART1：时钟 → GPIO 复用 → 波特率 → 8N1 → RX 中断 + NVIC
 *
 * 初始化顺序说明：
 *   1. 开 GPIOA / USART1 时钟（否则后续寄存器写无效）
 *   2. 配置 PA9 为复用推挽、PA10 为浮空输入（F103 默认映射，无需 AFIO 重映射）
 *   3. 清 UE（USART Enable，CR1[13]），写 BRR（Baud Rate Register，波特率寄存器）、CR2，再置 TE|RE|RXNEIE|UE
 *   4. NVIC 优先级并开启 USART1 IRQ（IRQn=37）
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
    USART1_CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE | USART_CR1_UE;

    NVIC_IRQ_SetPriority(USART1_IRQn, USART1_NVIC_PRIO);
    NVIC_IRQ_Enable(USART1_IRQn);
}

/**
 * @brief  将 len 字节写入 TX 环形缓冲并开启 TXE 中断
 * @param  buf  待发送缓冲区；允许 NULL（直接返回）
 * @param  len  字节数；≤0 时不发送
 *
 * 缓冲满时自旋等待（保持全局中断开，ISR 才能抽空 TX ring）。
 * 由 syscalls.c 的 _write() 调用；`\r\n` 转换在 _write 层完成。
 */
void USART1_Write(const char *buf, int len)
{
    int i;
    unsigned int head;
    unsigned int next;

    if (buf == 0 || len <= 0) {
        return;
    }

    for (i = 0; i < len; i++) {
        head = g_usart1_tx_head;
        next = usart1_ring_next(head, USART1_TX_BUF_SIZE);
        while (next == g_usart1_tx_tail) {
            /* TX ring 满：等 ISR 写出一字节腾出空位 */
        }
        g_usart1_tx_buf[head] = (unsigned char)buf[i];
        g_usart1_tx_head = next;
        USART1_CR1 |= USART_CR1_TXEIE;
    }
}

void USART1_SetRxHandle(USART1_RxHandle_t handle)
{
    g_usart1_rx_handle = handle;
}

/**
 * @brief  主循环消费 RX ring：弹出字节后调用已注册 handle（本地副本，NULL 不调）
 */
void USART1_ProcessRx(void)
{
    USART1_RxHandle_t handle;
    unsigned int tail;
    unsigned char byte;

    handle = g_usart1_rx_handle;

    for (;;) {
        tail = g_usart1_rx_tail;
        if (tail == g_usart1_rx_head) {
            break;
        }
        byte = g_usart1_rx_buf[tail];
        g_usart1_rx_tail = usart1_ring_next(tail, USART1_RX_BUF_SIZE);
        if (handle != 0) {
            handle(byte);
        }
    }
}

/**
 * @brief  USART1 中断：只搬 ring / 清 ORE，不调用用户 handle
 */
void USART1_IRQHandler(void)
{
    unsigned int sr;
    unsigned int head;
    unsigned int next;
    unsigned int tail;
    unsigned char byte;

    sr = USART1_SR;

    if ((sr & USART_SR_RXNE) != 0U) {
        byte = (unsigned char)USART1_DR;
        head = g_usart1_rx_head;
        next = usart1_ring_next(head, USART1_RX_BUF_SIZE);
        if (next != g_usart1_rx_tail) {
            g_usart1_rx_buf[head] = byte;
            g_usart1_rx_head = next;
        }
    } else if ((sr & USART_SR_ORE) != 0U) {
        (void)USART1_DR;
    }

    if (((USART1_CR1 & USART_CR1_TXEIE) != 0U) && ((sr & USART_SR_TXE) != 0U)) {
        tail = g_usart1_tx_tail;
        if (tail != g_usart1_tx_head) {
            USART1_DR = (unsigned int)g_usart1_tx_buf[tail];
            g_usart1_tx_tail = usart1_ring_next(tail, USART1_TX_BUF_SIZE);
        }
        if (g_usart1_tx_tail == g_usart1_tx_head) {
            USART1_CR1 &= ~USART_CR1_TXEIE;
        }
    }
}
