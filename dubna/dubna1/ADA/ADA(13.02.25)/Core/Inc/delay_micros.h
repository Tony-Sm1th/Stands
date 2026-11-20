#ifndef __DELAY_US_H__
#define __DELAY_US_H__

#ifdef __cplusplus
 extern "C" {
#endif

/******************************************************************************/
/* Подключение заголовочных файлов используемых модулей */
#include "main.h"

//#define DWT_CONTROL *(volatile unsigned long *)0xE0001000
//#define SCB_DEMCR   *(volatile unsigned long *)0xE000EDFC




/******************************************************************************/
/* inline func */
__STATIC_INLINE void DWT_Init(void)
{
	//FOR F7 only
	/*
	 volatile uint32_t *DWT_CONTROL = (uint32_t *) 0xE0001000;
	 volatile uint32_t *DWT_CYCCNT = (uint32_t *) 0xE0001004;
	 volatile uint32_t *DEMCR = (uint32_t *) 0xE000EDFC;
	 volatile uint32_t *LAR  = (uint32_t *) 0xE0001FB0;   // <-- added lock access register
	*DEMCR = *DEMCR | 0x01000000;     // enable trace
	*LAR = 0xC5ACCE55;                // <-- added unlock access to DWT (ITM, etc.)registers
	*DWT_CYCCNT = 0;                  // clear DWT cycle counter
	*DWT_CONTROL = *DWT_CONTROL | 1;  // enable DWT cycle counter
	*DWT_CONTROL */
	//
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; // разрешаем использовать счётчик
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;   // запускаем счётчик

}

__STATIC_INLINE void delay_us(uint32_t us)
{
	uint32_t us_count_tic =  us * (SystemCoreClock / 1000000U);
	DWT->CYCCNT = 0U;
	while(DWT->CYCCNT < us_count_tic);
}

__STATIC_INLINE void delay_ns(uint32_t ns)
{
	uint32_t ns_count_tic =  ns * (SystemCoreClock / 10000000U);
	DWT->CYCCNT = 0U;
	while(DWT->CYCCNT < ns_count_tic);
}

__STATIC_INLINE void delay_us_new(uint32_t us, uint32_t clk)
{
	uint32_t us_count_tic =  us * (clk);
	DWT->CYCCNT = 0U;
	while(DWT->CYCCNT < us_count_tic);
}

#ifdef __cplusplus
}
#endif
#endif //__DELAY_US_H__
/*******************************  END OF FILE  ********************************/
