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
#include "fatfs.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "common.h"
#define UTILS_IMPLEMENTATION
#include "utils.h"
#include "ElecADA.h"
#include "USART_cmd.h"
#include "uSD.h"
#include "delay_micros.h"

//#include <stm32f7xx_hal_sd.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define USB_BUFFLEN 500
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

SD_HandleTypeDef hsd1;
DMA_HandleTypeDef hdma_sdmmc1;

SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim5;
TIM_HandleTypeDef htim8;

UART_HandleTypeDef huart7;
UART_HandleTypeDef huart3;
DMA_HandleTypeDef hdma_uart7_tx;

/* USER CODE BEGIN PV */
extern char str[100];
extern void string_parse();
extern State_t st;
extern SystemFlags_t sf;

HAL_StatusTypeDef SD_Switch_To_4bit(SD_HandleTypeDef *hsd);
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_SPI1_Init(void);
static void MX_TIM2_Init(void);
static void MX_UART7_Init(void);
static void MX_SDMMC1_SD_Init(void);
static void MX_TIM8_Init(void);
static void MX_TIM5_Init(void);
/* USER CODE BEGIN PFP */
void Reset_SPI_DMA();
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  MX_USART3_UART_Init();
  MX_SPI1_Init();
  MX_TIM2_Init();
  MX_UART7_Init();
  MX_SDMMC1_SD_Init();
  MX_TIM8_Init();
  MX_TIM5_Init();
  MX_FATFS_Init();
  /* USER CODE BEGIN 2 */
	USART_net_init(&pc_uart);
	USART_set_parse_func(string_parse);
	HAL_UART_Receive_IT(&pc_uart, (uint8_t*) str, 1);
	DWT_Init();

	HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, SET);
	HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, SET);
	HAL_Delay(500);

	HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, RESET);
	HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, RESET);

	printf("[INFO] START F7\n");

	FRESULT fr = Mount_SD_Card();
	if (fr != FR_OK) {
		printf("[WARN] SD card not available, continuing without SD\n");
		sd_manager.is_ready = false;
	}

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	Start_ADC();
	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

		if (sf.adc_ready) {
			sf.adc_ready = false;
			Read_ADC();
			HAL_GPIO_WritePin(L_RSV_EXIT_GPIO_Port, L_RSV_EXIT_Pin, RESET);
		}
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 216;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 9;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SDMMC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SDMMC1_SD_Init(void)
{

  /* USER CODE BEGIN SDMMC1_Init 0 */

  /* USER CODE END SDMMC1_Init 0 */

  /* USER CODE BEGIN SDMMC1_Init 1 */

  /* USER CODE END SDMMC1_Init 1 */
  hsd1.Instance = SDMMC1;
  hsd1.Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;
  hsd1.Init.ClockBypass = SDMMC_CLOCK_BYPASS_DISABLE;
  hsd1.Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;
  hsd1.Init.BusWide = SDMMC_BUS_WIDE_1B;
  hsd1.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;
  hsd1.Init.ClockDiv = 0;
  /* USER CODE BEGIN SDMMC1_Init 2 */
//  HAL_SD_Init(&sd_card);
//  HAL_Delay(10);
//
//  if (HAL_SD_ConfigWideBusOperation(&sd_card, SDMMC_BUS_WIDE_4B) != HAL_OK)
//  {
//      Error_Handler();
//  }
  /* USER CODE END SDMMC1_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_16BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */
  /* USER CODE END SPI1_Init 2 */

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
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 107;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 59;
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
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 5;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

}

/**
  * @brief TIM5 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM5_Init(void)
{

  /* USER CODE BEGIN TIM5_Init 0 */

  /* USER CODE END TIM5_Init 0 */

  TIM_SlaveConfigTypeDef sSlaveConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM5_Init 1 */

  /* USER CODE END TIM5_Init 1 */
  htim5.Instance = TIM5;
  htim5.Init.Prescaler = 0;
  htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim5.Init.Period = 4294967295;
  htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim5.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim5) != HAL_OK)
  {
    Error_Handler();
  }
  sSlaveConfig.SlaveMode = TIM_SLAVEMODE_EXTERNAL1;
  sSlaveConfig.InputTrigger = TIM_TS_TI1F_ED;
  sSlaveConfig.TriggerFilter = 0;
  if (HAL_TIM_SlaveConfigSynchro(&htim5, &sSlaveConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim5, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM5_Init 2 */

  /* USER CODE END TIM5_Init 2 */

}

/**
  * @brief TIM8 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM8_Init(void)
{

  /* USER CODE BEGIN TIM8_Init 0 */

  /* USER CODE END TIM8_Init 0 */

  TIM_SlaveConfigTypeDef sSlaveConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM8_Init 1 */

  /* USER CODE END TIM8_Init 1 */
  htim8.Instance = TIM8;
  htim8.Init.Prescaler = 0;
  htim8.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim8.Init.Period = 65535;
  htim8.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim8.Init.RepetitionCounter = 0;
  htim8.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim8) != HAL_OK)
  {
    Error_Handler();
  }
  sSlaveConfig.SlaveMode = TIM_SLAVEMODE_EXTERNAL1;
  sSlaveConfig.InputTrigger = TIM_TS_ITR3;
  if (HAL_TIM_SlaveConfigSynchro(&htim8, &sSlaveConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim8, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM8_Init 2 */

  /* USER CODE END TIM8_Init 2 */

}

/**
  * @brief UART7 Initialization Function
  * @param None
  * @retval None
  */
static void MX_UART7_Init(void)
{

  /* USER CODE BEGIN UART7_Init 0 */

  /* USER CODE END UART7_Init 0 */

  /* USER CODE BEGIN UART7_Init 1 */

  /* USER CODE END UART7_Init 1 */
  huart7.Instance = UART7;
  huart7.Init.BaudRate = 115200;
  huart7.Init.WordLength = UART_WORDLENGTH_8B;
  huart7.Init.StopBits = UART_STOPBITS_1;
  huart7.Init.Parity = UART_PARITY_NONE;
  huart7.Init.Mode = UART_MODE_TX_RX;
  huart7.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart7.Init.OverSampling = UART_OVERSAMPLING_16;
  huart7.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart7.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart7) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN UART7_Init 2 */

  /* USER CODE END UART7_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream1_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream1_IRQn);
  /* DMA2_Stream3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream3_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn);

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
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, MCU1_S2_Pin|TEST_Pin|L_ERROR_Pin|L_HANDSHAKE_Pin
                          |L_DOSE_RDY_Pin|MCU1_S1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOF, MCU2_S1_Pin|MCU2_S2_Pin|L_RSV_EXIT_Pin|L_MAX_LAYER_DOSE_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(nPR_GPIO_Port, nPR_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(nCLR_GPIO_Port, nCLR_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LD1_Pin|CS_4_Pin|LD3_Pin|LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(USB_PowerSwitchOn_GPIO_Port, USB_PowerSwitchOn_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(UART_RE_DE_GPIO_Port, UART_RE_DE_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : MCU1_S2_Pin TEST_Pin L_ERROR_Pin L_HANDSHAKE_Pin
                           L_DOSE_RDY_Pin MCU1_S1_Pin */
  GPIO_InitStruct.Pin = MCU1_S2_Pin|TEST_Pin|L_ERROR_Pin|L_HANDSHAKE_Pin
                          |L_DOSE_RDY_Pin|MCU1_S1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : PE3 PE6 PE12 PE13
                           PE14 PE15 PE1 */
  GPIO_InitStruct.Pin = GPIO_PIN_3|GPIO_PIN_6|GPIO_PIN_12|GPIO_PIN_13
                          |GPIO_PIN_14|GPIO_PIN_15|GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : nQ_INT_Pin */
  GPIO_InitStruct.Pin = nQ_INT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(nQ_INT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : USER_Btn_Pin */
  GPIO_InitStruct.Pin = USER_Btn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USER_Btn_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PF0 PF1 PF2 PF3
                           PF4 PF5 PF6 PF9
                           PF10 PF11 PF12 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
                          |GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_9
                          |GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pins : MCU2_S1_Pin MCU2_S2_Pin L_RSV_EXIT_Pin L_MAX_LAYER_DOSE_Pin */
  GPIO_InitStruct.Pin = MCU2_S1_Pin|MCU2_S2_Pin|L_RSV_EXIT_Pin|L_MAX_LAYER_DOSE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pins : nPR_Pin UART_RE_DE_Pin */
  GPIO_InitStruct.Pin = nPR_Pin|UART_RE_DE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PC1 PC2 PC3 PC4
                           PC5 PC7 */
  GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4
                          |GPIO_PIN_5|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PA1 PA2 PA4 PA8
                           PA9 PA10 PA11 PA12 */
  GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_4|GPIO_PIN_8
                          |GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : nCLR_Pin */
  GPIO_InitStruct.Pin = nCLR_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(nCLR_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LD1_Pin CS_4_Pin LD3_Pin LD2_Pin */
  GPIO_InitStruct.Pin = LD1_Pin|CS_4_Pin|LD3_Pin|LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PB1 PB2 PB10 PB12
                           PB13 PB15 PB3 PB4
                           PB5 PB6 PB8 PB9 */
  GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_10|GPIO_PIN_12
                          |GPIO_PIN_13|GPIO_PIN_15|GPIO_PIN_3|GPIO_PIN_4
                          |GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_8|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : L_MAGNET_Pin */
  GPIO_InitStruct.Pin = L_MAGNET_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(L_MAGNET_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PG0 PG1 PG2 PG3
                           PG4 PG5 PG8 PG10
                           PG11 PG12 PG13 PG15 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
                          |GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_8|GPIO_PIN_10
                          |GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /*Configure GPIO pins : PD10 PD11 PD12 PD13
                           PD14 PD15 PD0 PD1
                           PD3 PD4 PD5 PD6
                           PD7 */
  GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13
                          |GPIO_PIN_14|GPIO_PIN_15|GPIO_PIN_0|GPIO_PIN_1
                          |GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6
                          |GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_PowerSwitchOn_Pin */
  GPIO_InitStruct.Pin = USB_PowerSwitchOn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(USB_PowerSwitchOn_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_OverCurrent_Pin */
  GPIO_InitStruct.Pin = USB_OverCurrent_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USB_OverCurrent_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : L_RSV_ENTRY_Pin L_START_STOP_Pin */
  GPIO_InitStruct.Pin = L_RSV_ENTRY_Pin|L_START_STOP_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
	HAL_NVIC_DisableIRQ(nQ_IRQ);
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
int _write(int file, char *ptr, int len) {
	(void) file;
	if (file == 1) {
		//HAL_UART_Transmit(&out_uart,(uint8_t*)ptr,len,HAL_MAX_DELAY);
		print_UART(pc_uart, (uint8_t*) ptr, len);
		//HAL_UART_Transmit(&pc_uart,(uint8_t*)ptr,len,HAL_MAX_DELAY);
		//HAL_UART_Transmit_DMA(&out_uart,(uint8_t*)ptr,len);
	}
	return len;
}

//void HAL_SD_TxCpltCallback(SD_HandleTypeDef *hsd)
////void BSP_SD_WriteCpltCallback(void)
//{
//	/* Prevent unused argument(s) compilation warning */
//	stop = DWT->CYCCNT;
//	printf("Write SD is ok. Time is %u us\n", abs((stop - start)/(SystemCoreClock / 1000000U)));
//
//	/* NOTE : This function should not be modified, when the callback is needed,
//            the HAL_SD_TxCpltCallback can be implemented in the user file
//	 */
//}
//
//void HAL_SD_RxCpltCallback(SD_HandleTypeDef *hsd)
////void BSP_SD_WriteCpltCallback(void)
//{
//	/* Prevent unused argument(s) compilation warning */
//	stop = DWT->CYCCNT;
//	printf("Read SD is ok. Time is %u us\n", abs((stop - start)/(SystemCoreClock / 1000000U)));
//
//	/* NOTE : This function should not be modified, when the callback is needed,
//            the HAL_SD_TxCpltCallback can be implemented in the user file
//	 */
//}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart == &out_uart) {
		sf.uart_lock = false;
	}
	if (huart == &pc_uart) {
		sf.uart_lock = false;
		HAL_GPIO_WritePin(UART_RE_DE_GPIO_Port, UART_RE_DE_Pin, RESET);

		if (dt.current_index >= dt.table_size) {
			if(!ts.start)
				Stop_Pulse_cnt();
		}
	}
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart == &out_uart) {
		USART_cmd_RxCpltCallback();
	}
	if (huart == &pc_uart) {
		USART_cmd_RxCpltCallback();
	}
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi) {
	if (hspi == &adc_spi) {
	}
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
	if (huart == &out_uart) {
		sf.uart_lock = false;
		// Опционально: логируем ошибку
	}
	if (huart == &pc_uart) {
		sf.uart_lock = false;
		// Опционально: логируем ошибку
	}
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim) {
	if (htim == &adc_timer) {
		sf.adc_ready = true;
		HAL_GPIO_WritePin(L_RSV_EXIT_GPIO_Port, L_RSV_EXIT_Pin, SET);

	}
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	if (htim == &pulse_cnt) {
	}

	if (htim == &adc_timer) {

	}
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	if (GPIO_Pin == nQ_INT_Pin && sf.pulse_run == true) {
		HAL_NVIC_DisableIRQ(nQ_IRQ);
		Start_Pulse_cnt();
	}
	if (GPIO_Pin == L_MAGNET_Pin) {
		On_Magnet_Arrived();
	}

}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi) {
	if (hspi == &adc_spi) {
		while (1) {
			HAL_GPIO_TogglePin(LD3_GPIO_Port, LD3_Pin);
			printf("SPI ERROR\n");
			HAL_Delay(1000);

		}
	}
}

HAL_StatusTypeDef HAL_SD_InitCard_FIX(SD_HandleTypeDef *hsd) {
	HAL_StatusTypeDef status;
	SD_InitTypeDef Init;

	Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;
	Init.ClockBypass = SDMMC_CLOCK_BYPASS_DISABLE;
	Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;
	Init.BusWide = SDMMC_BUS_WIDE_1B;
	Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;
	Init.ClockDiv = SDMMC_INIT_CLK_DIV;

	status = SDMMC_Init(hsd->Instance, Init);
	if (status != HAL_OK) {
		return HAL_ERROR;
	}

	return HAL_OK;
}
// Функция ручного переключения на 4 бита
/*
 * @brief  Ручное переключение SD-карты в 4-битный режим с проверками
 * @param  hsd: Указатель на SD handle
 * @retval HAL статус
 */
HAL_StatusTypeDef SD_Switch_To_4bit(SD_HandleTypeDef *hsd) {
	HAL_StatusTypeDef status;

	if (hsd == NULL) {
		return HAL_ERROR;
	}

	/* Проверяем состояние карты - должна быть инициализирована в 1-битном режиме */
	if (hsd->State != HAL_SD_STATE_READY) {
		return HAL_BUSY;
	}

	/* Проверяем, что карта поддерживает 4-битный режим */
	/* Для SD-карт (не MMC) */
	if (hsd->SdCard.CardType != CARD_SECURED) {
		/* �?спользуем стандартную HAL функцию для включения широкой шины */
		status = HAL_SD_ConfigWideBusOperation(hsd, SDMMC_BUS_WIDE_4B);

		if (status != HAL_OK) {
			/* Если не удалось, возвращаем ошибку */
			hsd->ErrorCode |= HAL_SD_ERROR_UNSUPPORTED_FEATURE;
			return HAL_ERROR;
		}
	} else {
		/* Secured карты не поддерживают широкую шину */
		hsd->ErrorCode |= HAL_SD_ERROR_UNSUPPORTED_FEATURE;
		return HAL_ERROR;
	}

	return HAL_OK;
}
//
//static void SDIO_SDCard_Test(void)
//{
////  FATFS FatFs;
////  FIL Fil;
////  FRESULT FR_Status;
////  FATFS *FS_Ptr;
////  UINT RWC, WWC; // Read/Write Word Counter
////  DWORD FreeClusters;
////  uint32_t TotalSize, FreeSpace;
////  char RW_Buffer[200];
//  do
//  {
//    //------------------[ Mount The SD Card ]--------------------
//    FR_Status = f_mount(&FatFs, SDPath, 1);
//    if (FR_Status != FR_OK)
//    {
//      printf("Error! While Mounting SD Card, Error Code: (%i)\r\n", FR_Status);
//      //USB_CDC_Print(TxBuffer);
//      break;
//    }
//    printf("SD Card Mounted Successfully! \r\n\n");
//    //USB_CDC_Print(TxBuffer);
//    //------------------[ Get & Print The SD Card Size & Free Space ]--------------------
//    f_getfree("", &FreeClusters, &FS_Ptr);
//    TotalSize = (uint32_t)((FS_Ptr->n_fatent - 2) * FS_Ptr->csize * 0.5);
//    FreeSpace = (uint32_t)(FreeClusters * FS_Ptr->csize * 0.5);
//    printf("Total SD Card Size: %lu Bytes\r\n", TotalSize);
//    //USB_CDC_Print(TxBuffer);
//    printf("Free SD Card Space: %lu Bytes\r\n\n", FreeSpace);
//    //USB_CDC_Print(TxBuffer);
//    //------------------[ Open A Text File For Write & Write Data ]--------------------
//    //Open the file
//    FR_Status = f_open(&Fil, "MyTextFile.txt", FA_WRITE | FA_READ | FA_CREATE_ALWAYS);
//    if(FR_Status != FR_OK)
//    {
//      printf("Error! While Creating/Opening A New Text File, Error Code: (%i)\r\n", FR_Status);
//      //USB_CDC_Print(TxBuffer);
//      break;
//    }
//
//    printf("Text File Created & Opened! Writing Data To The Text File..\r\n\n");
//    //USB_CDC_Print(TxBuffer);
//    // (1) Write Data To The Text File [ Using f_puts() Function ]
//    f_puts("Hello! From STM32 To SD Card Over SDMMC, Using f_puts()\n", &Fil);
//    // (2) Write Data To The Text File [ Using f_write() Function ]
//    strcpy(RW_Buffer, "Hello! From STM32 To SD Card Over SDMMC, Using f_write()\r\n");
//    f_write(&Fil, RW_Buffer, strlen(RW_Buffer), &WWC);
//    // Close The File
//    f_close(&Fil);
//    //------------------[ Open A Text File For Read & Read Its Data ]--------------------
//    // Open The File
//    FR_Status = f_open(&Fil, "MyTextFile.txt", FA_READ);
//    if(FR_Status != FR_OK)
//    {
//      printf( "Error! While Opening (MyTextFile.txt) File For Read.. \r\n");
//      //USB_CDC_Print(TxBuffer);
//      break;
//    }
//    // (1) Read The Text File's Data [ Using f_gets() Function ]
//    f_gets(RW_Buffer, sizeof(RW_Buffer), &Fil);
//    printf("Data Read From (MyTextFile.txt) Using f_gets():%s", RW_Buffer);
//    //USB_CDC_Print(TxBuffer);
//    // (2) Read The Text File's Data [ Using f_read() Function ]
//    f_read(&Fil, RW_Buffer, f_size(&Fil), &RWC);
//    printf( "Data Read From (MyTextFile.txt) Using f_read():%s", RW_Buffer);
//    //USB_CDC_Print(TxBuffer);
//    // Close The File
//    f_close(&Fil);
//    printf( "File Closed! \r\n\n");
//    //USB_CDC_Print(TxBuffer);
//    //------------------[ Open An Existing Text File, Update Its Content, Read It Back ]--------------------
//    // (1) Open The Existing File For Write (Update)
//    FR_Status = f_open(&Fil, "MyTextFile.txt", FA_OPEN_EXISTING | FA_WRITE);
//    FR_Status = f_lseek(&Fil, f_size(&Fil)); // Move The File Pointer To The EOF (End-Of-File)
//    if(FR_Status != FR_OK)
//    {
//      printf( "Error! While Opening (MyTextFile.txt) File For Update.. \r\n");
//      //USB_CDC_Print(TxBuffer);
//      break;
//    }
//    // (2) Write New Line of Text Data To The File
//    FR_Status = f_puts("This New Line Was Added During File Update!\r\n", &Fil);
//    f_close(&Fil);
//    memset(RW_Buffer,'\0',sizeof(RW_Buffer)); // Clear The Buffer
//    // (3) Read The Contents of The Text File After The Update
//    FR_Status = f_open(&Fil, "MyTextFile.txt", FA_READ); // Open The File For Read
//    f_read(&Fil, RW_Buffer, f_size(&Fil), &RWC);
//    printf( "Data Read From (MyTextFile.txt) After Update:\r\n%s", RW_Buffer);
//    //USB_CDC_Print(TxBuffer);
//    f_close(&Fil);
//    //------------------[ Delete The Text File ]--------------------
//    // Delete The File
//    /*
//    FR_Status = f_unlink(MyTextFile.txt);
//    if (FR_Status != FR_OK){
//        sprintf(TxBuffer, "Error! While Deleting The (MyTextFile.txt) File.. \r\n");
//        USC_CDC_Print(TxBuffer);
//    }
//    */
//  } while(0);
//  //------------------[ Test Complete! Unmount The SD Card ]--------------------
//  FR_Status = f_mount(NULL, "", 0);
//  if (FR_Status != FR_OK)
//  {
//      printf("\r\nError! While Un-mounting SD Card, Error Code: (%i)\r\n", FR_Status);
//      //USB_CDC_Print(TxBuffer);
//  } else{
//      printf( "\r\nSD Card Un-mounted Successfully! \r\n");
//      //USB_CDC_Print(TxBuffer);
//  }
//}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, SET);
	HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, RESET);
	//HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, SET);
	__disable_irq();
	while (1) {
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
