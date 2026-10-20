#include "LED.h"

#define LED_PORT			GPIOA
#define LED_PIN				GPIO_Pin_0

#define LED_HB_PERIOD_MS	1000u	/* 心跳周期 */
#define LED_HB_ON_MS		50u		/* 心跳点亮时长 */
#define LED_ALARM_PERIOD_MS	200u	/* 报警闪烁周期 */
#define LED_ALARM_ON_MS		100u	/* 报警点亮时长 */

/**
  * @brief  配置 PA0 为推挽输出
  */
void LED_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = LED_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(LED_PORT, &GPIO_InitStructure);

	LED_Off();
}

void LED_On(void)
{
#if LED_ACTIVE_HIGH
	GPIO_SetBits(LED_PORT, LED_PIN);
#else
	GPIO_ResetBits(LED_PORT, LED_PIN);
#endif
}

void LED_Off(void)
{
#if LED_ACTIVE_HIGH
	GPIO_ResetBits(LED_PORT, LED_PIN);
#else
	GPIO_SetBits(LED_PORT, LED_PIN);
#endif
}

/**
  * @brief  根据报警与静音状态刷新指示灯，内部只做时间比较，不阻塞
  */
void LED_Update(uint32_t now_ms, uint8_t alarm_active, uint8_t muted)
{
	uint32_t phase;

	if (alarm_active != 0u)
	{
		if (muted != 0u)
		{
			LED_On();	/* 已静音但仍有报警，指示灯常亮提示 */
			return;
		}

		phase = now_ms % LED_ALARM_PERIOD_MS;
		if (phase < LED_ALARM_ON_MS)
		{
			LED_On();
		}
		else
		{
			LED_Off();
		}
		return;
	}

	phase = now_ms % LED_HB_PERIOD_MS;
	if (phase < LED_HB_ON_MS)
	{
		LED_On();
	}
	else
	{
		LED_Off();
	}
}
