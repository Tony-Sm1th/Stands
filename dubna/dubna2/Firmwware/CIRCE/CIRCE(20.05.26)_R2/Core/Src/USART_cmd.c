/*
 * USART_cmd.c
 *
 *  Created on: Sep 21, 2024
 *      Author: Tony
 */
#include "USART_cmd.h"

char str[100];
USART_prop_ptr usartprop;
UART_HandleTypeDef *uart;
void (*parse_func)(void*);

void USART_net_init(UART_HandleTypeDef *huart)
{
	usartprop.usart_buf[0]=0;
	usartprop.usart_cnt=0;
	uart = huart;
}

void USART_set_parse_func(void (*func)(void*)){
	parse_func = func;
}

void  USART_cmd_RxCpltCallback()
{
	uint8_t b;
	b = str[0];
	if (usartprop.usart_cnt>BUFFER_SIZE)
	{
		usartprop.usart_cnt=0;
		HAL_UART_Receive_IT(uart,(uint8_t*)str,1);
		return;
	}
	usartprop.usart_buf[usartprop.usart_cnt] = b;
	if(b=='\n')
	{
		usartprop.usart_buf[usartprop.usart_cnt+1]=0;
		parse_func((char*)usartprop.usart_buf);
		usartprop.usart_cnt=0;

		HAL_UART_Receive_IT(uart,(uint8_t*)str,1);
		return;
	}
	usartprop.usart_cnt++;
	HAL_UART_Receive_IT(uart,(uint8_t*)str,1);
}

void atoh(char *ascii_ptr, char *hex_ptr,int len)
{
	int i;
	USART_enable_tx();
	printf("[ECHO_atoh] ");
	for(i = 0; i < (len / 2); i++)
	{
		if(ascii_ptr[i*2] == '\n' || ascii_ptr[i*2] == '\r' || ascii_ptr[i*2+1] == '\n' || ascii_ptr[i*2+1] == '\r') continue;
		*(hex_ptr+i)   = (*(ascii_ptr+(2*i)) <= '9') ? ((*(ascii_ptr+(2*i)) - '0') * 16 ) :  (((*(ascii_ptr+(2*i)) - 'A') + 10) << 4);
		*(hex_ptr+i)  |= (*(ascii_ptr+(2*i)+1) <= '9') ? (*(ascii_ptr+(2*i)+1) - '0') :  (*(ascii_ptr+(2*i)+1) - 'A' + 10);
		printf("0x%02x ", *(hex_ptr+i));
	}
	printf("\n\n");
}

void USART_serial_log(uint8_t *data, uint16_t len, uint8_t direction, bool format){
	if(direction == 1){
		printf("[%ldms][IN] ",HAL_GetTick());
	}
	else if(direction == 0){
		printf("[%ldms][OUT] ",HAL_GetTick());
	}
	if(format == true){
		for(int i=0;i<len;i++){
			printf("%02X ",data[i]);
		}
	}
	else{
		printf("%s",data);
	}
	printf("\n");
}

void USART_enable_rx(){
	HAL_GPIO_WritePin(RE_DE6_Port,RE_DE6_Pin,RESET);
}
void USART_enable_tx(){
	HAL_GPIO_WritePin(RE_DE6_Port,RE_DE6_Pin,SET);
}
