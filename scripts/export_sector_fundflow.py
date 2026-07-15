#!/usr/bin/env python3
"""导出东财行业板块主力资金净流入/净流出 TOP20。

用法:
  python3 scripts/export_sector_fundflow.py
  python3 scripts/export_sector_fundflow.py --out data/fundflow
  python3 scripts/export_sector_fundflow.py --top 20 --no-print

说明:
  - f62 为主力净流入（元），属行情商估算，非交易所官方持仓。
  - 默认优先 push2 实时接口，失败则回退 push2delay。
"""

from __future__ import annotations

import argparse
import csv
import json
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
from datetime import datetime, timedelta, timezone
from pathlib import Path

BJ = timezone(timedelta(hours=8))

HOSTS = (
    "https://push2.eastmoney.com",
    "https://push2delay.eastmoney.com",
)

INDUSTRY_FS = "m:90+t:2+f:!50"
FIELDS = "f12,f14,f2,f3,f62,f66,f69,f72,f184,f20"


def bj_now() -> datetime:
    return datetime.now(BJ)


def http_get_json(url: str, timeout: float = 15.0) -> dict:
    req = urllib.request.Request(
        url,
        headers={
            "User-Agent": "Mozilla/5.0",
            "Referer": "https://quote.eastmoney.com/",
        },
    )
    with urllib.request.urlopen(req, timeout=timeout) as resp:
        raw = resp.read().decode("utf-8", errors="replace")
    if not raw or raw.strip()[:1] not in "{[":
        raise ValueError(f"非 JSON 响应: {raw[:120]!r}")
    return json.loads(raw)


def fetch_board_flow(top: int, inflow: bool, retries: int = 4) -> list[dict]:
    """拉取行业板块资金流列表。inflow=True 净流入降序，False 净流出（升序）。"""
    params = {
        "pn": "1",
        "pz": str(top),
        "po": "1" if inflow else "0",
        "np": "1",
        "fltt": "2",
        "invt": "2",
        "fid": "f62",
        "fs": INDUSTRY_FS,
        "fields": FIELDS,
    }
    query = urllib.parse.urlencode(params)
    last_err: Exception | None = None

    for host in HOSTS:
        url = f"{host}/api/qt/clist/get?{query}"
        for attempt in range(retries):
            try:
                data = http_get_json(url)
                rows = ((data.get("data") or {}).get("diff")) or []
                if not rows:
                    raise ValueError("返回列表为空")
                return rows
            except (urllib.error.URLError, TimeoutError, ValueError, json.JSONDecodeError) as e:
                last_err = e
                time.sleep(0.4 * (attempt + 1))
    raise RuntimeError(f"拉取板块资金流失败: {last_err}")


def normalize_rows(rows: list[dict], side: str) -> list[dict]:
    out = []
    for i, x in enumerate(rows, 1):
        net = float(x.get("f62") or 0)
        huge = float(x.get("f66") or 0)
        chg = x.get("f3")
        try:
            chg_f = float(chg)
        except (TypeError, ValueError):
            chg_f = None
        out.append(
            {
                "排名": i,
                "方向": side,
                "板块代码": x.get("f12") or "",
                "板块名称": x.get("f14") or "",
                "涨跌幅%": chg_f,
                "主力净流入_元": net,
                "主力净流入_亿": round(net / 1e8, 4),
                "超大单净额_亿": round(huge / 1e8, 4),
                "主力净流入占比%": x.get("f184"),
            }
        )
    return out


def write_csv(path: Path, rows: list[dict]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    if not rows:
        path.write_text("", encoding="utf-8")
        return
    fieldnames = list(rows[0].keys())
    with path.open("w", newline="", encoding="utf-8-sig") as f:
        w = csv.DictWriter(f, fieldnames=fieldnames)
        w.writeheader()
        w.writerows(rows)


def print_table(title: str, rows: list[dict]) -> None:
    print(f"\n=== {title} ===")
    print(f'{"排名":<4} {"代码":<8} {"名称":<12} {"涨跌%":>8} {"净流入(亿)":>12} {"超大单(亿)":>10}')
    for r in rows:
        chg = r["涨跌幅%"]
        chg_s = f"{chg:.2f}" if chg is not None else "-"
        print(
            f'{r["排名"]:<4} {r["板块代码"]:<8} {r["板块名称"]:<12} '
            f'{chg_s:>8} {r["主力净流入_亿"]:>+12.2f} {r["超大单净额_亿"]:>+10.2f}'
        )


def main() -> int:
    parser = argparse.ArgumentParser(description="导出行业主力资金净流入/流出 TOP N")
    parser.add_argument("--top", type=int, default=20, help="各榜条数，默认 20")
    parser.add_argument(
        "--out",
        type=Path,
        default=Path("data/fundflow"),
        help="输出目录，默认 data/fundflow",
    )
    parser.add_argument("--no-print", action="store_true", help="不打印表格，只写文件")
    args = parser.parse_args()

    if args.top <= 0:
        print("--top 必须 > 0", file=sys.stderr)
        return 2

    now = bj_now()
    stamp = now.strftime("%Y%m%d_%H%M%S")
    day = now.strftime("%Y-%m-%d")

    try:
        inflow_raw = fetch_board_flow(args.top, inflow=True)
        outflow_raw = fetch_board_flow(args.top, inflow=False)
    except RuntimeError as e:
        print(e, file=sys.stderr)
        return 1

    inflow = normalize_rows(inflow_raw, "净流入")
    outflow = normalize_rows(outflow_raw, "净流出")

    out_dir = args.out
    inflow_path = out_dir / f"industry_inflow_top{args.top}_{stamp}.csv"
    outflow_path = out_dir / f"industry_outflow_top{args.top}_{stamp}.csv"
    latest_in = out_dir / f"industry_inflow_top{args.top}_latest.csv"
    latest_out = out_dir / f"industry_outflow_top{args.top}_latest.csv"
    meta_path = out_dir / f"industry_fundflow_meta_{stamp}.json"

    write_csv(inflow_path, inflow)
    write_csv(outflow_path, outflow)
    write_csv(latest_in, inflow)
    write_csv(latest_out, outflow)

    meta = {
        "as_of": now.isoformat(),
        "trade_date_guess": day,
        "source": "eastmoney clist f62",
        "note": "主力净流入为估算字段，仅供观察",
        "top": args.top,
        "files": {
            "inflow": str(inflow_path),
            "outflow": str(outflow_path),
            "inflow_latest": str(latest_in),
            "outflow_latest": str(latest_out),
        },
    }
    out_dir.mkdir(parents=True, exist_ok=True)
    meta_path.write_text(json.dumps(meta, ensure_ascii=False, indent=2), encoding="utf-8")

    if not args.no_print:
        print(f"北京时间 {now.strftime('%Y-%m-%d %H:%M:%S')} | 行业资金流 TOP{args.top}")
        print_table(f"净流入 TOP{args.top}", inflow)
        print_table(f"净流出 TOP{args.top}", outflow)
        print("\n已写出:")
        print(f"  {inflow_path}")
        print(f"  {outflow_path}")
        print(f"  {latest_in}")
        print(f"  {latest_out}")
        print(f"  {meta_path}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
