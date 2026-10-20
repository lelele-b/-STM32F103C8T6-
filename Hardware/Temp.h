#ifndef __TEMP_H
#define __TEMP_H

#include "stm32f10x.h"

/* 温度通道：原始值、毫伏、摄氏度换算 */
uint16_t Temp_GetRaw(void);
uint32_t Temp_GetMilliVolt(void);
int16_t  Temp_GetCelsius10(void);	/* 0.1 摄氏度，如 253 表示 25.3 C */

#endif
