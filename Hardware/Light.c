#include "Light.h"
#include "AD.h"

#define LIGHT_ADC_FULL	4095u	/* 12 位 ADC 满量程 */
#define LIGHT_VREF_MV	3300u	/* 参考电压 3.3 V */

/*
 * 光照强度的方向定义：
 *   1 = 采样值越大代表越亮（光敏二极管模块，AO 随光照升高）
 *   0 = 采样值越大代表越暗（光敏电阻对地接法，AO 随光照降低）
 * 如果屏上读数与实际明暗相反，只改这一行即可。
 */
#define LIGHT_RAW_RISES_WITH_LIGHT	1

/**
  * @brief  读取光照通道的原始 ADC 值
  */
uint16_t Light_GetRaw(void)
{
	return AD_Value[AD_IDX_LIGHT];
}

/**
  * @brief  光照通道电压，单位毫伏
  */
uint32_t Light_GetMilliVolt(void)
{
	return ((uint32_t)Light_GetRaw() * LIGHT_VREF_MV) / LIGHT_ADC_FULL;
}

/**
  * @brief  光照强度，单位 0.1 %，范围 0 ~ 1000
  */
uint16_t Light_GetPercent10(void)
{
	uint32_t raw = (uint32_t)Light_GetRaw();
	uint32_t p10;

#if LIGHT_RAW_RISES_WITH_LIGHT
	p10 = (raw * 1000u) / LIGHT_ADC_FULL;
#else
	p10 = ((LIGHT_ADC_FULL - raw) * 1000u) / LIGHT_ADC_FULL;
#endif

	if (p10 > 1000u)
	{
		p10 = 1000u;
	}

	return (uint16_t)p10;
}

/**
  * @brief  光照强度整数百分比，范围 0 ~ 100
  */
uint8_t Light_GetPercent(void)
{
	return (uint8_t)(Light_GetPercent10() / 10u);
}
