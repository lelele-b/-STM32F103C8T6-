#include "Temp.h"
#include "AD.h"
#include <math.h>

#define TEMP_ADC_FULL		4095u
#define TEMP_VREF_MV		3300u

/*
 * NTC 热敏电阻参数（按常见 10K B3950 模块填写）
 * 接线：3.3 V -- R_pullup -- ADC 引脚 -- NTC -- GND
 * 测温升高时 NTC 阻值下降，ADC 采样值随之下降。
 */
#define TEMP_PULLUP_R		10000.0f	/* 上拉电阻，单位欧姆 */
#define TEMP_NTC_R25		10000.0f	/* 25 C 时的 NTC 阻值 */
#define TEMP_NTC_B			3950.0f		/* B 值 */
#define TEMP_KELVIN_25		298.15f		/* 25 C 对应的开尔文温度 */
#define TEMP_KELVIN_ZERO	273.15f

/**
  * @brief  读取温度通道的原始 ADC 值
  */
uint16_t Temp_GetRaw(void)
{
	return AD_Value[AD_IDX_TEMP];
}

/**
  * @brief  温度通道电压，单位毫伏
  */
uint32_t Temp_GetMilliVolt(void)
{
	return ((uint32_t)Temp_GetRaw() * TEMP_VREF_MV) / TEMP_ADC_FULL;
}

/**
  * @brief  由采样电压反推 NTC 阻值，再用 B 值公式换算温度
  * @retval 0.1 摄氏度，限幅在 -40.0 ~ 120.0
  */
int16_t Temp_GetCelsius10(void)
{
	uint32_t mv = Temp_GetMilliVolt();
	float rt;
	float kelvin;
	float celsius;

	/* 电压贴边说明传感器开路或短路，直接返回限幅值，避免除零与 log(0) */
	if (mv == 0u)
	{
		return 1200;	/* NTC 短路，按超温处理 */
	}
	if (mv >= TEMP_VREF_MV)
	{
		return -400;	/* NTC 开路，按超低温处理 */
	}

	rt = TEMP_PULLUP_R * (float)mv / (float)(TEMP_VREF_MV - mv);
	kelvin = 1.0f / (1.0f / TEMP_KELVIN_25 + logf(rt / TEMP_NTC_R25) / TEMP_NTC_B);
	celsius = kelvin - TEMP_KELVIN_ZERO;

	if (celsius > 120.0f)
	{
		celsius = 120.0f;
	}
	if (celsius < -40.0f)
	{
		celsius = -40.0f;
	}

	if (celsius >= 0.0f)
	{
		return (int16_t)(celsius * 10.0f + 0.5f);
	}
	return (int16_t)(celsius * 10.0f - 0.5f);
}
