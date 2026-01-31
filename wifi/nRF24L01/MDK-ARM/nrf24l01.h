#ifndef __NRF24L01_H
#define __NRF24L01_H

#include "main.h"
#include "debug_uart.h"     // 引入之前的串口调试头文件（用于打印调试信息）
#include "spi.h"
#include "string.h"
/************************** 核心配置（用户仅需修改此处） **************************/
// 1. SPI句柄（根据实际使用的SPI修改，如hspi2）
#define NRF24L01_SPI_HANDLE   &hspi1
// 2. CE/CSN引脚定义（根据硬件接线修改）
#define NRF24L01_CE_PIN       GPIO_PIN_8
#define NRF24L01_CE_PORT      GPIOA

#define NRF24L01_CSN_PIN      GPIO_PIN_4
#define NRF24L01_CSN_PORT     GPIOA
// 3. 通信配置（收发双方必须一致）
#define NRF24L01_DATA_WIDTH   8       // 单次收发数据长度（1-32字节）
#define NRF24L01_RF_CHANNEL   0x02    // 射频频道（0-127，2.4G频段）
#define NRF24L01_TX_ADDR      {0x01,0x02,0x03,0x04,0x05}  // 发送地址（5字节）
#define NRF24L01_RX_ADDR      {0x01,0x02,0x03,0x04,0x05}  // 接收地址（5字节）

/************************** nRF24L01寄存器地址 **************************/
#define NRF_REG_CONFIG     0x00  // 配置寄存器
#define NRF_REG_EN_AA      0x01  // 自动应答使能
#define NRF_REG_EN_RXADDR  0x02  // 接收地址使能
#define NRF_REG_SETUP_AW   0x03  // 地址宽度设置
#define NRF_REG_SETUP_RETR 0x04  // 自动重传设置
#define NRF_REG_RF_CH      0x05  // 射频频道
#define NRF_REG_RF_SETUP   0x06  // 射频配置
#define NRF_REG_STATUS     0x07  // 状态寄存器
#define NRF_REG_OBSERVE_TX 0x08  // 发送观察
#define NRF_REG_RX_ADDR_P0 0x0A  // 接收管道0地址
#define NRF_REG_RX_ADDR_P1 0x0B  // 接收管道1地址
#define NRF_REG_TX_ADDR    0x10  // 发送地址
#define NRF_REG_RX_PW_P0   0x11  // 接收管道0数据宽度
#define NRF_REG_FIFO_STATUS 0x17 // FIFO状态寄存器

/************************** nRF24L01指令定义 **************************/
#define NRF_CMD_R_REGISTER 0x00  // 读寄存器（OR 寄存器地址）
#define NRF_CMD_W_REGISTER 0x20  // 写寄存器（OR 寄存器地址）
#define NRF_CMD_TX_PAYLOAD 0xA0  // 写发送数据
#define NRF_CMD_RX_PAYLOAD 0x61  // 读接收数据
#define NRF_CMD_FLUSH_TX   0xE1  // 清空发送FIFO
#define NRF_CMD_FLUSH_RX   0xE2  // 清空接收FIFO
#define NRF_CMD_REUSE_TX_PL 0xE3 // 重用上一个发送包
#define NRF_CMD_NOP        0xFF  // 空指令（读状态寄存器）

/************************** 函数声明 **************************/
// 底层SPI操作
void NRF24L01_CSN_Enable(void);    // CSN拉低（使能SPI）
void NRF24L01_CSN_Disable(void);   // CSN拉高（失能SPI）
void NRF24L01_CE_High(void);       // CE拉高
void NRF24L01_CE_Low(void);        // CE拉低
uint8_t NRF24L01_SPI_RW(uint8_t data); // SPI同步收发1字节

// 寄存器操作
uint8_t NRF24L01_ReadReg(uint8_t reg);  // 读单个寄存器
void NRF24L01_WriteReg(uint8_t reg, uint8_t val); // 写单个寄存器
void NRF24L01_ReadBuf(uint8_t reg, uint8_t *buf, uint8_t len); // 读多字节
void NRF24L01_WriteBuf(uint8_t reg, uint8_t *buf, uint8_t len); // 写多字节

// 核心功能
uint8_t NRF24L01_Check(void);     // 检测模块是否存在（返回1=存在，0=不存在）
void NRF24L01_Init(void);         // 模块初始化（进入接收模式）
uint8_t NRF24L01_SendData(uint8_t *tx_buf); // 发送数据（返回1=成功，0=失败）
uint8_t NRF24L01_ReceiveData(uint8_t *rx_buf); // 接收数据（返回1=有数据，0=无数据）

// 测试示例
void NRF24L01_Send_Test(void);    // 发送测试（主循环调用）
void NRF24L01_Receive_Test(void); // 接收测试（主循环调用）

#endif
