/*
 * utils.h
 *
 *  Created on: Apr 9, 2026
 *      Author: Tony
 */

#ifndef INC_UTILS_H_
#define INC_UTILS_H_
#define TO_US	(SystemCoreClock / 1000000U)

// sw=true: запомнить время в *pTimer
// sw=false: вернуть прошедшее время с момента запоминания
uint32_t Processing_Timer(bool sw, uint32_t *pTimer);

#endif /* INC_UTILS_H_ */

#ifdef UTILS_IMPLEMENTATION


//uint32_t Processing_Timer(bool sw){
//	if (sw == true) {
//		start_processing = DWT->CYCCNT;
//	} else {
//		//		IC_ext.one_cycle_time = DWT->CYCCNT / TO_US;
//		//		IC_end_processing = (IC_ext.one_cycle_time - ((IC_start_processing+DWT->CYCCNT)/(SystemCoreClock / 1000000U)));
//		//		IC_ext.one_cycle_time = DWT->CYCCNT / TO_US;
//		end_processing = abs( DWT->CYCCNT - start_processing) / TO_US;
//		DWT->CYCCNT = 0U;
//	}
//	return end_processing;
//}

uint32_t Processing_Timer(bool sw, uint32_t *pTimer) {
    if (sw == true) {
        // Запоминаем текущее время
        *pTimer = DWT->CYCCNT;
        return 0;
    } else {
        // Вычисляем прошедшее время
        uint32_t current = DWT->CYCCNT;
        uint32_t elapsed;

        if (current >= *pTimer) {
            elapsed = (current - *pTimer) / TO_US;
        } else {
            elapsed = (0xFFFFFFFF - *pTimer + current + 1) / TO_US;
        }

        return elapsed;
    }
}
#endif
