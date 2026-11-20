/*
 * ElecADA.c
 *
 *  Created on: Apr 3, 2026
 *      Author: Tony
 */

#include "ElecADA.h"
#include "uSD.h"

//#define SAMPLE

PULSE_cnt_t pcnt_1;
PULSE_cnt_t pcnt_2;
PULSE_cnt_t *reg_pcnt;
DOSES_table_t dt;
DOSE_set_t ds;

uint8_t mode;
uint8_t time_delay;
uint32_t start_tick;


SlidingWindow_t sw;

State_t st;
char buf[10000] = { 0 };
extern float CAPACITOR;
extern float INTEGRATOR_VOLT;
extern float pf;

uint16_t MAX_ADC = 65535;
uint8_t SPOT_PRINT = 10;

uint64_t dose_remainder = 0;

SystemFlags_t sf = { .first_entry = true, };
SystemState_t ss = { 0 };

static Timers_t tim = { 0 };

extern DMA_HandleTypeDef hdma_usart3_tx;

inline void Start_ADC() {
	HAL_TIM_Base_Start_IT(&adc_timer);
	HAL_TIM_PWM_Start_IT(&adc_timer, adc_channel);
}

inline void Stop_ADC() {
	HAL_TIM_Base_Stop_IT(&adc_timer);
	HAL_TIM_PWM_Stop_IT(&adc_timer, adc_channel);
}

inline void Start_Pulse_cnt() {
	Start_New_Session();  // Создаём новый файл для нового сеанса
	Reset_counters();
	Set_Prediction_Factor(pf);
	Start_First_Spot();
	HAL_TIM_Base_Start(&pulse_cnt_ovf);
	HAL_TIM_Base_Start_IT(&pulse_cnt);
	Processing_Timer(true, &tim.spot);
	sf.start_cycle = true;
	start_tick = HAL_GetTick();

}

inline void Stop_Pulse_cnt() {
	HAL_TIM_Base_Stop_IT(&pulse_cnt);
	HAL_TIM_Base_Stop_IT(&pulse_cnt_ovf);
	__HAL_TIM_SET_COUNTER(&pulse_cnt, 0);
	__HAL_TIM_SET_COUNTER(&pulse_cnt_ovf, 0);
	Close_SD_File();
	Reset_counters();
	sf.start_cycle = false;

}

void Reset_counters(void) {
	// Сохраняем важные флаги и данные SD
	bool uart_lock_backup = sf.uart_lock;
	bool sd_ready_backup = sd_manager.is_ready;
	bool sd_mounted_backup = sd_manager.is_mounted;
	uint32_t session_counter_backup = sd_manager.session_counter;
	char filename_backup[32];
	strcpy(filename_backup, sd_manager.current_filename);
	bool temp1 = sf.pulse_run;
	uint16_t temp2 = ss.last_adc;
	// Сброс структур
	memset(&sf, 0, sizeof(sf));
	memset(&ss, 0, sizeof(ss));
	memset(&tim, 0, sizeof(tim));
	memset(&sw, 0, sizeof(sw));
	sf.pulse_run = temp1;
	ss.last_adc = temp2;
	// Восстанавливаем SD данные
	sf.uart_lock = uart_lock_backup;
	sd_manager.is_ready = sd_ready_backup;
	sd_manager.is_mounted = sd_mounted_backup;
	sd_manager.session_counter = session_counter_backup;
	strcpy(sd_manager.current_filename, filename_backup);
	sf.first_entry = true;

	// Сбрасываем счётчики банков
	pcnt_1.counter = 0;
	pcnt_2.counter = 0;

	dt.current_index = 0;

	// Очищаем структуры банков
	memset(&pcnt_1, 0, sizeof(PULSE_cnt_t));
	memset(&pcnt_2, 0, sizeof(PULSE_cnt_t));

	// Сбрасываем таймеры
	__HAL_TIM_SET_COUNTER(&pulse_cnt, 0);
	__HAL_TIM_SET_COUNTER(&pulse_cnt_ovf, 0);
	__HAL_TIM_CLEAR_FLAG(&pulse_cnt, TIM_FLAG_UPDATE);

	Set_Prediction_Factor(0.99f);
}

void Reset_spot() {
	ss.last_pulses = 0;  // Инициализируем текущим значением
	ss.ef_dose = 0;
	ss.no_pulse_dose = 0;
	ss.spot_num = 0;
}
void Init_Dose_Table(void) {
	for (uint32_t i = 0; i < dt.table_size; i++) {
		SEPARATE(dt.dose_table[i], dt.whole[i], dt.parrots[i]);
	}
}

void Start_First_Spot(void) {
	uint32_t idx = dt.current_index;
	ds.target_dose = dt.dose_table[idx];
	ds.whole = dt.whole[idx];
	ds.parrots = dt.parrots[idx];
	Update_Early_Threshold(ds.target_dose);  // FPU вычисление
	__HAL_TIM_SET_COUNTER(&pulse_cnt, 0);
}

static inline void Set_Current_Spot() {
	uint32_t idx = dt.current_index;

	ds.target_dose = dt.dose_table[idx];
	ds.whole = dt.whole[idx];
	ds.parrots = dt.parrots[idx];
	Update_Early_Threshold(ds.target_dose);  // FPU вычисление
}

static inline void Next_Spot(void) {
	if (sf.transit_dose == true) {

		dt.current_index++;
		Set_Current_Spot();
		Reset_spot();
		__HAL_TIM_SET_COUNTER(&pulse_cnt, 0);
	} else {
		// Обычный спот: следующая доза из таблицы
		Reset_spot();
		__HAL_TIM_SET_COUNTER(&pulse_cnt, 0);
	}
}

void Dose_rdy() {
	HAL_GPIO_WritePin(L_DOSE_RDY_GPIO_Port, L_DOSE_RDY_Pin, SET);
}

// Быстрое вычисление порога (использует FPU)
void Update_Early_Threshold(uint64_t target_dose) {
	// 1 такт: int→float, 1 такт: FMUL, 1 такт: float→int
	sw.early_threshold =
			(uint64_t) ((float) target_dose * sw.prediction_factor);
}

// Установка коэффициента упреждения
void Set_Prediction_Factor(float factor) {
	if (factor >= 0.0f && factor <= 1.0f) {
		sw.prediction_factor = factor;
	}
}

//#define ADC_BUFFER_SIZE 100

inline void Read_ADC() {
	if (HAL_SPI_Receive(&adc_spi, (uint8_t*) &(ss.data), 1, 1000) != HAL_OK) {
		printf("Error\n");
		return;
	}
	// Ваш существующий код с mag_cnt (не трогаем)
	if (sf.transit_dose == true) {
		if (ss.mag_cnt >= 3333) {
			ss.mag_cnt = 0;
			HAL_GPIO_WritePin(TEST_GPIO_Port, TEST_Pin, SET);
		}
		ss.mag_cnt++;
	}

	// Основная логика
	if (sf.start_cycle == true) {
		Process_ADC_data_every_ovf(ss.data);
		//ss.last_adc = ss.data;

	} else {
		//ss.last_adc = ss.data;
	}
}

void On_Magnet_Arrived(void) {
	if (sf.transit_dose == false) {
		return;
	}

	sf.transit_dose = false;
	Processing_Timer(true, &(tim.spot));
	Next_Spot();
}

void Process_ADC_data_every_ovf(uint16_t data) {
	uint8_t bank = ss.active_bank;
	PULSE_cnt_t *pcnt = (bank == 0) ? &pcnt_1 : &pcnt_2;
	uint32_t pulses = __HAL_TIM_GET_COUNTER(&pulse_cnt);
	uint32_t sz = pcnt->counter;
	uint32_t delta_pulses = pulses - ss.last_pulses;

	uint16_t delta_adc;
	uint16_t temp_adc;

	if (data >= ss.last_adc) {
		delta_adc = data - ss.last_adc;
	} else {
		delta_adc = (MAX_ADC - ss.last_adc + data);
	}
	if (mode == 1) {
		ss.ef_dose += delta_adc;
		//temp_adc = (uint16_t) ss.ef_dose;
	} else if (mode == 2) {
		ss.ef_dose = pulses * MAX_ADC;
	} else {

		if (dt.whole[ss.spot_num] != 0) {
			bool dose_overflow;
			ss.adc_accumulator += delta_adc;
			if (ss.adc_accumulator >= MAX_ADC) {
				ss.adc_accumulator -= MAX_ADC;
				dose_overflow = true;
			} else {
				dose_overflow = false;
			}
			//delta_pulses = 0;
			// === ГИБРИДНЫЙ РЕЖИМ (есть импульсы в споте) ===
			if (delta_pulses > 0) {
				ss.ef_dose = (uint64_t) (MAX_ADC * pulses);

//		// Пришел новый импульс
//

				// Измеряем период
				uint32_t period = Processing_Timer(false, &(tim.pulse));
				Processing_Timer(true, &(tim.pulse));
				if (delta_pulses == 1 && period > 0) {
					pcnt->pulse_period_buf[sz] = period;
				}

			} else {
				if (dose_overflow) {
					ss.ef_dose += MAX_ADC + ss.adc_accumulator;
				}
			}
			//ss.ef_dose += ss.adc_accumulator;
			temp_adc = ss.adc_accumulator;
		} else {
			ss.ef_dose += delta_adc;
			temp_adc = (uint16_t) ss.ef_dose;
		}
	}
	//ss.last_adc = data;
//	if (ss.spot_num >= 100)
//		ss.spot_num = 0;
//	ss.spot_num++;
	ss.last_pulses = pulses;
if((HAL_GetTick() - start_tick) >= time_delay*1000){
	printf("dose is %lld\n", ss.ef_dose);
	Stop_Pulse_cnt();
}
	//Update_Rate_avg(delta_adc);

//	if (!sf.transit_dose) {
//
//		Try_Switch_Spot(ss.ef_dose, pulses, sz, temp_adc, pcnt);
//	} else {
//		pcnt->transit_dose_pulse[sz] = pulses;
//		pcnt->transit_dose_adc[sz] = (uint16_t) (ss.adc_accumulator);
//		pcnt->transit_dose[sz] = ss.ef_dose;
//
//	}
}

inline bool Try_Switch_Spot(uint64_t ef_dose, uint32_t pulses_in_spot,
		uint32_t sz, uint16_t data_adc, PULSE_cnt_t *pcnt) {

	if (ds.target_dose == 0)
		return false;

	SwitchType_t switch_type = SWITCH_NONE;
	uint64_t pred_val = ef_dose;
	uint32_t period = pcnt->pulse_period_buf[sz];

	// Читаем время спота
	uint32_t spot_time = Processing_Timer(false, &(tim.spot));

	// Определяем период
	if (pulses_in_spot > 0) {
		if (period > 0) {
			// Уже есть измеренный период
		} else {
			period = spot_time / pulses_in_spot;
		}
	} else {
		period = (__HAL_TIM_GET_AUTORELOAD(&adc_timer)) + 1;
	}

	if (period == 0) {
		period = (__HAL_TIM_GET_AUTORELOAD(&adc_timer)) + 1;
	}
	// Логика переключения (без изменений)
	if (sw.prediction_factor < 1.0f) {
		if (sw.ready) {
			if (Should_Early_Switch(ds.target_dose, ef_dose, &pred_val)) {
				if (pred_val >= ds.target_dose) {
					switch_type = SWITCH_PREDICTION;
				} else {
					switch_type = SWITCH_THRESHOLD;
				}
			}
		} else {
			if (ef_dose >= sw.early_threshold) {
				switch_type = SWITCH_THRESHOLD;
			}
		}
	} else {
		if (ef_dose >= ds.target_dose) {
			switch_type = SWITCH_EXACT;
			if (sw.ready) {
				pred_val =
						(uint64_t) ((double) ef_dose
								+ (double) sw.lin_coeff
										* (double) sw.prediction_factor);
			}
		}
	}

	if (switch_type == SWITCH_NONE)
		return false;

	// Сохраняем данные
	pcnt->spot_index[sz] = dt.current_index;
	pcnt->spot_dose[sz] = ef_dose;
	pcnt->dose_iter[sz] = pulses_in_spot;
	ss.omni_dose_offset += ef_dose;
	pcnt->omni_dose[sz] = ss.omni_dose_offset;
	pcnt->time_buf[sz] = spot_time;
	pcnt->early_switch_flag[sz] = switch_type;
	pcnt->predicted_dose[sz] = pred_val;
	pcnt->buffer_adc[sz] = data_adc;
	//ss.ef_dose = 0;

	if (pulses_in_spot > 0 && period > 0) {
		pcnt->pulse_period_buf[sz] = period;
	}

	// ss.spot_num = 0;
	pcnt->counter++;

	if (ss.omni_dose_offset >= MAX_OMNI_DOSE) {
		Stop_Pulse_cnt();
		printf("The END\n");
		return true;
	}

	sf.transit_dose = true;
	HAL_GPIO_WritePin(TEST_GPIO_Port, TEST_Pin, RESET);
	Dose_rdy();
	Next_Spot();

	if (pcnt->counter >= SPOT_PRINT) {
		pcnt->counter = 0;

		Make_package(SPOT_PRINT);

	}

	return true;
}

inline bool Should_Early_Switch(uint64_t target_dose, uint64_t current_dose,
		uint64_t *out_predicted) {
	if (!sw.ready) {
		if (out_predicted)
			*out_predicted = current_dose;
		return false;
	}

	uint64_t pred_val;
	if (sw.lin_coeff > 0.0f) {
		// lin_coeff - приращение за 30 мкс (одно измерение)
		pred_val = (uint64_t) ((double) current_dose
				+ (double) sw.lin_coeff * (double) sw.prediction_factor);
	} else {
		pred_val = current_dose;
	}

	if (out_predicted)
		*out_predicted = pred_val;

	uint64_t temp = target_dose
			- (double) sw.lin_coeff * (double) sw.prediction_factor;
	if (pred_val >= temp) {
		return true;
	}
	return (pred_val >= target_dose) || (current_dose >= sw.early_threshold);
}

//static double calc_median(uint64_t *buf) {
//    uint64_t sorted[WINDOW_SIZE];
//    memcpy(sorted, buf, WINDOW_SIZE * sizeof(uint64_t));
//
//    // Сортировка вставками
//    for (int i = 1; i < WINDOW_SIZE; i++) {
//        uint64_t tmp = sorted[i];
//        int j = i - 1;
//        while (j >= 0 && sorted[j] > tmp) {
//            sorted[j + 1] = sorted[j];
//            j--;
//        }
//        sorted[j + 1] = tmp;
//    }
//
//    // Медиана для чётного WINDOW_SIZE: среднее двух центральных
//    return (double)(sorted[MEDIAN_IDX1] + sorted[MEDIAN_IDX2]) / 2.0;
//}
//inline void Update_Rate_median(uint64_t delta) {
//	 if (!sw.ready) {
//	        sw.delta_buffer[sw.delta_idx] = delta;
//	        sw.delta_idx = (sw.delta_idx + 1) & WINDOW_SIZE_MASK;
//	        sw.delta_count++;
//
//	        if (sw.delta_count >= WINDOW_SIZE) {
//	            sw.ready = true;
//	            sw.lin_coeff = calc_median(sw.delta_buffer);
//	        }
//	    } else {
//	        sw.delta_buffer[sw.delta_idx] = delta;
//	        sw.delta_idx = (sw.delta_idx + 1) & WINDOW_SIZE_MASK;
//	        sw.lin_coeff = calc_median(sw.delta_buffer);
//	    }
//}

// Вместо общей дозы - передавать ТОЛЬКО ПРИРАЩЕНИЕ
inline void Update_Rate_avg(uint64_t delta) {
	// delta - это corrected_data (приращение АЦП за 30 мкс)
	// или 0, если был импульс

	// Обновляем скользящее окно
	if (!sw.ready) {
		sw.delta_buffer[sw.delta_idx] = delta;
		sw.delta_sum += delta;
		sw.delta_idx = (sw.delta_idx + 1) & WINDOW_SIZE_MASK;
		sw.delta_count++;

		if (sw.delta_count >= WINDOW_SIZE) {
			sw.ready = true;
			sw.avg_delta_per_period = (double) sw.delta_sum / WINDOW_SIZE;
			sw.lin_coeff = sw.avg_delta_per_period;
		}
	} else {
		sw.delta_sum -= sw.delta_buffer[sw.delta_idx];

		sw.delta_buffer[sw.delta_idx] = delta;
		sw.delta_sum += delta;
		sw.delta_idx = (sw.delta_idx + 1) & WINDOW_SIZE_MASK;

		sw.avg_delta_per_period = (double) sw.delta_sum / WINDOW_SIZE;
		sw.lin_coeff = sw.avg_delta_per_period;
	}
}

void Make_package(uint32_t cnt) {
	if (sf.uart_lock)
		return;
	sf.uart_lock = true;

	uint8_t old_bank = ss.active_bank;
	ss.active_bank ^= 1;
	PULSE_cnt_t *out_pcnt = (old_bank == 0) ? &pcnt_1 : &pcnt_2;

	char *buf_ptr = buf;
	size_t buf_remaining = sizeof(buf);
	int written;

	// === ЗАГОЛОВОК ===
	written =
			snprintf(buf_ptr, buf_remaining,
					"\033[2J\033[H\n"
							"Bank: %d | Factor: %.1f%% | Lin coeff: %.1f\n"
							"\n"
							"| %-5s | %-10s | %-15s | %-27s | %-20s | %-10s | %-14s | %-15s | %-10s | %-8s | %-15s | %-12s |\n"
							"|-------|------------|-----------------|-----------------------------|----------------------|------------|----------------|-----------------|------------|----------|-----------------|--------------|\n",
					old_bank, sw.prediction_factor * 100.0f, sw.lin_coeff,
					"Spot", "Dose pulse", "ADC", "Spot dose", "Transit dose",
					"Period(us)", "Time spot(us)", "Total dose", "Switch",
					"Dev%%", "Predicted", "Early thrsh");

	buf_ptr += written;
	buf_remaining -= written;

	// === ДАННЫЕ ПО СПОТАМ ===
	for (size_t i = 0; i < cnt; i++) {
		uint32_t spot_idx = out_pcnt->spot_index[i];
		uint32_t spot_whole =
				(spot_idx < dt.table_size) ? dt.whole[spot_idx] : 0;
		uint16_t spot_parrots =
				(spot_idx < dt.table_size) ? dt.parrots[spot_idx] : 0;
		uint32_t spot_target =
				(spot_idx < dt.table_size) ? dt.dose_table[spot_idx] : 0;

		char dose_iter_str[100];
		snprintf(dose_iter_str, sizeof(dose_iter_str), "%lu(%lu)",
				out_pcnt->dose_iter[i], spot_whole);

		char adc_str[100];
		snprintf(adc_str, sizeof(adc_str), "%u(%u)", out_pcnt->buffer_adc[i],
				spot_parrots);

		char spot_dose_str[100];
		snprintf(spot_dose_str, sizeof(spot_dose_str), "%llu(%lu)",
				out_pcnt->spot_dose[i], spot_target);

		char transit_dose_str[100];
		snprintf(transit_dose_str, sizeof(transit_dose_str), "%llu(%lu:%lu)",
				out_pcnt->transit_dose[i], out_pcnt->transit_dose_pulse[i],
				out_pcnt->transit_dose_adc[i]);

		const char *switch_type_str;
		switch ((SwitchType_t) out_pcnt->early_switch_flag[i]) {
		case SWITCH_EXACT:
			switch_type_str = "EXACT";
			break;
		case SWITCH_THRESHOLD:
			switch_type_str = "THRESHOLD";
			break;
		case SWITCH_PREDICTION:
			switch_type_str = "PREDICTION";
			break;
		default:
			switch_type_str = "-";
			break;
		}

		char deviation_str[100];
		if (spot_target > 0) {
			int64_t diff = (int64_t) out_pcnt->spot_dose[i]
					- (int64_t) spot_target;
			double dev_pct = (double) diff / (double) spot_target * 100.0;
			snprintf(deviation_str, sizeof(deviation_str), "%+.2f%%", dev_pct);
		} else {
			snprintf(deviation_str, sizeof(deviation_str), "-");
		}

		char predicted_str[100];
		if (out_pcnt->predicted_dose[i] > 0) {
			snprintf(predicted_str, sizeof(predicted_str), "%llu",
					out_pcnt->predicted_dose[i]);
		} else {
			snprintf(predicted_str, sizeof(predicted_str), "-");
		}

		char early_thr_str[100];
		uint64_t spot_early_thr =
				(spot_target > 0) ?
						(uint64_t) ((float) spot_target * sw.prediction_factor) :
						0;
		if (spot_early_thr > 0) {
			snprintf(early_thr_str, sizeof(early_thr_str), "%llu",
					spot_early_thr);
		} else {
			snprintf(early_thr_str, sizeof(early_thr_str), "-");
		}

		char period_str[100];
		if (out_pcnt->dose_iter[i] > 0 && out_pcnt->time_buf[i] > 0) {
			uint32_t real_period = out_pcnt->time_buf[i]
					/ out_pcnt->dose_iter[i];
			snprintf(period_str, sizeof(period_str), "%lu", real_period);
		} else {
			snprintf(period_str, sizeof(period_str), "-");
		}

		written =
				snprintf(buf_ptr, buf_remaining,
						"| %-5lu | %-10s | %-15s | %-27s | %-20s | %-10s | %-14lu | %-15llu | %-10s | %-8s | %-15s | %-12s |\n",
						spot_idx, dose_iter_str, adc_str, spot_dose_str,
						transit_dose_str, period_str, out_pcnt->time_buf[i],
						out_pcnt->omni_dose[i], switch_type_str, deviation_str,
						predicted_str, early_thr_str);

		if (written >= (int) buf_remaining)
			break;
		buf_ptr += written;
		buf_remaining -= written;
	}

	// === СТАТИСТИКА ПО АЦП (дельты, ток, линейные коэффициенты) ===
	if (buf_remaining > 300) {
		float charge_per_pulse = CAPACITOR * 1e-12f * INTEGRATOR_VOLT;
		float dose_per_sec = 0.0f;
		float current_nA = 0.0f;

		// === СБОР СТАТИСТИКИ ПО ДОЗЕ В АБСОЛЮТНЫХ ЗНАЧЕНИЯХ ===
		size_t valid_spots = 0;
		uint64_t sum_dose = 0;
		uint64_t min_dose = UINT64_MAX;
		uint64_t max_dose = 0;

		for (size_t i = 0; i < cnt; i++) {
			uint64_t spot_dose = out_pcnt->spot_dose[i];

			if (spot_dose > 0) {
				sum_dose += spot_dose;
				valid_spots++;

				if (spot_dose < min_dose)
					min_dose = spot_dose;
				if (spot_dose > max_dose)
					max_dose = spot_dose;
			}
		}

		double mean_dose = 0.0;
		double stddev_dose = 0.0;
		double cv_dose_pct = 0.0;

		if (valid_spots > 0) {
			mean_dose = (double) sum_dose / valid_spots;

			// Считаем сумму квадратов через double для избежания переполнения
			double sum_sq_double = 0.0;
			for (size_t i = 0; i < cnt; i++) {
				uint64_t spot_dose = out_pcnt->spot_dose[i];
				if (spot_dose > 0) {
					double diff = (double) spot_dose - mean_dose;
					sum_sq_double += diff * diff;
				}
			}

			stddev_dose = sqrt(sum_sq_double / valid_spots);
			if (mean_dose > 0) {
				cv_dose_pct = (stddev_dose / mean_dose) * 100.0;
			}
		}

		if (sw.ready) {
			uint32_t adc_period = (__HAL_TIM_GET_AUTORELOAD(&adc_timer)) + 1;
			dose_per_sec = sw.lin_coeff * (1000000.0f / (float) adc_period);
			float pulses_per_sec = dose_per_sec / (float) MAX_ADC;
			float current_amps = pulses_per_sec * charge_per_pulse;
			current_nA = current_amps * 1e9f;

			// Статистика дельт (разброс измерений АЦП)
			uint64_t sum = 0;
			uint64_t min_delta = UINT64_MAX;
			uint64_t max_delta = 0;

			for (int j = 0; j < WINDOW_SIZE; j++) {
				uint64_t val = sw.delta_buffer[j];
				sum += val;
				if (val < min_delta)
					min_delta = val;
				if (val > max_delta)
					max_delta = val;
			}

			double mean_delta = (double) sum / WINDOW_SIZE;
			double variance_delta = 0.0;
			for (int j = 0; j < WINDOW_SIZE; j++) {
				double diff = (double) sw.delta_buffer[j] - mean_delta;
				variance_delta += diff * diff;
			}
			variance_delta /= WINDOW_SIZE;
			double stddev_delta = sqrt(variance_delta);
			double cv_delta_pct =
					(mean_delta > 0) ?
							(stddev_delta / mean_delta * 100.0) : 0.0;

			written =
					snprintf(buf_ptr, buf_remaining,
							"\n========== ADC MEASUREMENTS STATS (per %ld us) ==========\n"
									"Window: %d samples (%.0f us total)\n"
									"Lin coeff: %.1f dose/period | Speed: %.0f dose/sec\n"
									"Current: %.3f nA (C=%.0fpF, U=%.1fV)\n"
									"\n--- Delta spread (dose per measurement) ---\n"
									"Mean: %.1f | Min: %llu | Max: %llu | Spread: %.1f%%\n"
									"StdDev: %.1f | Coeff Variation (stddev/mean): %.2f%% (lower = more stable)\n"
									"\n--- Dose spread (dose per spot) ---\n"
									"Valid spots: %zu\n"
									"Mean: %.0f | Min: %llu | Max: %llu | Spread: %.1f%%\n"
									"StdDev: %.0f | Coeff Variation (stddev/mean): %.2f%%\n"
									"\nRecent deltas: [", adc_period,
							WINDOW_SIZE,
							(float) WINDOW_SIZE * (float) adc_period,
							sw.lin_coeff, dose_per_sec, current_nA, CAPACITOR,
							INTEGRATOR_VOLT, mean_delta, min_delta, max_delta,
							(mean_delta > 0) ?
									((max_delta - min_delta) / mean_delta
											* 100.0) :
									0.0, stddev_delta, cv_delta_pct,
							valid_spots, mean_dose, min_dose, max_dose,
							(mean_dose > 0) ?
									((double) (max_dose - min_dose) / mean_dose
											* 100.0) :
									0.0, stddev_dose, cv_dose_pct);
		} else {
			written = snprintf(buf_ptr, buf_remaining,
					"\n========== ADC MEASUREMENTS STATS ==========\n"
							"Window not ready (%d/%d samples)\n"
							"\nRecent deltas: [", sw.delta_count,
					WINDOW_SIZE);
		}

		buf_ptr += written;
		buf_remaining -= written;

		// Печать буфера дельт
		for (int j = 0; j < WINDOW_SIZE && buf_remaining > 20; j++) {
			written = snprintf(buf_ptr, buf_remaining, "%s%llu",
					(j > 0 ? ", " : ""), sw.delta_buffer[j]);
			buf_ptr += written;
			buf_remaining -= written;
		}

		if (buf_remaining > 10) {
			written = snprintf(buf_ptr, buf_remaining, "]\n");
			buf_ptr += written;
			buf_remaining -= written;
		}
	}

	// === ОТПРАВКА ===
	size_t total_len = buf_ptr - buf;
	if (total_len > 0) {
		print_UART(pc_uart, (uint8_t*) buf, total_len);
	}

	// === СОХРАНЕНИЕ НА SD ===
	if (sd_manager.is_ready && !sd_manager.is_busy) {
		sf.sd_save_request = true;
		sf.sd_save_cnt = cnt;
		sf.sd_save_ptr = (void*) out_pcnt;
	}
}

