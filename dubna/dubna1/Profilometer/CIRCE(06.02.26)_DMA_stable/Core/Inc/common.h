/*
 * common.h
 *
 *  Created on: Mar 17, 2025
 *      Author: Tony
 */

#ifndef INC_COMMON_H_
#define INC_COMMON_H_
#define IC_FIRMWARE_VERSION "03.12.25"
//#include "stm32f4xx_hal.h"
//ADC CLK definition
//#define clk_timer htim5				//CLK
//#define clk_channel	TIM_CHANNEL_3	//CLK

extern TIM_HandleTypeDef htim2;		//CONV
#define conv_timer	htim2			//CONV
#define conv_channel TIM_CHANNEL_4	//CONV
//SPI definition
//extern SPI_HandleTypeDef hspi3;		//PC
//#define	pc_spi  hspi3
extern UART_HandleTypeDef huart1;
#define mcu_huart huart1
extern UART_HandleTypeDef huart3;
#define danya_huart huart3
extern SPI_HandleTypeDef hspi2;		//CFG
#define	cfg_spi  hspi2
extern SPI_HandleTypeDef hspi1;		//DATA
//extern DMA_HandleTypeDef hdma_spi1_rx;		//DATA
#define data_spi hspi1
//#define data_dma_spi hdma_spi1_rx
extern SPI_HandleTypeDef hspi3;		//CFG
#define	test_spi  hspi3
#define IC_nDVALID_IRQ	 EXTI15_10_IRQn


#endif /* INC_COMMON_H_ */
