#ifndef __DEBUG_UART_H
#define __DEBUG_UART_H

#include "stm32f1xx_hal.h"  // 替换为你的芯片型号头文件（如f4xx/f7xx）
#include "usart.h"          // CubeMX生成的串口头文件

/************************** 核心配置 **************************/
// 选择要重定向的串口句柄（默认串口2，可修改为huart1/huart3）
#define DEBUG_UART_HANDLE    &huart2
// 串口发送超时时间（ms），避免卡死
#define DEBUG_UART_TIMEOUT   100

/************************** 函数声明 **************************/
// 基础调试打印（等价于printf，支持格式化）
void debug_printf(const char *fmt, ...);
// 中断安全的调试打印（在中断服务函数中使用）
void debug_printf_isr(const char *fmt, ...);
// 串口接收单个字符（可选，用于调试输入）
uint8_t debug_getchar(void);

#endif