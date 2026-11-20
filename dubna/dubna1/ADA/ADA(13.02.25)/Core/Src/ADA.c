/*
 * ADA.c
 *
 *  Created on: Aug 11, 2025
 *      Author: Tony
 */
#include "ADA.h"
#define ref_voltage  3.3
#define base_voltage  3.3
#define base_resistance  200000

double step = ref_voltage/pow(2,16);
double base_current = (base_voltage)/base_resistance;
double max_current = -1000;
double min_current = 1000;
uint32_t start_processing;
uint32_t end_processing;
ADA_modes_t ADA;
uint8_t n = 0;
uint32_t sum;
bool PRINT = 0;
bool MEAN=0;


inline void ADA_Start_CNV(){
	//DWT->CYCCNT = 0;
	//__HAL_GPIO_EXTI_CLEAR_IT(ADA_nIRQ_Pin);
	//HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
	ADA.mode = MODE_ON;
	//HAL_TIM_Base_Start_IT(&CNV_timer);
	//HAL_TIM_PWM_Start_IT(&CNV_timer,CNV_CH);
	//end_processing = abs((DWT->CYCCNT-start_processing))/(SystemCoreClock / 1000000U);
	//delay_us(1);
	//ADA_ReadADC();

	//HAL_GPIO_WritePin(ADA_CNV_GPIO_Port, ADA_CNV_Pin, SET);
	//HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
	//printf("[INFO] Processing time %lu us\n", end_processing);
}

void ADA_Stop_CNV(){
	//HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);
	ADA.mode = MODE_OFF;
	//HAL_TIM_Base_Stop_IT(&CNV_timer);
	//HAL_TIM_PWM_Stop_IT(&CNV_timer,CNV_CH);
	//HAL_GPIO_WritePin(ADA_CNV_GPIO_Port, ADA_CNV_Pin, RESET);
	//ADA_modes.ADA_ADC_PROCESSING_MODE = ADA_ADC_PROCESSING_OFF;

}

void ADA_ReadADC(){

	HAL_GPIO_WritePin(ADA_CNV_GPIO_Port, ADA_CNV_Pin, SET);
	delay_us(5);
	HAL_GPIO_WritePin(ADA_CNV_GPIO_Port, ADA_CNV_Pin, RESET);

	if(HAL_SPI_Receive(&data_spi,(uint8_t*)&(ADA.buffer),ADA_BUFFER_SIZE,HAL_MAX_DELAY)!=HAL_OK){
		printf("Error\n");
	}
	if(PRINT == 1){
		double voltage = base_voltage-(double)ADA.buffer*step;
		if(MEAN==1){
		if(n<100){
			sum += ADA.buffer;
			n++;
		}else{
			ADA.buffer = (uint16_t)(sum/n);
			//double voltage = base_voltage-(double)ADA.buffer*step;
			double current = (voltage/base_resistance)*1000000000;
			if(current < min_current){
				min_current = current;
			}
			if(current > max_current){
				max_current = current;
			}

		//	printf("voltage = %u %fV run/max/min current = %f/%f/%fnA\n",ADA.buffer,voltage,current,max_current,min_current);
			printf("voltage = %u %fV\n",ADA.buffer,voltage);
			n=0;
			sum=0;

		}
	}else{
		printf("voltage = %u %fV\n",ADA.buffer,voltage);
	}
	}

	//if(ADA.buffer>60000){
	//	HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, SET);
	//}else{
	//	HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, RESET);
	//}
	//HAL_GPIO_WritePin(ADA_CNV_GPIO_Port, ADA_CNV_Pin, SET);

	//ADA_Start_CNV();

}

void string_parse(char* buf_str)
{
	__disable_irq();
	//IC_Read_cmd_Complete();
	if (strncmp(buf_str,"on:",3)==0){
		ADA.mode = MODE_ON;
		printf("Start\n");
		ADA_Start_CNV();

	}
	else if(strncmp(buf_str,"off:",4)==0){
		printf("Stop\n");
		ADA.mode = MODE_OFF;
		ADA_Stop_CNV();
	}else if (strncmp(buf_str,"p:",2)==0){
		PRINT ^= 1;
		min_current=1000;
		max_current=-1000;
		n=0;
		sum=0;
	}
	else if (strncmp(buf_str,"m:",2)==0){
		if(buf_str[2] == '1'){
			printf("Mean on\n");
			MEAN = 1;
		}else{
			printf("Mean off\n");
			MEAN = 0;
			}
		}
	else{
		printf("[ECHO] %s",buf_str);
	}
	__enable_irq();

}
