/*
 * profilometer.c
 *
 *  Created on: Jun 9, 2026
 *      Author: User
 */

#include "profilometer.h"

uint32_t millisec = 0;

uint32_t spots_of_layer[100];
uint8_t sended_spots = 0;

uint8_t tx_buff[402];
uint8_t rx_buff;

const command_t list_of_prcommands[7] = {
		{PROF_CONFIG, 13},
		{PROF_ACCUM_NOISE, 13},
		{PROF_NOISE_CANCEL, 13},
		{PROF_COM_LAYER, 401},
		{PROF_WORK_MODE, 13},
		{PROF_SETTINGS_MODE, 13},
		{PROF_SYNC_MODE, 13}
};

byte_protocol_t receive_prof = {
		.list_of_commands = list_of_prcommands,
		.tx_buff = tx_buff,
		.rx_buff = &rx_buff,
		.size_oloc = (uint8_t) sizeof(list_of_prcommands)
};

rs485_t rs_prof = {
		.gpio_pin = PROF_RE_DE_Pin,
		.gpio_port = PROF_RE_DE_Port,
		.sel_uart = &prof_uart
};

bool latch_flag = false;

PROF_MODE prof_mode = SETTINGS_MODE;
PWORK_MODE work_mode = WSTART;
PSETTINGS_MODE settings_mode = SSTART;

void prof_init()
{
	HAL_GPIO_WritePin(UART4_RE_DE_GPIO_Port, UART4_RE_DE_Pin, GPIO_PIN_RESET);
	HAL_UART_Receive_IT(rs_prof.sel_uart, receive_prof.rx_buff, 1);
}

void prof_parse_data(uint8_t* data)
{
	switch(data[0])
	{
		case PROF_CONFIG:
			break;
		case PROF_ACCUM_NOISE:
			break;
		case PROF_NOISE_CANCEL:
			break;
		case PROF_COM_LAYER:
			for(int i = 1; i < 101; ++i)
			{
				spots_of_layer[i] = data[i*4 - 3] << 24;
				spots_of_layer[i] = data[i*4 - 2] << 16;
				spots_of_layer[i] = data[i*4 - 1] << 8;
				spots_of_layer[i] = data[i*4 - 0] << 0;
			}

			settings_mode = SSPOTS_LOADED;

			break;
		case PROF_WORK_MODE:
			if(settings_mode == SSPOTS_LOADED)
			{
				prof_mode = WORK_MODE;
				work_mode = WWAITING_SS;
			}

			break;
		case PROF_SETTINGS_MODE:
			prof_mode = SETTINGS_MODE;

			break;
		case PROF_SYNC_MODE:
			prof_mode = SYNC_MODE;

			break;
	}
}

void prof_main_logic()
{
	switch(prof_mode)
	{
		case WORK_MODE:
			prof_logic();

			break;
		case SETTINGS_MODE:
			if(receive_prof.tx_flag == true)
			{
				prof_parse_data(tx_buff);
				receive_prof.tx_flag = false;
			}

			prof_settings_logic();

			break;
		case SYNC_MODE:
			if(receive_prof.tx_flag == true)
			{
				prof_parse_data(tx_buff);
				receive_prof.tx_flag = false;
			}

			prof_sync_logic();

			break;
	}
}

void prof_logic()
{
	static bool send_once = false;

	if(HAL_GPIO_ReadPin(LS_SS_PIU_GPIO_Port, LS_SS_PIU_Pin) == GPIO_PIN_SET && work_mode == WWAITING_SS && prof_mode == WORK_MODE)
	{
		work_mode = WSEND_DATA;
	}

	if(work_mode == WSEND_DATA && HAL_GPIO_ReadPin(LS_MM_PIU_GPIO_Port, LS_MM_PIU_Pin) == GPIO_PIN_SET)
	{
		if(send_once == false)
		{
			send_once = true;

			uint8_t buff[4];

			buff[0] = (spots_of_layer[sended_spots] & 0x000000FF) >> 0;
			buff[1] = (spots_of_layer[sended_spots] & 0x0000FF00) >> 8;
			buff[2] = (spots_of_layer[sended_spots] & 0x00FF0000) >> 16;
			buff[3] = (spots_of_layer[sended_spots] & 0xFF000000) >> 24;

			send_data(buff, 4, rs_prof, .crc = false);

			++sended_spots;
		}
	}

	if(work_mode == WSEND_DATA && HAL_GPIO_ReadPin(LS_MM_PIU_GPIO_Port, LS_MM_PIU_Pin) == GPIO_PIN_RESET && send_once == true)
	{
		send_once = false;
	}

	if(spots_of_layer[sended_spots] == 0)
	{
		work_mode = WSTART;
		settings_mode = SSTART;
		prof_mode = SETTINGS_MODE;

		sended_spots = 0;
	}
}

void prof_settings_logic()
{
	if(HAL_GPIO_ReadPin(LS_SS_PIU_GPIO_Port, LS_SS_PIU_Pin) == GPIO_PIN_SET)
	{
		if(!latch_flag)
		{
			latch_flag = true;

			prof_send_mns();
		}
	}
	else
	{
		latch_flag = false;
	}
}

void prof_sync_logic()
{
	if(HAL_GPIO_ReadPin(LS_SS_PIU_GPIO_Port, LS_SS_PIU_Pin) == GPIO_PIN_SET)
	{
		if(!latch_flag)
		{
			latch_flag = true;
		}
	}
	else
	{
		if(latch_flag)
		{
			latch_flag = false;

			prof_send_mns();
		}
	}
}

void prof_send_mns() // profilometer send center of beam and strips
{
	uint8_t buff[261];
	uint16_t strip_values[128];
	uint32_t center_of_beam = 0;


	rand_list(strip_values, 128);

	for(int i = 0; i < 128; ++i)
	{
		center_of_beam += strip_values[i];
	}

	center_of_beam = center_of_beam*100/128;
	center_of_beam = 0x64000064;

	for(int i = 0; i < 128; ++i)
	{
		buff[(i + 1)*2 - 2] = strip_values[i];
		buff[(i + 1)*2 - 1] = strip_values[i];
	}

	buff[259] = (center_of_beam & 0xFF000000) >> 24;
	buff[258] = (center_of_beam & 0x00FF0000) >> 16;
	buff[257] = (center_of_beam & 0x0000FF00) >> 8;
	buff[256] = (center_of_beam & 0x000000FF) >> 0;

	send_data(buff, 261, rs_prof);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart)
{
	receive_logic(huart, rs_prof, &receive_prof);
}

void rand_list(uint16_t* some_list, uint8_t size)
{
	srand(HAL_GetTick());

	for(int i = 0; i < size; ++i)
	{
		some_list[i] = rand() % 65535;
	}
}
