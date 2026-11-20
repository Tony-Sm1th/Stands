#include "api_smpd.h"

#define rs485_transmit(data, size, send_data, ...) \
	_rs485_transmit((data), (size), (send_data), (byte_protocol_settings_t){__VA_ARGS__})

static RECEIVE_MODE recv_mode = COMMAND_WAITING;
// static uint8_t recv_timeout = 0;
static uint8_t last_crc = 0;

static void inline _rs485_transmit(uint8_t* data, uint16_t size, rs485_t send_data, byte_protocol_settings_t settings)
{
	HAL_GPIO_WritePin(send_data.gpio_port, send_data.gpio_pin, GPIO_PIN_SET);

	if(settings.dma)
	{
		HAL_UART_Transmit_DMA(send_data.sel_uart, data, size);
		return;
	}
	else
	{
		HAL_UART_Transmit(send_data.sel_uart, data, size, 100);
		HAL_GPIO_WritePin(send_data.gpio_port, send_data.gpio_pin, GPIO_PIN_RESET);

	}
}

static void command_logic(UART_HandleTypeDef* fun_uart, rs485_t send_data, byte_protocol_t* receive_data)
{
	if(fun_uart == send_data.sel_uart)
	{
		static uint16_t rx_count = 0;
		static uint16_t index_of_buff = 0;
		static bool first_byte_flag = true;
		if(first_byte_flag == true)
		{
			for(int i = 0; i < receive_data->size_oloc; ++i)
			{
				if(receive_data->list_of_commands[i].id == *receive_data->rx_buff)
				{
					rx_count = receive_data->list_of_commands[i].size;
				}
			} 

			if(rx_count != 0)
			{
				receive_data->tx_buff[0] = *receive_data->rx_buff;
				--rx_count;
				index_of_buff = rx_count;
				first_byte_flag = false;
			}
		}
		else
		{
			receive_data->tx_buff[index_of_buff - (rx_count - 1)] = *receive_data->rx_buff;
			--rx_count;

			if(rx_count == 0)
			{
				// receive_data->tx_buff[index_of_buff - rx_count] = *receive_data->rx_buff;
				uint16_t tx_size = index_of_buff + 1;
				uint8_t crc = crc8_calc(receive_data->tx_buff, tx_size-1);
				send_data(&crc, 1, send_data, .crc = false);

				if(crc == receive_data->tx_buff[index_of_buff])
				{
					receive_data->tx_flag = true;
				}

				first_byte_flag = true;
			}
		}

		HAL_UART_Receive_IT(send_data.sel_uart, receive_data->rx_buff, 1);
	}
}

void receive_logic(UART_HandleTypeDef* fun_uart, rs485_t send_data, byte_protocol_t* receive_data)
{
	switch(recv_mode)
	{
		case COMMAND_WAITING:
			command_logic(fun_uart, send_data, receive_data);
			break;
		case CRC_WAITING:
			static uint8_t counter = 0;

			if(*receive_data->rx_buff == last_crc
				|| counter >= NO_RESPONSE)
			{
				recv_mode = COMMAND_WAITING;
				counter = 0;

				HAL_UART_Receive_IT(send_data.sel_uart, receive_data->rx_buff, 1);
				break;
			}

			++counter;
			send_data(&last_crc, 1, send_data, .crc = false);
			HAL_UART_Receive_IT(send_data.sel_uart, receive_data->rx_buff, 1);

			break;
	};
}

void transmit_logic(UART_HandleTypeDef* fun_uart, rs485_t send_data)
{
	if (fun_uart == send_data.sel_uart) {
		HAL_GPIO_WritePin(send_data.gpio_port, send_data.gpio_pin, RESET);
	}
}

void _send_data(uint8_t* data, uint16_t size, rs485_t send_data, byte_protocol_settings_t settings)
{
	if(settings.crc == false)
	{
		rs485_transmit(data, size, send_data, .dma = settings.dma);

		return;
	}

	uint8_t crc = crc8_calc(data, size - 1);

	data[size - 1] = crc;

	if(settings.block == false)
	{
		recv_mode = CRC_WAITING;
		last_crc = crc;
		rs485_transmit(data, size, send_data, .dma = settings.dma);

		return;
	}
	else
	{
		uint8_t rx_buff = 0;
		uint8_t counter = 0;

		do
		{
			++counter;

			rs485_transmit(data, size, send_data, .dma = settings.dma);

			HAL_UART_Receive(send_data.sel_uart, &rx_buff, 1, 100);

			if(counter >= NO_RESPONSE) {
				break;
			}
		} while(rx_buff != crc);
	}
}

uint8_t crc8_calc(uint8_t* data, uint16_t data_length)
{
	uint8_t crc = 0xFF;

	for(int i = 0; i < data_length; ++i)
	{
		crc ^= data[i];

		for(int j = 8; j > 0; --j)
		{
			if((crc & 1) == 1)
			{
				crc = (crc >> 1) ^ 0xb8;
			} else
			{
				crc >>= 1;
			}
		}
	}

	return crc;
}
