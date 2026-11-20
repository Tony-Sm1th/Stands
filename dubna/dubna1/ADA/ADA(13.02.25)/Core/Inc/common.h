/*
 * common.h
 *
 *  Created on: Mar 17, 2025
 *      Author: Tony
 */

#ifndef INC_COMMON_H_
#define INC_COMMON_H_
#define IC_FIRMWARE_VERSION "08.12.25"
//ADC CLK definition
/*
#define clk_timer htim1				//CLK
#define clk_channel	TIM_CHANNEL_4	//CLK

extern TIM_HandleTypeDef htim5;		//CONV
#define conv_timer	htim5			//CONV
#define conv_channel TIM_CHANNEL_1	//CONV
//SPI definition
extern SPI_HandleTypeDef hspi4;		//PC
#define	pc_spi  hspi4
extern SPI_HandleTypeDef hspi2;		//CFG
#define	cfg_spi  hspi2
extern SPI_HandleTypeDef hspi1;		//DATA
#define data_spi hspi1
#define IC_nDVALID_IRQ	EXTI4_IRQn
*/
//extern TIM_HandleTypeDef htim2;
//#define CNV_timer	htim2
#define CNV_CH		TIM_CHANNEL_4

extern SPI_HandleTypeDef hspi1;		//DATA
#define data_spi hspi1

extern UART_HandleTypeDef huart;		//DATA
#define uart_printf huart3
#endif /* INC_COMMON_H_ */
