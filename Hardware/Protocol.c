#include "Protocol.h"
#include "UART.h"
#include "Light.h"
#include "Temp.h"
#include "Alarm.h"

#define PROTOCOL_VERSION		"1.0"
#define PROTOCOL_LINE_SIZE		48u
#define PROTOCOL_PERIOD_MIN		1000u		/* 最快 1 秒一帧 */
#define PROTOCOL_PERIOD_MAX		60000u

static uint32_t s_PeriodMs = 1000u;
static char     s_Acc[PROTOCOL_LINE_SIZE];
static uint8_t  s_AccLen = 0;

static void protocol_exec(char *line);

void Protocol_Init(void)
{
	s_PeriodMs = 1000u;
	s_AccLen = 0;

	UART_SendString("\r\n# Env Monitor ");
	UART_SendString(PROTOCOL_VERSION);
	UART_SendString(" ready\r\n");
	UART_SendString("# UART1 115200 8N1, send h for help\r\n");
}

uint32_t Protocol_GetPeriodMs(void)
{
	return s_PeriodMs;
}

/**
  * @brief  发送一帧 JSON 格式监测数据，所有小数值以 0.1 单位输出
  */
void Protocol_SendReport(uint32_t now_ms)
{
	UART_SendString("{\"t\":");
	UART_SendUint(now_ms);

	UART_SendString(",\"light\":{\"raw\":");
	UART_SendUint(Light_GetRaw());
	UART_SendString(",\"pct\":");
	UART_SendDec1((int16_t)Light_GetPercent10());
	UART_SendString(",\"mv\":");
	UART_SendUint(Light_GetMilliVolt());

	UART_SendString("},\"temp\":{\"raw\":");
	UART_SendUint(Temp_GetRaw());
	UART_SendString(",\"c\":");
	UART_SendDec1(Temp_GetCelsius10());
	UART_SendString(",\"mv\":");
	UART_SendUint(Temp_GetMilliVolt());

	UART_SendString("},\"alarm\":");
	UART_SendUint(Alarm_GetBits());
	UART_SendString(",\"muted\":");
	UART_SendUint(Alarm_IsMuted());

	UART_SendString(",\"th\":{\"temp\":");
	UART_SendDec1(Alarm_GetTempHigh10());
	UART_SendString(",\"light\":");
	UART_SendUint(Alarm_GetLightLow());
	UART_SendString("}}\r\n");
}

/**
  * @brief  逐字节取出串口数据并拼成命令行，遇到换行后交给 protocol_exec 解析
  */
void Protocol_Poll(uint32_t now_ms)
{
	uint8_t ch;

	while (UART_RxPop(&ch) != 0u)
	{
		if (ch == (uint8_t)'\r')
		{
			continue;
		}

		if (ch == (uint8_t)'\n')
		{
			s_Acc[s_AccLen] = '\0';
			if (s_AccLen > 0u)
			{
				protocol_exec(s_Acc);
			}
			s_AccLen = 0;
			continue;
		}

		if (s_AccLen < (uint8_t)(PROTOCOL_LINE_SIZE - 1u))
		{
			s_Acc[s_AccLen] = (char)ch;
			s_AccLen++;
		}
		else
		{
			s_AccLen = 0;	/* 超长行直接丢弃，等待下一个换行 */
		}
	}

	(void)now_ms;
}

static char *skip_space(char *text)
{
	while ((*text == ' ') || (*text == '\t'))
	{
		text++;
	}

	return text;
}

/**
  * @brief  解析一位小数的定点数，如 40、40.5、-5.3
  * @retval 1 解析成功并写入 value10
  */
static uint8_t parse_fixed1(const char *text, int16_t *value10)
{
	int32_t int_part = 0;
	int32_t frac_part = 0;
	int32_t result;
	uint8_t negative = 0;
	uint8_t digits = 0;
	uint8_t has_frac = 0;

	while ((*text == ' ') || (*text == '\t'))
	{
		text++;
	}

	if (*text == '-')
	{
		negative = 1;
		text++;
	}
	else if (*text == '+')
	{
		text++;
	}

	while ((*text >= '0') && (*text <= '9'))
	{
		int_part = int_part * 10 + (int32_t)(*text - '0');
		digits++;
		if (digits > 5u)
		{
			return 0u;
		}
		text++;
	}

	if (digits == 0u)
	{
		return 0u;
	}

	if (*text == '.')
	{
		text++;
		if ((*text >= '0') && (*text <= '9'))
		{
			frac_part = (int32_t)(*text - '0');
			has_frac = 1u;
			text++;
		}
	}

	while ((*text == ' ') || (*text == '\t'))
	{
		text++;
	}

	if ((*text != '\0') && (has_frac == 0u))
	{
		return 0u;	/* 后面还有多余字符 */
	}

	result = int_part * 10 + frac_part;
	if (negative != 0u)
	{
		result = -result;
	}

	if ((result > 32767) || (result < -32768))
	{
		return 0u;
	}

	*value10 = (int16_t)result;
	return 1u;
}

/**
  * @brief  解析无符号整数
  */
static uint8_t parse_uint(const char *text, uint16_t *value)
{
	uint32_t result = 0;
	uint8_t  digits = 0;

	while ((*text == ' ') || (*text == '\t'))
	{
		text++;
	}

	while ((*text >= '0') && (*text <= '9'))
	{
		result = result * 10u + (uint32_t)(*text - '0');
		digits++;
		if (digits > 5u)
		{
			return 0u;
		}
		text++;
	}

	while ((*text == ' ') || (*text == '\t'))
	{
		text++;
	}

	if ((digits == 0u) || (*text != '\0') || (result > 65535u))
	{
		return 0u;
	}

	*value = (uint16_t)result;
	return 1u;
}

static void protocol_help(void)
{
	UART_SendString("# cmd: r=report  m=mute  v=version  h=help\r\n");
	UART_SendString("#      t=<celsius>  l=<percent>  p=<seconds>\r\n");
}

static void protocol_version(void)
{
	UART_SendString("# VER ");
	UART_SendString(PROTOCOL_VERSION);
	UART_SendString(" baud=");
	UART_SendUint(UART_BAUD_DEFAULT);
	UART_SendString(" period=");
	UART_SendUint(s_PeriodMs);
	UART_SendString(" th_temp=");
	UART_SendDec1(Alarm_GetTempHigh10());
	UART_SendString(" th_light=");
	UART_SendUint(Alarm_GetLightLow());
	UART_SendString("\r\n");
}

/**
  * @brief  执行一条下行命令
  */
static void protocol_exec(char *line)
{
	char    *arg;
	char    cmd;
	int16_t value10;
	uint16_t value;
	uint32_t now_ms;

	arg = skip_space(line);
	if (*arg == '\0')
	{
		return;
	}

	cmd = *arg;
	if ((cmd >= 'A') && (cmd <= 'Z'))
	{
		cmd = (char)(cmd - 'A' + 'a');
	}
	arg++;

	switch (cmd)
	{
		case 'h':
			protocol_help();
			break;

		case 'v':
			protocol_version();
			break;

		case 'r':
			now_ms = 0;
			UART_SendString("# report\r\n");
			Protocol_SendReport(now_ms);
			break;

		case 'm':
			Alarm_ToggleMute();
			UART_SendString("# mute=");
			UART_SendUint(Alarm_IsMuted());
			UART_SendString("\r\n");
			break;

		case 't':
			if (parse_fixed1(arg, &value10) == 0u)
			{
				UART_SendString("# ERR bad value, expect t=40 or t=40.5\r\n");
				break;
			}
			Alarm_SetTempHigh10(value10);
			UART_SendString("# th_temp=");
			UART_SendDec1(Alarm_GetTempHigh10());
			UART_SendString("\r\n");
			break;

		case 'l':
			if (parse_uint(arg, &value) == 0u)
			{
				UART_SendString("# ERR bad value, expect l=20\r\n");
				break;
			}
			if (value > 100u)
			{
				value = 100u;
			}
			Alarm_SetLightLow((uint8_t)value);
			UART_SendString("# th_light=");
			UART_SendUint(Alarm_GetLightLow());
			UART_SendString("\r\n");
			break;

		case 'p':
			if (parse_uint(arg, &value) == 0u)
			{
				UART_SendString("# ERR bad value, expect p=2\r\n");
				break;
			}
			if (((uint32_t)value * 1000u) < PROTOCOL_PERIOD_MIN)
			{
				s_PeriodMs = PROTOCOL_PERIOD_MIN;
			}
			else if (((uint32_t)value * 1000u) > PROTOCOL_PERIOD_MAX)
			{
				s_PeriodMs = PROTOCOL_PERIOD_MAX;
			}
			else
			{
				s_PeriodMs = (uint32_t)value * 1000u;
			}
			UART_SendString("# period=");
			UART_SendUint(s_PeriodMs);
			UART_SendString("\r\n");
			break;

		default:
			UART_SendString("# ERR unknown cmd, send h for help\r\n");
			break;
	}
}
