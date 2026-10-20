# 上位机使用说明

## 环境准备

```bash
pip install pyserial
```

串口读取需要 `pyserial`，其余全部使用 Python 标准库。Python 3.7+ 均可。

## 连接设备

1. STM32 的 PA9(TX) 接 USB-TTL 的 RX，PA10(RX) 接 USB-TTL 的 TX，GND 共地
2. 上电后设备默认每秒输出一帧 JSON
3. 查看串口号（Windows 设备管理器 → 端口），然后运行：

```bash
python tools/monitor.py --port COM3 --baud 115200 --csv data.csv
```

输出示例：

```
# Env Monitor 1.0 ready
# UART1 115200 8N1, send h for help
光照  50.0% (2048)  温度  25.3 C (2048)  状态 正常
光照  49.8% (2040)  温度  25.4 C (2051)  状态 正常
光照  12.0% ( 491)  温度  41.2 C (1520)  状态 温度超限
```

`--csv` 指定的文件会追加写入，字段包含主机时间、原始值、物理量与报警位。

## 无硬件时回放

把串口助手收到的内容另存为 `sample.log`，然后：

```bash
python tools/monitor.py --replay sample.log
```

回放模式只需要标准库，不需要 `pyserial`。

## 下发命令

任意串口助手（波特率 115200、加换行发送）即可：

```
v           查看版本与阈值
m           静音蜂鸣器
t 35.5      温度上限改为 35.5 C
l 30        光照下限改为 30 %
p 2         上报周期改为 2 秒
r           立即上报一帧
```

命令表与字段定义详见 [../docs/上位机协议.md](../docs/上位机协议.md)。
