#include "stm32f10x.h"                  // Device header
#include "OLED.h"
#include "AD.h"
#include "Buzzer.h"
#include "Light.h"
#include "Temp.h"
#include "LED.h"
#include "UART.h"
#include "Tick.h"
#include "Alarm.h"
#include "Protocol.h"

#define SAMPLE_PERIOD_MS	100u	/* 采样与报警判断周期 */
#define DISPLAY_PERIOD_MS	200u	/* 屏幕刷新周期 */
#define LINE_WIDTH			16u		/* OLED 每行 16 个字符 */

static char    s_Line[LINE_WIDTH + 1u];
static uint8_t s_Col;

/* 把整行先填满空格，保证旧内容一定被覆盖 */
static void line_begin(void)
{
	uint32_t i;

	for (i = 0u; i < LINE_WIDTH; i++)
	{
		s_Line[i] = ' ';
	}
	s_Line[LINE_WIDTH] = '\0';

	s_Col = 0;
}

static void line_add(const char *text)
{
	while ((*text != '\0') && (s_Col < LINE_WIDTH))
	{
		s_Line[s_Col] = *text;
		s_Col++;
		text++;
	}
}

static void uint_to_str(uint32_t value, char *out)
{
	char    reversed[11];
	uint32_t count = 0;
	uint32_t i;

	if (value == 0u)
	{
		out[0] = '0';
		out[1] = '\0';
		return;
	}

	while ((value > 0u) && (count < 10u))
	{
		reversed[count] = (char)('0' + (uint8_t)(value % 10u));
		value = value / 10u;
		count++;
	}

	for (i = 0u; i < count; i++)
	{
		out[i] = reversed[count - 1u - i];
	}
	out[count] = '\0';
}

/* 一位小数定点转字符串，如 253 -> "25.3"、-53 -> "-5.3" */
static void dec1_to_str(int16_t value10, char *out)
{
	int32_t  v = (int32_t)value10;
	uint8_t  i = 0;
	uint32_t abs_v;
	uint32_t int_part;

	if (v < 0)
	{
		out[i] = '-';
		i++;
		v = -v;
	}

	abs_v = (uint32_t)v;
	int_part = abs_v / 10u;

	if (int_part >= 100u)
	{
		out[i] = (char)('0' + (uint8_t)(int_part / 100u));
		i++;
		out[i] = (char)('0' + (uint8_t)((int_part / 10u) % 10u));
		i++;
		out[i] = (char)('0' + (uint8_t)(int_part % 10u));
		i++;
	}
	else if (int_part >= 10u)
	{
		out[i] = (char)('0' + (uint8_t)(int_part / 10u));
		i++;
		out[i] = (char)('0' + (uint8_t)(int_part % 10u));
		i++;
	}
	else
	{
		out[i] = (char)('0' + (uint8_t)int_part);
		i++;
	}

	out[i] = '.';
	i++;
	out[i] = (char)('0' + (uint8_t)(abs_v % 10u));
	i++;
	out[i] = '\0';
}

static void display_update(int16_t temp10, uint16_t light_p10)
{
	char    buf[12];
	uint8_t bits = Alarm_GetBits();

	line_begin();
	line_add("Light:");
	dec1_to_str((int16_t)light_p10, buf);
	line_add(buf);
	line_add("%");
	OLED_ShowString(1, 1, s_Line);

	line_begin();
	line_add("Temp :");
	dec1_to_str(temp10, buf);
	line_add(buf);
	line_add("C");
	OLED_ShowString(2, 1, s_Line);

	line_begin();
	line_add("ADC:");
	uint_to_str((uint32_t)Light_GetRaw(), buf);
	line_add(buf);
	line_add("/");
	uint_to_str((uint32_t)Temp_GetRaw(), buf);
	line_add(buf);
	OLED_ShowString(3, 1, s_Line);

	line_begin();
	if (bits == 0u)
	{
		line_add((Alarm_IsMuted() != 0u) ? "Status Muted" : "Status Normal");
	}
	else
	{
		line_add("ALARM");
		if ((bits & ALARM_BIT_TEMP) != 0u)
		{
			line_add(" TEMP");
		}
		if ((bits & ALARM_BIT_LIGHT) != 0u)
		{
			line_add(" LIGHT");
		}
		if (Alarm_IsMuted() != 0u)
		{
			line_add(" MUTED");
		}
	}
	OLED_ShowString(4, 1, s_Line);
}

int main(void)
{
	uint32_t now;
	uint32_t last_sample = 0;
	uint32_t last_display = 0;
	uint32_t last_report = 0;
	int16_t  temp10 = 0;
	uint16_t light_p10 = 0;

	Tick_Init();
	OLED_Init();
	AD_Init();
	Buzzer_Init();
	LED_Init();
	UART_Init(UART_BAUD_DEFAULT);
	Alarm_Init();
	Protocol_Init();

	OLED_Clear();
	OLED_ShowString(1, 1, "Env Monitor V1.0");

	while (1)
	{
		now = Tick_GetMs();

		/* 先处理上行命令，保证阈值修改能立即生效 */
		Protocol_Poll(now);

		if ((uint32_t)(now - last_sample) >= SAMPLE_PERIOD_MS)
		{
			last_sample = now;
			temp10 = Temp_GetCelsius10();
			light_p10 = Light_GetPercent10();
			Alarm_Poll(now, temp10, Light_GetPercent());
		}

		LED_Update(now, (Alarm_GetBits() != 0u) ? 1u : 0u, Alarm_IsMuted());

		if ((uint32_t)(now - last_report) >= Protocol_GetPeriodMs())
		{
			last_report = now;
			Protocol_SendReport(now);
		}

		if ((uint32_t)(now - last_display) >= DISPLAY_PERIOD_MS)
		{
			last_display = now;
			display_update(temp10, light_p10);
		}
	}
}
