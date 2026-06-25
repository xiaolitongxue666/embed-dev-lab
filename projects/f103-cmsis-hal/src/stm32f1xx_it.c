/**
 * @file    stm32f1xx_it.c
 * @brief   Cortex-M3 异常/中断处理实现
 *
 * @note    本 demo 未启用外设 NVIC 中断；以下处理函数满足链接与调试需求。
 *          Fault 类处理进入死循环，便于在调试器中查看 CFSR/BFAR 等寄存器。
 *
 * SysTick：HAL_Init 默认配置 SysTick 为 1 ms 节拍，供 HAL_GetTick/HAL_Delay 使用；
 *          本 demo 闪烁用 main.c 中忙等 delay，不依赖 HAL_Delay。
 */

#include "stm32f1xx_it.h"

/** @brief  不可屏蔽中断；本 demo 无 NMI 源配置 */
void NMI_Handler(void)
{
}

/** @brief  硬 fault；常见原因：非法指令、BusFault 升级、除零等 */
void HardFault_Handler(void)
{
    while (1) {
    }
}

/** @brief  存储器管理 fault（MPU 违规等；F103 无 MPU 时较少触发） */
void MemManage_Handler(void)
{
    while (1) {
    }
}

/** @brief  总线 fault；常见原因：访问未映射地址、对齐错误 */
void BusFault_Handler(void)
{
    while (1) {
    }
}

/** @brief  用法 fault；常见原因：未定义指令、非法状态 */
void UsageFault_Handler(void)
{
    while (1) {
    }
}

/** @brief  SVC 指令入口；RTOS 上下文切换时使用，本 demo 空实现 */
void SVC_Handler(void)
{
}

/** @brief  调试监视器；连接调试器时可能触发 */
void DebugMon_Handler(void)
{
}

/** @brief  PendSV；RTOS 低优先级上下文切换，本 demo 空实现 */
void PendSV_Handler(void)
{
}

/**
 * @brief  SysTick 1 ms 中断；维护 HAL 内部 uwTick 计数
 * @note   由 HAL_Init → HAL_InitTick 配置；即使不用 HAL_Delay 也应保留
 */
void SysTick_Handler(void)
{
    HAL_IncTick();
}
