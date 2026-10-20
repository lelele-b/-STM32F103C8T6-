#ifndef __LED_H
#define __LED_H

#include "stm32f10x.h"

/*
 * PA0 接指示灯（串限流电阻）
 *   1 = 输出高电平点亮（LED 阳极接 PA0，阴极经电阻到 GND）
 *   0 = 输出低电平点亮（3.3 V 经电阻接 LED，LED 阴极接 PA0）
 */
#define LED_ACTIVE_HIGH		1

void LED_Init(void);
void LED_On(void);
void LED_Off(void);

/*
 * 非阻塞刷新指示灯状态
 *   alarm_active = 0        无报警   -> 1 Hz 心跳（亮 50 ms）
 *   alarm_active = 1, muted = 0  报警   -> 5 Hz 快闪
 *   alarm_active = 1, muted = 1  已静音 -> 常亮
 */
void LED_Update(uint32_t now_ms, uint8_t alarm_active, uint8_t muted);

#endif
