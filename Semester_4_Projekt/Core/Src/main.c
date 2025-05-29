/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "OneWire.h"
#include "DS18B20.h"
#include "i2clcd.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
enum Menu_State{
    MENU_STATE_TEMP = 0, MENU_STATE_ROM,
    MENU_STATE_RES, MENU_STATE_THRESHOLD_T_LOW, MENU_STATE_THRESHOLD_T_HIGH
};

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define ADC_NUM_SAMPLES 100

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */
OneWire_HandleTypedef honew1;
DS18B20_HandleTypedef hds18b20;

int8_t tHigh, tLow;
char strbuf[20];

enum Menu_State curr_menu_state;

uint32_t adc_buffer[ADC_NUM_SAMPLES];
uint32_t adc_median;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_TIM2_Init(void);
static void MX_I2C1_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void print_menu(){
    switch(curr_menu_state){
    case MENU_STATE_TEMP:
        lcd_send_cmd((char)(0x80 | lcd_get_pos(0, 0)));
        lcd_send_string("Current Temperature:");

        lcd_send_cmd((char)(0x80 | lcd_get_pos(1, 7)));
        sprintf(strbuf, "%.3f", DS18B20_getTemp(&hds18b20));
        lcd_send_string(strbuf);

        lcd_send_cmd((char)(0x80 | lcd_get_pos(3, 18)));
        lcd_send_string("->");
        break;
    case MENU_STATE_ROM:
        lcd_send_cmd((char)(0x80 | lcd_get_pos(0, 5)));
        lcd_send_string("Sensor ROM");

        lcd_send_cmd((char)(0x80 | lcd_get_pos(1, 1)));
        sprintf(strbuf, "0x%02X%02X%02X%02X%02X%02X%02X%02X", hds18b20.rom[0], hds18b20.rom[1],
                                                              hds18b20.rom[2], hds18b20.rom[3],
                                                              hds18b20.rom[4], hds18b20.rom[5],
                                                              hds18b20.rom[6], hds18b20.rom[7]);
        lcd_send_string(strbuf);

        lcd_send_cmd((char)(0x80 | lcd_get_pos(3, 0)));
        lcd_send_string("<-");
        lcd_send_cmd((char)(0x80 | lcd_get_pos(3, 18)));
        lcd_send_string("->");
        break;
    case MENU_STATE_RES:
        lcd_send_cmd((char)(0x80 | lcd_get_pos(0, 1)));
        lcd_send_string("Current Resolution");

        lcd_send_cmd((char)(0x80 | lcd_get_pos(1, 7)));
        switch(DS18B20_getResolution(&hds18b20)){
        case DS18B20_RES_9Bit:
            lcd_send_string(" 9 Bit");
            break;
        case DS18B20_RES_10Bit:
            lcd_send_string("10 Bit");
            break;
        case DS18B20_RES_11Bit:
            lcd_send_string("11 Bit");
            break;
        case DS18B20_RES_12Bit:
            lcd_send_string("12 Bit");
            break;
        default:
            lcd_send_cmd((char)(0x80 | lcd_get_pos(1, 5)));
            lcd_send_string("WHAT Y'ALL");
            lcd_send_cmd((char)(0x80 | lcd_get_pos(2, 5)));
            lcd_send_string("DOIN' HERE");
            break;
        }

        lcd_send_cmd((char)(0x80 | lcd_get_pos(2, 3)));
        lcd_send_string("Set Resolution");

        lcd_send_cmd((char)(0x80 | lcd_get_pos(3, 7)));
        switch(__LL_ADC_CALC_DATA_TO_VOLTAGE(3, adc_median, LL_ADC_RESOLUTION_12B)){
        case 0:
            lcd_send_string(" 9 Bit");
            break;
        case 1:
            lcd_send_string("10 Bit");
            break;
        case 2:
            lcd_send_string("11 Bit");
            break;
        case 3:
            lcd_send_string("12 Bit");
            break;
        default:
            lcd_send_cmd((char)(0x80 | lcd_get_pos(1, 5)));
            lcd_send_string("WHAT Y'ALL");
            lcd_send_cmd((char)(0x80 | lcd_get_pos(2, 5)));
            lcd_send_string("DOIN' HERE");
            break;
        }

        lcd_send_cmd((char)(0x80 | lcd_get_pos(3, 0)));
        lcd_send_string("<-");
        lcd_send_cmd((char)(0x80 | lcd_get_pos(3, 18)));
        lcd_send_string("->");
        break;
    case MENU_STATE_THRESHOLD_T_LOW:
        lcd_send_cmd((char)(0x80 | lcd_get_pos(0, 1)));
        lcd_send_string("Current Alarm Low:");

        lcd_send_cmd((char)(0x80 | lcd_get_pos(1, 8)));
        sprintf(strbuf, "%d C", tLow);
        lcd_send_string(strbuf);

        lcd_send_cmd((char)(0x80 | lcd_get_pos(2, 3)));
        lcd_send_string("Set Alarm Low:");

        lcd_clear_line(3, 0);
        lcd_send_cmd((char)(0x80 | lcd_get_pos(3, 8)));
        sprintf(strbuf, "%d C", (int8_t)(__LL_ADC_CALC_DATA_TO_VOLTAGE(UINT8_MAX, adc_median, LL_ADC_RESOLUTION_12B)));
        lcd_send_string(strbuf);

        lcd_send_cmd((char)(0x80 | lcd_get_pos(3, 18)));
        lcd_send_string("->");
        lcd_send_cmd((char)(0x80 | lcd_get_pos(3, 0)));
        lcd_send_string("<-");
        break;
    case MENU_STATE_THRESHOLD_T_HIGH:
        lcd_send_cmd((char)(0x80 | lcd_get_pos(0, 1)));
        lcd_send_string("Current Alarm High");

        lcd_send_cmd((char)(0x80 | lcd_get_pos(1, 8)));
        sprintf(strbuf, "%d C", tHigh);
        lcd_send_string(strbuf);

        lcd_send_cmd((char)(0x80 | lcd_get_pos(2, 3)));
        lcd_send_string("Set Alarm High");

        lcd_clear_line(3, 0);
        lcd_send_cmd((char)(0x80 | lcd_get_pos(3, 8)));
        sprintf(strbuf, "%d C", (int8_t)(__LL_ADC_CALC_DATA_TO_VOLTAGE(UINT8_MAX, adc_median, LL_ADC_RESOLUTION_12B)));
        lcd_send_string(strbuf);

        lcd_send_cmd((char)(0x80 | lcd_get_pos(3, 0)));
        lcd_send_string("<-");
        break;
    default:
        lcd_send_cmd((char)(0x80 | lcd_get_pos(1, 5)));
        lcd_send_string("WHAT Y'ALL");
        lcd_send_cmd((char)(0x80 | lcd_get_pos(2, 5)));
        lcd_send_string("DOIN' HERE");
        break;
    }
}

void calcMedian(){
    uint32_t sum = 0;
    for (uint32_t i = 0; i < ADC_NUM_SAMPLES; ++i){
        sum += adc_buffer[i];
    }
    adc_median = sum/ADC_NUM_SAMPLES;
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc){
    if(hadc->Instance == ADC1){
        calcMedian();
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim){
    if(htim->Instance == htim2.Instance){ // OneWire Interrupt (Lib changes timings!)
        OneWire_TIM_Hook(&honew1);
    }
    if(htim->Instance == htim1.Instance){ // 100ms Interrupt
        DS18B20_ConvertT_single(&hds18b20);
        DS18B20_ReadScratchpad(&hds18b20);
        DS18B20_getThresholdT(&hds18b20, &tHigh, &tLow);

        if((int8_t)(DS18B20_getTemp(&hds18b20)) > tHigh || (int8_t)(DS18B20_getTemp(&hds18b20)) < tLow){
            HAL_GPIO_WritePin(LED_ALARM_GPIO_Port, LED_ALARM_Pin, GPIO_PIN_SET);
        }
        else HAL_GPIO_WritePin(LED_ALARM_GPIO_Port, LED_ALARM_Pin, GPIO_PIN_RESET);

        if (curr_menu_state == MENU_STATE_RES ||
            curr_menu_state == MENU_STATE_THRESHOLD_T_LOW ||
            curr_menu_state == MENU_STATE_THRESHOLD_T_HIGH){
            HAL_ADC_Start_DMA(&hadc1, adc_buffer, ADC_NUM_SAMPLES);
        }

        print_menu();
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
    switch(GPIO_Pin){
    case BUTTON1_Pin:
        if (curr_menu_state != MENU_STATE_TEMP){ // Call it Karl Marx, 'cause you ain't getting more left
            curr_menu_state--;
            lcd_clear();
        }
        break;
    case BUTTON2_Pin:
        if (curr_menu_state != MENU_STATE_THRESHOLD_T_HIGH){ // Call it Adolf, 'cause ... i'll better stop
            curr_menu_state++;
            lcd_clear();
        }
        break;
    case ACCEPT_BUTTON_Pin:
        switch(curr_menu_state){
        case MENU_STATE_RES:
            DS18B20_setResolution(&hds18b20,
                                 (DS18B20_Resolution)(__LL_ADC_CALC_DATA_TO_VOLTAGE(3, adc_median, LL_ADC_RESOLUTION_12B)));
            DS18B20_WriteScratchpad(&hds18b20);

            // copy Scratchpad into EEPROM... that lousy dev didnt provide a ds18b20 function for it
            OneWire_WriteByte(&honew1, DS18B20_CMD_CP_SCRATCHPAD);
            break;
        case MENU_STATE_THRESHOLD_T_LOW:
            DS18B20_setThresholdT(&hds18b20,
                    (int8_t)(__LL_ADC_CALC_DATA_TO_VOLTAGE(UINT8_MAX, adc_median, LL_ADC_RESOLUTION_12B)),
                    tHigh);
            DS18B20_WriteScratchpad(&hds18b20);
            OneWire_WriteByte(&honew1, DS18B20_CMD_CP_SCRATCHPAD);
            break;
        case MENU_STATE_THRESHOLD_T_HIGH:
            DS18B20_setThresholdT(&hds18b20,
                    tLow,
                    (int8_t)(__LL_ADC_CALC_DATA_TO_VOLTAGE(UINT8_MAX, adc_median, LL_ADC_RESOLUTION_12B)));
            DS18B20_WriteScratchpad(&hds18b20);
            OneWire_WriteByte(&honew1, DS18B20_CMD_CP_SCRATCHPAD);
            break;
        case MENU_STATE_TEMP: // keeping the compiler happy,
        case MENU_STATE_ROM:  // cause of the warning "-Wswitch" -.-
            break;
        }
        break;
    }
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
  MX_TIM2_Init();
  MX_I2C1_Init();
  MX_ADC1_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */

  OneWire_init(&honew1, &htim2, GPIOA, GPIO_PIN_1);
  DS18B20_init(&hds18b20, &honew1);
  DS18B20_ReadROM(&hds18b20);

  lcd_init();
  lcd_clear();

  HAL_TIM_Base_Start_IT(&htim1);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 100;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_12;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 199;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 49999;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 0;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */
  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_ALARM_GPIO_Port, LED_ALARM_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : ACCEPT_BUTTON_Pin */
  GPIO_InitStruct.Pin = ACCEPT_BUTTON_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(ACCEPT_BUTTON_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PA1 */
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : BUTTON1_Pin BUTTON2_Pin */
  GPIO_InitStruct.Pin = BUTTON1_Pin|BUTTON2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : LED_ALARM_Pin */
  GPIO_InitStruct.Pin = LED_ALARM_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_ALARM_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  HAL_NVIC_SetPriority(EXTI1_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

  HAL_NVIC_SetPriority(EXTI2_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  /* USER CODE END MX_GPIO_Init_2 */
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
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
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
