"""Grava a saida serial da LILYGO em arquivos de log e CSV.

Uso:
    python tools/serial_logger.py --port COM5
    python tools/serial_logger.py --port COM5 --start
"""

from __future__ import annotations

import argparse
import csv
import sys
from datetime import datetime
from pathlib import Path

import serial


DATA_COLUMNS = [
    "session",
    "cycle",
    "sample_index",
    "direction",
    "pwm",
    "adc",
    "scan_rate_mv_s",
    "interval_ms",
    "rssi_dbm",
    "snr_db",
]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Registra os pacotes recebidos pela LILYGO."
    )
    parser.add_argument("--port", required=True, help="Porta serial, por exemplo COM5")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path("data"),
        help="Diretorio de saida (padrao: data)",
    )
    parser.add_argument(
        "--start",
        action="store_true",
        help="Envia S para solicitar a varredura apos abrir a porta",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    args.output_dir.mkdir(parents=True, exist_ok=True)

    timestamp = datetime.now().strftime("%Y%m%d-%H%M%S")
    raw_path = args.output_dir / f"lora-raw-{timestamp}.log"
    csv_path = args.output_dir / f"potenciostato-{timestamp}.csv"

    try:
        port = serial.Serial(args.port, args.baud, timeout=1)
    except serial.SerialException as error:
        print(f"Nao foi possivel abrir {args.port}: {error}", file=sys.stderr)
        return 1

    print(f"Porta aberta: {args.port} @ {args.baud}")
    print(f"Log bruto: {raw_path}")
    print(f"Dados CSV: {csv_path}")
    print("Pressione Ctrl+C para encerrar.")

    try:
        with raw_path.open("w", encoding="utf-8", newline="") as raw_file, csv_path.open(
            "w", encoding="utf-8", newline=""
        ) as csv_file:
            writer = csv.writer(csv_file)
            writer.writerow(DATA_COLUMNS)

            if args.start:
                port.write(b"S\n")

            while True:
                raw = port.readline()
                if not raw:
                    continue

                line = raw.decode("utf-8", errors="replace").strip()
                if not line:
                    continue

                print(line)
                raw_file.write(line + "\n")
                raw_file.flush()

                if line.startswith("DATA,"):
                    values = line.split(",")[1:]
                    if len(values) == len(DATA_COLUMNS):
                        writer.writerow(values)
                        csv_file.flush()
                    else:
                        print(
                            f"Aviso: linha DATA com {len(values)} campos; "
                            f"esperados {len(DATA_COLUMNS)}.",
                            file=sys.stderr,
                        )
    except KeyboardInterrupt:
        print("\nEncerrado pelo usuario.")
    finally:
        port.close()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
