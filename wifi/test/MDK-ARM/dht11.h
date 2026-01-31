#ifndef __DHT11_H
#define __DHT11_H

#include "main.h"
#include "gpio.h"
#include "stdio.h"   // 标准输入输出，printf依赖
#include "delay.h"
/************************* 引脚宏定义（仅需修改这里，与实际硬件一致） *************************/
// 示例：DHT11接PA0，根据你的硬件接线修改（如GPIOB, GPIO_PIN_5）
#define DHT11_GPIO_PORT    GPIOA
#define DHT11_GPIO_PIN     GPIO_PIN_0

/************************* DHT11 数据存储结构体（不变，兼容原有调用） *************************/
// 湿度整数/小数（固定0）、温度整数/小数（固定0）、校验和
typedef struct
{
    uint8_t Humidity_Int;   // 湿度整数部分 (0-99%)
    uint8_t Humidity_Dec;   // 湿度小数部分（DHT11无小数，固定0）
    uint8_t Temperature_Int;// 温度整数部分 (0-50℃)
    uint8_t Temperature_Dec;// 温度小数部分（DHT11无小数，固定0）
    uint8_t Check_Sum;      // 校验和 = 前4字节之和
} DHT11_DataDef;

/************************* 函数声明（调用方式不变） *************************/
void DHT11_GPIO_Init(void);        // DHT11引脚初始化（HAL库版）
uint8_t DHT11_Read_Data(DHT11_DataDef *DHT11_Data); // 读取温湿度，0成功/1失败
uint8_t DHT11_Check_Response(void); // 检测DHT11响应信号（内部/外部调用均可）

#endif /* __DHT11_H */
