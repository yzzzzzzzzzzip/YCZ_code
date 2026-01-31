/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "adc.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
#include "esp8266.h"
#include "DS18B20.h"
#include "dht11.h"
#include "buzzer.h"
#include "mq_gas_sensor.h"
#include "thermistor.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
		uint16_t rising_cnt ;  // 上升沿、下降沿数值
		uint16_t falling_cnt ;
		float distance;
		// 定义定时周期（示例：1000ms = 1秒发送一次，可根据需求修改）
	#define MQTT_SEND_PERIOD_MS  5000
	// 记录上一次发送MQTT数据的时间戳（静态变量，仅在当前作用域有效，值会持续保存）
	static uint32_t g_last_mqtt_send_tick = 0;
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
	
		if (htim == &htim4)
   				ESP8266_AT_MQTT_Publish_Raw
				(g_OneNET_Property_Data.Alarm,g_OneNET_Property_Data.led,(uint8_t)&distance);


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
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_TIM3_Init();
  MX_ADC1_Init();
  MX_TIM2_Init();
  MX_TIM4_Init();
  /* USER CODE BEGIN 2 */
		rising_cnt = 0;
		falling_cnt = 0;
		Buzzer_TIM3_Init();
		
		
		HAL_ADCEx_Calibration_Start(&hadc1);
		uint16_t Joystick_ADC[2];
		HAL_ADC_Start_DMA(&hadc1,(uint32_t*)&Joystick_ADC,2);
		
		GasConcentration_t gas_result;
		
		//ESP8266_Get_LinkStatus();
		//ESP8266_ExitUnvarnishSend();
		
		ESP8266_Init();
		ESP8266_Aithinker_MQTT_Example();
		ESP8266_MQTT_Subscribe_OneNET();
		ESP8266_MQTT_Subscribe_OneNET_Repaly();
		ESP8266_Bind_OneNET_Report_Topic_With_Cmd();

		//ESP8266_Enter_Transparent_Mode();
		
		
		//SR04_trig();
		 //SR04_Init();
			 g_last_mqtt_send_tick = HAL_GetTick();
		
					
		//ESP8266_Link_Server(NET_PROTOCOL_MQTT,"mqtts.heclouds.com","1883",ID_NO_SINGLE);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

					//SR04_trig();
					//UART2_Debug_Print("%.2f",distance);
					
		if(g_OneNET_Property_Data.led==1)
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
		if(g_OneNET_Property_Data.led==0)
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
		
		if(g_OneNET_Property_Data.Alarm==1)
		Buzzer_On();
		if(g_OneNET_Property_Data.Alarm==0)
		Buzzer_Off();
		
		
				gas_result = Calculate_Gas_Concentration(Joystick_ADC);
				main_loop_task();
				//UART2_Debug_Print("CO: %.4f%%/ H2:%.4f%%", 
        //gas_result.co_concentration / 10000.0f, 
         //gas_result.h2_concentration / 10000.0f);

				uint32_t current_tick = HAL_GetTick(); // 获取当前系统节拍时间戳
		        // 判断：当前时间 - 上一次发送时间 >= 设定定时周期（非阻塞，无延时）
        if ((current_tick - g_last_mqtt_send_tick) >= MQTT_SEND_PERIOD_MS)
        {
            // 执行MQTT数据发送
            ESP8266_AT_MQTT_Publish_Raw(
                g_OneNET_Property_Data.Alarm,
                g_OneNET_Property_Data.led,
                (uint8_t)&distance
            );

            // 更新上一次发送时间戳（关键：确保下一次定时从当前时间开始计算）
            g_last_mqtt_send_tick = current_tick;
        }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
  if (htim == &htim2 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_4)
  {
   
    
      rising_cnt = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_3);
      falling_cnt = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_4);
			distance = (falling_cnt - rising_cnt) * 0.017;  
	}

}


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
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
