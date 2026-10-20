#ifndef __TICK_H
#define __TICK_H

#include "stm32f10x.h"

/* 系统毫秒时基：由 SysTick 中断累加，用于调度与指示灯闪烁 */
void     Tick_Init(void);
uint32_t Tick_GetMs(void);
void     Tick_Inc(void);

#endif
