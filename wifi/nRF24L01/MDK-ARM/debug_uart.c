#include "debug_uart.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

/************************** 全局缓冲区 **************************/
// 打印缓冲区（支持最长256字节的调试信息）
static uint8_t debug_buf[256];

/************************** 底层字符发送 **************************/
// 重写标准库fputc（基础printf重定向核心）
#if defined(__CC_ARM) || defined(__ARMCC_VERSION)  // Keil MDK编译器
int fputc(int ch, FILE *f)
{
    // 发送单个字符到调试串口
    HAL_UART_Transmit(DEBUG_UART_HANDLE, (uint8_t *)&ch, 1, DEBUG_UART_TIMEOUT);
    return ch;
}
#elif defined(__GNUC__)  // STM32CubeIDE（GCC编译器）
int _write(int fd, char *ptr, int len)
{
    HAL_UART_Transmit(DEBUG_UART_HANDLE, (uint8_t *)ptr, len, DEBUG_UART_TIMEOUT);
    return len;
}
#endif

/************************** 基础调试打印 **************************/
// 功能：支持格式化输出（如debug_printf("数值：%d，浮点：%.2f\r\n", 123, 3.14)）
// 用法：和printf完全一致，非中断环境使用
void debug_printf(const char *fmt, ...)
{
    va_list args;
    uint16_t len = 0;
    
    // 1. 格式化字符串到缓冲区
    va_start(args, fmt);
    len = vsnprintf((char *)debug_buf, sizeof(debug_buf), fmt, args);
    va_end(args);
    
    // 2. 串口发送格式化后的字符串
    if(len > 0 && len < sizeof(debug_buf))
    {
        HAL_UART_Transmit(DEBUG_UART_HANDLE, debug_buf, len, DEBUG_UART_TIMEOUT);
    }
}

/************************** 中断安全打印 **************************/
// 功能：在中断服务函数中安全打印（禁用中断避免冲突）
// 用法：仅在TIM/EXTI/UART等中断函数中使用
void debug_printf_isr(const char *fmt, ...)
{
    va_list args;
    uint16_t len = 0;
    uint32_t primask = 0;
    
    // 1. 关闭全局中断（核心：避免中断嵌套导致打印异常）
    primask = __get_PRIMASK();
    __disable_irq();
    
    // 2. 格式化字符串到缓冲区
    va_start(args, fmt);
    len = vsnprintf((char *)debug_buf, sizeof(debug_buf), fmt, args);
    va_end(args);
    
    // 3. 串口发送（用轮询方式，避免中断）
    if(len > 0 && len < sizeof(debug_buf))
    {
        for(uint16_t i=0; i<len; i++)
        {
            HAL_UART_Transmit(DEBUG_UART_HANDLE, &debug_buf[i], 1, DEBUG_UART_TIMEOUT);
        }
    }
    
    // 4. 恢复全局中断
    if(!primask)
    {
        __enable_irq();
    }
}

/************************** 串口接收字符 **************************/
// 功能：阻塞接收串口单个字符（用于调试输入，如命令行交互）
// 返回值：接收到的字符
uint8_t debug_getchar(void)
{
    uint8_t ch = 0;
    // 阻塞等待接收（超时100ms，无数据返回0）
    HAL_UART_Receive(DEBUG_UART_HANDLE, &ch, 1, 100);
    return ch;
}
