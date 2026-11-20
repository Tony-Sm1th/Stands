/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define L_RSV2_Pin GPIO_PIN_14
#define L_RSV2_GPIO_Port GPIOC
#define L_RSV1_Pin GPIO_PIN_15
#define L_RSV1_GPIO_Port GPIOC
#define L_START_STOP_Pin GPIO_PIN_0
#define L_START_STOP_GPIO_Port GPIOC
#define L_START_STOP_EXTI_IRQn EXTI0_IRQn
#define L_FAULT_Pin GPIO_PIN_1
#define L_FAULT_GPIO_Port GPIOC
#define L_HANDSHAKE_Pin GPIO_PIN_2
#define L_HANDSHAKE_GPIO_Port GPIOC
#define L_HANDSHAKE_EXTI_IRQn EXTI2_IRQn
#define L_DOSE_RDY_Pin GPIO_PIN_3
#define L_DOSE_RDY_GPIO_Port GPIOC
#define LD1_Pin GPIO_PIN_0
#define LD1_GPIO_Port GPIOA
#define LD2_Pin GPIO_PIN_1
#define LD2_GPIO_Port GPIOA
#define IC_nRESET_Pin GPIO_PIN_3
#define IC_nRESET_GPIO_Port GPIOA
#define IC_DCLK_Pin GPIO_PIN_5
#define IC_DCLK_GPIO_Port GPIOA
#define IC_DIN_Pin GPIO_PIN_6
#define IC_DIN_GPIO_Port GPIOA
#define IC_nDVALID_Pin GPIO_PIN_5
#define IC_nDVALID_GPIO_Port GPIOC
#define IC_nDVALID_EXTI_IRQn EXTI9_5_IRQn
#define IC_DDC_CONV_Pin GPIO_PIN_10
#define IC_DDC_CONV_GPIO_Port GPIOB
#define IC_CLK_CFG_Pin GPIO_PIN_13
#define IC_CLK_CFG_GPIO_Port GPIOB
#define IC_DIN_CFG_Pin GPIO_PIN_15
#define IC_DIN_CFG_GPIO_Port GPIOB
#define RE_DE_6_Pin GPIO_PIN_8
#define RE_DE_6_GPIO_Port GPIOA
#define SPI3_CS_Pin GPIO_PIN_7
#define SPI3_CS_GPIO_Port GPIOB
#define SPI3_CS_EXTI_IRQn EXTI9_5_IRQn

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
