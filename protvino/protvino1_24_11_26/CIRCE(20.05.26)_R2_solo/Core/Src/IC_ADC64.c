/*
 * IC_ADC64.c
 *
 *  Created on: Sep 12, 2024
 *      Author: Tony
 */

#include "IC_ADC64.h"

//data from PC array
static uint16_t IC_cmd[IC_CH_NUM + CG_WORD + CHECK_WORD];
//On/Off USART messages
bool USART_CMD_ON = 0;
//dummy arr for spi
bool cpy_lock = false;
bool send_uart = false;
bool processing_is_completed = false;
static uint16_t dummy[128] = { 0 };
static uint16_t print_buf[IC_CH_NUM + CG_WORD + CHECK_WORD] = {0};
//union for center of gravity
static union {
	float f;
	uint32_t u;
} float_union;
volatile static uint32_t IC_start_processing;
volatile static uint32_t IC_end_processing;
//static float CG;
/*static ch_buf[IC_CH_NUM]= {50,49,20,23,25,17,52,21,27,58,59,54,63,30,31,32,64,29,24,19,51,55,57,56,28,18,61,60,26,53,62,22,
 1,4,34,37,39,10,43,6,44,16,36,14,15,8,45,48,5,41,42,33,13,3,38,40,35,11,9,12,2,46,47,7,
 2,36,3,13,35,44,38,6,10,47,46,9,40,8,12,45,16,48,15,1,33,4,34,43,37,39,5,11,14,42,7,41,
 28,27,51,25,18,17,24,20,56,21,62,57,26,32,63,22,30,31,64,23,19,55,50,52,29,49,53,60,54,61,58,59
 };*/
static uint8_t ch_buf[IC_CH_NUM] = { 63, 60, 30, 27, 25, 54, 21, 58, 20, 48, 28,
		50, 49, 56, 19, 16, 59, 23, 22, 31, 51, 61, 26, 24, 29, 53, 55, 52, 62,
		18, 17, 57, 14, 15, 44, 41, 39, 47, 12, 43, 37, 6, 5, 10, 1, 34, 33, 32,
		0, 35, 40, 45, 13, 9, 7, 8, 36, 46, 3, 4, 38, 11, 2, 42, 126, 92, 125,
		115, 93, 84, 90, 122, 118, 81, 82, 119, 88, 120, 116, 83, 112, 80, 113,
		127, 95, 124, 94, 85, 91, 89, 123, 117, 114, 86, 121, 87, 100, 101, 77,
		103, 110, 111, 104, 108, 72, 107, 66, 71, 102, 96, 65, 106, 98, 97, 64,
		105, 109, 73, 78, 76, 99, 79, 75, 68, 74, 67, 70, 69

};
//Struct
//Data struct
IC_Data_t IC_Data;
//Config struct
static IC_Config_t IC_Config = {
		.cfg_in =
				(RANGE1 << CFG_RANGE1_POS) 		|
				(RANGE0 << CFG_RANGE0_POS) 		|
				(FORMAT << CFG_FORMAT_POS) 		|
				(VERSION << CFG_VERSION_POS) 	|
				(TEST << CFG_TEST_POS),
				.period = IC_DEFAULT_PERIOD_TIME, .max_iter = 20, };
//data for PC array
static IC_PC_t IC_PC = { .port = 0, .port_buf = {
		[0] = { [IC_CH_NUM + CG_WORD + CHECK_WORD - 1] = CHECK_CODE }, // init first port
		[1] = { [IC_CH_NUM + CG_WORD + CHECK_WORD - 1] = CHECK_CODE }, // init second port
},
		.data_from_DDC = IC_PC.port_buf[0], .data_to_PC = NULL, // pointer
		.data_lock = false };
//Mode struct
IC_Mode_t IC_Mode = {
		.change_mode = 				IC_AS_CABLE,
		.mean_mode = 				IC_MEAN_OFF,
		.noise_accum_mode = 		IC_NOISE_ACCUM_OFF,
		.noise_accum_first_entry = 	true,
		.noise_delete_mode = 		IC_NOISE_DELETE_OFF,
		.program_trigger_mode =		IC_PROGRAM_TRIGGER_OFF,
		.ext_trigger_mode = 		IC_EXT_TRIGGER_OFF,
		.ref_mode = 				IC_REF_MODE_OFF,
};
//External trigger
static IC_ext_t IC_ext = { 0, 0, 0, 0 };
//Reference mode
static IC_ref_t IC_ref = {
		.ref_data		= { 0 },
		.percent		= 30,
		.delta			= { 0 },
		.IC_REF_ALARM 	= false
};
//PAGE 21 ddc264.pdf
void IC_Reset() {
	__disable_irq();
	//////////SET PWM//////////
	HAL_TIM_PWM_Stop(&conv_timer, conv_channel);
	__HAL_TIM_SET_COUNTER(&conv_timer, 0);					//counter to zero
	__HAL_TIM_SET_AUTORELOAD(&conv_timer, IC_Config.period - 1);//timer period
	__HAL_TIM_SET_COMPARE(&conv_timer, conv_channel,
			(IC_Config.period >> 1) - 1);//width of a pulse or integration time
	//////////SET PWM//////////

	HAL_GPIO_WritePin(IC_nRESET_GPIO_Port, IC_nRESET_Pin, RESET);
	delay_us(IC_Trst);
	HAL_GPIO_WritePin(IC_nRESET_GPIO_Port, IC_nRESET_Pin, SET);
	delay_us(IC_Twtrst);
	HAL_SPI_Transmit(&cfg_spi, (uint8_t*) &IC_Config.cfg_in, 1, HAL_MAX_DELAY);
	delay_us(IC_Twtwr);
	HAL_SPI_Receive(&data_spi, (uint8_t*) IC_Config.cfg_out, IC_CFG_OUT_SIZE, HAL_MAX_DELAY);

	HAL_TIM_PWM_Start(&conv_timer, conv_channel);
	__enable_irq();
}

void IC_ReadADC() {
	IC_Processing_Timer(START);
#if CHANNEL_TEST_MODE == 0
//	HAL_SPI_TransmitReceive_DMA(&data_spi, (uint8_t*) dummy, (uint8_t*) IC_PC.port_buf[IC_PC.port], IC_CH_NUM);
	HAL_SPI_TransmitReceive(&data_spi, (uint8_t*) dummy, (uint8_t*) IC_PC.port_buf[IC_PC.port], IC_CH_NUM,HAL_MAX_DELAY);
	//	IC_PC.data_from_DDC = IC_PC.port_buf[IC_PC.port];
	//	IC_PC.port ^= 1;
	//	if (IC_PC.data_lock == false) {
	//			IC_PC.data_from_DDC = IC_PC.port_buf[IC_PC.port];
	//			IC_PC.data_lock = true;
	//			IC_PC.port ^= 1;
	//
	//		}
	IC_PC.data_from_DDC = IC_PC.port_buf[IC_PC.port];
	//IC_PC.data_lock = true;
	IC_PC.port ^= 1;
	HAL_GPIO_TogglePin(LD1_GPIO_Port, LD1_Pin);
	IC_ReadADC_Complete();

#else
	for(int i = 0; i<IC_CH_NUM;i++)
		IC_PC.data_from_DDC[i] = IC_CH_NUM-i;
	IC_ReadADC_Complete();
#endif

}

inline void IC_ReadADC_Complete() {
	//	if(processing_is_completed == true && IC_Mode.noise_accum_mode != IC_NOISE_ACCUM_ON){
	//		return;
	//	}
	//	if(IC_PC.data_lock == true && IC_Mode.noise_accum_mode != IC_NOISE_ACCUM_ON){
	//		return;
	//	}

	//	if(IC_PC.port == 0){
	//		IC_PC.data_from_DDC[50] = 1111;
	//	}else{
	//		IC_PC.data_from_DDC[50] = 2222;
	//	}
	if (IC_Mode.mean_mode == IC_MEAN_ON) {
		IC_Mean_Calc();
		if (IC_Config.iter_num != 0)
			return;
	} else {
		if (IC_Mode.noise_delete_mode == IC_NOISE_DELETE_ON){
			IC_Normal_Calc();
		}
	}
	//HAL_GPIO_WritePin(DDC_ready_GPIO_Port, DDC_ready_Pin,SET);
	if (IC_Mode.noise_accum_mode == IC_NOISE_ACCUM_ON) {
		if (IC_Mode.noise_accum_first_entry == true) {
			//IC_Config.max_iter *= 50;
			//memset(IC_Data.noise_buf,0,sizeof(*IC_Data.noise_buf)*IC_CH_NUM);
			IC_Config.noise_max = 0;
			IC_Config.iter_num = 0;
			IC_Mode.noise_delete_mode_save = IC_Mode.noise_delete_mode;
			IC_Mode.noise_delete_mode = IC_NOISE_DELETE_OFF;
			IC_Mode.mean_mode_save = IC_Mode.mean_mode;
			IC_Mode.mean_mode = IC_MEAN_ON;
			IC_Mode.noise_accum_first_entry = false;
			//IC_Mode.program_trigger_mode_save = IC_Mode.program_trigger_mode;
			//IC_Mode.program_trigger_mode = IC_PROGRAM_TRIGGER_OFF;
			return;
		} else {
			IC_Noise_Accumulate();
			//IC_Config.max_iter /= 50;
			IC_Mode.mean_mode = IC_Mode.mean_mode_save;
			IC_Mode.noise_delete_mode = IC_Mode.noise_delete_mode_save;
			//IC_Mode.program_trigger_mode = IC_PROGRAM_TRIGGER_MODE_SAVE;
			IC_Mode.noise_accum_first_entry = true;
			if (IC_Mode.read_adc_mode == IC_READ_ADC_OFF) {
				IC_ReadADC_off();
			}
			if (IC_Mode.ext_trigger_mode == IC_EXT_TRIGGER_ON) {
				//IC_Mode.ext_trigger_mode_save = IC_Mode.ext_trigger_mode;
				IC_ReadADC_off();
				IC_Ext_trig_on();
				IC_Show_Info();
				return;
			}
			IC_Show_Info();
		}
	}
	IC_Center_Of_Gravity();

	if (IC_Mode.program_trigger_mode == IC_PROGRAM_TRIGGER_ON) {
		if (IC_Program_Trigger() == IC_PROGRAM_TRIGGER_NOT_FIRED)
			return;
	}
	IC_Processing_Timer(STOP);

	//HAL_NVIC_DisableIRQ(EXTI0_IRQn);
	if (IC_Mode.ext_trigger_mode == IC_EXT_TRIGGER_ON) {
		if (IC_Mode.ref_mode == IC_REF_MODE_OFF) {

			IC_ext.beam_cycle_count++;
			////
			if (IC_ext.beam_cycle_count < IC_EXT_TRIG_TRIM_CYCLE_START) {
				return;
			}
			if (IC_ext.beam_cycle_count
					< (IC_ext.cycle_in_beam - IC_EXT_TRIG_TRIM_CYCLE_END)) {
				for (uint8_t i = 0; i < IC_CH_NUM; i++) {
					IC_ext.mean[i] += IC_PC.data_from_DDC[i];
				}
				//printf("Cycle count %d\n",IC_ext.beam_cycle_count);
				return;
			}
			for (uint8_t i = 0; i < IC_CH_NUM; i++) {
				uint16_t avg = (uint16_t) (IC_ext.mean[i]
													   / (IC_ext.cycle_in_beam
															   - (IC_EXT_TRIG_TRIM_CYCLE_START
																	   + IC_EXT_TRIG_TRIM_CYCLE_END)));
				IC_PC.data_from_DDC[i] = avg;
				IC_ext.mean[i] = 0;
			}
		}
		IC_ReadADC_off();
		IC_Ext_trig_on();
	}
	if (USART_CMD_ON) {
		//Get_time();
		printf("[INFO] Data to PC\n");
		printf("[INFO] PORT1\n");
		for (int i = 0; i < IC_CH_NUM + CG_WORD + CHECK_WORD; i++) {
			if (i == (IC_CH_NUM / 2))
				printf("[INFO] PORT2\n");
			//printf("[%-2d] IC_PC.data_to_PC %d\n",i,IC_PC.data_to_PC[i]);
			//printf("[%-2d] IC_PC.data_to_PC %d\n",i,IC_PC.port_buf[IC_PC.port][i]);
			printf("[%-2d]%d\n", i, IC_PC.data_from_DDC[i]);
		}
	}

	if (IC_Mode.ref_mode == IC_REF_MODE_ON) {
		if (IC_ref.IC_REF_ALARM == true) {
			HAL_GPIO_TogglePin(LD1_GPIO_Port, LD1_Pin);
			return;
		}
		IC_ref.IC_REF_ALARM = IC_Ref_comparison();
	}
	if(processing_is_completed == false){
		memcpy(print_buf,IC_PC.data_from_DDC,(IC_CH_NUM + CG_WORD + CHECK_WORD)*2);
		processing_is_completed = true;
	}

	IC_ADC_To_PC();
	IC_PC.data_lock = false;
	//IC_PC.port ^= 1;

}

inline void IC_Center_Of_Gravity() {
	// Используем float для точного расчета

	float numerator = 0.0f;
	float denominator = 0.0f;

	IC_PC.data_from_DDC[32] = (IC_PC.data_from_DDC[33] + IC_PC.data_from_DDC[0]) / 2; //TODO:remove
	//	for(int i = 1; i <IC_CH_NUM;i++){
	//		IC_PC.data_from_DDC[ch_buf[i]]=0;
	//	}
	int i = 0;
	for (; i <= IC_CH_NUM - 8; i += 8) {
		// Загружаем веса
		float w0 = (float) IC_PC.data_from_DDC[ch_buf[i]];
		float w1 = (float) IC_PC.data_from_DDC[ch_buf[i + 1]];
		float w2 = (float) IC_PC.data_from_DDC[ch_buf[i + 2]];
		float w3 = (float) IC_PC.data_from_DDC[ch_buf[i + 3]];
		float w4 = (float) IC_PC.data_from_DDC[ch_buf[i + 4]];
		float w5 = (float) IC_PC.data_from_DDC[ch_buf[i + 5]];
		float w6 = (float) IC_PC.data_from_DDC[ch_buf[i + 6]];
		float w7 = (float) IC_PC.data_from_DDC[ch_buf[i + 7]];

		// Суммируем веса (denominator)
		float sum_w = w0 + w1 + w2 + w3 + w4 + w5 + w6 + w7;
		denominator += sum_w;

		// Суммируем взвешенные значения (numerator)
		//		numerator += (float) (ch_buf[i] + 1) * w0
		//				+ (float) (ch_buf[i + 1] + 1) * w1
		//				+ (float) (ch_buf[i + 2] + 1) * w2
		//				+ (float) (ch_buf[i + 3] + 1) * w3
		//				+ (float) (ch_buf[i + 4] + 1) * w4
		//				+ (float) (ch_buf[i + 5] + 1) * w5
		//				+ (float) (ch_buf[i + 6] + 1) * w6
		//				+ (float) (ch_buf[i + 7] + 1) * w7;
		//	}
		numerator += (float) (i + 1) * w0
				+ (float) (i + 1 + 1) * w1
				+ (float) (i + 2 + 1) * w2
				+ (float) (i + 3 + 1) * w3
				+ (float) (i + 4 + 1) * w4
				+ (float) (i + 5 + 1) * w5
				+ (float) (i + 6 + 1) * w6
				+ (float) (i + 7 + 1) * w7;
	}

	float result = (denominator > 0.0001f) ? (numerator / denominator) : (IC_CH_NUM * 0.5f);

	// Преобразуем float в битовое представление (uint32_t)

	float_union.f = result;
	uint32_t float_bits = float_union.u;

	// Разбиваем uint32_t на два uint16_t
	// Little-endian порядок:
	IC_PC.data_from_DDC[IC_CH_NUM] = (uint16_t) (float_bits & 0xFFFF); // Младшие 16 бит
	IC_PC.data_from_DDC[IC_CH_NUM + 1] =
			(uint16_t) ((float_bits >> 16) & 0xFFFF);  // Старшие 16 бит

	// Альтернативно, можно использовать:
	// uint32_t float_bits = *(uint32_t*)&result;
}

inline void IC_Mean_Calc() {
	for (int i = 0; i < IC_CH_NUM; i++) {
		if (IC_Config.iter_num != 0) {
			IC_Data.mean[i] += IC_PC.data_from_DDC[i];
			//IC_Data.mean[IC_CH_NUM-1-i] = IC_CH_NUM-i;
		} else {
			IC_Data.mean[i] = IC_PC.data_from_DDC[i];
			//IC_Data.mean[IC_CH_NUM-1-i] += IC_CH_NUM-i;
		}
	}

	if (IC_Config.iter_num < IC_Config.max_iter - 1) {
		IC_Config.iter_num++;
	} else {
		for (int i = 0; i < IC_CH_NUM; i++) {
			//IC_Data.mean[i] = IC_Data.mean[i]>>6;
			uint16_t avg = (uint16_t) (IC_Data.mean[i] / IC_Config.max_iter);
			//IC_Data.noise_buf[i] = 1000;
			IC_PC.data_from_DDC[i] = IC_Noise_Remove(avg, IC_Data.noise_buf[i]);
			//IC_PC.data_from_DDC[i] = 0;
		}
		IC_Config.iter_num = 0;
	}
}

void IC_RMS_Calc() {
	for (int i = 0; i < IC_CH_NUM; i++) {
		if (IC_Config.iter_num != 0) {
			IC_Data.mean[i] += pow(IC_PC.data_from_DDC[i], 2);
			//IC_Data.mean[IC_CH_NUM-1-i] = IC_CH_NUM-i;
		} else {
			IC_Data.mean[i] = pow(IC_PC.data_from_DDC[i], 2);
			//IC_Data.mean[IC_CH_NUM-1-i] += IC_CH_NUM-i;
		}
	}

	if (IC_Config.iter_num < IC_Config.max_iter - 1) {
		IC_Config.iter_num++;
	} else {
		//HAL_GPIO_WritePin(DDC_ready_GPIO_Port, DDC_ready_Pin,SET);
		//HAL_NVIC_DisableIRQ(EXTI0_IRQn);
		for (int i = 0; i < IC_CH_NUM; i++) {
			//IC_Data.mean[i] = IC_Data.mean[i]>>6;
			uint16_t rms = (uint16_t) sqrt(
					(IC_Data.mean[i] / IC_Config.max_iter));
			IC_PC.data_from_DDC[i] = IC_Noise_Remove(rms, IC_Data.noise_buf[i]);

			//IC_PC.port_buf[port][i] = (uint16_t)(IC_Data.mean[i]>>6);
			//IC_PC.data_to_PC[i] = (uint16_t)(IC_Data.mean[i]>>6);
		}
		IC_Config.iter_num = 0;
	}
}

inline void IC_Normal_Calc() {
	for (int i = 0; i < IC_CH_NUM; i++) {
		IC_PC.data_from_DDC[i] = IC_Noise_Remove(IC_PC.data_from_DDC[i],
				IC_Data.noise_buf[i]);
	}
}

void IC_Noise_Accumulate() {
	for (int i = 0; i < IC_CH_NUM; i++) {
		IC_Data.noise_buf[i] = IC_PC.data_from_DDC[i];
		if (IC_Config.noise_max < IC_Data.noise_buf[i])
			IC_Config.noise_max = IC_Data.noise_buf[i];
	}

	//memcpy(IC_Data.noise_buf,IC_PC.port_buf[port],sizeof(*IC_Data.noise_buf)*IC_CH_NUM);
	IC_Mode.noise_accum_mode = IC_NOISE_ACCUM_OFF;
}

inline uint16_t IC_Noise_Remove(uint16_t data, uint16_t noise) {
	if (IC_Mode.noise_delete_mode != IC_NOISE_DELETE_ON)
		return data;
	if (data > noise)
		return data - noise;
	else
		return 0;
}

inline uint8_t IC_Program_Trigger() {
	//double percent = IC_Config.noise_max+IC_Config.noise_max*IC_Config.trigger_percent;
	uint8_t proper_result = 0;
	for (int i = 0; i < IC_CH_NUM; i++) {
		//if(IC_PC.port_buf[port][i] >= percent){
		if (IC_PC.data_from_DDC[i] >= IC_Config.trigger) {
			proper_result++;
			if (proper_result >= IC_PROGRAMM_TRIGGER_LVL)
				return IC_PROGRAM_TRIGGER_FIRED;
		} else
			proper_result = 0;
	}

	return IC_PROGRAM_TRIGGER_NOT_FIRED;
}

inline void IC_Ext_Trig() {
	IC_ext.beam_cycle_count = 0;
	//IC_Mode.ext_trigger_mode = IC_EXT_TRIGGER_ON;
	IC_Ext_trig_off();
	IC_ReadADC_on();
	//HAL_NVIC_EnableIRQ(IC_nDVALID_IRQ);//DVALID On
	//HAL_NVIC_DisableIRQ(EXTI1_IRQn);//EXT TRIG OFF
	//HAL_TIM_PWM_Start(&conv_timer, conv_channel); //start CONV
	//IC_Reset();
}

inline bool IC_Ref_comparison() {
	for (uint8_t i = 0; i < IC_CH_NUM; i++) {
		uint16_t dif = abs(IC_PC.data_from_DDC[i] - IC_ref.ref_data[i]);
		if (IC_ref.ref_data[i] >= 1000 && dif > IC_ref.delta[i]) {
			//if(dif > IC_ref.delta[i] ){
			HAL_GPIO_WritePin(FAULT_Port, FAULT_Pin,
					GPIO_PIN_SET);
			///
			//			IC_PC.data_to_PC = IC_PC.data_from_DDC;
			///
			return true;
		}
	}
	return false;
}

void IC_Ext_trig_on() {
	//__HAL_GPIO_EXTI_CLEAR_IT(IC_EXT_TRIG_Pin);
	HAL_NVIC_EnableIRQ(EXTI1_IRQn);
}

void IC_Ext_trig_off() {
	HAL_NVIC_DisableIRQ(EXTI1_IRQn);
	//__HAL_GPIO_EXTI_CLEAR_IT(IC_EXT_TRIG_Pin);
}

void IC_ReadADC_on() {
	__HAL_GPIO_EXTI_CLEAR_IT(IC_nDVALID_Pin);
	HAL_NVIC_EnableIRQ(IC_nDVALID_IRQ);			//DVALID On
}

void IC_ReadADC_off() {
	HAL_NVIC_DisableIRQ(IC_nDVALID_IRQ);			//DVALID Off
	__HAL_GPIO_EXTI_CLEAR_IT(IC_nDVALID_Pin);
}

inline void IC_ADC_To_PC() {
    if (send_uart == true) {
        // Размер: 128 каналов (2 байта) + центр тяжести (4 байта) + проверочное слово (2 байта) = 262 байта
        const uint32_t BINARY_DATA_SIZE = (IC_CH_NUM * sizeof(uint16_t)) + sizeof(float) + sizeof(uint16_t);

        // Буфер как массив байт, а не uint16_t!
        uint8_t binary_buffer[BINARY_DATA_SIZE];
        int offset = 0;

        // 1. 128 каналов (uint16) - отправляем как есть, 2 байта на канал
        for (int i = 0; i < IC_CH_NUM; i++) {
            uint16_t value = IC_PC.data_from_DDC[ch_buf[i]];
            // Копируем 2 байта (little-endian)
            binary_buffer[offset++] = (uint8_t)(value & 0xFF);        // младший байт
            binary_buffer[offset++] = (uint8_t)((value >> 8) & 0xFF); // старший байт
        }

        // 2. Центр тяжести (float 4 байта)
        uint32_t cg_uint = float_union.u;  // берем битовое представление float
        binary_buffer[offset++] = (uint8_t)(cg_uint & 0xFF);
        binary_buffer[offset++] = (uint8_t)((cg_uint >> 8) & 0xFF);
        binary_buffer[offset++] = (uint8_t)((cg_uint >> 16) & 0xFF);
        binary_buffer[offset++] = (uint8_t)((cg_uint >> 24) & 0xFF);

        // 3. Проверочное слово (uint16)
        uint16_t check_word = IC_PC.data_from_DDC[IC_CH_NUM + CG_WORD + CHECK_WORD - 1];
        binary_buffer[offset++] = (uint8_t)(check_word & 0xFF);
        binary_buffer[offset++] = (uint8_t)((check_word >> 8) & 0xFF);

        // Отладка: выводим первые несколько значений
//        printf("[INFO] First channel: %u\n", IC_PC.data_from_DDC[ch_buf[0]]);
//        printf("[INFO] CG float: %f, raw: 0x%08X\n", float_union.f, float_union.u);
//        printf("[INFO] Check word: 0x%04X\n", check_word);

        // Отправка
        USART_enable_tx();
        HAL_UART_Transmit(&out_huart, binary_buffer, BINARY_DATA_SIZE, HAL_MAX_DELAY);
        USART_enable_rx();

        processing_is_completed = false;
        send_uart = false;
    }
}

inline void IC_Processing_Timer(IC_processing_timer_t t){
	if (t == START) {
		IC_start_processing = DWT->CYCCNT;
	} else {
		//		IC_ext.one_cycle_time = DWT->CYCCNT / TO_US;
		//		IC_end_processing = (IC_ext.one_cycle_time - ((IC_start_processing+DWT->CYCCNT)/(SystemCoreClock / 1000000U)));
		//		IC_ext.one_cycle_time = DWT->CYCCNT / TO_US;
		IC_end_processing = abs( DWT->CYCCNT - IC_start_processing) / TO_US;
		IC_ext.one_cycle_time = DWT->CYCCNT / TO_US;
		DWT->CYCCNT = 0U;
	}

}

void IC_Show_Info() {
	uint16_t r1 = IC_Config.cfg_in >> 10 & 0x1;
	uint16_t r0 = IC_Config.cfg_in >> 9 & 0x1;
	char *str = "empty";
	printf("[INFO]Firmware v.%s\n", IC_FIRMWARE_VERSION);
	printf("[INFO]//////////SETTINGS DATA//////////\n");
	//Get_time();
	if (IC_Mode.read_adc_mode == IC_READ_ADC_ON)
		str = "running";
	else
		str = "stopped";
	printf("[INFO] ADC is %s\n", str);
	printf("[INFO] CFG register(hex): %04x\n", IC_Config.cfg_in);
	printf("[INFO] CLKDIV %16u\n", IC_Config.cfg_in >> 13 & 0x1);
	printf("[INFO] RANGE[1] %14u\n", r1);
	printf("[INFO] RANGE[0] %14u\n", r0);
	printf("[INFO] FORMAT %16u\n", IC_Config.cfg_in >> 8 & 0x1);
	printf("[INFO] VERSION %15u\n", IC_Config.cfg_in >> 7 & 0x1);
	printf("[INFO] TEST %18u\n\n", IC_Config.cfg_in >> 2 & 0x1);
	printf("[INFO] Integration (CONV) time: %lu us:\n", IC_Config.period / 2);
	if (r1 == 0 && r0 == 0) {
		str = "12.5";
	} else if (r1 == 0 && r0 == 1) {
		str = "50";
	} else if (r1 == 1 && r0 == 0) {
		str = "100";
	} else if (r1 == 1 && r0 == 1) {
		str = "150";
	}
	printf("[INFO] Cap range: %s pC\n\n", str);
	printf("[INFO] Check data from DDC264\n");
	//char* msg;
	if (IC_Config.cfg_out[28] == 0x0030 && IC_Config.cfg_out[92] == 0x0030
			&& IC_Config.cfg_out[29] == 0xf066
			&& IC_Config.cfg_out[93] == 0xf066
			&& IC_Config.cfg_out[30] == 0x0124
			&& IC_Config.cfg_out[94] == 0x0124
			&& IC_Config.cfg_out[31] == 0x80f6
			&& IC_Config.cfg_out[95] == 0x80f6)
		str = "CORRECT";
	else
		str = "WRONG";
	printf("[INFO] CHECK SUM [%s] %x%x%x%x\n\n", str, IC_Config.cfg_out[28],
			IC_Config.cfg_out[29], IC_Config.cfg_out[30],
			IC_Config.cfg_out[31]);
	//char* mean_mode;
	if (IC_Mode.mean_mode == IC_MEAN_OFF)
		str = "OFF";
	else
		str = "ON";
	printf("[INFO] Mean value mode is %s\n", str);
	printf("[INFO] Number of iterations for mean value: %lu\n\n",
			IC_Config.max_iter);
	//char* value_order;
	if (IC_Mode.change_mode == IC_IN_ROW)
		str = "IN ROW";
	else
		str = "AS CABLE";
	printf("[INFO] Value order: %s\n\n", str);
	//char* noise;
	if (IC_Mode.noise_delete_mode == IC_NOISE_DELETE_ON)
		str = "ON";
	else
		str = "OFF";
	printf("[INFO] Noise calibration mode is %s\n\n", str);

	if (IC_Mode.program_trigger_mode == IC_PROGRAM_TRIGGER_ON)
		str = "ON";
	else
		str = "OFF";
	printf("[INFO] Program trigger mode is %s\n", str);
	//printf("[INFO] Trigger fires above noise of %d is %2.f%%(%3.1f)\n",IC_Config.noise_max, IC_Config.trigger_percent*100,(IC_Config.noise_max+IC_Config.noise_max*IC_Config.trigger_percent));
	printf("[INFO] Trigger fires above %u\n", IC_Config.trigger);
	printf("[INFO] Max noise is %u\n", IC_Config.noise_max);
	printf("[INFO] Number of trigger channels is %d\n\n",
			IC_PROGRAMM_TRIGGER_LVL);
	if (IC_Mode.ext_trigger_mode == IC_EXT_TRIGGER_ON)
		str = "ON";
	else
		str = "OFF";
	printf("[INFO] External trigger mode is %s\n", str);
	printf("[INFO] Beam time width is %lu ms\n", IC_ext.beam_width);
	printf("[INFO] Time width of cycle %.2f ms (%lu us)\n",
			IC_ext.one_cycle_time / 1000., IC_ext.one_cycle_time);
	printf("[INFO] Processing time %lu us\n", IC_end_processing);
	printf("[INFO] Cycles in one beam %lu\n\n", IC_ext.cycle_in_beam);

	if (IC_Mode.ref_mode == IC_REF_MODE_ON)
		str = "ON";
	else
		str = "OFF";
	printf("[INFO] Reference mode is %s\n", str);
	printf("[INFO] Compliance percentage %u\n", 100 - IC_ref.percent);
	//printf("[INFO] Tolerance is %d\n\n",IC_ref.delta);
	printf("[INFO]//////////SETTINGS DATA//////////\n\n");
	//for(int i = 0; i<IC_IC_config.cfg_out_SIZE;i++)
	//printf("[%-2u] IC_IC_config.cfg_out %x\n",i,IC_IC_config.cfg_out[i]);
}

//USART handling
void string_parse(char *buf_str) {

	//__disable_irq();
	//IC_Read_cmd_Complete();
	if (strncmp(buf_str, "on:", 3) == 0) {
		USART_CMD_ON = 1;
	} else if (strncmp(buf_str, "off:", 4) == 0) {
		USART_CMD_ON = 0;
	} else if (strncmp(buf_str, "send:", 5) == 0) {
		IC_ReadADC_on();
		for (int i = 0; i < IC_CH_NUM + CHECK_WORD; i++) {
			printf("[%i]%d\n", i, IC_PC.data_to_PC[i]);
		}
	} else if (strncmp(buf_str, "r:", 2) == 0) {
		//printf("[ECHO] %s\n",buf_str);
		IC_Reset();
	} else if (strncmp(buf_str, "c:", 2) == 0) {
		char ret[2];
		atoh(buf_str + 2, ret, 4);
		IC_Config.cfg_in = ret[0] << 8 | ret[1];
	} else if (strncmp(buf_str, "k:", 2) == 0) {
		char ret[8];
		atoh(buf_str + 2, ret, 16);
		IC_cmd[0] = ret[0] << 8 | ret[1];
		IC_cmd[1] = ret[2] << 8 | ret[3];
		IC_cmd[2] = ret[4] << 8 | ret[5];
		//IC_cmd[3] = ret[6]<<8|ret[7];
		//IC_Read_cmd_Complete();
	}
	/*else if(strncmp(buf_str,"t:",2)==0){
	 Get_time();
	 }
	 */
	else if (strncmp(buf_str, "m:", 2) == 0) {
		if (buf_str[2] == '1')
			//printf("Hello");
			IC_Mode.change_mode = IC_AS_CABLE;
		else
			IC_Mode.change_mode = IC_IN_ROW;
	} else if (strncmp(buf_str, "me:", 3) == 0) {
		if (buf_str[3] == '1') {
			//printf("Hello");
			IC_Mode.mean_mode = IC_MEAN_ON;
			printf("[INFO] Mean mode is ON\n");
		} else {
			IC_Mode.mean_mode = IC_MEAN_OFF;
			printf("[INFO] Mean mode is OFF\n");
		}
	} else if (strncmp(buf_str, "it:", 3) == 0) {
		uint32_t ret;
		ret = strtol(buf_str + 3, NULL, 10);
		printf("[INFO] ret %ld\n", ret);
		IC_Config.max_iter = (uint32_t) ret;
	} else if (strncmp(buf_str, "i:", 2) == 0) {
		IC_Show_Info();
	} else if (strncmp(buf_str, "n:", 2) == 0) {
		if (buf_str[2] == '1') {
			//IC_Mode.mean_mode_save = IC_Mode.mean_mode;
			//IC_Mode.mean_mode = IC_MEAN_ON;
			IC_Mode.noise_accum_mode = IC_NOISE_ACCUM_ON;
			if (IC_Mode.ext_trigger_mode == IC_EXT_TRIGGER_ON) {
				//IC_Mode.ext_trigger_mode_save = IC_Mode.ext_trigger_mode;
				IC_Ext_trig_off();
				IC_ReadADC_on();
			}
			//IC_Config.iter_num = 0;
			//IC_Config.max_iter *= 50;

			//memset(IC_Data.noise_buf,0,sizeof(*IC_Data.noise_buf)*IC_CH_NUM);
			//IC_Config.noise_max = 0;
			//IC_Config.iter_num = 0;
			//IC_Reset();
			//__enable_irq();
		}

	} else if (strncmp(buf_str, "d:", 2) == 0) {
		if (buf_str[2] == '1') {
			IC_Mode.noise_delete_mode = IC_NOISE_DELETE_ON;
		} else {
			//memset(IC_Data.noise_buf,0,sizeof(*IC_Data.noise_buf)*IC_CH_NUM);
			//IC_Config.noise_max = 0;
			IC_Mode.noise_delete_mode = IC_NOISE_DELETE_OFF;
		}
	} else if (strncmp(buf_str, "s:", 2) == 0) {
		if (buf_str[2] == '1')
			IC_ADC_To_PC();
	} else if (strncmp(buf_str, "p:", 2) == 0) {
		if (buf_str[2] == '1') {
			IC_Config.iter_num = 0;
			//IC_Reset();
			uint16_t p = 0;
			p = atoi(buf_str + 3);
			//if(p <= 100)
			//IC_Config.trigger_percent = p/100.;
			IC_Config.trigger = p;
			//printf("%d,%f\n",p,IC_Config.trigger_percent);
			printf("%d\n", p);
			IC_Mode.program_trigger_mode = IC_PROGRAM_TRIGGER_ON;
		} else {
			IC_Mode.program_trigger_mode = IC_PROGRAM_TRIGGER_OFF;
		}
	} else if (strncmp(buf_str, "ch:", 3) == 0) {
		uint16_t p = 0;
		p = atoi(buf_str + 3);
		//printf("%d\n",p);
		printf("%d\n", IC_PC.data_to_PC[p - 1]);
	} else if (strncmp(buf_str, "e:", 2) == 0) {
		if (buf_str[2] == '1') {
			uint32_t cycle_time = 0;
			//HAL_TIM_PWM_Stop(&conv_timer, conv_channel); //stop CONV
			//HAL_NVIC_DisableIRQ(IC_nDVALID_IRQ);//DVALID Off
			IC_ext.beam_width = atoi(buf_str + 3); //time width of the beam in ms
			if (IC_Mode.mean_mode == IC_MEAN_ON)
				cycle_time = IC_Config.max_iter * IC_Config.period / 2;
			else
				cycle_time = IC_Config.period / 2;
			//cycle_in_beam = (uint16_t)(beam_width/one_cycle_time);
			//printf("width %ld\n",w);
			if (IC_ext.beam_width * 1000 < cycle_time) {
				IC_ext.beam_width = IC_DEFAULT_BEAM_WIDTH / 1000;
			}
			IC_ext.cycle_in_beam = (IC_ext.beam_width * 1000) / cycle_time;
			uint32_t sum = IC_EXT_TRIG_TRIM_CYCLE_START
					+ IC_EXT_TRIG_TRIM_CYCLE_END;
			if (IC_ext.cycle_in_beam <= sum)
				IC_ext.cycle_in_beam = sum + 1;
			//printf("cycle %d\n",cycle_in_beam);
			//IC_Mode.program_trigger_mode == IC_PROGRAM_TRIGGER_OFF;
			//HAL_TIM_PWM_Start(&conv_timer, TIM_CHANNEL_4); //start CONV
			IC_Mode.ext_trigger_mode = IC_EXT_TRIGGER_ON;
			IC_ReadADC_off();
			IC_Ext_trig_on();
			//HAL_NVIC_EnableIRQ(EXTI1_IRQn); //Enable external interruption
		} else {
			IC_Mode.ext_trigger_mode = IC_EXT_TRIGGER_OFF;
			IC_Ext_trig_off();
			IC_ReadADC_on();
		}
		IC_Show_Info();
		//IC_Config.iter_num = 0;
		__enable_irq();

	} else if (strncmp(buf_str, "ref:", 4) == 0) {
		if (buf_str[4] == '1') {
			IC_Mode.ref_mode = IC_REF_MODE_ON;
			HAL_GPIO_WritePin(FAULT_Port, FAULT_Pin,
					GPIO_PIN_RESET);
			IC_ref.IC_REF_ALARM = false;
			for (uint8_t i = 1; i < IC_CH_NUM + 1; i++) {
				IC_cmd[i] = 0;
				memcpy(IC_ref.ref_data, IC_cmd + 1, sizeof(IC_ref.ref_data));
			}
			for (uint8_t i = 0; i < IC_CH_NUM; i++) {
				IC_ref.delta[i] = IC_ref.ref_data[i] / 10;
			}
		} else {
			IC_Mode.ref_mode = IC_REF_MODE_OFF;
		}
	} else if (strncmp(buf_str, "read:", 5) == 0) {
		if (buf_str[5] == '1') {
			if (IC_Mode.ext_trigger_mode == IC_EXT_TRIGGER_ON) {
				goto exit;
			}
			IC_Mode.read_adc_mode = IC_READ_ADC_ON;
			IC_ReadADC_on();
		} else {
			IC_Mode.read_adc_mode = IC_READ_ADC_OFF;
			IC_ReadADC_off();
		}
	} else {
		printf("[ECHO] %s", buf_str);
	}
	exit: IC_Config.iter_num = 0;
	//__enable_irq();
}

void IC_Read_cmd_UART(char *buf_str) {
	//__disable_irq();
	uint8_t data = 0;
	uint16_t IC_Cmd[129];
	for (int i = 0; i < 10; i++) {
		IC_Cmd[i] = 0;
	}
	IC_Cmd[0] = buf_str[0] | buf_str[1] << 8;
	IC_Cmd[1] = buf_str[2] << 8 | buf_str[3];
	IC_Cmd[2] = buf_str[4] << 8 | buf_str[5];
	IC_Cmd[3] = buf_str[7] << 8 | buf_str[6];
	IC_Cmd[4] = buf_str[9] << 8 | buf_str[8];
	//	for(int i = 0; i<10;i++){
	//						printf("[INFO] IC_cmd %x\n",IC_Cmd[i]);
	//					}
	//					printf("\n");
	switch ((uint8_t) IC_Cmd[0]) {
	case IC_CFG_IN:
		//__disable_irq();
		data = IC_Cmd[0] >> 8;
		//CONFIGURE DDC264 register
		//uint16_t mask = 0b0000000010000000;//protect VERSION BIT
		uint16_t mask = 0 | (VERSION << CFG_VERSION_POS);//protect VERSION BIT
		IC_Config.cfg_in = mask | (data & 0x4) << 8 | (data & 0x2) << 8
				| (data & 0x1) << 2;
		//CONFIGURE integration period time. CONV is period/2
		IC_Config.period = IC_Cmd[1] << 16 | IC_Cmd[2];
		if (IC_Config.period < IC_MIN_PERIOD_TIME) {
			IC_Config.period = IC_DEFAULT_PERIOD_TIME;
			//IC_Reset();
		}
		//CONFIGURE mean value mode
		//Toggle IC_MEAN mode
		if (((data >> 3) & 0x1) == 1) {
//			  HAL_TIM_PWM_Stop(&clk_timer, clk_channel); //start clk
//			  HAL_Delay(1000);
//			  HAL_TIM_PWM_Start(&clk_timer, clk_channel); //start clk
			NVIC_SystemReset();


			//IC_Mode.mean_mode = IC_MEAN_ON; //Mean ON
		} else {
			IC_Mode.mean_mode = IC_MEAN_OFF; //Mean OFF
		}
		if (IC_Cmd[3] != 0) {
			IC_Config.max_iter = (uint32_t) IC_Cmd[3];
		}
		if (((data >> 4) & 0x1) == 1) {
			IC_Mode.noise_delete_mode = IC_NOISE_DELETE_ON;
		} else {
			IC_Mode.noise_delete_mode = IC_NOISE_DELETE_OFF;
		}
		//Program trigger
		if (((data >> 5) & 0x1) == 1) {
			uint16_t p = (IC_Cmd[4]);
			IC_Mode.program_trigger_mode = IC_PROGRAM_TRIGGER_ON;
			//if(p<=100)
			//IC_Config.trigger_percent = p/100.;
			IC_Config.trigger = p;
		} else {
			IC_Mode.program_trigger_mode = IC_PROGRAM_TRIGGER_OFF;
		}
		IC_Reset();
		//External trigger
		if (((data >> 6) & 0x1) == 1) {
			uint32_t cycle_time = 0;
			IC_ext.beam_width = IC_Cmd[4]; //time width of the beam in ms
			if (IC_Mode.mean_mode == IC_MEAN_ON)
				cycle_time = IC_Config.max_iter * IC_Config.period / 2;
			else
				cycle_time = IC_Config.period / 2;
			if (IC_ext.beam_width * 1000 < cycle_time) {
				IC_ext.beam_width = IC_DEFAULT_BEAM_WIDTH / 1000;
			}
			IC_ext.cycle_in_beam = (IC_ext.beam_width * 1000) / cycle_time;
			uint32_t sum = IC_EXT_TRIG_TRIM_CYCLE_START
					+ IC_EXT_TRIG_TRIM_CYCLE_END;
			if (IC_ext.cycle_in_beam <= sum)
				IC_ext.cycle_in_beam = sum + 1;
			IC_Mode.ext_trigger_mode = IC_EXT_TRIGGER_ON;
			IC_ReadADC_off();
			IC_Ext_trig_on();
		} else {
			IC_Mode.ext_trigger_mode = IC_EXT_TRIGGER_OFF;
			IC_Ext_trig_off();
			IC_ReadADC_on();
		}
		//Reference mode percent
		if (((data >> 7) & 0x1) == 1) {
			uint16_t temp = 100 - IC_Cmd[5];
			if (temp > 0 && temp <= 100) {
				IC_ref.percent = temp;
			}
		}
		IC_Show_Info();// TODO:
		//IC_Config.iter_num = 0;
		//__enable_irq();
		break;
	case IC_ACCUM_NOISE:
		//__disable_irq();
		IC_Mode.noise_accum_mode = IC_NOISE_ACCUM_ON;
		if (IC_Mode.ext_trigger_mode == IC_EXT_TRIGGER_ON) {
			IC_Ext_trig_off();
		}
		IC_ReadADC_on();
		IC_Show_Info();// TODO:
		//__enable_irq();
		break;
	case IC_REF:
		//__disable_irq();
		data = IC_Cmd[0] >> 8;
		if ((data & 0x1) == 1) {
			for (uint8_t i = 0; i < IC_CH_NUM; i++) {
				IC_ref.ref_data[i] = IC_Cmd[i + 1];
				IC_ref.delta[i] = (uint16_t) (IC_ref.ref_data[i] / 100.
						* IC_ref.percent);
				printf("[%d]Ref data: %u percent %u\n", i, IC_ref.ref_data[i],
						IC_ref.delta[i]);
			}
		}
		if (((data >> 1) & 0x1) == 1) {
			IC_Mode.ref_mode = IC_REF_MODE_ON;
			HAL_GPIO_WritePin(FAULT_Port, FAULT_Pin,
					GPIO_PIN_RESET);
			IC_ref.IC_REF_ALARM = false;
		} else {
			IC_Mode.ref_mode = IC_REF_MODE_OFF;
			HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(FAULT_Port, FAULT_Pin,
					GPIO_PIN_RESET);
		}
		IC_Show_Info();
		//__enable_irq();
		break;
	case IC_READ_ADC:
		//__disable_irq();
		if ((data & 0x1) == 1) {
			if (IC_Mode.ext_trigger_mode == IC_EXT_TRIGGER_ON) {
				return;
			}
			IC_Mode.read_adc_mode = IC_READ_ADC_ON;
			IC_ReadADC_on();
		} else {
			IC_Mode.read_adc_mode = IC_READ_ADC_OFF;
			IC_ReadADC_off();
		}
		__enable_irq();
		break;
	case IC_SEND_DATA:
		__disable_irq();
		//char dma_buffer[512]; // Размер подберите под ваши нужды
		//int offset = 0;
		send_uart  = true;
		//IC_ADC_To_PC();
		//		for (int i = 0; i < IC_CH_NUM + CG_WORD + CHECK_WORD; i++) {
		//			if (i < IC_CH_NUM) {
		//				printf("[%i]%d\n", i, print_buf[ch_buf[i]]);
		//				//offset += snprintf(&dma_buffer[offset], sizeof(dma_buffer) - offset,
		//				                     //     "[%i]%d\n", i, print_buf[ch_buf[i]]);
		//
		//			} else {
		//				printf("[%i]%d\n", i, print_buf[i]);
		//				//offset += snprintf(&dma_buffer[offset], sizeof(dma_buffer) - offset,
		//				                         // "[%i]%d\n", i, print_buf[i]);
		//			}
		//		}
		//printf(dma_buffer);
		//cpy_lock = false;
		//printf("%hn\n",IC_PC.data_to_PC);
		//IC_ADC_To_PC();
		//__enable_irq();

	}
	__enable_irq();
}

