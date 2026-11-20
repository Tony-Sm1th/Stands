/*
 * dosimeter.c
 *
 *  Created on: Jun 9, 2026
 *      Author: User
 */

#include "dosimeter.h"

uint32_t spots_of_layer[100];
uint8_t sended_spots = 0;

uint16_t rx_count = 0;
uint8_t rx_buff;
uint16_t index_of_buff = 0;
uint8_t tx_buff[402];
uint16_t tx_size = 0;

const command_t list_of_dcommands[4] = {
		{LAYER_SPOTS, 401},
		{DOS_WORK_MODE, 2},
		{DOS_SYNC_MODE, 2},
		{DOS_SETTINGS_MODE, 2}
};

rs485_t rs_dos = {
	.gpio_port = DOS_RE_DE_Port,
	.gpio_pin = DOS_RE_DE_Pin,
	.sel_uart = &dos_uart
};

byte_protocol_t receive_dos = {
	.list_of_commands = list_of_dcommands,
	.tx_buff = tx_buff,
	.rx_buff = &rx_buff,
	.size_oloc = sizeof(receive_dos.list_of_commands),
	.tx_flag = false
};

bool first_byte_flag = true;
bool tx_flag = false;
bool latch_flag = false;

DOSIMETER_MODE dosimeter_mode = SETTINGS_MODE;
DWORK_MODE work_mode = WSTART;

void dos_init()
{
	HAL_GPIO_WritePin(UART4_RE_DE_GPIO_Port, UART4_RE_DE_Pin, GPIO_PIN_RESET);

	HAL_UART_Receive_IT(&dos_uart, &rx_buff, 1);
}

void dos_main_logic()
{
	switch(dosimeter_mode)
	{
		case SETTINGS_MODE:
			if(receive_dos.tx_flag == true)
			{
				dos_parse_data(tx_buff);
				receive_dos.tx_flag = false;
			}

			dos_settings_logic();

			break;
		case WORK_MODE:
			dos_logic();

			break;
		case SYNC_MODE:
			if(receive_dos.tx_flag == true)
			{
				dos_parse_data(tx_buff);
				receive_dos.tx_flag = false;
			}

			dos_sync_logic();

			break;
	}
}

void dos_parse_data(uint8_t* data)
{
	switch(data[0])
	{
		case LAYER_SPOTS:
			for(int i = 1; i < 101; ++i)
			{
				spots_of_layer[i - 1] = data[i*4 - 3] << 0;
				spots_of_layer[i - 1] = data[i*4 - 2] << 8;
				spots_of_layer[i - 1] = data[i*4 - 1] << 16;
				spots_of_layer[i - 1] = data[i*4 - 0] << 24;
			}

			break;
		case DOS_WORK_MODE:
			dosimeter_mode = WORK_MODE;
			work_mode = WWAITING_SS;
			break;
		case DOS_SYNC_MODE:
			dosimeter_mode = SYNC_MODE;
			break;
		case DOS_SETTINGS_MODE:
			dosimeter_mode = SETTINGS_MODE;
			break;
	}
}

void dos_settings_logic()
{
	if(HAL_GPIO_ReadPin(LS_SS_PIU_GPIO_Port, LS_SS_PIU_Pin) == GPIO_PIN_SET)
	{
		if(!latch_flag)
		{
			latch_flag = true;

			send_dose();
		}
	}
	else
	{
		latch_flag = false;
	}
}

void dos_sync_logic()
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

			send_dose();
		}
	}
}

void send_dose()
{
	uint32_t dos_result = 0;

	uint8_t buff[5];
	memset(buff, 0, 5);

	srand(HAL_GetTick());
	dos_result = rand();

	dos_result = 0x64000000;

	buff[0] =  (dos_result & 0x000000FF) >> 0;
	buff[1] |= (dos_result & 0x0000FF00) >> 8;
	buff[2] |= (dos_result & 0x00FF0000) >> 16;
	buff[3] |= (dos_result & 0xFF000000) >> 24;

	send_data(buff, 5, rs_dos);
}

void dos_logic()
{
	static bool send_once = false;

	if(HAL_GPIO_ReadPin(LS_SS_PIU_GPIO_Port, LS_SS_PIU_Pin) == GPIO_PIN_SET && work_mode == WWAITING_SS)
	{
		work_mode = WSEND_DATA;
	}

	if(work_mode == WSEND_DATA && HAL_GPIO_ReadPin(LS_SS_PIU_GPIO_Port, LS_SS_PIU_Pin) == GPIO_PIN_SET)
	{
		if(send_once == false)
		{
			send_once = true;

			HAL_GPIO_WritePin(LS_TO_DR_PIU_GPIO_Port, LS_TO_DR_PIU_Pin, GPIO_PIN_SET);

			uint8_t buff[4];

			buff[0] = (spots_of_layer[sended_spots] & 0x000000FF) >> 0;
			buff[1] = (spots_of_layer[sended_spots] & 0x0000FF00) >> 8;
			buff[2] = (spots_of_layer[sended_spots] & 0x00FF0000) >> 16;
			buff[3] = (spots_of_layer[sended_spots] & 0xFF000000) >> 24;

			send_data(buff, 4, rs_dos, .crc = false);

			++sended_spots;
		}
	}
	else
	{
		dosimeter_mode = SETTINGS_MODE;
		work_mode = WSTART;
		sended_spots = 0;
	}

	if(work_mode == WSEND_DATA && HAL_GPIO_ReadPin(LS_MM_PIU_GPIO_Port, LS_MM_PIU_Pin) == GPIO_PIN_SET && send_once == true)
	{
		send_once = false;
	}

	if(HAL_GPIO_ReadPin(LS_MM_PIU_GPIO_Port, LS_MM_PIU_Pin) == GPIO_PIN_SET)
	{
		HAL_GPIO_WritePin(LS_TO_DR_PIU_GPIO_Port, LS_TO_DR_PIU_Pin, GPIO_PIN_RESET);
	}
}

void HAL_UART_RxCpltCallBack(UART_HandleTypeDef* huart)
{
	receive_logic(huart, rs_dos, &receive_dos);
}

