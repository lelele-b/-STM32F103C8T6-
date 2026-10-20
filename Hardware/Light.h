#ifndef __LIGHT_H
#define __LIGHT_H

#include "stm32f10x.h"

/* 光照通道：原始值、毫伏、百分比换算 */
uint16_t Light_GetRaw(void);
uint32_t Light_GetMilliVolt(void);
uint16_t Light_GetPercent10(void);	/* 0.1 % 精度，取值 0 ~ 1000 */
uint8_t  Light_GetPercent(void);	/* 整数百分比，取值 0 ~ 100 */

#endif
