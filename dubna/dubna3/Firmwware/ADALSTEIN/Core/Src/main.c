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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "common.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

DAC_HandleTypeDef hdac;

UART_HandleTypeDef huart5;

/* USER CODE BEGIN PV */

uint32_t millisec = 0;

uint16_t dac_value = 0;
uint16_t prev_dac_value = 0;
uint16_t prev_dac_value_stable = 0;

char* commands[8] = {0, "ADC", "ON", "OFF", "POWEROFF", "TEST_VOLTAGE", "SET_VOLTAGE", "?"};

uint8_t tx_buff[100];
uint8_t tx_size = 0;
uint8_t rx_buff;
uint8_t rx_count = 0;

bool tx_flag = false;
bool mantigora_on_flag = false;
bool repeat_flag = false;
bool dac_set_flag = false;
bool on_finish_flag = true;
bool dac_voltage_setted_flag = false;
bool ok_flag = true;
bool up_flag = true;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_DAC_Init(void);
static void MX_UART5_Init(void);
/* USER CODE BEGIN PFP */
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
  MX_ADC1_Init();
  MX_DAC_Init();
  MX_UART5_Init();
  /* USER CODE BEGIN 2 */
  HAL_GPIO_WritePin(ON_ALLOW_GPIO_Port, ON_ALLOW_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(ON_MAN_LED_GPIO_Port, ON_MAN_LED_Pin, GPIO_PIN_SET);
  HAL_Delay(500);
  HAL_GPIO_WritePin(ON_ALLOW_GPIO_Port, ON_ALLOW_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(ON_MAN_LED_GPIO_Port, ON_MAN_LED_Pin, GPIO_PIN_RESET);
  HAL_Delay(500);

  HAL_GPIO_WritePin(ON_ALLOW_GPIO_Port, ON_ALLOW_Pin, GPIO_PIN_SET);

  HAL_DAC_Start(&hdac, DAC_CHANNEL_2);
  HAL_DAC_SetValue(&hdac, DAC_CHANNEL_2, DAC_ALIGN_12B_R, dac_value);

  HAL_UART_Receive_IT(&main_uart, &rx_buff, 1);
  HAL_GPIO_WritePin(UART5_RE_DE_GPIO_Port, UART5_RE_DE_Pin, RECEIVE);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	dac_set();

	if(tx_flag)
	{
		process_data(tx_buff);
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
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
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
  hadc1.Init.ContinuousConvMode = DISABLE;
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
  sConfig.Channel = ADC_CHANNEL_2;
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
  * @brief DAC Initialization Function
  * @param None
  * @retval None
  */
static void MX_DAC_Init(void)
{

  /* USER CODE BEGIN DAC_Init 0 */

  /* USER CODE END DAC_Init 0 */

  DAC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN DAC_Init 1 */

  /* USER CODE END DAC_Init 1 */

  /** DAC Initialization
  */
  hdac.Instance = DAC;
  if (HAL_DAC_Init(&hdac) != HAL_OK)
  {
    Error_Handler();
  }

  /** DAC channel OUT2 config
  */
  sConfig.DAC_Trigger = DAC_TRIGGER_NONE;
  sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
  if (HAL_DAC_ConfigChannel(&hdac, &sConfig, DAC_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DAC_Init 2 */

  /* USER CODE END DAC_Init 2 */

}

/**
  * @brief UART5 Initialization Function
  * @param None
  * @retval None
  */
static void MX_UART5_Init(void)
{

  /* USER CODE BEGIN UART5_Init 0 */

  /* USER CODE END UART5_Init 0 */

  /* USER CODE BEGIN UART5_Init 1 */

  /* USER CODE END UART5_Init 1 */
  huart5.Instance = UART5;
  huart5.Init.BaudRate = 115200;
  huart5.Init.WordLength = UART_WORDLENGTH_8B;
  huart5.Init.StopBits = UART_STOPBITS_1;
  huart5.Init.Parity = UART_PARITY_NONE;
  huart5.Init.Mode = UART_MODE_TX_RX;
  huart5.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart5.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart5) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN UART5_Init 2 */

  /* USER CODE END UART5_Init 2 */

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
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, ON_MAN_Pin|ON_MAN_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(ON_ALLOW_GPIO_Port, ON_ALLOW_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(UART5_RE_DE_GPIO_Port, UART5_RE_DE_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : ON_MAN_Pin ON_MAN_LED_Pin */
  GPIO_InitStruct.Pin = ON_MAN_Pin|ON_MAN_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : ON_ALLOW_Pin */
  GPIO_InitStruct.Pin = ON_ALLOW_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(ON_ALLOW_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PA11 PA12 */
  GPIO_InitStruct.Pin = GPIO_PIN_11|GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF10_OTG_FS;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : HAND_SHAKE_Pin */
  GPIO_InitStruct.Pin = HAND_SHAKE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(HAND_SHAKE_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : UART5_RE_DE_Pin */
  GPIO_InitStruct.Pin = UART5_RE_DE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(UART5_RE_DE_GPIO_Port, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	//1 = TX, 0 = RX control pin - PG3 usr_name - UART_RS485
	if(rx_buff == '\n')
	{
		tx_buff[rx_count - 1] = '\0';
		tx_size = rx_count + 1;
		rx_count = 0;

		tx_flag = true;
	}
	else
	{
		tx_buff[rx_count] = rx_buff;

		++rx_count;
	}

	HAL_UART_Receive_IT(&main_uart, &rx_buff, 1);
}

void process_data(uint8_t* data) {
	tx_flag = false;
	uint8_t command = 0;

	for(uint8_t i = 0; i < sizeof(commands)/sizeof(commands[0]); ++i)
	{
		if(strncmp(commands[i], (char*) tx_buff, tx_size-1) == 0)
		{
			command = i;
			break;
		}
		else
		{
			continue;
		}
	}

	switch(command) {
		case CASE_ADC:
			uint16_t adc_value = get_adc_value();
			float fadc_value = ((float) adc_value)/4095*1650;

			printf("MANTIGORA VOLTAGE: %.3f\n", fadc_value);

			break;
		case CASE_ON:
			if(!mantigora_on_flag)
			{
				printf("MANTIGORA ONNING...\n");
				mantigora_on_flag = true;
				on_finish_flag = false;
				HAL_GPIO_WritePin(ON_ALLOW_GPIO_Port, ON_ALLOW_Pin, GPIO_PIN_RESET);
				dac_set_flag = true;
				HAL_GPIO_WritePin(ON_MAN_LED_GPIO_Port, ON_MAN_LED_Pin, GPIO_PIN_SET);
				HAL_GPIO_WritePin(ON_MAN_GPIO_Port, ON_MAN_Pin, GPIO_PIN_SET);
			}
			else
			{
				printf("MANTIGORA ALREADY ONNED\n");
			}

			break;
		case CASE_OFF:
			if(mantigora_on_flag)
			{
				printf("MANTIGORA OFFING...\n");

				mantigora_on_flag = false;
				HAL_GPIO_WritePin(ON_ALLOW_GPIO_Port, ON_ALLOW_Pin, GPIO_PIN_RESET);
				dac_set_flag = true;
			}
			else
			{
				printf("MANTIGORA ALREADY OFFED\n");
			}

			break;
		case CASE_POWEROFF:
			printf("MANTIGORA POWEROFF!!!\n");

			shutdown();

			break;
		case CASE_TEST_VOLTAGE:
			test_voltage();

			break;
		case CASE_SET_VOLTAGE:
			if(mantigora_on_flag)
			{
				HAL_GPIO_WritePin(ON_ALLOW_GPIO_Port, ON_ALLOW_Pin, GPIO_PIN_RESET);

				on_finish_flag = false;
				dac_set_flag = true;

				printf("SET_VOLTAGE\n");
			} else {
				printf("ERROR_SET_VOLTAGE\n");
			}

			break;
		case CASE_QUESTION_MARK:
			printf("List of commands:\n"
				   "ADC - receive voltage from ADC*500\n"
				   "ON - on mantigora voltage source steady\n"
				   "OFF - off mantigora voltage source steady\n"
				   "POWEROFF - shutdown mantigora abruptly\n"
				   "TEST_VOLTAGE - test mantigora voltage rightness\n"
				   "SET_VOLTAGE - set mantigora voltage if voltage specified\n"
				   "? - show that list\n"
				   "XXXX - set value for buffer that after be used for SET_VOLTAGE,\n"
				   "TEST_VOLTAGE or ON. After OFF/SHUTDOWN this value wiped. Limits:\n"
				   "0-1500.");
		default:
			ok_flag = false;
			break;
	};

	if (tx_buff[0] >= '0' && tx_buff[0] <= '9')
	{
		uint16_t dec = atoi((char*) tx_buff);

		if(( (dec >= MIN_DAC_VALUE) &&
		   (dec <= MAX_DAC_VALUE) ) ||
		   (dec - MIN_DAC_VALUE == 0))
		{
			dac_value = (uint16_t) (  ( (uint32_t)(dec*4095) )/1650  );
			dac_voltage_setted_flag = false;
			printf("%d\n", dec);
			ok_flag = true;
		}
	}
	else if (!ok_flag)
	{
		printf("ERROR_WRONG_COMMAND\n");
		printf("TYPE \"?\" FOR HELP!\n");
		ok_flag = true;
	}

	memset(tx_buff, 0, tx_size);
}

void send_data(uint8_t* data) {
	//1 = TX, 0 = RX
	HAL_GPIO_WritePin(UART5_RE_DE_GPIO_Port, UART5_RE_DE_Pin, TRANSFER);
	HAL_UART_Transmit(&main_uart, tx_buff, 2, HAL_MAX_DELAY);
	HAL_GPIO_WritePin(UART5_RE_DE_GPIO_Port, UART5_RE_DE_Pin, RECEIVE);
}

void dac_set() {
	if((HAL_GetTick() - millisec > 30) && dac_set_flag)
	{
		millisec = HAL_GetTick();

		if(dac_value > prev_dac_value_stable)
		{
			up_flag = true;
		}
		else if(dac_value < prev_dac_value_stable)
		{
			up_flag = false;
		}

		if(mantigora_on_flag && !on_finish_flag)
		{
			if((dac_value > prev_dac_value) &&
			   (dac_value > prev_dac_value) &&
			   up_flag)
			{
				prev_dac_value += THREE_VOLTS_MANTIGORA;

				HAL_DAC_SetValue(&hdac, DAC_CHANNEL_2, DAC_ALIGN_12B_R, prev_dac_value);
			}
			else if((dac_value < prev_dac_value) &&
					  (prev_dac_value > dac_value) &&
					  !up_flag)
			{
				prev_dac_value -= THREE_VOLTS_MANTIGORA;

				HAL_DAC_SetValue(&hdac, DAC_CHANNEL_2, DAC_ALIGN_12B_R, prev_dac_value);
			}
			else
			{
				on_finish_flag = true;
			}
		}
		else if(!mantigora_on_flag && prev_dac_value > 0)
		{
			if(prev_dac_value - THREE_VOLTS_MANTIGORA <= 0)
			{
				prev_dac_value = 0;
			}
			else
			{
				prev_dac_value -= THREE_VOLTS_MANTIGORA;
			}

			HAL_DAC_SetValue(&hdac, DAC_CHANNEL_2, DAC_ALIGN_12B_R, prev_dac_value);
		}
		else
		{
			dac_set_flag = 0;

			if(mantigora_on_flag == 0)
			{
				dac_value = 0;

				HAL_GPIO_WritePin(ON_MAN_GPIO_Port, ON_MAN_Pin, GPIO_PIN_RESET);
				HAL_GPIO_WritePin(ON_MAN_LED_GPIO_Port, ON_MAN_LED_Pin, GPIO_PIN_RESET);
			}

			HAL_DAC_SetValue(&hdac, DAC_CHANNEL_2, DAC_ALIGN_12B_R, dac_value);

			HAL_GPIO_WritePin(ON_ALLOW_GPIO_Port, ON_ALLOW_Pin, GPIO_PIN_SET);

			dac_voltage_setted_flag = true;
			prev_dac_value = dac_value;
			prev_dac_value_stable = dac_value;
		}
	}
}

void shutdown()
{
	HAL_GPIO_WritePin(ON_MAN_GPIO_Port, ON_MAN_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(ON_MAN_LED_GPIO_Port, ON_MAN_LED_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(ON_ALLOW_GPIO_Port, ON_ALLOW_Pin, GPIO_PIN_RESET);

	dac_value = 0;
	HAL_DAC_SetValue(&hdac, DAC_CHANNEL_2, DAC_ALIGN_12B_R, dac_value);
	prev_dac_value = dac_value;
	dac_voltage_setted_flag = true;

	HAL_GPIO_WritePin(ON_ALLOW_GPIO_Port, ON_ALLOW_Pin, GPIO_PIN_SET);
}

void test_voltage()
{
	uint16_t adc_value = get_adc_value();

	if((adc_value < (dac_value+124)) &&
	  (adc_value > (dac_value-124)) &&
	   dac_voltage_setted_flag)
	{
		printf("VOLTAGE FINE!\n");
		return;
	}
	else if(!dac_voltage_setted_flag)
	{
		printf("ERROR NEW DAC VALUE NOT SETTED PLEASE WAIT\n");
		return;
	}
	else
	{
		shutdown();

		printf("ERROR TESTING VOLTAGE\n");
		return;
	}

	printf("VOLTAGE UNEXPECTED ERROR!\n");
}

uint16_t get_adc_value()
{
	uint16_t adc_value = 0;
	HAL_ADC_Start(&hadc1);
	HAL_ADC_PollForConversion(&hadc1, 100);
	adc_value = HAL_ADC_GetValue(&hadc1);
	HAL_ADC_Stop(&hadc1);

	return adc_value;
}

int _write(int file, char *ptr, int len)
{
	(void)file;

	if(file)
	{
		HAL_GPIO_WritePin(UART5_RE_DE_GPIO_Port, UART5_RE_DE_Pin, TRANSFER);
		HAL_UART_Transmit(&main_uart, (uint8_t*) ptr, len, HAL_MAX_DELAY);
		HAL_GPIO_WritePin(UART5_RE_DE_GPIO_Port, UART5_RE_DE_Pin, RECEIVE);
	}

	return len;
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
