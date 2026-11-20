/*
 * uSD.h
 *
 * Created on: May 12, 2026
 * Author: Tony
 */

#ifndef INC_USD_H_
#define INC_USD_H_

#include "common.h"
#include "ElecADA.h"

// SD карта менеджер
typedef struct {
    bool is_mounted;
    bool is_ready;
    bool is_busy;
    uint32_t session_counter;
    char current_filename[32];
    FATFS fatfs;
} SD_Manager_t;

// Глобальная переменная для управления SD
extern SD_Manager_t sd_manager;

// Функции для работы с SD картой
FRESULT Mount_SD_Card(void);
void Unmount_SD_Card(void);
void Start_New_Session(void);
void Init_Session_Counter(void);
void Save_to_SD(uint32_t cnt, PULSE_cnt_t *out_pcnt);
bool Open_SD_File(void);
void Close_SD_File(void);

#endif /* INC_USD_H_ */
