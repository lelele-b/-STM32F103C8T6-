#ifndef __AD_H__
#define __AD_H__

#include "stm32f10x.h"

/* 双通道 ADC 的采样结果下标，由 AD_Init() 配置的扫描顺序决定 */
#define AD_IDX_LIGHT	0	/* PA4 / ADC1_IN4 光照通道 */
#define AD_IDX_TEMP		1	/* PA5 / ADC1_IN5 温度通道 */

void AD_Init(void);
extern uint16_t AD_Value[2];

#endif
