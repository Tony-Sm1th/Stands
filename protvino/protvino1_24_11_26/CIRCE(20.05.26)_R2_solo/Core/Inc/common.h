/*
 * common.h
 *
 *  Created on: Mar 17, 2025
 *      Author: Tony
 */

#ifndef INC_COMMON_H_
#define INC_COMMON_H_
#define IC_FIRMWARE_VERSION "01.01.26"

extern TIM_HandleTypeDef htim2;		//CONV
#define conv_timer	htim2			//CONV
#define conv_channel TIM_CHANNEL_3	//CONV

extern TIM_HandleTypeDef htim9;		//CLK
#define clk_timer	htim9			//CLK
#define clk_channel TIM_CHANNEL_1	//CLK

extern UART_HandleTypeDef huart6;	//OUT UART
#define out_huart huart6
#define RE_DE6_Port 	RE_DE_6_GPIO_Port
#define RE_DE6_Pin 		RE_DE_6_Pin

extern IWDG_HandleTypeDef hiwdg;
#define watch_dog_reset hiwdg



extern SPI_HandleTypeDef hspi2;		//CFG
#define	cfg_spi  hspi2

extern SPI_HandleTypeDef hspi1;		//DATA
#define data_spi hspi1

#define IC_nDVALID_IRQ	 EXTI9_5_IRQn//nDVALID

#define FAULT_Port		L_FAULT_GPIO_Port
#define FAULT_Pin		L_FAULT_Pin



#endif /* INC_COMMON_H_ */
