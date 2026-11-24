/*
 * ElecADA.h
 *
 *  Created on: Apr 3, 2026
 *      Author: Tony
 */

#ifndef INC_ELECADA_H_
#define INC_ELECADA_H_

#include "common.h"
#include "api_smpd.h"


#define MAX_OMNI_DOSE       0xFFFFFFFFFFFFFFFFULL
#define TRANSIT_DELAY       ((1ULL << 32) - 1)
#define SPI_BUFFER_SIZE     1000

#define DOSE_TABLE_SIZE     10000

#define SEPARATE(value, pulses, parrots) \
        do { \
            (pulses) = (value) / MAX_ADC; \
            (parrots) = (value) % MAX_ADC; \
        } while(0)

#define MEDIAN_IDX1 (WINDOW_SIZE / 2 - 1)   // для 8: индекс 3
#define MEDIAN_IDX2 (WINDOW_SIZE / 2)       // для 8: индекс 4

#define GET_BYTE(value, byte_num) ((((unsigned long long)(value)) >> ((byte_num) * 8)) & 0xFF)

extern uint8_t SPOT_PRINT;
extern uint16_t MAX_ADC;

/* ============================================================
 * SystemFlags_t
 * Все флаги, которые меняются в прерываниях или читаются из main
 * при активном ISR, помечены volatile.
 * ============================================================ */
typedef struct {
	volatile bool adc_run;
	volatile bool pulse_run;
	volatile bool start_cycle;
	volatile bool uart_lock;
	volatile bool dose_view;
	volatile bool first_entry;
	volatile bool transit_dose;
	volatile bool adc_ready;
	volatile bool sd_save_request;   // Запрос на сохранение
	volatile uint32_t sd_save_cnt;
	volatile void *sd_save_ptr;

	volatile bool start_stop;
	volatile bool sync_latch_high;
	volatile bool sync_latch_low;

} SystemFlags_t;

typedef struct {
	volatile bool start;
	volatile int64_t adc_dose;
	volatile uint32_t pulse_dose;
	volatile uint64_t mix_dose;
	volatile int64_t last_adc_dose;
	volatile uint32_t start_tick;
	volatile uint32_t cnt;
	volatile uint32_t last_pulses_sec;
	volatile int64_t  last_adc_sec;
	volatile int32_t time_update;

	volatile uint32_t temp_pulses;
	volatile int32_t temp_adc;
	volatile uint32_t pulses;
	volatile uint64_t temp_dose;
	volatile bool first_entry;
	 uint8_t mode;



}Testing_t;
/* ============================================================
 * SystemState_t
 * Всё, что меняется в ISR (Process_ADC_data_every_ovf, Read_ADC)
 * и читается в main / Make_package.
 * ============================================================ */
typedef struct {
	volatile uint64_t omni_dose_offset;   // 8 байт, offset 0
	volatile uint32_t spot_num;           // 4 байта, offset 8
	volatile uint32_t mag_cnt;            // 4 байта, offset 12
	volatile uint32_t last_pulses;        // 4 байта, offset 16
	volatile uint32_t last_spot_idx;      // 4 байта, offset 20
	volatile uint16_t last_adc;
	volatile uint64_t adc_accumulator;
	volatile uint64_t ef_dose;
	volatile uint64_t no_pulse_dose;
	volatile uint16_t data;
	volatile uint16_t dummy;
	volatile uint8_t active_bank;        // 1 байт, offset 28
} SystemState_t;

/* ============================================================
 * Timers_t
 * Используется в ISR и в main, поэтому volatile.
 * ============================================================ */
typedef struct {
	volatile uint32_t pulse;   // Для периода импульсов
	volatile uint32_t spot;    // Для времени спота
} Timers_t;

typedef enum {
	STATE_WAIT,
	STATE_CMD,
	STATE_ADC_READ,
	STATE_PROCESS,
	STATE_SEND,
	STATE_SPI_ERROR
} State_t;

/* ============================================================
 * DOSES_table_t
 * Таблица доз инициализируется один раз, но current_index
 * читается/пишется в ISR — поэтому volatile.
 * ============================================================ */
typedef struct __attribute__((aligned(4))) {
	uint32_t dose_table[DOSE_TABLE_SIZE];    // Значения доз
	uint32_t whole[DOSE_TABLE_SIZE];         // Целые части (импульсы)
	uint16_t parrots[DOSE_TABLE_SIZE];       // Дробные части (АЦП)
	uint32_t table_size;                     // Реальный размер таблицы
	volatile uint32_t current_index;         // Текущий индекс
} DOSES_table_t;

/* ============================================================
 * DOSE_set_t
 * Читается в ISR (Try_Switch_Spot), пишется в main.
 * ============================================================ */
typedef struct {
	volatile uint32_t target_dose;
	volatile uint32_t whole;
	volatile uint16_t parrots;
} DOSE_set_t;

/* ============================================================
 * PULSE_cnt_t
 * Массивы заполняются в ISR, читаются в Make_package.
 * counter меняется в ISR — volatile.
 * ============================================================ */
typedef struct __attribute__((aligned(4))) {
	volatile uint64_t omni_dose[SPI_BUFFER_SIZE];
	volatile uint32_t transit_dose_pulse[SPI_BUFFER_SIZE];
	volatile uint32_t transit_dose_adc[SPI_BUFFER_SIZE];
	volatile uint64_t transit_dose[SPI_BUFFER_SIZE];
	volatile uint64_t spot_dose[SPI_BUFFER_SIZE];
	volatile uint32_t time_buf[SPI_BUFFER_SIZE];
	volatile uint32_t pulse_period_buf[SPI_BUFFER_SIZE];
	volatile uint32_t counter;
	volatile uint32_t dose_iter[SPI_BUFFER_SIZE];
	volatile uint32_t spot_index[SPI_BUFFER_SIZE];
	volatile uint16_t buffer_adc[SPI_BUFFER_SIZE];
	volatile uint8_t early_switch_flag[SPI_BUFFER_SIZE];
	volatile uint64_t predicted_dose[SPI_BUFFER_SIZE];

	volatile uint32_t adc_offset;
	volatile uint16_t last_adc;
	volatile uint16_t last_adc_value[SPI_BUFFER_SIZE];
	volatile uint64_t spot_accumulated;
} PULSE_cnt_t;

/* ============================================================
 * SlidingWindow_t
 * Все поля меняются в ISR (Update_Rate_avg/median) и
 * читаются в main (Make_package).
 * ============================================================ */
#define WINDOW_SIZE        8
#define ADC_RATE           30.0
#define WINDOW_SIZE_MASK   (WINDOW_SIZE - 1)

typedef struct {
	volatile uint64_t delta_buffer[WINDOW_SIZE];
	volatile uint64_t delta_sum;
	volatile uint8_t delta_idx;
	volatile uint8_t delta_count;
	volatile bool ready;
	volatile double avg_delta_per_period;
	volatile uint64_t early_threshold;
	volatile double prediction_factor;
	volatile double lin_coeff;
} SlidingWindow_t;

/* ============================================================
 * SwitchType_t
 * ============================================================ */
typedef enum {
	SWITCH_NONE = 0, SWITCH_EXACT,        // 1 - точное достижение target
	SWITCH_THRESHOLD,    // 2 - по порогу early_threshold
	SWITCH_PREDICTION    // 3 - по прогнозу
} SwitchType_t;

/* ============================================================
 * Глобальные объекты
 * ============================================================ */
extern SystemFlags_t sf;
extern SystemState_t ss;
extern SlidingWindow_t sw;
extern DOSES_table_t dt;
extern DOSE_set_t ds;

/* ============================================================
 * Прототипы функций
 * ============================================================ */
void Start_ADC();
void Stop_ADC();
void Start_Pulse_cnt();
void Stop_Pulse_cnt();
void Read_ADC();
void Count_Pulse();
void Send_data();
void Make_package(uint32_t size);


void Process_ADC_data_every_sample(uint16_t data);
void Process_ADC_data_for_test(uint16_t data);
void Process_ADC_data_work(uint16_t data);
void Reset_counters();
void Reset_spot();
void Init_Dose_Table(void);
void Start_First_Spot(void);
void On_Magnet_Arrived();

void Update_Early_Threshold(uint64_t target_dose);
bool Should_Early_Switch(uint64_t target_dose, uint64_t current_dose,
		uint64_t *out_predicted);

bool Try_Switch_Spot(uint64_t ef_dose, uint32_t pulses_in_spot, uint32_t sz,
		uint16_t data_adc, PULSE_cnt_t *pcnt);
void Set_Prediction_Factor(float factor);
void Change_spot();
void Update_Rate_avg(uint64_t dose);
void Update_Rate_median(uint64_t delta);


//spiu
extern rs485_t rs485;
extern const command_t list_of_dcommands[4];

typedef enum {
	WORK_MODE = 1,
	SETTINGS_MODE = 2,
	SYNC_MODE = 3
} DOSIMETER_MODE;

void Send_data_sync();
void Process_ADC_data_sync(uint16_t data);
void tx_processing();
void dose_parse_data(uint8_t *data);
///

#endif /* INC_ELECADA_H_ */
