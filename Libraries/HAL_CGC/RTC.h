#ifndef __RTC_H__
#define __RTC_H__

#include "stm32f4xx.h"

//#define RTC_CLOCK_SOURCE_LSE          /* LSE used as RTC source clock */
#define RTC_CLOCK_SOURCE_LSI     /* LSI used as RTC source clock. The RTC Clock may varies due to LSI frequency dispersion.*/
										 
										 
extern void RTC_Config(void);										 

#endif
