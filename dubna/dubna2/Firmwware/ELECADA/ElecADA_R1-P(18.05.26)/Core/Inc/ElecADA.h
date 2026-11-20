/*
 * ElecADA.h
 *
 *  Created on: Apr 3, 2026
 *      Author: Tony
 */

#ifndef INC_ELECADA_H_
#define INC_ELECADA_H_
//#include "utils.h"
//#include "USART_cmd.h"

#include "common.h"


#define MAX_OMNI_DOSE 	 	0xFFFFFFFFFFFFFFFFULL
#define TRANSIT_DELAY		((1ULL << 32) - 1)
#define SPI_BUFFER_SIZE		1000

//#define SPOT_PRINT			10
//#define MAX_ADC				59578
#define DOSE_TABLE_SIZE 	10000
#define WINDOW_SIZE			8
#define SEPARATE(value, pulses, parrots) \
		do { \
			(pulses) = (value) / MAX_ADC; \
			(parrots) = (value) % MAX_ADC; \
		} while(0)
extern uint8_t SPOT_PRINT;
extern uint16_t MAX_ADC;


typedef struct {
    bool adc_run;
    bool pulse_run;
    bool uart_lock;
    bool dose_view;
    bool first_entry;
    bool transit_dose;
    bool adc_ready;
    volatile bool sd_save_request;  // Запрос на сохранение
    volatile uint32_t sd_save_cnt;
    volatile void *sd_save_ptr;
} SystemFlags_t;

// Системное состояние
typedef struct {
	volatile uint32_t spot_num;
	volatile uint64_t omni_dose_offset;
	volatile uint8_t active_bank;
	uint32_t mag_cnt;
	uint16_t data;
	uint16_t dummy;
} SystemState_t;


// Таймеры
typedef struct {
	uint32_t pulse;   // Для периода импульсов
	uint32_t spot;    // Для времени спота
} Timers_t;

typedef enum{
	STATE_WAIT,
	STATE_CMD,
	STATE_ADC_READ,
	STATE_PROCESS,
	STATE_SEND,
	STATE_SPI_ERROR
}State_t;


// Таблица доз
typedef struct __attribute__((aligned(4))) {
	uint32_t dose_table[DOSE_TABLE_SIZE];    // Значения доз
	uint32_t whole[DOSE_TABLE_SIZE];         // Целые части (импульсы)
	uint16_t parrots[DOSE_TABLE_SIZE];       // Дробные части (АЦП)
	uint32_t table_size;                     // Реальный размер таблицы
	volatile uint32_t current_index;         // Текущий индекс (volatile для прерываний)
} DOSES_table_t;


// Текущая уставка дозы
typedef struct {
	uint32_t target_dose;   // Текущая целевая доза
	uint32_t whole;         // Целых импульсов
	uint16_t parrots;       // Остаток в АЦП
} DOSE_set_t;





//typedef enum{
//	ADC_ON		=	1,
//	ADC_OFF	=	0,
//}ADC_state_t;

typedef struct __attribute__((aligned(4))){
	uint64_t omni_dose[SPI_BUFFER_SIZE];

	uint32_t transit_dose_pulse[SPI_BUFFER_SIZE];
	uint32_t transit_dose_adc[SPI_BUFFER_SIZE];

	uint64_t spot_dose[SPI_BUFFER_SIZE];

	uint32_t time_buf[SPI_BUFFER_SIZE];
	uint32_t pulse_period_buf[SPI_BUFFER_SIZE];  // Массив периодов для каждого спота

	uint32_t counter;
	uint32_t dose_iter[SPI_BUFFER_SIZE];
	uint32_t spot_index[SPI_BUFFER_SIZE];     // Индекс спота в таблице (ДОБАВИТЬ)

	uint16_t buffer_adc[SPI_BUFFER_SIZE];
	//uint32_t spot_num;
	//	ADA_switches_t mode;

	uint8_t early_switch_flag[SPI_BUFFER_SIZE];  // 1=точное, 2=порог, 3=прогноз
	uint64_t predicted_dose[SPI_BUFFER_SIZE];    // Прогнозируемая доза
}PULSE_cnt_t;


// Структура для скользящего окна (оптимизировано под FPU)
typedef struct {
	uint64_t buffer[WINDOW_SIZE];      // Кольцевой буфер скоростей (целые)
	uint64_t sum;                       // Сумма в окне (целая)
	uint64_t early_threshold;           // Порог раннего переключения (целый)
	float lin_coeff;                    // Средняя скорость (float)
	float prediction_factor;            // Коэффициент упреждения (0.0f - 1.0f)
	uint8_t idx;                        // Индекс в буфере (0..7)
	uint8_t count;                      // Счетчик измерений
	bool ready;                         // Флаг готовности окна
} SlidingWindow_t;

/*
1.00	= target	Только EXACT (без упреждения)
0.99	99% target	PREDICTION (тонкое упреждение), THRESHOLD — редко
0.95	95% target	PREDICTION сначала, THRESHOLD — подстраховка
0.90	90% target	THRESHOLD срабатывает чаще (грубое упреждение)
0.80	80% target	THRESHOLD основной (очень грубо)
 *
 */

// Тип переключения спота
typedef enum {
	SWITCH_NONE = 0,
	SWITCH_EXACT,        // 1 - точное достижение target
	SWITCH_THRESHOLD,    // 2 - по порогу early_threshold
	SWITCH_PREDICTION    // 3 - по прогнозу
} SwitchType_t;




extern SystemFlags_t sf;
extern SystemState_t ss;
extern SlidingWindow_t sw;
extern DOSES_table_t dt;
extern DOSE_set_t ds;

void Start_ADC();
void Stop_ADC();
void Start_Pulse_cnt();
void Stop_Pulse_cnt();
void Read_ADC();
void Count_Pulse();
void Send_data();
void Make_package(uint32_t size);
void Process_ADC_data_every_sample(uint16_t data);
void Process_ADC_data_every_ovf(uint16_t data);
void Process_ADC_data_every_ovf_test(uint16_t data);
void Reset_counters();
void Init_Dose_Table(void);
void Start_First_Spot(void);
void On_Magnet_Arrived();
//void Next_Spot(void);
//void Set_Current_Spot(void);


void Update_Early_Threshold(uint64_t target_dose);
bool Should_Early_Switch(uint64_t target_dose, uint64_t current_dose, uint32_t period, uint64_t *out_predicted);

bool Try_Switch_Spot(uint64_t ef_dose, uint32_t n_spot,uint32_t sz, PULSE_cnt_t *pcnt);
void Set_Prediction_Factor(float factor);
void Change_spot();


void Update_window(uint64_t data, uint32_t period);

#endif /* INC_ELECADA_H_ */
