/*
 * uSD.c
 *
 * Created on: May 12, 2026
 * Author: Tony
 */

#include "uSD.h"
#include "ElecADA.h"

#define SD_BUFFER_SIZE 16384  // 16KB буфер для ускорения

// Инициализация SD менеджера
SD_Manager_t sd_manager = {
    .is_mounted = false,
    .is_ready = false,
    .is_busy = false,
    .session_counter = 0,
    .current_filename = {0}
};

// Внешние переменные из ElecADA.c
extern DOSES_table_t dt;
extern uint16_t MAX_ADC;
extern uint8_t SPOT_PRINT;
extern SlidingWindow_t sw;

// Статический файловый объект для всего сеанса
static FIL sd_file;
static bool file_is_open = false;
static bool header_written = false;

/**
 * @brief  Инициализация счётчика сессий на основе существующих файлов
 */
void Init_Session_Counter(void) {
    FIL file;
    FRESULT res;
    uint32_t max_session = 0;
    char filename[32];

    for (uint32_t i = 1; i <= 999; i++) {
        snprintf(filename, sizeof(filename), "dose_%03lu.csv", i);
        res = f_open(&file, filename, FA_READ);
        if (res == FR_OK) {
            max_session = i;
            f_close(&file);
        } else {
            break;
        }
    }

    sd_manager.session_counter = max_session;
}

/**
 * @brief  Начать новую сессию (новый файл)
 */
void Start_New_Session(void) {
    // Закрываем предыдущий файл, если открыт
    if (file_is_open) {
        f_close(&sd_file);
        file_is_open = false;
    }
    header_written = false;

    sd_manager.session_counter++;
    memset(sd_manager.current_filename, 0, sizeof(sd_manager.current_filename));
    snprintf(sd_manager.current_filename, sizeof(sd_manager.current_filename),
             "dose_%03lu.csv", sd_manager.session_counter);
}

/**
 * @brief  Открыть файл для записи (вызывается один раз в начале сеанса)
 */
bool Open_SD_File(void) {
    if (!sd_manager.is_ready) return false;

    FRESULT res;

    // Проверяем, существует ли файл и нужен ли заголовок
    res = f_open(&sd_file, sd_manager.current_filename, FA_READ);
    if (res == FR_NO_FILE || (res == FR_OK && f_size(&sd_file) == 0)) {
        header_written = false;
    }
    if (res == FR_OK) f_close(&sd_file);

    // Открываем файл для записи
    res = f_open(&sd_file, sd_manager.current_filename, FA_OPEN_APPEND | FA_WRITE);
    if (res == FR_NO_FILE) {
        res = f_open(&sd_file, sd_manager.current_filename, FA_CREATE_ALWAYS | FA_WRITE);
        header_written = false;
    }

    if (res != FR_OK) {
        return false;
    }

    // Пишем заголовок если нужно
    if (!header_written) {
        char header[] = "Spot;Dose pulse;ADC;Spot dose;Transit dose;Period(us);Time spot(us);Total dose;Switch;Dev%;Predicted;Early thrsh\n";
        UINT bw;
        f_write(&sd_file, header, strlen(header), &bw);
        header_written = true;
    }

    // Перемещаемся в конец файла
    f_lseek(&sd_file, f_size(&sd_file));

    file_is_open = true;
    return true;
}

/**
 * @brief  Закрыть файл (вызывается в конце сеанса)
 */
void Close_SD_File(void) {
    if (file_is_open) {
        f_close(&sd_file);
        file_is_open = false;
    }
}

/**
 * @brief  Сохранение данных на SD карту (оптимизированная версия)
 */
void Save_to_SD(uint32_t cnt, PULSE_cnt_t *out_pcnt) {
    // Защита от повторного входа
    static bool is_saving = false;
    if (is_saving) return;
    is_saving = true;

    if (!sd_manager.is_ready) {
        is_saving = false;
        return;
    }
    if (cnt == 0 || out_pcnt == NULL) {
        is_saving = false;
        return;
    }

    if (sd_manager.current_filename[0] == '\0') {
        Start_New_Session();
    }

    // Открываем файл один раз за сеанс
    if (!file_is_open) {
        if (!Open_SD_File()) {
            return;
        }
    }

    static char buffer[SD_BUFFER_SIZE];
    int pos = 0;

    // Накопление данных в буфер
    for (size_t i = 0; i < cnt && i < SPOT_PRINT; i++) {
        uint32_t spot_idx = out_pcnt->spot_index[i];
        if (spot_idx >= dt.table_size) continue;

        uint32_t period = (out_pcnt->time_buf[i] > 0 && out_pcnt->dose_iter[i] > 0)
                        ? out_pcnt->time_buf[i] / out_pcnt->dose_iter[i] : out_pcnt->time_buf[i];

        const char *switch_str;
        switch ((SwitchType_t)out_pcnt->early_switch_flag[i]) {
            case SWITCH_EXACT:      switch_str = "EXACT";      break;
            case SWITCH_THRESHOLD:  switch_str = "THRESHOLD";  break;
            case SWITCH_PREDICTION: switch_str = "PREDICTION"; break;
            default:                switch_str = "-";          break;
        }

        double dev_pct = 0.0;
        if (dt.dose_table[spot_idx] > 0) {
            int64_t diff = (int64_t)out_pcnt->spot_dose[i] - (int64_t)dt.dose_table[spot_idx];
            dev_pct = (double)diff / (double)dt.dose_table[spot_idx] * 100.0;
        }

        uint64_t transit_dose = (uint64_t)out_pcnt->transit_dose_pulse[i] * MAX_ADC + out_pcnt->transit_dose_adc[i];
        uint64_t early_thr = (dt.dose_table[spot_idx] > 0)
                            ? (uint64_t)((float)dt.dose_table[spot_idx] * sw.prediction_factor) : 0;

        // Формируем строку
        char line[4096];
        int line_len = snprintf(line, sizeof(line),
            "%lu;%lu;%u;%llu;%llu;%lu;%lu;%llu;%s;%+.2f;%llu;%llu\n",
            spot_idx, out_pcnt->dose_iter[i], out_pcnt->buffer_adc[i],
            out_pcnt->spot_dose[i], transit_dose, period,
            out_pcnt->time_buf[i], out_pcnt->omni_dose[i],
            switch_str, dev_pct, out_pcnt->predicted_dose[i], early_thr);

        // Добавляем в буфер
        if (pos + line_len < SD_BUFFER_SIZE) {
            memcpy(buffer + pos, line, line_len);
            pos += line_len;
        } else {
            break;
        }
    }

    // Запись буфера на диск
    if (pos > 0) {
        UINT bw;
        f_write(&sd_file, buffer, pos, &bw);

        // fsync - сброс кэша на диск (без закрытия файла)
        f_sync(&sd_file);
    }
    is_saving = false;
}

/**
 * @brief  Размонтирование SD карты
 */
void Unmount_SD_Card(void) {
    // Закрываем файл перед размонтированием
    if (file_is_open) {
        f_close(&sd_file);
        file_is_open = false;
    }

    if (sd_manager.is_mounted) {
        f_mount(NULL, "0:/", 0);
        sd_manager.is_mounted = false;
        sd_manager.is_ready = false;
        printf("[SD] Unmounted\n");
    }
}

/**
 * @brief  Монтирование SD-карты
 */
FRESULT Mount_SD_Card(void) {
    FRESULT fr;
    char msg[1000];
    int pos = 0;

    if (HAL_SD_Init(&sd_card) != HAL_OK) {
        pos += snprintf(msg + pos, sizeof(msg) - pos, "[SD] Init FAILED\n");
        sd_manager.is_ready = false;
        print_UART(spiu_uart, (uint8_t*)msg, pos);
        return FR_DISK_ERR;
    }
    pos += snprintf(msg + pos, sizeof(msg) - pos, "[SD] Init OK\n");

    if (HAL_SD_ConfigWideBusOperation(&sd_card, SDMMC_BUS_WIDE_4B) == HAL_OK) {
        pos += snprintf(msg + pos, sizeof(msg) - pos, "[SD] 4-bit mode\n");
    } else {
        pos += snprintf(msg + pos, sizeof(msg) - pos, "[SD] 1-bit mode\n");
    }

    fr = f_mount(&sd_manager.fatfs, "0:/", 1);

    if (fr == FR_OK) {
        pos += snprintf(msg + pos, sizeof(msg) - pos, "[SD] Mount OK\n");
        sd_manager.is_mounted = true;
        sd_manager.is_ready = true;

        Init_Session_Counter();

        FATFS *fs_ptr;
        DWORD free_clusters;
        if (f_getfree("0:/", &free_clusters, &fs_ptr) == FR_OK) {
            uint32_t total_size = (uint32_t)((fs_ptr->n_fatent - 2) * fs_ptr->csize * 0.5);
            uint32_t free_space = (uint32_t)(free_clusters * fs_ptr->csize * 0.5);
            pos += snprintf(msg + pos, sizeof(msg) - pos, "[SD] Total: %lu KB, Free: %lu KB\n",
                           total_size, free_space);
        }
    } else {
        pos += snprintf(msg + pos, sizeof(msg) - pos, "[SD] Mount ERROR: %d\n", fr);
        sd_manager.is_ready = false;
    }

    print_UART(out_uart, (uint8_t*)msg, pos);
    return fr;
}
