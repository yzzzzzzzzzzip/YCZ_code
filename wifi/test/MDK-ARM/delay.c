#include "delay.h"
#include "tim.h" // 包含TIM4的头文件
// 基于SysTick的微秒延时，适配STM32F103 72MHz系统时钟（与你工程一致）
// 不修改HAL库SysTick配置，与HAL_Delay(ms)完全兼容
// 微秒延时（基于TIM6，精度1us，不受中断影响）
// 基于TIM4的微秒延时，适配STM32F103 72MHz系统时钟
void delay_us(uint32_t nus)
{
    // 关键：判断TIM4是否未启动，未启动则开启（仅执行一次）
    if ((htim4.Instance->CR1 & TIM_CR1_CEN) == 0)
    {
        HAL_TIM_Base_Start(&htim4); // 开启TIM4定时器计数
    }
    __HAL_TIM_SET_COUNTER(&htim4, 0); // 清空计数器
    while (__HAL_TIM_GET_COUNTER(&htim4) < nus); // 等待计数达到目标值
}

// 毫秒延时：基于微秒延时实现，兼容原有调用
void delay_ms(uint32_t nms)
{
    while (nms--) delay_us(1000);
}