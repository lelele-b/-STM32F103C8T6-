#include "Alarm.h"
#include "Buzzer.h"

#define ALARM_TEMP_HYST10		20		/* 2.0 摄氏度回差，避免阈值附近反复抖动 */
#define ALARM_LIGHT_HYST		5u		/* 5 % 回差 */
#define ALARM_CONFIRM_COUNT		3u		/* 连续 3 次采样确认后才动作 */

static int16_t s_TempHigh10 = ALARM_TEMP_HIGH10_DEFAULT;
static uint8_t s_LightLow = ALARM_LIGHT_LOW_DEFAULT;
static uint8_t s_Bits = 0;
static uint8_t s_Muted = 0;
static uint8_t s_TempCount = 0;
static uint8_t s_LightCount = 0;

void Alarm_Init(void)
{
	s_TempHigh10 = ALARM_TEMP_HIGH10_DEFAULT;
	s_LightLow = ALARM_LIGHT_LOW_DEFAULT;
	s_Bits = 0;
	s_Muted = 0;
	s_TempCount = 0;
	s_LightCount = 0;

	Buzzer_OFF();
}

int16_t Alarm_GetTempHigh10(void)
{
	return s_TempHigh10;
}

void Alarm_SetTempHigh10(int16_t value)
{
	if (value < -400)
	{
		value = -400;
	}
	if (value > 1200)
	{
		value = 1200;
	}

	s_TempHigh10 = value;
	s_TempCount = 0;
}

uint8_t Alarm_GetLightLow(void)
{
	return s_LightLow;
}

void Alarm_SetLightLow(uint8_t value)
{
	if (value > 100u)
	{
		value = 100u;
	}

	s_LightLow = value;
	s_LightCount = 0;
}

/**
  * @brief  更新报警状态，带回差与连续确认，同时驱动蜂鸣器
  */
void Alarm_Poll(uint32_t now_ms, int16_t temp10, uint8_t light_pct)
{
	(void)now_ms;

	/* 温度：高于上限置位，回落到上限以下 2 C 才清除 */
	if ((s_Bits & ALARM_BIT_TEMP) == 0u)
	{
		if (temp10 > s_TempHigh10)
		{
			s_TempCount++;
			if (s_TempCount >= ALARM_CONFIRM_COUNT)
			{
				s_Bits |= ALARM_BIT_TEMP;
				s_TempCount = 0;
			}
		}
		else
		{
			s_TempCount = 0;
		}
	}
	else
	{
		if (temp10 <= (int16_t)(s_TempHigh10 - ALARM_TEMP_HYST10))
		{
			s_Bits &= (uint8_t)~ALARM_BIT_TEMP;
			s_TempCount = 0;
		}
	}

	/* 光照：低于下限置位，回升到下限以上 5 % 才清除 */
	if ((s_Bits & ALARM_BIT_LIGHT) == 0u)
	{
		if (light_pct < s_LightLow)
		{
			s_LightCount++;
			if (s_LightCount >= ALARM_CONFIRM_COUNT)
			{
				s_Bits |= ALARM_BIT_LIGHT;
				s_LightCount = 0;
			}
		}
		else
		{
			s_LightCount = 0;
		}
	}
	else
	{
		if (light_pct >= (uint8_t)(s_LightLow + ALARM_LIGHT_HYST))
		{
			s_Bits &= (uint8_t)~ALARM_BIT_LIGHT;
			s_LightCount = 0;
		}
	}

	if ((s_Bits != 0u) && (s_Muted == 0u))
	{
		Buzzer_ON();
	}
	else
	{
		Buzzer_OFF();
	}
}

uint8_t Alarm_GetBits(void)
{
	return s_Bits;
}

uint8_t Alarm_IsMuted(void)
{
	return s_Muted;
}

void Alarm_ToggleMute(void)
{
	s_Muted = (s_Muted == 0u) ? 1u : 0u;

	if (s_Muted != 0u)
	{
		Buzzer_OFF();
	}
}
