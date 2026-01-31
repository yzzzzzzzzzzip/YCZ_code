#include "nrf24l01.h"

/************************** 底层SPI操作实现 **************************/
// CSN拉低（使能SPI通信）
void NRF24L01_CSN_Enable(void)
{
    HAL_GPIO_WritePin(NRF24L01_CSN_PORT, NRF24L01_CSN_PIN, GPIO_PIN_RESET);
}

// CSN拉高（失能SPI通信）
void NRF24L01_CSN_Disable(void)
{
    HAL_GPIO_WritePin(NRF24L01_CSN_PORT, NRF24L01_CSN_PIN, GPIO_PIN_SET);
}

// SPI同步收发1字节（核心底层函数，增加调试打印）
uint8_t NRF24L01_SPI_RW(uint8_t data)
{
    uint8_t rx_data = 0;
    HAL_StatusTypeDef spi_status;
    
    // 新增：打印要发送的字节（调试SPI发送）
    debug_printf("SPI发送字节：0x%02X | ", data);
    
    // 执行SPI收发，保存返回状态
    spi_status = HAL_SPI_TransmitReceive(NRF24L01_SPI_HANDLE, &data, &rx_data, 1, 100);
    
    // 新增：打印接收的字节和SPI通信状态
    if(spi_status == HAL_OK)
    {
        debug_printf("SPI接收字节：0x%02X | 状态：HAL_OK\r\n", rx_data);
    }
    else if(spi_status == HAL_TIMEOUT)
    {
        debug_printf("SPI接收字节：0x%02X | 状态：HAL_TIMEOUT（超时）\r\n", rx_data);
        rx_data = 0xFF;
    }
    else if(spi_status == HAL_ERROR)
    {
        debug_printf("SPI接收字节：0x%02X | 状态：HAL_ERROR（错误）\r\n", rx_data);
        rx_data = 0xFF;
    }
    
    return rx_data;
}

/************************** 寄存器操作实现 **************************/
// 读单个寄存器
uint8_t NRF24L01_ReadReg(uint8_t reg)
{
    uint8_t reg_val;
    NRF24L01_CSN_Enable();
    NRF24L01_SPI_RW(NRF_CMD_R_REGISTER | reg); // 发送读寄存器指令
    reg_val = NRF24L01_SPI_RW(NRF_CMD_NOP);    // 读取寄存器值
    NRF24L01_CSN_Disable();
    return reg_val;
}

// 写单个寄存器
void NRF24L01_WriteReg(uint8_t reg, uint8_t val)
{
    NRF24L01_CSN_Enable();
    NRF24L01_SPI_RW(NRF_CMD_W_REGISTER | reg); // 发送写寄存器指令
    NRF24L01_SPI_RW(val);                      // 写入寄存器值
    NRF24L01_CSN_Disable();
}
// 写多字节数据（如地址、发送数据）
void NRF24L01_WriteBuf(uint8_t reg, uint8_t *buf, uint8_t len)
{
    uint8_t i;
    NRF24L01_CSN_Enable();
    NRF24L01_SPI_RW(NRF_CMD_W_REGISTER | reg);
    for(i=0; i<len; i++)
    {
        NRF24L01_SPI_RW(buf[i]);
    }
    NRF24L01_CSN_Disable();
}
// 读多字节数据（如地址、接收数据）
void NRF24L01_ReadBuf(uint8_t reg, uint8_t *buf, uint8_t len)
{
    uint8_t i;
    NRF24L01_CSN_Enable();
    NRF24L01_SPI_RW(NRF_CMD_R_REGISTER | reg);
    for(i=0; i<len; i++)
    {
        buf[i] = NRF24L01_SPI_RW(NRF_CMD_NOP);
    }
    NRF24L01_CSN_Disable();
}
/************************** 核心功能实现 **************************/
// 检测nRF24L01模块是否存在（增加调试打印，输出读写缓冲区）
uint8_t NRF24L01_Check(void)
{
    uint8_t check_buf[5] = {0xA5, 0xA5, 0xA5, 0xA5, 0xA5};
    uint8_t temp_buf[5] = {0};
    uint8_t i;
    
    debug_printf("=== 开始检测nRF24L01模块 ===\r\n");
    
    // 新增：打印要写入的测试地址（check_buf）
    debug_printf("准备写入测试地址（check_buf）：");
    for(i=0; i<5; i++)
    {
        debug_printf("0x%02X ", check_buf[i]);
    }
    debug_printf("\r\n");
    
    // 写入测试地址到TX_ADDR寄存器
    NRF24L01_WriteBuf(NRF_REG_TX_ADDR, check_buf, 5);
    HAL_Delay(10); // 短暂延时，确保写入完成
    
    // 读取TX_ADDR寄存器的值到temp_buf
    NRF24L01_ReadBuf(NRF_REG_TX_ADDR, temp_buf, 5);
    
    // 新增：打印读回的地址（temp_buf）
    debug_printf("读回的测试地址（temp_buf）：");
    for(i=0; i<5; i++)
    {
        debug_printf("0x%02X ", temp_buf[i]);
    }
    debug_printf("\r\n");
    
    // 对比读写数据，判断模块是否存在
    if(memcmp(check_buf, temp_buf, 5) == 0)
    {
        debug_printf("=== nRF24L01模块检测成功！ ===\r\n");
        return 1;
    }
    else
    {
        debug_printf("=== nRF24L01模块检测失败！ ===\r\n");
        return 0;
    }
}
/************************** 核心初始化（重点修复：强制置1PWR_UP，新增显式验证） **************************/
void NRF24L01_Init(void)
{
    uint8_t tx_addr[5] = NRF24L01_TX_ADDR;
    uint8_t rx_addr[5] = NRF24L01_RX_ADDR;
    uint8_t config_val = 0;

    debug_printf("\r\n==================== 开始初始化nRF24L01 ====================\r\n");
    HAL_Delay(100); // 模块上电稳定延时，必须保留
    

    /************************** 1. 核心配置：强制置1PWR_UP位（CONFIG寄存器bit1） **************************/
    // 0x0F = 00001111 → bit1=1（PWR_UP=工作模式）、bit0=1（接收模式）、bit3=1（16位CRC）
    NRF24L01_WriteReg(NRF_REG_CONFIG, 0x0F);
    // 立即读取验证：确认PWR_UP位是否真的置1
    config_val = NRF24L0 1_ReadReg(NRF_REG_CONFIG);
    debug_printf("CONFIG寄存器值：0x%02X | 二进制：%08b\r\n", config_val, config_val);
    debug_printf("PWR_UP位(bit1)：%d | 1=工作模式，0=掉电模式\r\n", (config_val >> 1) & 0x01);
    debug_printf("PRIM_RX位(bit0)：%d | 1=接收模式，0=发送模式\r\n", config_val & 0x01);

    /************************** 2. 基础通信配置（收发双方一致） **************************/
    NRF24L01_WriteReg(NRF_REG_EN_AA, 0x01);    // 使能管道0自动应答
    NRF24L01_WriteReg(NRF_REG_EN_RXADDR, 0x01); // 使能管道0
    NRF24L01_WriteReg(NRF_REG_SETUP_AW, 0x03);  // 地址宽度5字节
    NRF24L01_WriteReg(NRF_REG_SETUP_RETR, 0x1A);// 500us重传间隔，10次重传
    NRF24L01_WriteReg(NRF_REG_RF_CH, NRF24L01_RF_CHANNEL); // 射频频道
    NRF24L01_WriteReg(NRF_REG_RF_SETUP, 0x07);  // 0dBm功率，1Mbps速率
    NRF24L01_WriteReg(NRF_REG_RX_PW_P0, NRF24L01_DATA_WIDTH); // 数据宽度

    /************************** 3. 配置收发地址 **************************/
    NRF24L01_WriteBuf(NRF_REG_TX_ADDR, tx_addr, 5);
    NRF24L01_WriteBuf(NRF_REG_RX_ADDR_P0, rx_addr, 5);

    /************************** 4. 清空FIFO，避免残留数据 **************************/
    NRF24L01_CSN_Enable();
    NRF24L01_SPI_RW(NRF_CMD_FLUSH_TX);
    NRF24L01_CSN_Disable();
    NRF24L01_CSN_Enable();
    NRF24L01_SPI_RW(NRF_CMD_FLUSH_RX);
    NRF24L01_CSN_Disable();

    /************************** 5. 配置完成，拉高CE进入接收模式 **************************/
    HAL_Delay(10); // 配置收尾延时，确保所有寄存器生效
    
    debug_printf("==================== nRF24L01初始化完成！ ====================\r\n");
}
