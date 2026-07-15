#!/usr/bin/env python3
"""导出东财行业板块主力资金净流入/净流出 TOP N，并附带龙头股。

用法:
  python3 scripts/export_sector_fundflow.py
  python3 scripts/export_sector_fundflow.py --out data/fundflow
  python3 scripts/export_sector_fundflow.py --top 30 --leaders 3

说明:
  - f62 为主力净流入（元），属行情商估算，非交易所官方持仓。
  - 默认优先 push2 实时接口，失败则回退 push2delay。
  - 每个行业导出：成交额(f6)最大 N 只 + 涨幅(f3)最大 N 只（默认各 3）。
  - 终端/TXT 使用中文显示宽度对齐；CSV 供 Excel。
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
BOARD_FIELDS = "f12,f14,f2,f3,f62,f66,f69,f72,f184,f20"
STOCK_FIELDS = "f12,f14,f2,f3,f6,f20"


def bj_now() -> datetime:
    return datetime.now(BJ)


def display_width(text: str) -> int:
    """Approximate terminal display width (CJK = 2 columns)."""
    width = 0
    for ch in str(text):
        o = ord(ch)
        if (
            0x1100 <= o <= 0x115F
            or 0x2E80 <= o <= 0x303E
            or 0x3040 <= o <= 0xA4CF
            or 0xAC00 <= o <= 0xD7A3
            or 0xF900 <= o <= 0xFAFF
            or 0xFE10 <= o <= 0xFE19
            or 0xFE30 <= o <= 0xFE6F
            or 0xFF00 <= o <= 0xFF60
            or 0xFFE0 <= o <= 0xFFE6
        ):
            width += 2
        else:
            width += 1
    return width


def pad_display(text: object, width: int, align: str = "left") -> str:
    s = "" if text is None else str(text)
    pad = max(0, width - display_width(s))
    if align == "right":
        return " " * pad + s
    if align == "center":
        left = pad // 2
        return " " * left + s + " " * (pad - left)
    return s + " " * pad


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


def clist_get(params: dict, retries: int = 4) -> list[dict]:
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
            except urllib.error.HTTPError as e:
                last_err = e
                # 502/503: switch host immediately
                if e.code in (502, 503, 504):
                    break
                time.sleep(0.35 * (attempt + 1))
            except (urllib.error.URLError, TimeoutError, ValueError, json.JSONDecodeError) as e:
                last_err = e
                time.sleep(0.35 * (attempt + 1))
    raise RuntimeError(f"clist 请求失败: {last_err}")


def fetch_board_flow(top: int, inflow: bool) -> list[dict]:
    """拉取行业板块资金流列表。inflow=True 净流入降序，False 净流出（升序）。"""
    return clist_get(
        {
            "pn": "1",
            "pz": str(top),
            "po": "1" if inflow else "0",
            "np": "1",
            "fltt": "2",
            "invt": "2",
            "fid": "f62",
            "fs": INDUSTRY_FS,
            "fields": BOARD_FIELDS,
        }
    )


def _num(value: object) -> float | None:
    if value in (None, "-", ""):
        return None
    try:
        return float(value)  # type: ignore[arg-type]
    except (TypeError, ValueError):
        return None


def normalize_stock(x: dict) -> dict:
    amt = _num(x.get("f6")) or 0.0
    return {
        "code": str(x.get("f12") or ""),
        "name": str(x.get("f14") or ""),
        "price": _num(x.get("f2")),
        "change_pct": _num(x.get("f3")),
        "amount_yi": round(amt / 1e8, 4),
    }


def fetch_board_top_stocks(board_code: str, n: int, fid: str) -> list[dict]:
    """按指定字段取板块前 N（fid=f6 成交额，fid=f3 涨幅）。"""
    rows = clist_get(
        {
            "pn": "1",
            "pz": str(max(n, 1)),
            "po": "1",
            "np": "1",
            "fltt": "2",
            "invt": "2",
            "fid": fid,
            "fs": f"b:{board_code}+f:!50",
            "fields": STOCK_FIELDS,
        }
    )
    out = []
    for i, x in enumerate(rows[:n], 1):
        item = normalize_stock(x)
        item["rank"] = i
        out.append(item)
    return out


def fill_leader_cols(row: dict, prefix: str, leaders: list[dict], n: int) -> None:
    for j in range(1, n + 1):
        s = leaders[j - 1] if j <= len(leaders) else None
        row[f"{prefix}第{j}_代码"] = s["code"] if s else ""
        row[f"{prefix}第{j}_名称"] = s["name"] if s else ""
        row[f"{prefix}第{j}_现价"] = s["price"] if s else ""
        row[f"{prefix}第{j}_涨跌%"] = s["change_pct"] if s else ""
        row[f"{prefix}第{j}_成交额_亿"] = s["amount_yi"] if s else ""


def enrich_boards(rows: list[dict], side: str, leaders: int) -> tuple[list[dict], list[dict]]:
    """返回 (板块行, 成分股明细行)。"""
    board_out: list[dict] = []
    stock_out: list[dict] = []

    for i, x in enumerate(rows, 1):
        board_code = str(x.get("f12") or "")
        board_name = str(x.get("f14") or "")
        net = float(x.get("f62") or 0)
        huge = float(x.get("f66") or 0)
        chg_f = _num(x.get("f3"))

        amount_tops: list[dict] = []
        pct_tops: list[dict] = []
        if board_code and leaders > 0:
            try:
                amount_tops = fetch_board_top_stocks(board_code, leaders, "f6")
                time.sleep(0.05)
                pct_tops = fetch_board_top_stocks(board_code, leaders, "f3")
                time.sleep(0.05)
            except RuntimeError:
                amount_tops, pct_tops = [], []

        row = {
            "排名": i,
            "方向": side,
            "板块代码": board_code,
            "板块名称": board_name,
            "涨跌幅%": chg_f,
            "主力净流入_元": net,
            "主力净流入_亿": round(net / 1e8, 4),
            "超大单净额_亿": round(huge / 1e8, 4),
            "主力净流入占比%": x.get("f184"),
        }
        fill_leader_cols(row, "成交额", amount_tops, leaders)
        fill_leader_cols(row, "涨幅", pct_tops, leaders)
        board_out.append(row)

        for s in amount_tops:
            stock_out.append(
                {
                    "方向": side,
                    "板块排名": i,
                    "板块代码": board_code,
                    "板块名称": board_name,
                    "板块涨跌幅%": chg_f,
                    "板块主力净流入_亿": round(net / 1e8, 4),
                    "排序口径": "成交额",
                    "股票排名": s["rank"],
                    "股票代码": s["code"],
                    "股票名称": s["name"],
                    "现价": s["price"],
                    "涨跌幅%": s["change_pct"],
                    "成交额_亿": s["amount_yi"],
                }
            )
        for s in pct_tops:
            stock_out.append(
                {
                    "方向": side,
                    "板块排名": i,
                    "板块代码": board_code,
                    "板块名称": board_name,
                    "板块涨跌幅%": chg_f,
                    "板块主力净流入_亿": round(net / 1e8, 4),
                    "排序口径": "涨幅",
                    "股票排名": s["rank"],
                    "股票代码": s["code"],
                    "股票名称": s["name"],
                    "现价": s["price"],
                    "涨跌幅%": s["change_pct"],
                    "成交额_亿": s["amount_yi"],
                }
            )

    return board_out, stock_out


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


def _fmt_leader_cell(row: dict, prefix: str, j: int) -> str:
    name = row.get(f"{prefix}第{j}_名称") or "-"
    code = row.get(f"{prefix}第{j}_代码") or ""
    if prefix == "成交额":
        amt = row.get(f"{prefix}第{j}_成交额_亿")
        if amt in ("", None):
            return "-"
        return f"{name}({code}) {float(amt):.1f}亿"
    pct = row.get(f"{prefix}第{j}_涨跌%")
    if pct in ("", None):
        return "-"
    return f"{name}({code}) {float(pct):+.2f}%"


def format_aligned_lines(title: str, rows: list[dict], leaders: int) -> list[str]:
    cols = [
        ("排名", 4, "right"),
        ("代码", 8, "left"),
        ("名称", 20, "left"),
        ("涨跌%", 7, "right"),
        ("净流入(亿)", 10, "right"),
    ]
    # Dynamic leader columns: keep readable width.
    for j in range(1, leaders + 1):
        cols.append((f"成交额#{j}", 30, "left"))
    for j in range(1, leaders + 1):
        cols.append((f"涨幅#{j}", 28, "left"))

    lines = [title, ""]
    header_cells = []
    for key, width, align in cols:
        header_cells.append(pad_display(key, width, align))
    lines.append("  ".join(header_cells))
    lines.append("  ".join("-" * w for _, w, _ in cols))

    for r in rows:
        chg = r["涨跌幅%"]
        chg_s = f"{chg:.2f}" if chg is not None else "-"
        net_s = f"{r['主力净流入_亿']:+.2f}"
        values = [
            str(r["排名"]),
            r["板块代码"],
            r["板块名称"],
            chg_s,
            net_s,
        ]
        for j in range(1, leaders + 1):
            values.append(_fmt_leader_cell(r, "成交额", j))
        for j in range(1, leaders + 1):
            values.append(_fmt_leader_cell(r, "涨幅", j))

        line_cells = []
        for (key, width, align), val in zip(cols, values):
            line_cells.append(pad_display(val, width, align))
        lines.append("  ".join(line_cells))
    return lines


def print_table(title: str, rows: list[dict], leaders: int) -> None:
    for line in format_aligned_lines(f"=== {title} ===", rows, leaders):
        print(line)
    print()


def write_aligned_txt(path: Path, title: str, rows: list[dict], leaders: int) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    lines = format_aligned_lines(title, rows, leaders)
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="导出行业主力资金净流入/流出 TOP N，并附成交额/涨幅最大股票"
    )
    parser.add_argument("--top", type=int, default=30, help="各榜行业条数，默认 30")
    parser.add_argument(
        "--leaders",
        type=int,
        default=3,
        help="每个行业取成交额最大 / 涨幅最大的股票数，默认 3",
    )
    parser.add_argument(
        "--out",
        type=Path,
        default=Path("data/fundflow"),
        help="输出目录，默认 data/fundflow",
    )
    parser.add_argument("--no-print", action="store_true", help="不打印表格，只写文件")
    args = parser.parse_args()

    if args.top <= 0 or args.leaders < 0:
        print("--top 必须 > 0，--leaders 必须 >= 0", file=sys.stderr)
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

    print("正在补充各行业成交额TOP / 涨幅TOP股票...", file=sys.stderr)
    inflow, inflow_stocks = enrich_boards(inflow_raw, "净流入", args.leaders)
    outflow, outflow_stocks = enrich_boards(outflow_raw, "净流出", args.leaders)

    out_dir = args.out
    inflow_path = out_dir / f"industry_inflow_top{args.top}_{stamp}.csv"
    outflow_path = out_dir / f"industry_outflow_top{args.top}_{stamp}.csv"
    stocks_path = out_dir / f"industry_top_stocks_{stamp}.csv"
    inflow_txt = out_dir / f"industry_inflow_top{args.top}_{stamp}.txt"
    outflow_txt = out_dir / f"industry_outflow_top{args.top}_{stamp}.txt"
    latest_in = out_dir / f"industry_inflow_top{args.top}_latest.csv"
    latest_out = out_dir / f"industry_outflow_top{args.top}_latest.csv"
    latest_stocks = out_dir / "industry_top_stocks_latest.csv"
    latest_in_txt = out_dir / f"industry_inflow_top{args.top}_latest.txt"
    latest_out_txt = out_dir / f"industry_outflow_top{args.top}_latest.txt"
    meta_path = out_dir / f"industry_fundflow_meta_{stamp}.json"

    write_csv(inflow_path, inflow)
    write_csv(outflow_path, outflow)
    write_csv(stocks_path, inflow_stocks + outflow_stocks)
    write_csv(latest_in, inflow)
    write_csv(latest_out, outflow)
    write_csv(latest_stocks, inflow_stocks + outflow_stocks)

    title_in = (
        f"行业主力净流入 TOP{args.top} | 成交额TOP{args.leaders} + 涨幅TOP{args.leaders} | {stamp}"
    )
    title_out = (
        f"行业主力净流出 TOP{args.top} | 成交额TOP{args.leaders} + 涨幅TOP{args.leaders} | {stamp}"
    )
    write_aligned_txt(inflow_txt, title_in, inflow, args.leaders)
    write_aligned_txt(outflow_txt, title_out, outflow, args.leaders)
    write_aligned_txt(latest_in_txt, title_in, inflow, args.leaders)
    write_aligned_txt(latest_out_txt, title_out, outflow, args.leaders)

    meta = {
        "as_of": now.isoformat(),
        "trade_date_guess": day,
        "source": "eastmoney clist f62 + board constituents by f6/f3",
        "note": "主力净流入为估算字段；成交额龙头按 f6，涨幅龙头按 f3；TXT 为中文对齐文本",
        "top": args.top,
        "leaders": args.leaders,
        "files": {
            "inflow": str(inflow_path),
            "outflow": str(outflow_path),
            "top_stocks": str(stocks_path),
            "inflow_txt": str(inflow_txt),
            "outflow_txt": str(outflow_txt),
            "inflow_latest": str(latest_in),
            "outflow_latest": str(latest_out),
            "top_stocks_latest": str(latest_stocks),
            "inflow_latest_txt": str(latest_in_txt),
            "outflow_latest_txt": str(latest_out_txt),
        },
    }
    out_dir.mkdir(parents=True, exist_ok=True)
    meta_path.write_text(json.dumps(meta, ensure_ascii=False, indent=2), encoding="utf-8")

    if not args.no_print:
        print(
            f"北京时间 {now.strftime('%Y-%m-%d %H:%M:%S')} | "
            f"行业资金流 TOP{args.top} + 成交额TOP{args.leaders} + 涨幅TOP{args.leaders}"
        )
        print_table(f"净流入 TOP{args.top}", inflow, args.leaders)
        print_table(f"净流出 TOP{args.top}", outflow, args.leaders)
        print("\n已写出:")
        print(f"  {inflow_path}")
        print(f"  {outflow_path}")
        print(f"  {stocks_path}")
        print(f"  {inflow_txt}")
        print(f"  {outflow_txt}")
        print(f"  {latest_in}")
        print(f"  {latest_out}")
        print(f"  {latest_stocks}")
        print(f"  {latest_in_txt}")
        print(f"  {latest_out_txt}")
        print(f"  {meta_path}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
