/*
 * USART_cmd.c
 *
 *  Created on: Sep 21, 2024
 *      Author: Tony
 */
#include "USART_cmd.h"

char str[100];
uint32_t target_offset = 0;
 float pf= 0.99;
USART_prop_ptr usartprop;
UART_HandleTypeDef *uart;
void (*parse_func)(void*);
//uint32_t stop = 0;
#define TABLE_SIZE 100



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
		//USART_enable_UARTrx();
		HAL_UART_Receive_IT(uart,(uint8_t*)str,1);
		//HAL_UART_Receive_IT(uart,(uint8_t*)str,1);
		return;
	}
	usartprop.usart_buf[usartprop.usart_cnt] = b;
	if(b=='\n')
	{
		usartprop.usart_buf[usartprop.usart_cnt+1]=0;
		parse_func((char*)usartprop.usart_buf);
		usartprop.usart_cnt=0;
		//HAL_UART_Receive_IT(uart,(uint8_t*)str,1);
		// USART_enable_UARTrx();
		HAL_UART_Receive_IT(uart,(uint8_t*)str,1);
		return;
	}
	usartprop.usart_cnt++;
	//HAL_UART_Receive_IT(uart,(uint8_t*)str,1);
	//USART_enable_UARTrx();
	HAL_UART_Receive_IT(uart,(uint8_t*)str,1);
}

void atoh(char *ascii_ptr, char *hex_ptr,int len)
{
	int i;
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


void string_parse(char* buf_str)
{
	if (strncmp(buf_str,"target_off:",11)==0){

		target_offset = strtol(buf_str + 11, NULL, 10);


		printf("target offset %ld\n",target_offset);
		//	Change_spot();

		//target_dose =strtol(buf_str + 7, NULL, 10);
	}
	else if (strncmp(buf_str,"target:",7)==0){
		for(int i =0; i<TABLE_SIZE;i++){
			dt.dose_table[i] = strtol(buf_str + 7, NULL, 10)+i*target_offset;
		}
		dt.table_size = TABLE_SIZE;

		Init_Dose_Table();
		printf("dose %ld, whole %ld, parrots %d\n", dt.dose_table[0],dt.whole[0],dt.parrots[0]);
		//	Change_spot();

		//target_dose =strtol(buf_str + 7, NULL, 10);
	}
	else if (strncmp(buf_str,"pf:",3)==0){
	    int32_t pf_int = strtol(buf_str + 3, NULL, 10);
	    if (pf_int >= 0 && pf_int <= 100) {
	        pf = (float)pf_int / 100.0f;
	        printf("Prediction factor set to: %.1f%%\n", pf * 100.0f);
	    } else {
	        printf("ERROR: pf must be 0-100\n");
	    }
	}
	else if (strncmp(buf_str,"spot_num:",9)==0){
			SPOT_PRINT = strtol(buf_str + 9, NULL, 10);
			//Set_Prediction_Factor(pf);
			printf("SPOT_PRINT %d\n", SPOT_PRINT);
		}
	else if(strncmp(buf_str, "pulse:",6)==0){
		if(buf_str[6] == '1'){
			sf.pulse_run=true;

			HAL_NVIC_EnableIRQ(nQ_IRQ);
			//Start_Pulse_cnt();
		}
		else{
			HAL_NVIC_DisableIRQ(nQ_IRQ);
			sf.pulse_run=false;
			Stop_Pulse_cnt();
		}
	}
	else if(strncmp(buf_str, "np:",3)==0){
			if(buf_str[3] == '1'){
				HAL_GPIO_WritePin(nPR_GPIO_Port,nPR_Pin,RESET);
				HAL_GPIO_WritePin(nCLR_GPIO_Port,nCLR_Pin,SET);
				HAL_GPIO_WritePin(nPR_GPIO_Port,nPR_Pin,SET);
				printf("np ON\n");
			}
			else{
				HAL_GPIO_WritePin(nPR_GPIO_Port,nPR_Pin,RESET);
				printf("np OFF\n");

			}
		}
	else if(strncmp(buf_str, "nc:",3)==0){
			if(buf_str[3] == '1'){
				HAL_GPIO_WritePin(nCLR_GPIO_Port,nCLR_Pin,RESET);
				HAL_GPIO_WritePin(nPR_GPIO_Port,nPR_Pin,SET);
				HAL_GPIO_WritePin(nCLR_GPIO_Port,nCLR_Pin,SET);
				printf("nclr ON\n");
			}
			else{
				HAL_GPIO_WritePin(nCLR_GPIO_Port,nCLR_Pin,RESET);
				printf("nclr OFF\n");

			}
		}
	else if(strncmp(buf_str, "r:",2)==0){
		HAL_GPIO_WritePin(nPR_GPIO_Port,nPR_Pin,RESET);
		HAL_GPIO_WritePin(nCLR_GPIO_Port,nCLR_Pin,RESET);
		HAL_GPIO_WritePin(nPR_GPIO_Port,nPR_Pin,SET);
		HAL_GPIO_WritePin(nCLR_GPIO_Port,nCLR_Pin,SET);
		printf("RESET\n");
		}
	else if(strncmp(buf_str, "max_adc:",8)==0){
		MAX_ADC = strtol(buf_str + 8, NULL, 10);
		//Set_Prediction_Factor(pf);
		printf("MAX_ADC %d\n", MAX_ADC);
	}
	else if (strncmp(buf_str,"?:",2)==0){
	    char help_buf[2048];
	    int len = snprintf(help_buf, sizeof(help_buf),
	        "\n--- Available Commands ---\n"
	        "target:<value>      - Set target dose for all spots\n"
	        "target_off:<value>  - Set offset step between spots\n"
	        "pf:<value>          - Set prediction factor (0-100, where 100=100%%)\n"
	        "                    - 100 = no early switch (exact target)\n"
	        "                    - 90  = switch at 90%% of target\n"
	        "                    - 99  = minimal early switch\n"
	        "spot_num:<value>    - Set number of spots to print in package\n"
	        "pulse:1             - Start pulse counting / enable system\n"
	        "pulse:0             - Stop pulse counting / disable system\n"
	        "max_adc:<value>     - Set max adc value in parrots\n"
	        "r                   - Reset trigger (restart system)\n"
	        "?:                  - Show this help\n"
	        "--- Current Settings ---\n"
	        "Target dose:    %lu\n"
	        "Target offset:  %ld\n"
	        "Prediction factor: %.1f%%\n"
	        "Spot print:     %d\n"
	        "Pulse run:      %s\n"
	        "Max adc:        %u\n"
	        "--- SD Card Info ---\n"
	        "SD ready:       %s\n"
	        "Current file:   %s\n"
	        "Session:        %lu\n"
	        "------------------------\n",
	        dt.dose_table[0],
	        target_offset,
	        pf * 100.0f,
	        SPOT_PRINT,
	        sf.pulse_run ? "ON" : "OFF",
	        MAX_ADC,
	        sd_manager.is_ready ? "YES" : "NO",
	        sd_manager.current_filename[0] != '\0' ? sd_manager.current_filename : "not set",
	        sd_manager.session_counter);

	    print_UART(pc_uart, (uint8_t*)help_buf, len);
	}
	else{

		printf("[ECHO] %s",buf_str);
	}

}

void print_UART(UART_HandleTypeDef p_uart, const uint8_t *buf, size_t len){
	HAL_GPIO_WritePin(UART_RE_DE_GPIO_Port,UART_RE_DE_Pin,SET);
	HAL_UART_Transmit_DMA(&p_uart, buf, len);
	//HAL_GPIO_WritePin(UART_RE_DE_GPIO_Port,UART_RE_DE_Pin,RESET);
}
