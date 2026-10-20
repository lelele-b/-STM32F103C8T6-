#ifndef __ALARM_H
#define __ALARM_H

#include "stm32f10x.h"

#define ALARM_BIT_TEMP		0x01u	/* 温度超过上限 */
#define ALARM_BIT_LIGHT		0x02u	/* 光照低于下限 */

#define ALARM_TEMP_HIGH10_DEFAULT	400		/* 40.0 摄氏度 */
#define ALARM_LIGHT_LOW_DEFAULT		20u		/* 20 % */

void    Alarm_Init(void);

int16_t Alarm_GetTempHigh10(void);
void    Alarm_SetTempHigh10(int16_t value);
uint8_t Alarm_GetLightLow(void);
void    Alarm_SetLightLow(uint8_t value);

void    Alarm_Poll(uint32_t now_ms, int16_t temp10, uint8_t light_pct);
uint8_t Alarm_GetBits(void);

uint8_t Alarm_IsMuted(void);
void    Alarm_ToggleMute(void);

#endif
