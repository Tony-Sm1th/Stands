/*
 * ADA.h
 *
 *  Created on: Aug 11, 2025
 *      Author: Tony
 */

#ifndef INC_ADA_H_
#define INC_ADA_H_
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include "delay_micros.h"
#include "main.h"
#include "USART_cmd.h"
#include "common.h"


#define T_CNVH 1 //us
#define ADA_BUFFER_SIZE 1

typedef enum{
	MODE_ON		=	1,
	MODE_OFF	=	0,
}ADA_switches_t;

typedef struct{
	uint16_t buffer;
	//bool ADA_ADC_PROCESSING_MODE;
	ADA_switches_t mode;
}ADA_modes_t;

//ADA_modes_enum_t ADA_enum;
extern ADA_modes_t ADA;

void ADA_Start_CNV(void);
void ADA_Stop_CNV(void);
void ADA_ReadADC(void);
void USB_exec_command(uint8_t *RxBuf,uint16_t RxBufLen,bool RxFlag);
#endif /* INC_ADA_H_ */
