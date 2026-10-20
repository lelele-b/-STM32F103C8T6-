# 基于 STM32F103C8T6 的多模态环境监测系统

基于 STM32F103C8T6（Blue Pill）最小系统板的环境监测终端：ADC + DMA 连续采集光照与温度两路模拟量，OLED 本地显示，USART1 实时向上位机上报 JSON 数据帧，并支持超限声光报警与串口命令在线调参。

## 功能特性

- **双通道 ADC + DMA**：ADC1 扫描 PA4/PA5 两路，DMA 循环搬运，CPU 不参与采样
- **光照通道**：原始值、毫伏、百分比三种量纲
- **温度通道**：NTC 10K B3950 热敏电阻，B 值公式换算摄氏度
- **毫秒时基**：SysTick 1 ms 中断，全部任务非阻塞调度，无 `delay_ms` 空转
- **OLED 显示**：128×64 四行，显示光照、温度、原始 ADC、系统状态
- **串口上报**：USART1 115200 8N1，默认 1 Hz 输出单行 JSON，上位机直接 `json.loads`
- **声光报警**：温度超上限、光照低于下限两条报警，蜂鸣器 + PA0 指示灯联动，带回差与连续 3 次确认
- **在线调参**：串口命令可改报警阈值、上报周期、静音蜂鸣器（掉电不保存）

## 硬件连接

| 功能 | 引脚 | 说明 |
| --- | --- | --- |
| 光照传感器 | PA4 | ADC1_IN4 |
| 温度传感器（NTC） | PA5 | ADC1_IN5 |
| 串口 TX | PA9 | USART1_TX，接 USB-TTL 的 RX |
| 串口 RX | PA10 | USART1_RX，接 USB-TTL 的 TX |
| 指示灯 | PA0 | 推挽输出，串限流电阻（极性见 `Hardware/LED.h`） |
| 蜂鸣器 | PB12 | 低电平响 |
| OLED SCL | PB8 | 软件 I2C，开漏输出 |
| OLED SDA | PB9 | 软件 I2C，开漏输出 |

> PA0 同时是 WKUP 与 ADC1_IN0，本项目作普通推挽输出使用，与 ADC 的 PA4/PA5 不冲突。

## 目录结构

```
├── Hardware/           外设驱动与应用模块
│   ├── AD.c/.h         ADC1 扫描 + DMA 采集，输出 AD_Value[2]
│   ├── Light.c/.h      光照通道换算（原始值 / 毫伏 / 百分比）
│   ├── Temp.c/.h       温度通道换算（NTC B 值公式）
│   ├── LED.c/.h        PA0 指示灯状态机
│   ├── UART.c/.h       USART1 收发 + 接收环形缓冲
│   ├── Tick.c/.h       SysTick 毫秒时基
│   ├── Alarm.c/.h      阈值判断、回差、静音、蜂鸣器驱动
│   ├── Protocol.c/.h   JSON 上报帧与串口命令解析
│   ├── OLED.c/.h       OLED 驱动与字库
│   └── Buzzer.c/.h     蜂鸣器驱动
├── User/               main.c、中断服务、标准外设库配置
├── Library/            STM32F10x 标准外设库
├── Start/              启动文件与 CMSIS
├── tools/              上位机脚本
├── docs/               串口协议说明
└── project.uvprojx     Keil MDK 工程
```

## 编译

1. Keil MDK 打开 `project.uvprojx`（需安装 `Keil.STM32F1xx_DFP` 器件支持包与 ARMCC V5 编译器）
2. `Project → Rebuild all target files`
3. 生成的固件位于 `Objects/project.hex`，用 ST-Link / DAP-Link 烧录

当前工程编译结果：`0 Error(s), 0 Warning(s)`，`Code=10158  RO-data=1854  RW-data=72  ZI-data=1832`。

## 串口协议

完整字段说明见 [docs/上位机协议.md](docs/上位机协议.md)。设备每秒输出一行 JSON：

```json
{"t":12345,"light":{"raw":2048,"pct":50.0,"mv":1650},"temp":{"raw":2048,"c":25.3,"mv":1650},"alarm":0,"muted":0,"th":{"temp":40.0,"light":20}}
```

以 `#` 开头的行是设备的调试/应答信息，上位机可直接忽略。

下行命令（以换行结尾）：

| 命令 | 说明 |
| --- | --- |
| `h` | 帮助 |
| `v` | 版本与当前参数 |
| `r` | 立即上报一帧 |
| `m` | 蜂鸣器静音 / 取消静音 |
| `t=40.5` | 设置温度报警上限（摄氏度，可带一位小数） |
| `l=20` | 设置光照报警下限（百分比，0–100） |
| `p=2` | 设置上报周期（秒，1–60） |

## 上位机

```bash
pip install pyserial
python tools/monitor.py --port COM3 --baud 115200 --csv data.csv
```

脚本逐行解析 JSON、实时打印，并把每条记录追加到 CSV。没有硬件时可用日志回放：

```bash
python tools/monitor.py --replay sample.log
```

## 指示灯状态

| 系统状态 | PA0 指示灯 |
| --- | --- |
| 无报警 | 1 Hz 心跳（每周期亮 50 ms） |
| 报警且未静音 | 5 Hz 快闪 |
| 报警但已静音 | 常亮 |

## 参数调节

| 参数 | 位置 | 默认值 |
| --- | --- | --- |
| 光照方向（采样值随光照升降） | `Hardware/Light.c` 的 `LIGHT_RAW_RISES_WITH_LIGHT` | `1` |
| NTC 上拉电阻 / 25 ℃ 阻值 / B 值 | `Hardware/Temp.c` | 10 kΩ / 10 kΩ / 3950 |
| 指示灯极性 | `Hardware/LED.h` 的 `LED_ACTIVE_HIGH` | `1`（高电平点亮） |
| 串口波特率 | `Hardware/UART.h` 的 `UART_BAUD_DEFAULT` | 115200 |
| 温度报警上限 | `Hardware/Alarm.h` 的 `ALARM_TEMP_HIGH10_DEFAULT` | 40.0 ℃ |
| 光照报警下限 | `Hardware/Alarm.h` 的 `ALARM_LIGHT_LOW_DEFAULT` | 20 % |

若温度读数明显偏离实际值，请优先核对 `TEMP_PULLUP_R` / `TEMP_NTC_R25` / `TEMP_NTC_B` 三个常数与手上传感器的数据手册是否一致。

## 说明

- 报警阈值与上报周期只保存在 RAM 中，复位后恢复默认值
- 上位机与设备共用一条串口链路：设备发送 JSON 帧，同时接收命令，二者互不阻塞
- 编译产物（`Objects/`、`Listings/`）与 Keil 用户配置已加入 `.gitignore`
