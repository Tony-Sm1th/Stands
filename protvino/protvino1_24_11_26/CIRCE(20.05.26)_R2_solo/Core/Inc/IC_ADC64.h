/*
 * IC_ADC64.h
 *
 *  Created on: Sep 12, 2024
 *      Author: Tony
 */

#ifndef INC_IC_ADC64_H_
#define INC_IC_ADC64_H_
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


extern void SPI_Error_Handler(SPI_HandleTypeDef *hspi, char *str);
//extern IWDG_HandleTypeDef hiwdg;
//extern TIM_OC_InitTypeDef sConfigOC;

//Test data instead of DDC264 data
#define CHANNEL_TEST_MODE 0

//#include "stm32f4xx_hal.h"
//PAGE 23 in ddc264 datasheet
//CFG register bits
#define CFG_RANGE1_POS    (10)
#define CFG_RANGE0_POS    (9)
#define CFG_FORMAT_POS    (8)
#define CFG_VERSION_POS   (7)
#define CFG_TEST_POS      (2)

#define RANGE1			1
#define RANGE0			1
#define	FORMAT			0
#define VERSION			1	//0 - DDC264C  1 - DDC264CK
#define TEST			0
//Bit placement
//#define	VERSION_BIT		7
//#define	FORMAT_BIT		8

//PAGE 21 in ddc264 datasheet
//delay in microseconds for configuration
#define IC_Trst 		100
#define IC_Twtrst 		100
#define IC_Twtwr 		100

//ddc264 cfg out size
#define IC_CFG_OUT_SIZE 128
//ddc264 number of channels
#define IC_CH_NUM		128
//port quantity
#define PORT_NUM		2 				//X and Y

//time constants
#define IC_MIN_PERIOD_TIME		320 	//us
#define IC_DEFAULT_PERIOD_TIME	2000 	//us
#define IC_DEFAULT_BEAM_WIDTH	1000	//us
#define IC_ZERO_CONV_TIME		0

//External trigger skip cycles
#define IC_EXT_TRIG_TRIM_CYCLE_START	2		//cycles to skip during start ext trigger
#define IC_EXT_TRIG_TRIM_CYCLE_END	2		//cycles to skip during end ext trigger

//Check code after 64 word of data
#define CHECK_WORD				1
#define CHECK_CODE				0xAAAA
#define CG_WORD					2		//Center of gravity

//Program trigger level
#define IC_PROGRAMM_TRIGGER_LVL	4			//Trigger fires after that amount of correct values in a row

#define TO_US	(SystemCoreClock / 1000000U)
//Reference delta in percent
//#define IC_REF_DELTA_PERCENT	30
typedef enum{
	STATE_WAIT,
	STATE_CMD,
	STATE_READING,
	STATE_PROCESSING
}IC_state;
//Commands from F7 controller
typedef enum{
	IC_CFG_IN		= 0x05,
	IC_ACCUM_NOISE	= 0x15,
	IC_REF			= 0x25,
	IC_READ_ADC		= 0x35,
	IC_SEND_DATA	= 0x45
}IC_cmd_t;

//Struct definition
//Modes
typedef enum{
	IC_MEAN_ON						= 1,
	IC_MEAN_OFF						= 0,
	IC_IN_ROW						= 0,
	IC_AS_CABLE						= 1,
	IC_NOISE_ACCUM_ON 				= 1,
	IC_NOISE_ACCUM_OFF 				= 0,
	IC_NOISE_DELETE_ON				= 1,
	IC_NOISE_DELETE_OFF				= 0,
	IC_PROGRAM_TRIGGER_ON 			= 1,
	IC_PROGRAM_TRIGGER_OFF			= 0,
	IC_PROGRAM_TRIGGER_FIRED		= 0,
	IC_PROGRAM_TRIGGER_NOT_FIRED	= 1,
	IC_EXT_TRIGGER_ON 				= 1,
	IC_EXT_TRIGGER_OFF				= 0,
	IC_REF_MODE_ON					= 1,
	IC_REF_MODE_OFF					= 0,
	IC_READ_ADC_ON					= 1,
	IC_READ_ADC_OFF					= 0,
}IC_modes_t;

typedef enum{
	STOP,
	START
}IC_processing_timer_t;
//Data struct
typedef struct {
    uint16_t dout[IC_CH_NUM];    		// DDC264 data buffer
    uint16_t noise_buf[IC_CH_NUM]; 		// noise buffer
    uint32_t mean[IC_CH_NUM];    		// mean value buffer
} IC_Data_t;
//Config struct
typedef struct {
    uint16_t cfg_in;             		// Config register
    uint16_t cfg_out[IC_CFG_OUT_SIZE]; 	// Config output data
    uint32_t period;             		// Integration period [CONV]
    uint32_t iter_num;          	  	// Current iteration
    uint32_t max_iter;           		// Max iteration for mean calc
    uint16_t trigger;            		// Programmable trigger threshold
    uint16_t noise_max;          	  	// Max noise level
} IC_Config_t;
//Data struct to PC
typedef struct{
	uint8_t	  port;										//Current port
	uint16_t  port_buf[PORT_NUM][IC_CH_NUM+CG_WORD+CHECK_WORD];	//Data buffer
	uint16_t* data_from_DDC;							//Data pointer for data receiving
	uint16_t* data_to_PC;								//Data pointer for data transfering
	bool 	  data_lock;								//Processing data flag
}IC_PC_t;
//Mode struct
typedef struct {
    bool change_mode;            // Alter data flow. from 1 to 64 or as cable demanded
    bool mean_mode;              // Mean value mode
    bool mean_mode_save;         // Backup mean value mode state
    bool noise_delete_mode_save; // Backup noise delete mode state
    bool noise_accum_mode;       // Backup noise accumulation mode state
    bool noise_accum_first_entry;// Flag of the first entry into noise accumulation function
    bool noise_delete_mode;      //	Delete noise mode
    bool program_trigger_mode;   // Program trigger mode
    bool ext_trigger_mode;       // External trigger mode
    //bool ext_trigger_mode_save;  // Backup External trigger mode state
    bool ref_mode;               // Reference comparison mode
    bool read_adc_mode;			//Read ADC mode
} IC_Mode_t;
//External trigger struct
volatile typedef struct{
	uint32_t beam_width; 		 //width of the beam
	uint32_t cycle_in_beam; 	 //number of cycles in beam
	uint16_t beam_cycle_count;	 //counter
	uint32_t one_cycle_time;	 //time of the one cycle
	uint32_t mean[IC_CH_NUM];	 //array for mean beam
}IC_ext_t;
//Ref mode struct
typedef struct{
	uint16_t ref_data[IC_CH_NUM];//reference data buffer
	uint16_t percent;			//set level of delta in percents out of ref_data
	uint16_t delta[IC_CH_NUM];	 //Acceptable tolerance of the beam
	bool	IC_REF_ALARM;		 //Flag of alarm when beam out of tolerance
}IC_ref_t;

//PAGE 21 ddc264.pdf
void IC_Reset(void);			//Reset and send configuration to DDC264
//PAGE 19 ddc264.pdf
//Read DDC264 after nDVALID
void IC_ReadADC(void);			//Read DDC264 after nDVALID interrupt
void IC_ReadADC_Complete(void);	//Callback after receiving all words
void IC_Read_cmd_Complete(void);//Callback after reading all commands and answer
void IC_Mean_Calc(void);		//Calculate mean value
void IC_RMS_Calc(void);
void IC_Normal_Calc(void);		//Process data
void IC_Noise_Accumulate(void);	//Accumulate noise
uint16_t IC_Noise_Remove(uint16_t data, uint16_t noise);//Calculate with noise cancellation
void IC_Center_Of_Gravity(void);
uint8_t IC_Program_Trigger(void);//Program trigger implementation
void IC_Ext_Trig(void);			//External trigger implementation
void IC_Ext_trig_on(void);		//External trigger on
void IC_Ext_trig_off(void);		//External trigger off
void IC_ReadADC_on(void);		//Start SPI for reading ADC
void IC_ReadADC_off(void);		//Stop SPI for reading ADC
bool IC_Ref_comparison();		//Compare to ref data
void IC_ADC_To_PC(void);		//Transmit data to PC
void IC_Show_Info(void);		//Show register info via UART
void IC_Read_cmd_UART();
void IC_Processing_Timer(IC_processing_timer_t t);
void Get_time(void);			//Get timestamp
void USB_exec_command(uint8_t *RxBuf,uint16_t RxBufLen,bool RxFlag);
#endif /* INC_IC_ADC64_H_ */
