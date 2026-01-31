#include "dht11.h"
#include "delay.h"  // 微秒延时依赖，下方会提供HAL库版delay

/************************* 私有宏定义（HAL库版引脚电平控制） *************************/
#define DHT11_PIN_HIGH  HAL_GPIO_WritePin(DHT11_GPIO_PORT, DHT11_GPIO_PIN, GPIO_PIN_SET)
#define DHT11_PIN_LOW   HAL_GPIO_WritePin(DHT11_GPIO_PORT, DHT11_GPIO_PIN, GPIO_PIN_RESET)

/************************* 引脚初始化：HAL库版（推挽输出，空闲置高） *************************/
// 初始化为输出模式（发送起始信号），通信时动态切换为输入
void DHT11_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0}; // HAL库推荐初始化结构体为0

    // 1. 使能GPIO时钟（HAL库宏，自动适配APB1/APB2，无需区分）
    __HAL_RCC_GPIOA_CLK_ENABLE(); // 若修改为GPIOB，对应__HAL_RCC_GPIOB_CLK_ENABLE()

    // 2. 配置为推挽输出模式（50MHz，与硬件匹配）
    GPIO_InitStruct.Pin = DHT11_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;  // 推挽输出
    GPIO_InitStruct.Pull = GPIO_NOPULL;          // 无上下拉（输出模式无需）
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;// 高速50MHz
    HAL_GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStruct);

    DHT11_PIN_HIGH;  // DHT11空闲状态，总线置高
}

/************************* 私有函数：切换GPIO为上拉输入（HAL库版，接收数据用） *************************/
static void DHT11_GPIO_To_Input(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = DHT11_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;     // 输入模式
    GPIO_InitStruct.Pull = GPIO_PULLUP;         // 上拉输入（防止总线浮空，必加）
    HAL_GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStruct);
}

/************************* 私有函数：切换GPIO为推挽输出（HAL库版，发送信号用） *************************/
static void DHT11_GPIO_To_Output(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = DHT11_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStruct);
}

/************************* 检测DHT11响应信号（逻辑不变，HAL库版引脚读取） *************************/
// 返回值：0=检测到响应，1=未检测到/超时
uint8_t DHT11_Check_Response(void)
{
    uint32_t timeout = 0; // 用uint32_t避免溢出，确保超时有效
    DHT11_GPIO_To_Input();

    // 等待DHT11拉低总线（最大等待200us）
    while (HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_GPIO_PIN) == GPIO_PIN_RESET)
    {
        timeout++;
        delay_us(1);
        if (timeout > 200)
        {
            printf("【调试】等待DHT11拉低总线超时！\r\n");
            return 1;
        }
    }

    timeout = 0;
    // 等待DHT11拉高总线（最大等待200us）
    while (HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_GPIO_PIN) == GPIO_PIN_SET)
    {
        timeout++;
        delay_us(1);
        if (timeout > 300)
        {
            printf("【调试】等待DHT11拉高总线超时！\r\n");
            return 1;
        }
    }
    return 0;
}

/************************* 发送DHT11起始信号（时序不变，HAL库版引脚控制） *************************/
static void DHT11_Send_Start_Signal(void)
{
    DHT11_GPIO_To_Output();
    DHT11_PIN_LOW;
    delay_ms(20);
    DHT11_PIN_HIGH;
    delay_us(30);
    delay_us(10); // 新增：延迟10us再切换输入，避免毛刺
    DHT11_GPIO_To_Input();
}
/************************* 读取1位数据（HAL库版引脚读取，时序不变） *************************/
static uint8_t DHT11_Read_One_Bit(void)
{
    uint8_t timeout = 0;
    uint8_t bit = 0;

    // DHT11发送每位前先拉低50us
    while (HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_GPIO_PIN) == GPIO_PIN_RESET)
    {
        timeout++;
        delay_us(1);
        if (timeout > 60) break;
    }
    delay_us(35);  // 延时40us，判断后续电平：高=1，低=0
    // 协议：26~28us高=0，70us高=1
    if (HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_GPIO_PIN) == GPIO_PIN_SET)
    {
        bit = 1;
    }
    // 等待该位传输完成（总线拉低）
    while (HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_GPIO_PIN) == GPIO_PIN_SET)
    {
        timeout++;
        delay_us(1);
        if (timeout > 80) break;
    }
    return bit;
}

/************************* 读取1字节数据（8位，先高后低，逻辑不变） *************************/
static uint8_t DHT11_Read_One_Byte(void)
{
    uint8_t i, byte = 0;
    for (i = 0; i < 8; i++)
    {
        byte <<= 1;          // 左移接收高位
        byte |= DHT11_Read_One_Bit();  // 接收当前位
    }
    return byte;
}

/************************* 读取温湿度主函数（调用方式完全不变） *************************/
// 入参：结构体指针，存储温湿度；返回值：0成功（校验通过），1失败（无响应/校验错）
uint8_t DHT11_Read_Data(DHT11_DataDef *DHT11_Data)
{
    uint8_t buf[5];
    uint8_t i;

    DHT11_Send_Start_Signal();
	
    if (DHT11_Check_Response() != 0)
    {
        printf("【调试】响应检测失败！\r\n"); // 定位是否是响应问题
        return 1;
    }
    printf("【调试】响应检测成功！\r\n");

    for (i = 0; i < 5; i++)
    {
        buf[i] = DHT11_Read_One_Byte();
        printf("【调试】读取第%d字节：0x%02X\r\n", i+1, buf[i]);
    }

    if ((buf[0] + buf[1] + buf[2] + buf[3]) == buf[4])
    {
        // 赋值逻辑
        return 0;
    }
    else
    {
        printf("【调试】校验和错误！前4字节和：0x%02X，校验和：0x%02X\r\n",
               buf[0]+buf[1]+buf[2]+buf[3], buf[4]);
        return 1;
    }
}

