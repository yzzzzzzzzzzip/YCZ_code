#include "ds18b20.h"

/************************* 私有宏定义（引脚电平控制） *************************/
#define DS18B20_PIN_HIGH  HAL_GPIO_WritePin(DS18B20_GPIO_PORT, DS18B20_GPIO_PIN, GPIO_PIN_SET)
#define DS18B20_PIN_LOW   HAL_GPIO_WritePin(DS18B20_GPIO_PORT, DS18B20_GPIO_PIN, GPIO_PIN_RESET)
#define DS18B20_PIN_READ  HAL_GPIO_ReadPin(DS18B20_GPIO_PORT, DS18B20_GPIO_PIN)

/************************* 切换GPIO为推挽输出（发送信号用） *************************/
static void DS18B20_GPIO_To_Output(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DS18B20_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DS18B20_GPIO_PORT, &GPIO_InitStruct);
}

/************************* 切换GPIO为上拉输入（接收数据用） *************************/
static void DS18B20_GPIO_To_Input(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DS18B20_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP; // 上拉输入，防止总线浮空
    HAL_GPIO_Init(DS18B20_GPIO_PORT, &GPIO_InitStruct);
}

/************************* 复位DS18B20并检测存在脉冲（带详细调试） *************************/
uint8_t DS18B20_Reset(void)
{
    uint8_t retry = 0;
    printf("【DS18B20调试】开始设备复位流程\r\n");
    
    // 步骤1：切换为输出模式，拉低总线480us（协议要求）
    DS18B20_GPIO_To_Output();
    DS18B20_PIN_LOW;
    printf("【DS18B20调试】拉低总线480us...\r\n"); 
    delay_us(480); 

    // 步骤2：释放总线（拉高），延时60us等待DS18B20响应
    DS18B20_PIN_HIGH;
    delay_us(60);  
    printf("【DS18B20调试】释放总线60us，切换输入模式检测存在脉冲\r\n");
    DS18B20_GPIO_To_Input();

    // 步骤3：检测DS18B20的存在脉冲（正常会拉低总线60~240us）
    printf("【DS18B20调试】检测存在脉冲（等待总线拉低）...\r\n");
    while (DS18B20_PIN_READ && retry < 220)  // DS18B20_PIN_READ=1表示总线未拉低
    {
        retry++;
        delay_us(1);
    }
    if (retry >= 200) 
    {
        printf("【DS18B20调试】错误：未检测到存在脉冲（总线始终为高），复位失败！retry=%d\r\n", retry);
        return 1; // 无响应，初始化失败
    }
    else
    {
        printf("【DS18B20调试】检测到总线拉低，存在脉冲有效！retry=%d\r\n", retry);
        retry = 0;
    }

    // 步骤4：等待存在脉冲结束（DS18B20释放总线，拉高）
    printf("【DS18B20调试】等待存在脉冲结束（等待总线拉高）...\r\n");
    while (!DS18B20_PIN_READ && retry < 240) // !DS18B20_PIN_READ=1表示总线仍拉低
    {
        retry++;
        delay_us(1);
    }
    if (retry >= 240)
    {
        printf("【DS18B20调试】错误：存在脉冲超时（总线始终为低），复位失败！retry=%d\r\n", retry);
        return 1; // 响应超时
    }
    else
    {
        printf("【DS18B20调试】存在脉冲结束，总线拉高！retry=%d\r\n", retry);
    }
    
    // 复位全流程成功
    printf("【DS18B20调试】复位流程完成，设备检测成功！\r\n");
    return 0; 
}

/************************* 初始化DS18B20（对外接口） *************************/
uint8_t DS18B20_Init(void)
{
    // 初始化GPIO时钟
    __HAL_RCC_GPIOB_CLK_ENABLE(); // 根据引脚端口修改，如GPIOB则启用GPIOB时钟
    // 复位并检测设备
    return DS18B20_Reset();
}

/************************* 向DS18B20写入1位数据 *************************/
void DS18B20_Write_Bit(uint8_t bit)
{
    DS18B20_GPIO_To_Output();
    DS18B20_PIN_LOW;
    delay_us(1); // 拉低总线1us

    // 写1：拉低后15us内释放总线；写0：保持拉低60us
    if (bit) DS18B20_PIN_HIGH;
    delay_us(60); // 保持电平60us
    DS18B20_PIN_HIGH;
    delay_us(1); // 恢复总线
}

/************************* 向DS18B20写入1字节数据 *************************/
void DS18B20_Write_Byte(uint8_t byte)
{
    uint8_t i;
    for (i = 0; i < 8; i++)
    {
        DS18B20_Write_Bit(byte & 0x01); // 从低位到高位依次写入
        byte >>= 1;
    }
}

/************************* 从DS18B20读取1位数据 *************************/
uint8_t DS18B20_Read_Bit(void)
{
    uint8_t bit = 0;
    DS18B20_GPIO_To_Output();
    DS18B20_PIN_LOW;
    delay_us(1); // 拉低总线1us
    DS18B20_PIN_HIGH;
    delay_us(1); // 释放总线1us
    DS18B20_GPIO_To_Input();

    // 读取总线电平（15us内采样）
    if (DS18B20_PIN_READ) bit = 1;
    delay_us(60); // 等待该位传输完成
    return bit;
}

/************************* 从DS18B20读取1字节数据 *************************/
uint8_t DS18B20_Read_Byte(void)
{
    uint8_t i, byte = 0;
    for (i = 0; i < 8; i++)
    {
        byte >>= 1;
        if (DS18B20_Read_Bit()) byte |= 0x80; // 从低位到高位依次读取
    }
    return byte;
}

/************************* 读取温度值（对外接口） *************************/
float DS18B20_Read_Temperature(void)
{
    uint8_t temp_low, temp_high;
    int16_t temp_raw;
    float temp;

    // 1. 复位DS18B20
    if (DS18B20_Reset() != 0) return -999.0f; // 复位失败，返回无效值

    // 2. 发送跳过ROM指令（单传感器时使用）
    DS18B20_Write_Byte(DS18B20_CMD_SKIP_ROM);
    // 3. 发送温度转换指令
    DS18B20_Write_Byte(DS18B20_CMD_CONVERT_TEMP);

    // 4. 等待转换完成（DS18B20转换完成后会拉低总线）
    while (!DS18B20_Read_Bit());

    // 5. 再次复位，准备读取数据
    if (DS18B20_Reset() != 0) return -999.0f;

    // 6. 发送跳过ROM+读取暂存器指令
    DS18B20_Write_Byte(DS18B20_CMD_SKIP_ROM);
    DS18B20_Write_Byte(DS18B20_CMD_READ_SCRATCHPAD);

    // 7. 读取温度寄存器（低字节+高字节）
    temp_low = DS18B20_Read_Byte();
    temp_high = DS18B20_Read_Byte();

    // 8. 解析温度值（12位分辨率，处理正负温）
    temp_raw = (temp_high << 8) | temp_low;
    if (temp_raw & 0xF800) // 负温：补码转换
    {
        temp_raw = ~temp_raw + 1;
        temp = -temp_raw * 0.0625f;
    }
    else // 正温
    {
        temp = temp_raw * 0.0625f;
    }
    return temp;
}
