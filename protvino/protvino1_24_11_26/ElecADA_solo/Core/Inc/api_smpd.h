#ifndef _API_SMPD_INCLIUDE_H
#define _API_SMPD_INCLIUDE_H

#include "main.h"
#include <stdbool.h>

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart7;
extern UART_HandleTypeDef huart6;
extern UART_HandleTypeDef huart4;

// #define dos_uart huart1
#define profx_uart huart6
#define profy_uart huart7
#define dos_uart huart4

#define GET_BITS(value, bit_mask, shift) (value & bit_mask) >> shift
#define GET_RBITS(value, bit_mask, shift) (value & bit_mask) << shift

#define DOS_RE_DE_Port UART4_RE_DE_GPIO_Port
#define DOS_RE_DE_Pin UART4_RE_DE_Pin
#define PROFX_RE_DE_Port USART6_RE_DE_GPIO_Port
#define PROFX_RE_DE_Pin USART6_RE_DE_Pin
#define PROFY_RE_DE_Port UART7_RE_DE_GPIO_Port
#define PROFY_RE_DE_Pin UART7_RE_DE_Pin
// #define DOS_RE_DE_Port USART6_RE_DE_GPIO_Port
// #define DOS_RE_DE_Pin USART6_RE_DE_Pin

#define NO_RESPONSE 10

#define send_data(data, size, send_data, ...) \
	_send_data((data), (size), (send_data), (byte_protocol_settings_t){.crc = true, __VA_ARGS__})



typedef enum {
	FIRST_BYTE_SHIFT = 0,
	SECOND_BYTE_SHIFT = 8,
	THIRD_BYTE_SHIFT = 16,
	FOURTH_BYTE_SHIFT = 24
} SHIFT_BYTES;

typedef enum {
	BM_FIRST_BYTE = 0xFF << 0,
	BM_SECOND_BYTE = 0xFF << 8,
	BM_THIRD_BYTE = 0xFF << 16,
	BM_FOURTH_BYTE = 0xFF << 24
} BYTE_BITS_MASKS;

typedef enum
{
	WRITE_DOS_SPOTS = 0x01,
	WRITE_PROFX_SPOTS = 0x02,
	WRITE_PROFY_SPOTS = 0x03,
	WRITE_ALL_SPOTS_COMPLETED = 0x04,
	START_TREATMENT = 0x11,
	CLEAR_ALL_SPOTS = 0x05,
	WRITE_PUMA_SPOTS = 0x06,
	SETTINGS_DATA_PROF = 0x20
} MPIU_COMMANDS;

typedef enum
{
	SWRITE_DOS_SPOTS = 0x01,
	SWRITE_PROFX_SPOTS = 0x02,
	SWRITE_PROFY_SPOTS = 0x03,
	NEXT_LAYER = 0x04,
	SWRTIE_SIZE_LAYERS = 0x05,
	SWORK_MODE = 0x11,
	SSYNC_MODE = 0x12,
	SSETTINGS_MODE = 0x13,
	SSETTINGS_DATA_PROF = 0x20,
	SSETTINGS_DATA_DOS = 0x21,
	SSENDS_DATA = 0x30 // ONLY ONE COMMAND FOR SENDING FROM SLAVE
} SPIU_COMMANDS;

typedef enum
{
	DOS_MPIU = 0xD1,
	DOS_SPIU = 0xD2,
	PROF_X = 0xB1,
	PROF_Y = 0xB2,
	PUMA = 0xA0,
	MPIU = 0xC0
} MPIU_PCB_IDS;

typedef enum {
	LAYER_MAGNETS = 0x01,
	PWORK = 0x11
} PUMA_COMMANDS;

typedef enum {
	PROF_CONFIG = 0x5,
	PROF_ACCUM_NOISE = 0x15,
	PROF_NOISE_CANCEL = 0x25,
	PROF_COM_LAYER = 0x35, //com - center of mass, forbidden for ARM HFB
	PROF_WORK_MODE = 0x45, //forbidden for ARM HFB
	PROF_SYNC_MODE = 0x55,
	PROF_SETTINGS_MODE = 0x65
} PROF_COMMANDS;

typedef struct {
	GPIO_TypeDef* gpio_port; //GPIO port of RE_DE port for RS485
	uint16_t gpio_pin; //GPIO port of RE_DE pin for RS485
	UART_HandleTypeDef* sel_uart; //UART using in that RS485 interface
} rs485_t;

typedef struct {
	uint8_t id;
	uint16_t size;
} command_t;

typedef struct {
	uint8_t size_oloc; //how many commands
	uint8_t* tx_buff; //buff for transfer and receiving data
	uint8_t* rx_buff; //buff for receive data
	bool tx_flag; //flag for transfer
	const command_t* list_of_commands; //list of all commands PCB
} byte_protocol_t;

typedef enum {
	CRC_WAITING,
	COMMAND_WAITING
} RECEIVE_MODE;


typedef enum{
	LAYER_SPOTS = 0x05,
	DOS_WORK_MODE = 0x15,
	DOS_SYNC_MODE = 0x25,
	DOS_SETTINGS_MODE = 0x35,
	DOS_CONFIG = 0x45
} DOS_COMMANDS;

typedef struct {
	bool crc;
	bool block;
	bool dma;
} byte_protocol_settings_t;

void transmit_logic(UART_HandleTypeDef* fun_uart, rs485_t send_data);

//logic for receiving data from master
void receive_logic(UART_HandleTypeDef* fun_uart, rs485_t send_data, byte_protocol_t* receive_data);

void _send_data(uint8_t* data, uint16_t size, rs485_t send_data, byte_protocol_settings_t settings);

//CRC-8/TECH-3250
uint8_t crc8_calc(uint8_t* data, uint16_t data_length);

#endif //_API_SMPD_INCLIUDE_H
