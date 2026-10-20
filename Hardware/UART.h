#ifndef __UART_H
#define __UART_H

#include "stm32f10x.h"

#define UART_BAUD_DEFAULT	115200u

void    UART_Init(uint32_t baud);

void    UART_SendByte(uint8_t byte);
void    UART_SendString(const char *str);
void    UART_SendUint(uint32_t value);
void    UART_SendInt(int32_t value);
void    UART_SendDec1(int16_t value10);	/* 输出一位小数，如 253 -> "25.3" */

uint8_t UART_RxPop(uint8_t *byte);		/* 从接收环形缓冲取一个字节，返回 1 表示取到 */

void    UART_IRQHandler(void);			/* 由 USART1_IRQHandler 调用 */

#endif
