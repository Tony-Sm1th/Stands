/*
 * common.h
 *
 *  Created on: Mar 17, 2025
 *      Author: Tony
 */

#ifndef INC_COMMON_H_
#define INC_COMMON_H_

#include "main.h"
#include "fatfs.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#define IC_FIRMWARE_VERSION "10.04.26"

extern UART_HandleTypeDef huart3;
#define out_uart huart7

extern UART_HandleTypeDef huart7;
#define spiu_uart huart3

extern TIM_HandleTypeDef htim5;
#define pulse_cnt		htim5
//#define TIM_CHANNEL

extern TIM_HandleTypeDef htim8;
#define pulse_cnt_ovf	htim8

extern TIM_HandleTypeDef htim2;
#define adc_timer 		htim2
#define adc_channel		TIM_CHANNEL_1

extern IWDG_HandleTypeDef hiwdg;
#define watch_dog_reset hiwdg

extern SPI_HandleTypeDef hspi1;
#define adc_spi			hspi1

extern SD_HandleTypeDef hsd1;
#define sd_card			hsd1

#define nQ_IRQ			EXTI4_IRQn
#define adc_timer_IRQ	TIM2_IRQn
#define pulse_timer_IRQ	TIM5_IRQn
uint32_t Processing_Timer(bool sw, volatile uint32_t *pTimer);

void print_UART(UART_HandleTypeDef p_uart, const uint8_t *buf, size_t len);

#endif /* INC_COMMON_H_ */
