#!/usr/bin/env python3
"""Проверка USB-протокола (PING → PONG 1). Надёжнее, чем .NET SerialPort на части ПК."""
import argparse
import sys
import time

try:
    import serial
except ImportError:
    print("Установите: pip install pyserial", file=sys.stderr)
    sys.exit(1)


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("port", nargs="?", default="COM14", help="COM-порт, напр. COM14 или /dev/ttyACM0")
    ap.add_argument("-b", "--baud", type=int, default=115200)
    args = ap.parse_args()

    ser = serial.Serial(args.port, args.baud, timeout=1.5)
    try:
        time.sleep(2.0)
        if ser.in_waiting:
            print("drain:", ser.read(ser.in_waiting).decode("ascii", errors="replace"))

        ser.write(b"PING\n")
        ser.flush()
        time.sleep(0.15)
        line = ser.readline()
        print("PING ->", repr(line))

        ser.write(b"M1 0\n")
        ser.flush()
        time.sleep(0.15)
        print("M1 0 ->", repr(ser.readline()))
    finally:
        ser.close()


if __name__ == "__main__":
    main()
