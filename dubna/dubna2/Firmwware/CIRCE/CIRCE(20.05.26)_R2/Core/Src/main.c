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
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usbd_cdc_if.h"
#include "IC_ADC64.h"
#include "USART_cmd.h"
#include "delay_micros.h"
#include "common.h"
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
I2C_HandleTypeDef hi2c1;

QSPI_HandleTypeDef hqspi;

SD_HandleTypeDef hsd;
DMA_HandleTypeDef hdma_sdio;

SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef hspi2;
SPI_HandleTypeDef hspi3;
DMA_HandleTypeDef hdma_spi1_rx;
DMA_HandleTypeDef hdma_spi1_tx;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim9;

UART_HandleTypeDef huart6;
DMA_HandleTypeDef hdma_usart6_rx;
DMA_HandleTypeDef hdma_usart6_tx;

/* USER CODE BEGIN PV */
extern char str[100];
volatile uint8_t rx_index = 0;
volatile uint8_t reception_complete = 0;
extern void string_parse();
extern void IC_Read_cmd_UART();
extern bool USART_CMD_ON;
//extern IC_Data_t IC_Data;
volatile IC_state state;
//extern bool USART_CMD_ON;
extern IC_Mode_t IC_Mode;
uint8_t usbTxBuf[USB_BUFFLEN];
uint16_t usbTxBufLen;
uint8_t usbRxBuf[USB_BUFFLEN];
uint16_t usbRxBufLen;
volatile bool usbRxFlag;
bool irq_on_off;
bool line_is_busy = false;
bool start_reading = false;
//RTC_TimeTypeDef sTime = {0};
//RTC_DateTypeDef DateToUpdate = {0};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_SPI1_Init(void);
static void MX_SPI2_Init(void);
static void MX_TIM2_Init(void);
static void MX_SPI3_Init(void);
static void MX_SDIO_SD_Init(void);
static void MX_USART6_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_QUADSPI_Init(void);
static void MX_TIM9_Init(void);
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

  /* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  /* USER CODE BEGIN SysInit */
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_SPI1_Init();
  MX_SPI2_Init();
  MX_TIM2_Init();
  MX_USB_DEVICE_Init();
  MX_SPI3_Init();
  MX_SDIO_SD_Init();
  MX_USART6_UART_Init();
  MX_I2C1_Init();
  MX_QUADSPI_Init();
  MX_TIM9_Init();
  /* USER CODE BEGIN 2 */
  HAL_UART_Receive_DMA(&out_huart, (uint8_t*)str, 10);
  //set parse function for uart
  USART_set_parse_func(IC_Read_cmd_UART);

  DWT_Init();	//start us timer
  HAL_GPIO_WritePin(LD1_GPIO_Port,LD1_Pin,SET);
  HAL_GPIO_WritePin(LD2_GPIO_Port,LD2_Pin,SET);
  HAL_Delay(500);
  HAL_GPIO_WritePin(LD1_GPIO_Port,LD1_Pin,RESET);
  HAL_GPIO_WritePin(LD2_GPIO_Port,LD2_Pin,RESET);
  printf("[INFO] START F4\n");
  state = STATE_WAIT;
  HAL_TIM_PWM_Start(&clk_timer, clk_channel); //start clk

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1)
	{
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		switch(state){
		case STATE_WAIT:
			break;
		case STATE_READING:
			state = STATE_WAIT;
			IC_ReadADC();
			break;
		case STATE_PROCESSING:
			state = STATE_WAIT;
			IC_ReadADC_Complete();
			break;

		}


		if(usbRxFlag == true){
			usbRxFlag = false;
			string_parse((char*)usbRxBuf);
		}

		//HAL_GPIO_TogglePin(test_GPIO_Port,test_Pin);
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
  RCC_OscInitStruct.PLL.PLLN = 180;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the peripherals clock
  */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SDIO|RCC_PERIPHCLK_CLK48;
  PeriphClkInitStruct.PLLSAI.PLLSAIM = 4;
  PeriphClkInitStruct.PLLSAI.PLLSAIN = 192;
  PeriphClkInitStruct.PLLSAI.PLLSAIQ = 2;
  PeriphClkInitStruct.PLLSAI.PLLSAIP = RCC_PLLSAIP_DIV8;
  PeriphClkInitStruct.PLLSAIDivQ = 1;
  PeriphClkInitStruct.Clk48ClockSelection = RCC_CLK48CLKSOURCE_PLLSAIP;
  PeriphClkInitStruct.SdioClockSelection = RCC_SDIOCLKSOURCE_CLK48;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
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
  * @brief QUADSPI Initialization Function
  * @param None
  * @retval None
  */
static void MX_QUADSPI_Init(void)
{

  /* USER CODE BEGIN QUADSPI_Init 0 */

  /* USER CODE END QUADSPI_Init 0 */

  /* USER CODE BEGIN QUADSPI_Init 1 */

  /* USER CODE END QUADSPI_Init 1 */
  /* QUADSPI parameter configuration*/
  hqspi.Instance = QUADSPI;
  hqspi.Init.ClockPrescaler = 255;
  hqspi.Init.FifoThreshold = 1;
  hqspi.Init.SampleShifting = QSPI_SAMPLE_SHIFTING_NONE;
  hqspi.Init.FlashSize = 1;
  hqspi.Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_1_CYCLE;
  hqspi.Init.ClockMode = QSPI_CLOCK_MODE_0;
  hqspi.Init.FlashID = QSPI_FLASH_ID_1;
  hqspi.Init.DualFlash = QSPI_DUALFLASH_DISABLE;
  if (HAL_QSPI_Init(&hqspi) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN QUADSPI_Init 2 */

  /* USER CODE END QUADSPI_Init 2 */

}

/**
  * @brief SDIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_SDIO_SD_Init(void)
{

  /* USER CODE BEGIN SDIO_Init 0 */

  /* USER CODE END SDIO_Init 0 */

  /* USER CODE BEGIN SDIO_Init 1 */

  /* USER CODE END SDIO_Init 1 */
  hsd.Instance = SDIO;
  hsd.Init.ClockEdge = SDIO_CLOCK_EDGE_RISING;
  hsd.Init.ClockBypass = SDIO_CLOCK_BYPASS_DISABLE;
  hsd.Init.ClockPowerSave = SDIO_CLOCK_POWER_SAVE_DISABLE;
  hsd.Init.BusWide = SDIO_BUS_WIDE_1B;
  hsd.Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
  hsd.Init.ClockDiv = 0;
  if (HAL_SD_Init(&hsd) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_SD_ConfigWideBusOperation(&hsd, SDIO_BUS_WIDE_4B) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SDIO_Init 2 */

  /* USER CODE END SDIO_Init 2 */

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
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */
  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_16BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief SPI3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI3_Init(void)
{

  /* USER CODE BEGIN SPI3_Init 0 */

  /* USER CODE END SPI3_Init 0 */

  /* USER CODE BEGIN SPI3_Init 1 */

  /* USER CODE END SPI3_Init 1 */
  /* SPI3 parameter configuration*/
  hspi3.Instance = SPI3;
  hspi3.Init.Mode = SPI_MODE_MASTER;
  hspi3.Init.Direction = SPI_DIRECTION_2LINES;
  hspi3.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi3.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi3.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi3.Init.NSS = SPI_NSS_SOFT;
  hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi3.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi3.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi3.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi3.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI3_Init 2 */

  /* USER CODE END SPI3_Init 2 */

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
  htim2.Init.Prescaler = 89;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 499;
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
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

}

/**
  * @brief TIM9 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM9_Init(void)
{

  /* USER CODE BEGIN TIM9_Init 0 */

  /* USER CODE END TIM9_Init 0 */

  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM9_Init 1 */

  /* USER CODE END TIM9_Init 1 */
  htim9.Instance = TIM9;
  htim9.Init.Prescaler = 0;
  htim9.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim9.Init.Period = 17;
  htim9.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim9.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim9) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 8;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim9, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM9_Init 2 */

  /* USER CODE END TIM9_Init 2 */
  HAL_TIM_MspPostInit(&htim9);

}

/**
  * @brief USART6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART6_UART_Init(void)
{

  /* USER CODE BEGIN USART6_Init 0 */

  /* USER CODE END USART6_Init 0 */

  /* USER CODE BEGIN USART6_Init 1 */

  /* USER CODE END USART6_Init 1 */
  huart6.Instance = USART6;
  huart6.Init.BaudRate = 115200;
  huart6.Init.WordLength = UART_WORDLENGTH_8B;
  huart6.Init.StopBits = UART_STOPBITS_1;
  huart6.Init.Parity = UART_PARITY_NONE;
  huart6.Init.Mode = UART_MODE_TX_RX;
  huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart6.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart6) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART6_Init 2 */

  /* USER CODE END USART6_Init 2 */

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
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
  /* DMA2_Stream1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream1_IRQn);
  /* DMA2_Stream3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn);
  /* DMA2_Stream5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream5_IRQn);
  /* DMA2_Stream6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream6_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream6_IRQn);

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
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, L_RSV2_Pin|L_RSV1_Pin|L_FAULT_Pin|L_DOSE_RDY_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LD1_Pin|LD2_Pin|RE_DE_6_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(IC_nRESET_GPIO_Port, IC_nRESET_Pin, GPIO_PIN_SET);

  /*Configure GPIO pins : PC13 PC4 */
  GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : L_RSV2_Pin L_RSV1_Pin */
  GPIO_InitStruct.Pin = L_RSV2_Pin|L_RSV1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : L_START_STOP_Pin L_HANDSHAKE_Pin */
  GPIO_InitStruct.Pin = L_START_STOP_Pin|L_HANDSHAKE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : L_FAULT_Pin L_DOSE_RDY_Pin */
  GPIO_InitStruct.Pin = L_FAULT_Pin|L_DOSE_RDY_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : LD1_Pin LD2_Pin */
  GPIO_InitStruct.Pin = LD1_Pin|LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : IC_nRESET_Pin RE_DE_6_Pin */
  GPIO_InitStruct.Pin = IC_nRESET_Pin|RE_DE_6_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PA4 PA9 PA10 PA15 */
  GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : IC_nDVALID_Pin */
  GPIO_InitStruct.Pin = IC_nDVALID_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(IC_nDVALID_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PB12 */
  GPIO_InitStruct.Pin = GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : SPI3_CS_Pin */
  GPIO_InitStruct.Pin = SPI3_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(SPI3_CS_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  HAL_NVIC_SetPriority(EXTI2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);

  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
int _write(int file, char *ptr, int len)
{
	(void)file;
	if(file == 1){
			USART_enable_tx();
			HAL_UART_Transmit(&out_huart,(uint8_t*)ptr,len,HAL_MAX_DELAY);
			USART_enable_rx();

//		static uint8_t rc = USBD_OK;
//
//		do {
//			rc = CDC_Transmit_FS((uint8_t*)ptr, len);
//		} while (USBD_BUSY == rc);
//
//		if (USBD_FAIL == rc) {
//			/// NOTE: Should never reach here.
//			/// TODO: Handle this error.
//			return 0;
//		}


	}
	return len;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if(huart==&out_huart)
	{
		// 1. Останавливаем DMA
		HAL_UART_DMAStop(&out_huart);

		//	        // 2. Очищаем флаг ORE (самая частая проблема!)
		//	        if(__HAL_UART_GET_FLAG(&mcu_huart, UART_FLAG_ORE))
		//	        {
		//	            __HAL_UART_CLEAR_OREFLAG(&mcu_huart);
		//	        }

		// 3. Обрабатываем команду
		//	        state = STATE_CMD;
		IC_Read_cmd_UART(str);

		// 4. Перезапускаем DMA с задержкой
		//HAL_Delay(1);  // Минимальная задержка
		HAL_UART_Receive_DMA(&out_huart, (uint8_t*)str, 10);
	}
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi){
	HAL_GPIO_TogglePin(LD2_GPIO_Port,LD2_Pin);
	if(hspi == &data_spi){
		state = STATE_PROCESSING;
	}
}

void HAL_SPI_TxRxHalfCpltCallback(SPI_HandleTypeDef *hspi){
	HAL_GPIO_TogglePin(LD2_GPIO_Port,LD2_Pin);
	if(hspi == &data_spi){
	}
}

//void SPI_DMAHalfTransmitReceiveCplt(DMA_HandleTypeDef *hdma){
//	if(hdma == & hdma_spi1_rx){
//			IC_ReadADC_Complete();
//		}
//}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if(GPIO_Pin == IC_nDVALID_Pin) {
		state = STATE_READING;
	}
//	else if(IC_PC_Int_Pin){
//		IC_ADC_To_PC();
//	}
//	else if (GPIO_Pin == IC_EXT_TRIG_Pin && IC_Mode.ext_trigger_mode == IC_EXT_TRIGGER_ON) {
//		IC_Ext_Trig();
//	}
}

void Reset_SPI(SPI_HandleTypeDef *hspi, char *str) {
	// Deinit SPI
	HAL_SPI_DeInit(hspi);

	// Init SPI
	if (HAL_SPI_Init(hspi) != HAL_OK) {
		printf("[ERROR] Failed to reinitialize %s SPI\n",str);
	} else {
		printf("[INFO] %s SPI reinitialized successfully\n",str);
	}
}

void Reset_SPI_DMA(SPI_HandleTypeDef *hspi, char *str) {
	// stop DMA
	HAL_DMA_Abort(hspi->hdmatx);
	HAL_DMA_Abort(hspi->hdmarx);

	// Deinit SPI
	HAL_SPI_DeInit(hspi);

	// Init SPI
	if (HAL_SPI_Init(hspi) != HAL_OK) {
		printf("[ERROR] Failed to reinitialize %s SPI\n",str);
	} else {
		printf("[INFO] %s SPI and DMA reinitialized successfully\n",str);
	}
}


/**
 * @brief SPI Error Callback
 */
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
	if(hspi == &data_spi) {
		// Сохраняем время ошибки для отладки
		static uint32_t last_error_time = 0;
		uint32_t current_time = HAL_GetTick();

		// Защита от спама ошибок
		if((current_time - last_error_time) < 1000) {
			return; // Не чаще 1 раза в секунду
		}
		last_error_time = current_time;

		// Получаем код ошибки
		uint32_t error_code = HAL_SPI_GetError(hspi);

		printf("\n[SPI ERROR] =================================\n");
		printf("[SPI ERROR] Time: %lu ms\n", current_time);
		printf("[SPI ERROR] Code: 0x%08lX\n", error_code);
		printf("[SPI ERROR] SPI State: %d\n", hspi->State);

		// Анализ конкретных ошибок
		if(error_code & HAL_SPI_ERROR_MODF) {
			printf("[SPI ERROR] MODF - Mode Fault\n");
			// Автоматически очищаем флаг
			__HAL_SPI_CLEAR_MODFFLAG(hspi);
		}

		if(error_code & HAL_SPI_ERROR_CRC) {
			printf("[SPI ERROR] CRC Error\n");
			__HAL_SPI_CLEAR_CRCERRFLAG(hspi);
		}

		if(error_code & HAL_SPI_ERROR_OVR) {
			printf("[SPI ERROR] OVR - Overrun Error\n");
			// Важно правильно очистить OVR флаг
			__HAL_SPI_CLEAR_OVRFLAG(hspi);
		}

		if(error_code & HAL_SPI_ERROR_FRE) {
			printf("[SPI ERROR] FRE - Frame Error\n");
			__HAL_SPI_CLEAR_FREFLAG(hspi);
		}

		if(error_code & HAL_SPI_ERROR_DMA) {
			printf("[SPI ERROR] DMA Transfer Error\n");
			printf("[SPI ERROR] DMA RX State: %d\n", hspi->hdmarx ? hspi->hdmarx->State : -1);
			printf("[SPI ERROR] DMA TX State: %d\n", hspi->hdmatx ? hspi->hdmatx->State : -1);

			// Дополнительная диагностика DMA
			if(hspi->hdmarx) {
				printf("[SPI ERROR] DMA RX NDTR: %d\n", (int)hspi->hdmarx->Instance->NDTR);
				printf("[SPI ERROR] DMA RX CR: 0x%08lX\n", hspi->hdmarx->Instance->CR);
			}
		}

		if(error_code & HAL_SPI_ERROR_FLAG) {
			printf("[SPI ERROR] FLAG - Undefined Flag Error\n");
			// Сбрасываем все флаги
			//            __HAL_SPI_CLEAR_EOTFLAG(hspi);
			//            __HAL_SPI_CLEAR_TXTFFLAG(hspi);
		}

		if(error_code & HAL_SPI_ERROR_ABORT) {
			printf("[SPI ERROR] ABORT - Transfer Aborted\n");
		}


		printf("[SPI ERROR] =================================\n\n");

		// Автоматическое восстановление
		//        SPI_Auto_Recovery(hspi);
	}
}

void SPI_Error_Handler(SPI_HandleTypeDef *hspi, char *str) {
	uint32_t error = HAL_SPI_GetError(hspi);
	if (error & HAL_SPI_ERROR_OVR) {
		printf("[ERROR] %s SPI overrun error\n",str);
	}
	if (error & HAL_SPI_ERROR_MODF) {
		printf("[ERROR] %s SPI mode fault error\n",str);
	}
	if (error & HAL_SPI_ERROR_CRC) {
		printf("[ERROR] %s SPI CRC error\n",str);
	}
	if (error & HAL_SPI_ERROR_DMA) {
		printf("[ERROR] %s SPI DMA error\n",str);
	}
	if (error & HAL_SPI_ERROR_FLAG) {
		printf("[ERROR] %s SPI flag error\n",str);
	}

	// Сброс SPI
	Reset_SPI(hspi,str);
}
void USB_RXCallback(uint8_t* Buf, uint32_t *Len){
	memcpy(usbRxBuf, Buf, *Len);
	//usbRxBufLen = *Len;
	usbRxFlag = true;
	USART_CMD_ON = 0;
	if(NVIC_GetEnableIRQ(IC_nDVALID_IRQ) != 0){
		// if(__HAL_GPIO_EXTI_GET_IT(IC_nDVALID_IRQ)){
		irq_on_off = true;
	}else{
		irq_on_off = false;
	}
	IC_ReadADC_off();
	//USB_exec_command(usbRxBuf,usbRxBufLen,usbRxFlag);
	//string_parse((char*)Buf);
}

void USB_exec_command(uint8_t* RxBuf,uint16_t RxBufLen, bool RxFlag){
	if(RxFlag && RxBufLen){
		//uint32_t timeNowMS = HAL_GetTick();
		//usbTxBufLen = snprintf((char *) usbTxBuf, USB_BUFFLEN,"[%02lu:%02lu] %u\r\n", (timeNowMS /60000), (timeNowMS / 1000)%60, usbRxBufLen);
		if(strncmp((char*)RxBuf,"on:",3)==0){
			HAL_GPIO_WritePin(LD1_GPIO_Port,LD1_Pin,SET);
			//usbTxBufLen = snprintf((char *) usbTxBuf, USB_BUFFLEN,"On\n");
			//CDC_Transmit_FS(usbTxBuf, usbTxBufLen);
			printf("On\n");
		}else if(strncmp((char*)RxBuf,"off:",4) == 0){
			HAL_GPIO_WritePin(LD1_GPIO_Port,LD1_Pin,RESET);
			printf("Off\n");
		}

		RxFlag = 0;
		RxBufLen = 0;
	}
}
/*
void Get_time(){
	HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN); // RTC_FORMAT_BIN , RTC_FORMAT_BCD
	printf( "\n[UPTIME] %d:%d:%d\n", sTime.Hours, sTime.Minutes, sTime.Seconds);
	HAL_RTC_GetDate(&hrtc, &DateToUpdate, RTC_FORMAT_BIN);
			//printf("Date %d-%d-20%d\n", DateToUpdate.Date, DateToUpdate.Month, DateToUpdate.Year);

}
 */
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
