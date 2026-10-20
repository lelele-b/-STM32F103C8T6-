#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""多模态环境监测系统 上位机

从 STM32 的 USART1 读取单行 JSON 数据帧，实时打印并写入 CSV。

用法:
    pip install pyserial
    python tools/monitor.py --port COM3 --baud 115200 --csv data.csv
    python tools/monitor.py --replay sample.log      # 无硬件时回放日志

日志文件就是串口原始输出的文本，一行一帧。
"""

import argparse
import csv
import json
import sys
from datetime import datetime

CSV_FIELDS = [
    "host_time",
    "t_ms",
    "light_raw",
    "light_pct",
    "light_mv",
    "temp_raw",
    "temp_c",
    "temp_mv",
    "alarm",
    "muted",
    "th_temp",
    "th_light",
]


def parse_args():
    parser = argparse.ArgumentParser(description="STM32 环境监测系统上位机")
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument("--port", help="串口名，例如 COM3 或 /dev/ttyUSB0")
    source.add_argument("--replay", metavar="FILE", help="回放串口日志文件")
    parser.add_argument("--baud", type=int, default=115200, help="波特率，默认 115200")
    parser.add_argument("--csv", metavar="FILE", help="把解析结果追加写入 CSV")
    return parser.parse_args()


def iter_serial(port, baud):
    try:
        import serial
    except ImportError:
        sys.exit("需要 pyserial：pip install pyserial")

    with serial.Serial(port, baud, timeout=1) as ser:
        while True:
            raw = ser.readline()
            if not raw:
                continue
            yield raw.decode("utf-8", errors="replace").strip()


def iter_replay(path):
    with open(path, "r", encoding="utf-8", errors="replace") as handle:
        for line in handle:
            yield line.strip()


def flatten(frame):
    light = frame.get("light", {})
    temp = frame.get("temp", {})
    threshold = frame.get("th", {})
    return {
        "host_time": datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
        "t_ms": frame.get("t"),
        "light_raw": light.get("raw"),
        "light_pct": light.get("pct"),
        "light_mv": light.get("mv"),
        "temp_raw": temp.get("raw"),
        "temp_c": temp.get("c"),
        "temp_mv": temp.get("mv"),
        "alarm": frame.get("alarm"),
        "muted": frame.get("muted"),
        "th_temp": threshold.get("temp"),
        "th_light": threshold.get("light"),
    }


def describe_alarm(frame):
    bits = frame.get("alarm", 0)
    if not bits:
        return "正常"
    parts = []
    if bits & 0x01:
        parts.append("温度超限")
    if bits & 0x02:
        parts.append("光照不足")
    text = "+".join(parts) or f"bit{bits}"
    if frame.get("muted"):
        text += "(已静音)"
    return text


def main():
    args = parse_args()
    lines = iter_replay(args.replay) if args.replay else iter_serial(args.port, args.baud)

    writer = None
    handle = None
    if args.csv:
        handle = open(args.csv, "a", newline="", encoding="utf-8-sig")
        writer = csv.DictWriter(handle, fieldnames=CSV_FIELDS)
        if handle.tell() == 0:
            writer.writeheader()

    print("等待数据，Ctrl+C 退出 ...")
    count = 0
    try:
        for line in lines:
            if not line:
                continue
            if line.startswith("#"):
                print(line)
                continue
            try:
                frame = json.loads(line)
            except json.JSONDecodeError:
                continue

            row = flatten(frame)
            print(
                "光照 {pct:>5}% ({raw:>4})  温度 {temp:>5} C ({traw:>4})  状态 {alarm}".format(
                    pct=row["light_pct"],
                    raw=row["light_raw"],
                    temp=row["temp_c"],
                    traw=row["temp_raw"],
                    alarm=describe_alarm(frame),
                )
            )
            if writer is not None:
                writer.writerow(row)
                handle.flush()
            count += 1
    except KeyboardInterrupt:
        pass
    finally:
        if handle is not None:
            handle.close()
    print(f"\n共解析 {count} 帧")


if __name__ == "__main__":
    main()
