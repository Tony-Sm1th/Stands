#ifndef COMMON
#define COMMON
#include "main.h"
#include <string.h>
#include <stdbool.h>

#define MIN_DAC_VOLTAGE 0.0008058608058608059
//0.4029304029304029
#define THREE_VOLTS_MANTIGORA 12

extern UART_HandleTypeDef huart5;
#define main_uart huart5

typedef enum {
	ADC_VALUE = 0xC010,
	MANTIGORA_ON = 0xC020,
	MANTIGORA_OFF = 0xC021,
	MANTIGORA_SHUTDOWN = 0xC022,
	TEST_VOLTAGE = 0xC030,
	SET_VOLTAGE = 0xC031
} ADALSTEIN_COMMAND;

typedef enum {
	ERROR_TEST_VOLTAGE = 0xE001,
	ERROR_SET_VOLTAGE = 0xE002,
	ERROR_WRONG_COMMAND = 0xE003,
	ERROR_NEW_DAC_VALUE_NOT_SETTED = 0xE004,
	ERROR_BUSY_ADALSTEIN = 0xE005
} ADALSTEIN_ERROR;

typedef enum {
	MIN_DAC_VALUE = 0x000,
	MAX_DAC_VALUE = 0xE8B
} ADALSTEIN_DAC;

typedef enum {
	TRANSFER = 1,
	RECEIVE = 0
} ADALSTEIN_TRANSMIT;

typedef enum {
	NONE,
	CASE_ADC,
	CASE_ON,
	CASE_OFF,
	CASE_POWEROFF,
	CASE_TEST_VOLTAGE,
	CASE_SET_VOLTAGE,
	CASE_QUESTION_MARK
} ADALSTEIN_CASE_COMMANDS;

void send_data(uint8_t* data);
void process_data(uint8_t* data);
uint16_t get_adc_value();
void dac_set();
void shutdown();
void test_voltage();
int _write(int file, char *ptr, int len);

#endif
