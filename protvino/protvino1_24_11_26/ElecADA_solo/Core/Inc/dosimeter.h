/*
 * dosimeter.h
 *
 *  Created on: Jun 9, 2026
 *      Author: User
 */

#ifndef INC_DOSIMETER_H_
#define INC_DOSIMETER_H_

#include "api_smpd.h"

typedef enum {
	WORK_MODE = 1,
	SETTINGS_MODE = 2,
	SYNC_MODE = 3
} DOSIMETER_MODE;

typedef enum {
	WSTART = 1,
	WWAITING_SS = 2,
	WSEND_DATA = 3
} DWORK_MODE;

//initialisation of dosimeter
void dos_init();

//Finite state machine
void dos_main_logic();

//Dosimeter work mode logic
void dos_logic();

//Dosimeter settings mode logic
void dos_settings_logic();

//Dosimeter sync mode logic
void dos_sync_logic();

//Parse data from PIU
void dos_parse_data(uint8_t* data);

//Send dose
void send_dose();

#endif /* INC_DOSIMETER_H_ */
