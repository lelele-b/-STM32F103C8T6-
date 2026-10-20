#include "Tick.h"

static volatile uint32_t s_TickMs = 0;

/**
  * @brief  启动 1 ms 周期的 SysTick 中断，作为系统毫秒时基
  */
void Tick_Init(void)
{
	/* SystemCoreClock 由 system_stm32f10x.c 配置为 72 MHz */
	(void)SysTick_Config(SystemCoreClock / 1000u);

	s_TickMs = 0;
}

/**
  * @brief  读取当前毫秒计数，调用方用无符号差值比较即可处理回绕
  */
uint32_t Tick_GetMs(void)
{
	return s_TickMs;
}

/**
  * @brief  由 SysTick_Handler 调用，每 1 ms 递增一次
  */
void Tick_Inc(void)
{
	s_TickMs++;
}
