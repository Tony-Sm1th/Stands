/*
 * USART_cmd.h
 *
 *  Created on: Sep 21, 2024
 *      Author: Tony
 */

#ifndef INC_USART_CMD_H_
#define INC_USART_CMD_H_
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "main.h"
#include "common.h"

//extern UART_HandleTypeDef huart3;
#define BUFFER_SIZE 200
typedef struct USART_prop{
uint8_t usart_buf[BUFFER_SIZE];
uint8_t usart_cnt;
} USART_prop_ptr;
void USART_enable_rx(void);	//set RE/DE for receiving
void USART_enable_tx(void);	//set RE/DE for transmitting
void USART_net_init(UART_HandleTypeDef *huart);
void USART_set_parse_func(void (*func)(void*));
void USART_cmd_RxCpltCallback(void);
void atoh(char *ascii_ptr, char *hex_ptr,int len);
void USART_serial_log(uint8_t *data, uint16_t len, uint8_t direction,bool format);

#endif /* INC_USART_CMD_H_ */
