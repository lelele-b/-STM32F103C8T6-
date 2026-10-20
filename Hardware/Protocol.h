#ifndef __PROTOCOL_H
#define __PROTOCOL_H

#include "stm32f10x.h"

void     Protocol_Init(void);
void     Protocol_Poll(uint32_t now_ms);		/* 解析上位机下行命令 */
void     Protocol_SendReport(uint32_t now_ms);	/* 发送一帧监测数据 */
uint32_t Protocol_GetPeriodMs(void);			/* 当前上报周期 */

#endif
