/*
 * ElecADA.c
 *
 *  Created on: Apr 3, 2026
 *      Author: Tony
 */


#include "ElecADA.h"
#include "uSD.h"

//#define SAMPLE

//SPI_ADC_t 	SPI_ADC_1;
//SPI_ADC_t 	SPI_ADC_2;
//SPI_ADC_t	*reg_adc;

PULSE_cnt_t	pcnt_1;
PULSE_cnt_t	pcnt_2;
PULSE_cnt_t *reg_pcnt;
DOSES_table_t dt;
DOSE_set_t ds;

SlidingWindow_t sw;

State_t		st;
//ADC_state_t ADC_st;
char buf[10000] = {0};
extern float CAPACITOR;
extern float INTEGRATOR_VOLT;
extern float pf;

uint16_t MAX_ADC = 32767;
uint8_t SPOT_PRINT = 10;

SystemFlags_t sf = {
    .first_entry = true,
};
SystemState_t ss = {0};

static Timers_t tim = {0};

extern DMA_HandleTypeDef hdma_usart3_tx;


inline void Start_ADC(){
	HAL_NVIC_EnableIRQ(adc_timer_IRQ);  // Защита от вложенности
	HAL_TIM_Base_Start_IT(&adc_timer);
	HAL_TIM_PWM_Start_IT(&adc_timer, adc_channel);
}

inline void Stop_ADC(){
	HAL_TIM_Base_Stop_IT(&adc_timer);
	HAL_TIM_PWM_Stop_IT(&adc_timer, adc_channel);
}

inline void Start_Pulse_cnt(){
    Start_New_Session();  // Создаём новый файл для нового сеанса
	Reset_counters();
	Set_Prediction_Factor(pf);
	Start_First_Spot();
	HAL_TIM_Base_Start(&pulse_cnt_ovf);
	HAL_TIM_Base_Start_IT(&pulse_cnt);
	//HAL_NVIC_DisableIRQ(pulse_timer_IRQ);
	//HAL_NVIC_EnableIRQ(pulse_timer_IRQ);
	Start_ADC();


	//__HAL_TIM_GENERATE_EVENT(&pulse_cnt, TIM_EVENTSOURCE_UPDATE);
}

inline void Stop_Pulse_cnt(){
	HAL_TIM_Base_Stop_IT(&pulse_cnt);
	HAL_TIM_Base_Stop_IT(&pulse_cnt_ovf);
	__HAL_TIM_SET_COUNTER(&pulse_cnt,0);
	__HAL_TIM_SET_COUNTER(&pulse_cnt_ovf,0);
	Stop_ADC();
	Close_SD_File();
	Reset_counters();

}


void Reset_counters(void) {
    // Сохраняем важные флаги и данные SD
    bool uart_lock_backup = sf.uart_lock;
    bool sd_ready_backup = sd_manager.is_ready;
    bool sd_mounted_backup = sd_manager.is_mounted;
    uint32_t session_counter_backup = sd_manager.session_counter;
    char filename_backup[32];
    strcpy(filename_backup, sd_manager.current_filename);

    // Сброс структур
    memset(&sf, 0, sizeof(sf));
    memset(&ss, 0, sizeof(ss));
    memset(&tim, 0, sizeof(tim));

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

    memset(&sw, 0, sizeof(SlidingWindow_t));
    Set_Prediction_Factor(0.99f);
}

void Init_Dose_Table(void) {
	for (uint32_t i = 0; i < dt.table_size; i++) {
		SEPARATE(dt.dose_table[i],
				dt.whole[i],
				dt.parrots[i]);
	}
}

void Start_First_Spot(void) {
	uint32_t idx = dt.current_index;
	ds.target_dose = dt.dose_table[idx];
	ds.whole = dt.whole[idx];
	ds.parrots = dt.parrots[idx];
	Update_Early_Threshold(ds.target_dose);  // FPU вычисление
	Processing_Timer(true, &tim.spot);
	__HAL_TIM_SET_AUTORELOAD(&pulse_cnt, dt.whole[0]);
	__HAL_TIM_SET_COUNTER(&pulse_cnt, 0);
	HAL_TIM_GenerateEvent(&pulse_cnt, TIM_EVENTSOURCE_UPDATE);
	__HAL_TIM_SET_AUTORELOAD(&pulse_cnt, TRANSIT_DELAY);

}

static inline void Set_Current_Spot() {
	uint32_t idx = dt.current_index;

	ds.target_dose = dt.dose_table[idx];
	ds.whole = dt.whole[idx];
	ds.parrots = dt.parrots[idx];

	Update_Early_Threshold(ds.target_dose);  // FPU вычисление
	//Processing_Timer(true, &spot_timer);
	__HAL_TIM_SET_AUTORELOAD(&pulse_cnt, ds.whole);
}


static inline void Next_Spot(void) {
	if (sf.transit_dose == true) {
		HAL_TIM_GenerateEvent(&pulse_cnt, TIM_EVENTSOURCE_UPDATE);
		HAL_GPIO_WritePin(L_RSV_EXIT_GPIO_Port, L_RSV_EXIT_Pin, RESET);

		dt.current_index++;
		Set_Current_Spot();
		// Транзит: ставим TRANSIT_DELAY
		// HAL_TIM_GenerateEvent(&pulse_cnt, TIM_EVENTSOURCE_UPDATE);
		// __HAL_TIM_SET_AUTORELOAD(&pulse_cnt, TRANSIT_DELAY);
	} else {
		// Обычный спот: следующая доза из таблицы
		//__HAL_TIM_SET_COUNTER(&pulse_cnt, 0);
		HAL_TIM_GenerateEvent(&pulse_cnt, TIM_EVENTSOURCE_UPDATE);
		__HAL_TIM_SET_AUTORELOAD(&pulse_cnt, TRANSIT_DELAY);
		//Processing_Timer(true, &spot_timer);

	}
}



//void On_Magnet_Start(){
//	TRANSIT_DOSE = true;
//}

void Dose_rdy()
{
	HAL_GPIO_WritePin(L_DOSE_RDY_GPIO_Port, L_DOSE_RDY_Pin, SET);
}


// Быстрое вычисление порога (использует FPU)
void Update_Early_Threshold(uint64_t target_dose) {
	// 1 такт: int→float, 1 такт: FMUL, 1 такт: float→int
	sw.early_threshold = (uint64_t)((float)target_dose * sw.prediction_factor);
}

// Установка коэффициента упреждения
void Set_Prediction_Factor(float factor) {
	if (factor >= 0.0f && factor <= 1.0f) {
		sw.prediction_factor = factor;
	}
}


inline void Read_ADC(){

//	if(HAL_SPI_GetState(&adc_spi) == HAL_SPI_STATE_BUSY ){
//		return;
//	}

	if(HAL_SPI_Receive(&adc_spi, (uint8_t*)&(ss.data), 1,1000) != HAL_OK){
		printf("Error\n");
	}
	if(sf.transit_dose == true){
		if(ss.mag_cnt >= 3333){
			ss.mag_cnt = 0;

			HAL_GPIO_WritePin(TEST_GPIO_Port, TEST_Pin, SET);

		}
		ss.mag_cnt++;
	}
	Process_ADC_data_every_ovf(ss.data);
}


void On_Magnet_Arrived(void) {
	if (sf.transit_dose == false) {
		return;
	}

	uint8_t bank = ss.active_bank;
	PULSE_cnt_t *pcnt = (bank == 0) ? &pcnt_1 : &pcnt_2;
	size_t sz = pcnt->counter;

	// Читаем актуальные данные
	uint32_t transit_pulses = __HAL_TIM_GET_COUNTER(&pulse_cnt);
	uint16_t current_adc = ss.data;
	uint16_t old_adc = pcnt->buffer_adc[sz];
	uint16_t corrected_adc = current_adc;

	// === ТА ЖЕ ОБРАБОТКА, ЧТО И В PROCESS_ADC ===
	if (transit_pulses > ss.spot_num) {
		if (current_adc > old_adc) {
			corrected_adc = 0;
		}
		ss.spot_num = transit_pulses;
	}

	pcnt->transit_dose_adc[sz] = corrected_adc;

	pcnt->transit_dose_pulse[sz] = transit_pulses;

	// Прибавляем к предыдущему споту
	if (sz > 0) {
		size_t prev = sz - 1;
		uint64_t transit_dose = (uint64_t)transit_pulses * MAX_ADC + corrected_adc;
		//pcnt->spot_dose[prev] += transit_dose;
		ss.omni_dose_offset += transit_dose;
		pcnt->omni_dose[prev] = ss.omni_dose_offset;
	}

	if (ss.omni_dose_offset >= MAX_OMNI_DOSE) {
		Stop_Pulse_cnt();
		printf("The END\n");
		return;
	}

	sf.transit_dose = false;
	ss.spot_num = 0;
	Processing_Timer(true, &(tim.spot));
	Next_Spot();
}



//inline void Read_ADC() {
//		//HAL_GPIO_WritePin(L_RSV_EXIT_GPIO_Port, L_RSV_EXIT_Pin, SET);
//
//    uint32_t timeout = 10000;  // Максимальное число попыток
//
//    // Ждем готовности TX
//    while (!(SPI1->SR & SPI_SR_TXE)) {
//        if (--timeout == 0) return;
//    }
//
//    // Отправляем dummy для тактирования
//    *((__IO uint16_t*)&SPI1->DR) = 0x0000;
//
//    // Ждем RX с таймаутом
////    timeout = 10000;
////    while (!(SPI1->SR & SPI_SR_RXNE)) {
////        if (--timeout == 0) {
////            // Сброс SPI при ошибке
////            SPI1->CR1 &= ~SPI_CR1_SPE;
////            SPI1->CR1 |= SPI_CR1_SPE;
////            return;
////        }
////    }
//
//    uint16_t adc_data = *((__IO uint16_t*)&SPI1->DR);
//    if(TRANSIT_DOSE == true){
//           		if(mag_cnt >= 3333){
//           			mag_cnt = 0;
////           			HAL_GPIO_WritePin(TEST_GPIO_Port, TEST_Pin, SET);
//           			HAL_GPIO_TogglePin(L_RSV_EXIT_GPIO_Port, L_RSV_EXIT_Pin);
//
//           		}
//           		mag_cnt++;
//           	}
//    Process_ADC_data_every_ovf(adc_data);
//
//}


void Process_ADC_data_every_ovf(uint16_t data) {
    uint8_t bank = ss.active_bank;
    PULSE_cnt_t *pcnt = (bank == 0) ? &pcnt_1 : &pcnt_2;
    uint32_t n_spot = __HAL_TIM_GET_COUNTER(&pulse_cnt);
    uint32_t sz = pcnt->counter;
    uint16_t old = pcnt->buffer_adc[sz];
    uint16_t corrected_data = data;

    // Обработка перехода импульса
    if (n_spot > ss.spot_num) {
        uint32_t period = Processing_Timer(false, &(tim.pulse));
        Processing_Timer(true, &(tim.pulse));
        if (n_spot > 1 && period > 0 ) {
               pcnt->pulse_period_buf[sz] = period;
           }
        if (data > old) {
            corrected_data = 0;
        }
        ss.spot_num = n_spot;
    }

    pcnt->buffer_adc[sz] = corrected_data;
    uint64_t ef_dose = (uint64_t)n_spot * MAX_ADC + corrected_data;

    if (sf.transit_dose == true) {
        return;
    }

    Try_Switch_Spot(ef_dose, n_spot, sz, pcnt);
}

inline bool Try_Switch_Spot(uint64_t ef_dose, uint32_t n_spot,
                                   uint32_t sz, PULSE_cnt_t *pcnt) {
    if (ds.target_dose == 0) return false;

    SwitchType_t switch_type = SWITCH_NONE;
    uint64_t pred_val = ef_dose;
    uint32_t period = pcnt->pulse_period_buf[sz];  //  Читаем из массива

    // Читаем время спота ОДИН раз
    uint32_t spot_time = Processing_Timer(false, &(tim.spot));

    // Определяем период
    if (n_spot > 0) {
        // Есть импульсы — период из pulse_period_buf
        if (period > 0) {
            // Уже есть измеренный период
        } else {
            period = spot_time / n_spot;  // Оценка
        }
    } else {
        // Нет импульсов — период АЦП
        period = (__HAL_TIM_GET_AUTORELOAD(&adc_timer)) + 1;
    }

    //  Защита от нуля
    if (period == 0) {
        period = (__HAL_TIM_GET_AUTORELOAD(&adc_timer)) + 1;
    }

    if (sw.prediction_factor < 1.0f) {
        if (sw.ready) {
            if (Should_Early_Switch(ds.target_dose, ef_dose, period, &pred_val)) {
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
                pred_val = (uint64_t)((double)ef_dose +
                                     (double)sw.lin_coeff * (double)period *
                                     (double)sw.prediction_factor);
            }
        }
    }

    if (switch_type == SWITCH_NONE) return false;

    // ✅ spot_time уже прочитан выше — не читаем повторно!
    if (spot_time > 0) {
        Update_window(ef_dose, spot_time);
    }

    pcnt->spot_index[sz]       = dt.current_index;
    pcnt->spot_dose[sz]        = ef_dose;
    pcnt->dose_iter[sz]        = n_spot;
    ss.omni_dose_offset           += ef_dose;
    pcnt->omni_dose[sz]        = ss.omni_dose_offset;
    pcnt->time_buf[sz]         = spot_time;
    pcnt->early_switch_flag[sz] = switch_type;
    pcnt->predicted_dose[sz]   = pred_val;

    ss.spot_num = 0;
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
                                       uint32_t period, uint64_t *out_predicted) {
    if (!sw.ready) {
        if (out_predicted) *out_predicted = current_dose;
        return false;
    }

    uint64_t pred_val;
    if (sw.lin_coeff > 0.0f) {
        pred_val = (uint64_t)((double)current_dose +
                                       (double)sw.lin_coeff * (double)period *
                                       (double)sw.prediction_factor);
    } else {
        pred_val = current_dose;  // Без прогноза — только порог
    }

    if (out_predicted) *out_predicted = pred_val;

    return (pred_val >= target_dose) || (current_dose >= sw.early_threshold);
}


void Update_window(uint64_t dose, uint32_t pulse) {
    if (pulse == 0) return;

    uint64_t dose_rate = dose / pulse;

    // Если окно не готово, просто добавляем значения
    if (!sw.ready) {
        sw.buffer[sw.idx] = dose_rate;
        sw.sum += dose_rate;
        sw.idx = (sw.idx + 1) & (WINDOW_SIZE - 1);
        sw.count++;

        if (sw.count >= WINDOW_SIZE) {
            sw.ready = true;
        }
        return;
    }

    // Когда окно готово - вычитаем старое, добавляем новое
    sw.sum -= sw.buffer[sw.idx];
    sw.buffer[sw.idx] = dose_rate;
    sw.sum += dose_rate;
    sw.idx = (sw.idx + 1) & (WINDOW_SIZE - 1);

    // Обновляем среднюю скорость
    sw.lin_coeff = (float)sw.sum * 0.125f;
}





void Make_package(uint32_t cnt) {
    if (sf.uart_lock) return;
    sf.uart_lock = true;

    uint8_t old_bank = ss.active_bank;
    ss.active_bank ^= 1;
    PULSE_cnt_t *out_pcnt = (old_bank == 0) ? &pcnt_1 : &pcnt_2;

    char *buf_ptr = buf;
    size_t buf_remaining = sizeof(buf);
    int written;

    // Заголовок
    written = snprintf(buf_ptr, buf_remaining,
        "\033[2J\033[H\n"
        "Bank: %d | Factor: %.1f%% | Lin coeff: %.1f\n"
        "\n"
        "| %-5s | %-10s | %-15s | %-27s | %-20s | %-10s | %-14s | %-15s | %-10s | %-8s | %-15s | %-12s |\n"
        "|-------|------------|-----------------|-----------------------------|----------------------|------------|----------------|-----------------|------------|----------|-----------------|--------------|\n",
        old_bank,
        sw.prediction_factor * 100.0f,
        sw.lin_coeff,
        "Spot", "Dose pulse", "ADC", "Spot dose", "Transit dose",
        "Period(us)", "Time spot(us)", "Total dose", "Switch", "Dev%", "Predicted", "Early thrsh");

    buf_ptr += written;
    buf_remaining -= written;

    // Данные
    for (size_t i = 0; i < cnt; i++) {
        uint32_t spot_idx = out_pcnt->spot_index[i];
        uint32_t spot_whole = (spot_idx < dt.table_size) ? dt.whole[spot_idx] : 0;
        uint16_t spot_parrots = (spot_idx < dt.table_size) ? dt.parrots[spot_idx] : 0;
        uint32_t spot_target = (spot_idx < dt.table_size) ? dt.dose_table[spot_idx] : 0;

        char dose_iter_str[100];
        snprintf(dose_iter_str, sizeof(dose_iter_str), "%lu(%lu)",
                 out_pcnt->dose_iter[i], spot_whole);

        char adc_str[100];
        snprintf(adc_str, sizeof(adc_str), "%u(%u)",
                 out_pcnt->buffer_adc[i], spot_parrots);

        char spot_dose_str[100];
        snprintf(spot_dose_str, sizeof(spot_dose_str), "%llu(%lu)",
                 out_pcnt->spot_dose[i], spot_target);

        uint64_t transit_full = (uint64_t)out_pcnt->transit_dose_pulse[i] * MAX_ADC + out_pcnt->transit_dose_adc[i];
        char transit_dose_str[100];
        snprintf(transit_dose_str, sizeof(transit_dose_str), "%llu(%lu)",
                 transit_full, out_pcnt->transit_dose_pulse[i]);

        const char *switch_type_str;
        switch ((SwitchType_t)out_pcnt->early_switch_flag[i]) {
            case SWITCH_EXACT:      switch_type_str = "EXACT";      break;
            case SWITCH_THRESHOLD:  switch_type_str = "THRESHOLD";  break;
            case SWITCH_PREDICTION: switch_type_str = "PREDICTION"; break;
            default:                switch_type_str = "-";          break;
        }

        char deviation_str[100];
        if (spot_target > 0) {
            int64_t diff = (int64_t)out_pcnt->spot_dose[i] - (int64_t)spot_target;
            double dev_pct = (double)diff / (double)spot_target * 100.0;
            snprintf(deviation_str, sizeof(deviation_str), "%+.2f%%", dev_pct);
        } else {
            snprintf(deviation_str, sizeof(deviation_str), "-");
        }

        char predicted_str[100];
        if (out_pcnt->predicted_dose[i] > 0) {
            snprintf(predicted_str, sizeof(predicted_str), "%llu", out_pcnt->predicted_dose[i]);
        } else {
            snprintf(predicted_str, sizeof(predicted_str), "-");
        }

        char early_thr_str[100];
        uint64_t spot_early_thr = (spot_target > 0)
            ? (uint64_t)((float)spot_target * sw.prediction_factor)
            : 0;
        if (spot_early_thr > 0) {
            snprintf(early_thr_str, sizeof(early_thr_str), "%llu", spot_early_thr);
        } else {
            snprintf(early_thr_str, sizeof(early_thr_str), "-");
        }

        char period_str[100];
        if (out_pcnt->dose_iter[i] > 0 && out_pcnt->time_buf[i] > 0) {
            // Рассчитываем реальный период импульса
            uint32_t real_period = out_pcnt->time_buf[i] / out_pcnt->dose_iter[i];
            snprintf(period_str, sizeof(period_str), "%lu", real_period);
        } else {
            snprintf(period_str, sizeof(period_str), "-");
        }

        written = snprintf(buf_ptr, buf_remaining,
            "| %-5lu | %-10s | %-15s | %-27s | %-20s | %-10s | %-14lu | %-15llu | %-10s | %-8s | %-15s | %-12s |\n",
            spot_idx,
            dose_iter_str,
            adc_str,
            spot_dose_str,
            transit_dose_str,
            period_str,
            out_pcnt->time_buf[i],
            out_pcnt->omni_dose[i],
            switch_type_str,
            deviation_str,
            predicted_str,
            early_thr_str);

        if (written >= (int)buf_remaining) break;
        buf_ptr += written;
        buf_remaining -= written;
    }

    // Статистика
    if (buf_remaining > 500) {
        double sum_dev = 0.0, sum_abs_dev = 0.0;
        uint32_t valid = 0;

        float pred_min = 0.0f, pred_max = 0.0f, pred_sum = 0.0f;
        uint8_t pred_count = 0;

        //  Для стабильности периода
        float per_min = 0.0f, per_max = 0.0f, per_sum = 0.0f;
        uint8_t per_count = 0;
        float freq_avg = 0.0f;

        for (size_t i = 0; i < cnt; i++) {
            uint32_t st = dt.dose_table[out_pcnt->spot_index[i]];
            if (st > 0) {
                int64_t diff = (int64_t)out_pcnt->spot_dose[i] - (int64_t)st;
                double dev = (double)diff / (double)st * 100.0;
                sum_dev += dev;
                sum_abs_dev += (dev < 0 ? -dev : dev);
                valid++;
            }

            // Период
            if (out_pcnt->time_buf[i] > 0) {
                uint32_t p;
                if (out_pcnt->dose_iter[i] > 0) {
                    p = out_pcnt->time_buf[i] / out_pcnt->dose_iter[i];
                } else {
                    p = out_pcnt->time_buf[i];
                }

                // Статистика периода
                if (per_count == 0) {
                    per_min = per_max = p;
                } else {
                    if (p < per_min) per_min = p;
                    if (p > per_max) per_max = p;
                }
                per_sum += p;
                per_count++;

                // Predicted addition
                if (sw.ready && sw.lin_coeff > 0.0f) {
                    float pred_add = sw.lin_coeff * p * sw.prediction_factor;
                    if (pred_count == 0) {
                        pred_min = pred_max = pred_add;
                    } else {
                        if (pred_add < pred_min) pred_min = pred_add;
                        if (pred_add > pred_max) pred_max = pred_add;
                    }
                    pred_sum += pred_add;
                    pred_count++;
                }
            }
        }

        double avg_dev = valid ? sum_dev / valid : 0.0;
        double avg_abs_dev = valid ? sum_abs_dev / valid : 0.0;
        float per_avg = per_count ? per_sum / per_count : 0.0f;
        float per_spread = per_avg ? (per_max - per_min) / per_avg * 100.0f : 0.0f;
        freq_avg = per_avg ? 1000000.0f / per_avg : 0.0f;  // ✅ Частота в Гц
        float pred_avg = pred_count ? pred_sum / pred_count : 0.0f;
        float pred_spread = pred_avg ? (pred_max - pred_min) / pred_avg * 100.0f : 0.0f;

        const char *dose_status = (avg_abs_dev <= 2.0) ? "STABLE" : "UNSTABLE";
        const char *per_status = (per_spread <= 2.0) ? "STABLE" : "UNSTABLE";
        const char *pred_status = (pred_spread <= 2.0) ? "STABLE" : "UNSTABLE";

        written = snprintf(buf_ptr, buf_remaining,
            "\n--- Stats ---\n"
            "Window: %d | Lin coeff: %.1f | Factor: %.1f%%\n"
            "Samples: %d/%d | Sum: %llu\n"
            "\n--- Dose Stability ---\n"
            "Avg deviation: %+.2f%% | Abs avg: %.2f%%\n"
            "Status: %s (<= 2%%)\n"
            "\n--- Period Stability (%d spots) ---\n"
            "Avg period: %.0f us | Avg freq: %.1f Hz\n"
            "Min/Max period: %.0f / %.0f us | Spread: %.1f%%\n"
            "Status: %s (<= 2%%)\n"
            "\n--- Predicted Addition (lin_coeff * period * factor) ---\n"
            "Avg addition: %.0f | Spread: %.1f%%\n"
            "Min/Max: %.0f / %.0f\n"
            "Status: %s (<= 2%%)\n"
            "\n--- Buffer (dose/us) ---\n[",
            WINDOW_SIZE,
            sw.lin_coeff,
            sw.prediction_factor * 100.0f,
            sw.idx, sw.count, sw.sum,
            avg_dev, avg_abs_dev,
            dose_status,
            per_count,
            per_avg, freq_avg,
            per_min, per_max, per_spread,
            per_status,
            pred_avg, pred_spread,
            pred_min, pred_max,
            pred_status);

        buf_ptr += written;
        buf_remaining -= written;

        for (int j = 0; j < WINDOW_SIZE && buf_remaining > 20; j++) {
            written = snprintf(buf_ptr, buf_remaining,
                "%s%.0f", (j > 0 ? ", " : ""), (float)sw.buffer[j]);
            buf_ptr += written;
            buf_remaining -= written;
        }

        if (buf_remaining > 5) {
            written = snprintf(buf_ptr, buf_remaining, "]\n");
            buf_ptr += written;
            buf_remaining -= written;
        }
    }
    // === СРЕДНИЙ ТОК (по импульсам за спот) ===
    float sum_current = 0.0f;
    uint8_t current_count = 0;
    float charge_per_pulse = CAPACITOR * 1e-12f * INTEGRATOR_VOLT;  // Кулоны

    for (size_t i = 0; i < cnt; i++) {
        if (out_pcnt->dose_iter[i] > 0 && out_pcnt->time_buf[i] > 0) {
            uint32_t pulses = out_pcnt->dose_iter[i];           // количество импульсов за спот
            uint32_t time_us = out_pcnt->time_buf[i];           // время спота в микросекундах

            // Заряд за спот (Кулоны)
            float charge_spot = (float)pulses * charge_per_pulse;

            // Время спота в секундах
            float time_sec = time_us * 1e-6f;

            // Ток в Амперах, переводим в наноамперы
            float current_nA = (charge_spot / time_sec) * 1e9f;

            sum_current += current_nA;
            current_count++;
        }
    }

    if (current_count > 0) {
        written = snprintf(buf_ptr, buf_remaining,
            "\nAverage current: %.3f nA (C=%.0fpF, U=%.1fV)\n",
            sum_current / current_count, CAPACITOR, INTEGRATOR_VOLT);  // фемтокулоны
        buf_ptr += written;
        buf_remaining -= written;
    }

    size_t total_len = buf_ptr - buf;
    if (total_len > 0) {
       print_UART(pc_uart, (uint8_t*)buf, total_len);
    }


    if (sd_manager.is_ready && !sd_manager.is_busy) {
        sf.sd_save_request = true;
        sf.sd_save_cnt = cnt;
        sf.sd_save_ptr = (void*)out_pcnt;
    }

}



