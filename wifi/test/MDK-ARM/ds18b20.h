#ifndef __DS18B20_H
#define __DS18B20_H

#include "main.h"
#include "gpio.h"
#include "stdio.h"   // 标准输入输出，printf依赖
#include "delay.h"

/************************* 引脚宏定义（根据实际硬件修改） *************************/
// 示例：DS18B20数据引脚接PB5
#define DS18B20_GPIO_PORT    GPIOB
#define DS18B20_GPIO_PIN     GPIO_PIN_5

/************************* DS18B20 指令定义（协议固定） *************************/
#define DS18B20_CMD_SKIP_ROM        0xCC    // 跳过ROM指令（单传感器时使用）
#define DS18B20_CMD_CONVERT_TEMP    0x44    // 温度转换指令
#define DS18B20_CMD_READ_SCRATCHPAD 0xBE    // 读取暂存器指令

/************************* 函数声明 *************************/
uint8_t DS18B20_Init(void);                  // 初始化DS18B20（复位+检测存在脉冲）
float DS18B20_Read_Temperature(void);        // 读取温度值（返回浮点型，保留两位小数）
uint8_t DS18B20_Reset(void);                 // 复位DS18B20，返回0表示检测到设备
void DS18B20_Write_Byte(uint8_t byte);       // 向DS18B20写入1字节
uint8_t DS18B20_Read_Byte(void);             // 从DS18B20读取1字节
void DS18B20_Write_Bit(uint8_t bit);         // 向DS18B20写入1位
uint8_t DS18B20_Read_Bit(void);              // 从DS18B20读取1位

#endif /* __DS18B20_H */
