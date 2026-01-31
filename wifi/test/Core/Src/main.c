/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body (适配ARM Compiler 6 | USART2重定向+DHT11采集)
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"   // 标准输入输出，printf依赖
#include "dht11.h"   // DHT11驱动头文件（HAL库版）
#include "delay.h"   // 微秒延时头文件（适配HAL库，DHT11时序核心）
#include "ds18b20.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// ARM Compiler 6 关闭半主机模式（替代AC5的#pragma import，核心修复）
#define __NO_SYSTEM_INIT 1
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
DHT11_DataDef dht11_data;  // DHT11温湿度数据存储结构体
uint8_t dht11_read_sta;    // DHT11读取状态：0=成功，1=失败（无响应/校验错）
uint32_t print_cnt = 0;    // 打印计数变量，辅助测试
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// ARM Compiler 6 适配：声明标准输出/输入（系统已定义__FILE，无需重写）
extern FILE __stdout;
extern FILE __stdin;

// 半主机模式必要空实现（避免链接错误，适配AC6）
void _ttywrch(int ch) { (void)ch; }
void _sys_exit(int x) { (void)x; while(1); }
int _sys_open(const char *name, int mode) { (void)name; (void)mode; return -1; }
int _sys_close(int fd) { (void)fd; return 0; }
int _sys_read(int fd, char *ptr, int len) { (void)fd; (void)ptr; (void)len; return 0; }
int _sys_write(int fd, char *ptr, int len) { (void)fd; (void)ptr; (void)len; return 0; }

// 重写fputc：printf底层实现，USART2串口输出（逻辑不变，适配AC6）
int fputc(int ch, FILE *f)
{
    HAL_UART_Transmit(&huart2, (uint8_t*)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

// 重写fgetc：scanf底层实现，USART2串口输入（可选保留，适配AC6）
int fgetc(FILE *f)
{
    uint8_t ch = 0;
    HAL_UART_Receive(&huart2, &ch, 1, HAL_MAX_DELAY);
    // 取消注释开启回显：串口助手显示输入的字符
    // HAL_UART_Transmit(&huart2, &ch, 1, HAL_MAX_DELAY);
    return (int)ch;
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_TIM4_Init();
  /* USER CODE BEGIN 2 */
  // 1. 初始化DHT11（HAL库版，已适配当前工程）
	printf("初始化完成，即将进入主循环...\r\n");
  DHT11_GPIO_Init();
  // 2. DHT11上电稳定延时（协议要求至少1秒，不可省略）
	printf("主循环执行中...\r\n");
  HAL_Delay(1000);
		// 初始化DS18B20
	if (DS18B20_Init() == 0)
	{
			printf("DS18B20初始化成功 ✅\r\n");
	}
	else
	{
			printf("DS18B20初始化失败 ❌\r\n");
	}
  // 3. 串口打印初始化成功提示（串口助手直接可见）
  printf("========================= 系统初始化完成 =========================\r\n");
  printf("STM32 USART2 串口重定向成功 | 波特率：%d 8N1\r\n", huart2.Init.BaudRate);
  printf("系统时钟：72MHz (HSE+PLL9倍频)\r\n");
  printf("编译器：ARM Compiler 6 (ARMCLANG) | DHT11采集开始...\r\n");
  printf("===============================================================\r\n\r\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

				/* USER CODE BEGIN 3 */
		float temp = DS18B20_Read_Temperature();
		if (temp != -999.0f)
		{
				printf("当前温度：%.2f ℃\r\n", temp);
		}
		else
		{
				printf("温度读取失败 ❌\r\n");
		}
		
    // 关键：DHT11最小采样周期为2秒，不可频繁读取（必须≥2000ms）
    HAL_Delay(2000);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
    // 错误时可添加指示灯闪烁（如LED每500ms闪一次），方便硬件排查
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  // 断言失败时，串口打印错误文件和行号，方便调试
  printf("【程序断言失败】File: %s, Line: %d\r\n", file, line);
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
