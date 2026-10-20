#include "UART.h"

#define UART_RX_BUF_SIZE	128u	/* 接收环形缓冲长度，必须是 2 的幂附近即可 */

static volatile uint8_t  s_RxBuf[UART_RX_BUF_SIZE];
static volatile uint16_t s_RxHead = 0;
static volatile uint16_t s_RxTail = 0;

/**
  * @brief  初始化 USART1：PA9 = TX，PA10 = RX，8 位数据、1 位停止、无校验
  */
void UART_Init(uint32_t baud)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef  NVIC_InitStructure;

	s_RxHead = 0;
	s_RxTail = 0;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

	/* PA9 -> TX，复用推挽输出 */
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* PA10 -> RX，上拉输入 */
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	USART_InitStructure.USART_BaudRate = baud;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART1, &USART_InitStructure);

	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	USART_Cmd(USART1, ENABLE);
}

/**
  * @brief  发送一个字节，等待发送寄存器空后写入
  */
void UART_SendByte(uint8_t byte)
{
	while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET)
	{
	}

	USART_SendData(USART1, byte);
}

void UART_SendString(const char *str)
{
	while (*str != '\0')
	{
		UART_SendByte((uint8_t)(*str));
		str++;
	}
}

void UART_SendUint(uint32_t value)
{
	char    buf[11];
	uint8_t index = 0;
	uint8_t out = 0;
	char    reversed[11];

	if (value == 0u)
	{
		UART_SendByte((uint8_t)'0');
		return;
	}

	while ((value > 0u) && (index < 10u))
	{
		reversed[index] = (char)('0' + (uint8_t)(value % 10u));
		value = value / 10u;
		index++;
	}

	while (index > 0u)
	{
		index--;
		buf[out] = reversed[index];
		out++;
	}
	buf[out] = '\0';

	UART_SendString(buf);
}

void UART_SendInt(int32_t value)
{
	if (value < 0)
	{
		UART_SendByte((uint8_t)'-');
		value = -value;
	}

	UART_SendUint((uint32_t)value);
}

/**
  * @brief  以一位小数格式输出，如 253 输出 25.3、-53 输出 -5.3
  */
void UART_SendDec1(int16_t value10)
{
	int32_t value = (int32_t)value10;
	uint32_t abs_value;

	if (value < 0)
	{
		UART_SendByte((uint8_t)'-');
		value = -value;
	}

	abs_value = (uint32_t)value;
	UART_SendUint(abs_value / 10u);
	UART_SendByte((uint8_t)'.');
	UART_SendUint(abs_value % 10u);
}

/**
  * @brief  从接收环形缓冲读取一个字节
  * @retval 1 表示取到数据，0 表示缓冲为空
  */
uint8_t UART_RxPop(uint8_t *byte)
{
	if (s_RxHead == s_RxTail)
	{
		return 0u;
	}

	*byte = s_RxBuf[s_RxTail];
	s_RxTail = (uint16_t)((s_RxTail + 1u) % UART_RX_BUF_SIZE);

	return 1u;
}

/**
  * @brief  USART1 接收中断处理：读走数据并存入环形缓冲
  */
void UART_IRQHandler(void)
{
	if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
	{
		uint8_t  data = (uint8_t)USART_ReceiveData(USART1);
		uint16_t next = (uint16_t)((s_RxHead + 1u) % UART_RX_BUF_SIZE);

		if (next != s_RxTail)
		{
			s_RxBuf[s_RxHead] = data;
			s_RxHead = next;
		}
		/* 缓冲满时丢弃新数据，交给上位机重试 */
	}
}
