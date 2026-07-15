#!/usr/bin/env python3
"""Export East Money industry fund-flow rankings with stock leaders."""

from __future__ import annotations

import argparse
import csv
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
from datetime import datetime
from pathlib import Path
from typing import Any


INDUSTRY_URL = (
    "https://push2.eastmoney.com/api/qt/clist/get?"
    "pn=1&pz={pz}&po=1&np=1&fltt=2&invt=2&fid={fid}&fs=m:90+t:2"
    "&fields=f12,f14,f2,f3,f62,f184,f66,f69,f72,f75,f78,f81,f84,f87,f124,f104,f105,f128"
)

STOCK_URL = (
    "https://push2.eastmoney.com/api/qt/clist/get?"
    "pn=1&pz=100&po=1&np=1&fltt=2&invt=2"
    "&fid={fid}&fs=b:{board_code}"
    "&fields=f12,f14,f2,f3,f6,f8,f62,f184"
)

USER_AGENT = (
    "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 "
    "(KHTML, like Gecko) Chrome/124.0.0.0 Safari/537.36"
)

HEADERS = {
    "User-Agent": USER_AGENT,
    "Referer": "https://data.eastmoney.com/bkzj/hy.html",
}


def display_width(text: str) -> int:
    width = 0
    for ch in text:
        # Full-width CJK and fullwidth forms take 2 columns in most terminals.
        if ("\u1100" <= ch <= "\u115f"
            or "\u2e80" <= ch <= "\u303e"
            or "\u3040" <= ch <= "\ua4cf"
            or "\uac00" <= ch <= "\ud7a3"
            or "\uf900" <= ch <= "\ufaff"
            or "\ufe10" <= ch <= "\ufe19"
            or "\ufe30" <= ch <= "\ufe6f"
            or "\uff00" <= ch <= "\uff60"
            or "\uffe0" <= ch <= "\uffe6"):
            width += 2
        else:
            width += 1
    return width


def pad_display(text: str, width: int, align: str = "left") -> str:
    text = "" if text is None else str(text)
    pad = max(0, width - display_width(text))
    if align == "right":
        return " " * pad + text
    if align == "center":
        left = pad // 2
        right = pad - left
        return " " * left + text + " " * right
    return text + " " * pad


def to_float(value: Any) -> float | None:
    if value in (None, "-", ""):
        return None
    try:
        return float(value)
    except (TypeError, ValueError):
        return None


def fmt_yi(value: Any, digits: int = 2) -> str:
    num = to_float(value)
    if num is None:
        return "-"
    return f"{num / 1e8:.{digits}f}"


def fmt_pct(value: Any, digits: int = 2) -> str:
    num = to_float(value)
    if num is None:
        return "-"
    return f"{num:.{digits}f}%"


def fmt_price(value: Any) -> str:
    num = to_float(value)
    if num is None:
        return "-"
    return f"{num:.2f}"


def http_get_json(url: str, retries: int = 3, sleep_s: float = 0.8) -> dict[str, Any]:
    last_error: Exception | None = None
    for attempt in range(1, retries + 1):
        try:
            req = urllib.request.Request(url, headers=HEADERS)
            with urllib.request.urlopen(req, timeout=20) as resp:
                raw = resp.read().decode("utf-8", errors="replace")
            import json

            return json.loads(raw)
        except (urllib.error.URLError, TimeoutError, ValueError) as exc:
            last_error = exc
            if attempt < retries:
                time.sleep(sleep_s * attempt)
    raise RuntimeError(f"request failed after {retries} retries: {last_error}")


def fetch_industry_rows(sort_field: str, limit: int) -> list[dict[str, Any]]:
    payload = http_get_json(INDUSTRY_URL.format(pz=max(limit, 50), fid=sort_field))
    diff = ((payload.get("data") or {}).get("diff")) or []
    return list(diff)[:limit]


def fetch_sector_stocks(board_code: str) -> list[dict[str, Any]]:
    """Fetch sector constituents once (sorted by amount desc on API)."""
    if not board_code:
        return []
    url = STOCK_URL.format(board_code=urllib.parse.quote(board_code), fid="f6")
    payload = http_get_json(url)
    return list(((payload.get("data") or {}).get("diff")) or [])


def format_stock_triple(
    stocks: list[dict[str, Any]],
    *,
    sort_key: str,
    metric: str,
    limit: int,
) -> tuple[str, str, str]:
    ranked = sorted(
        stocks,
        key=lambda s: to_float(s.get(sort_key)) or float("-inf"),
        reverse=True,
    )[:limit]
    names: list[str] = []
    codes: list[str] = []
    metrics: list[str] = []
    for s in ranked:
        names.append(str(s.get("f14") or "-"))
        codes.append(str(s.get("f12") or "-"))
        if metric == "amount":
            metrics.append(f"{fmt_yi(s.get('f6'))}亿")
        else:
            metrics.append(fmt_pct(s.get("f3")))
    return (" / ".join(names), " / ".join(codes), " / ".join(metrics))


def build_rows(
    industries: list[dict[str, Any]],
    leaders: int,
    sleep_s: float,
) -> list[dict[str, str]]:
    rows: list[dict[str, str]] = []
    total = len(industries)
    for idx, item in enumerate(industries, start=1):
        board_code = str(item.get("f12") or "")
        name = str(item.get("f14") or "-")
        print(f"[{idx}/{total}] {name} ({board_code}) ...", file=sys.stderr)

        stocks = fetch_sector_stocks(board_code)
        amt_names, amt_codes, amt_vals = format_stock_triple(
            stocks, sort_key="f6", metric="amount", limit=leaders
        )
        pct_names, pct_codes, pct_vals = format_stock_triple(
            stocks, sort_key="f3", metric="pct", limit=leaders
        )
        rows.append(
            {
                "排名": str(idx),
                "行业": name,
                "行业代码": board_code,
                "涨跌幅": fmt_pct(item.get("f3")),
                "主力净流入(亿)": fmt_yi(item.get("f62")),
                "主力净占比": fmt_pct(item.get("f184")),
                "超大单净流入(亿)": fmt_yi(item.get("f66")),
                "大单净流入(亿)": fmt_yi(item.get("f72")),
                "中单净流入(亿)": fmt_yi(item.get("f78")),
                "小单净流入(亿)": fmt_yi(item.get("f84")),
                "成交额TOP股票": amt_names,
                "成交额TOP代码": amt_codes,
                "成交额TOP(亿)": amt_vals,
                "涨幅TOP股票": pct_names,
                "涨幅TOP代码": pct_codes,
                "涨幅TOP": pct_vals,
                "领涨股": str(item.get("f128") or "-"),
                "领涨股涨跌幅": fmt_pct(item.get("f104")),
            }
        )
        time.sleep(sleep_s)
    return rows


def write_csv(path: Path, rows: list[dict[str, str]]) -> None:
    if not rows:
        raise RuntimeError("no rows to write")
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8-sig", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
        writer.writeheader()
        writer.writerows(rows)


def write_aligned_txt(path: Path, rows: list[dict[str, str]], title: str) -> None:
    """Write a terminal-friendly aligned table (CJK-aware)."""
    cols = [
        ("排名", 4, "right"),
        ("行业", 12, "left"),
        ("涨跌幅", 8, "right"),
        ("主力净流入(亿)", 14, "right"),
        ("主力净占比", 10, "right"),
        ("成交额TOP股票", 36, "left"),
        ("成交额TOP(亿)", 28, "left"),
        ("涨幅TOP股票", 36, "left"),
        ("涨幅TOP", 28, "left"),
    ]
    lines: list[str] = [title, ""]
    header = "  ".join(pad_display(c[0], c[1], c[2]) for c in cols)
    sep = "  ".join("-" * c[1] for c in cols)
    lines.extend([header, sep])
    for row in rows:
        line = "  ".join(
            pad_display(row.get(c[0], "-"), c[1], c[2]) for c in cols
        )
        lines.append(line)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def print_table(rows: list[dict[str, str]], title: str) -> None:
    cols = [
        ("排名", 4, "right"),
        ("行业", 12, "left"),
        ("涨跌幅", 8, "right"),
        ("主力净流入(亿)", 14, "right"),
        ("主力净占比", 10, "right"),
        ("成交额TOP股票", 36, "left"),
        ("成交额TOP(亿)", 28, "left"),
        ("涨幅TOP股票", 36, "left"),
        ("涨幅TOP", 28, "left"),
    ]
    print(title)
    print("  ".join(pad_display(c[0], c[1], c[2]) for c in cols))
    print("  ".join("-" * c[1] for c in cols))
    for row in rows:
        print("  ".join(pad_display(row.get(c[0], "-"), c[1], c[2]) for c in cols))
    print()


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Export East Money industry fund-flow TOP N with stock leaders"
    )
    parser.add_argument("--top", type=int, default=30, help="industry top N (default 30)")
    parser.add_argument(
        "--leaders",
        type=int,
        default=3,
        help="top stocks per industry for amount & pct change (default 3)",
    )
    parser.add_argument(
        "--outdir",
        type=Path,
        default=Path("data/fundflow"),
        help="output directory",
    )
    parser.add_argument(
        "--sleep",
        type=float,
        default=0.25,
        help="sleep seconds between sector stock requests",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    if args.top <= 0 or args.leaders <= 0:
        print("--top/--leaders must be > 0", file=sys.stderr)
        return 2

    stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    print("Fetching industry inflow ranking...", file=sys.stderr)
    inflow = fetch_industry_rows("f62", args.top)
    print("Fetching industry outflow ranking...", file=sys.stderr)
    outflow = fetch_industry_rows("f62", args.top)
    # outflow needs ascending net inflow; API fid=f62 with po=1 is desc.
    # Re-fetch with po=0 for true outflow ranking.
    outflow_url = (
        "https://push2.eastmoney.com/api/qt/clist/get?"
        f"pn=1&pz={max(args.top, 50)}&po=0&np=1&fltt=2&invt=2&fid=f62&fs=m:90+t:2"
        "&fields=f12,f14,f2,f3,f62,f184,f66,f69,f72,f75,f78,f81,f84,f87,f124,f104,f105,f128"
    )
    outflow_payload = http_get_json(outflow_url)
    outflow = list(((outflow_payload.get("data") or {}).get("diff")) or [])[: args.top]

    print("Building inflow rows + stock leaders...", file=sys.stderr)
    inflow_rows = build_rows(inflow, args.leaders, args.sleep)
    print("Building outflow rows + stock leaders...", file=sys.stderr)
    outflow_rows = build_rows(outflow, args.leaders, args.sleep)

    inflow_csv = args.outdir / f"industry_inflow_top{args.top}_{stamp}.csv"
    outflow_csv = args.outdir / f"industry_outflow_top{args.top}_{stamp}.csv"
    inflow_txt = args.outdir / f"industry_inflow_top{args.top}_{stamp}.txt"
    outflow_txt = args.outdir / f"industry_outflow_top{args.top}_{stamp}.txt"

    write_csv(inflow_csv, inflow_rows)
    write_csv(outflow_csv, outflow_rows)
    write_aligned_txt(
        inflow_txt,
        inflow_rows,
        f"行业主力净流入 TOP{args.top} | 每行业成交额TOP{args.leaders} + 涨幅TOP{args.leaders} | {stamp}",
    )
    write_aligned_txt(
        outflow_txt,
        outflow_rows,
        f"行业主力净流出 TOP{args.top} | 每行业成交额TOP{args.leaders} + 涨幅TOP{args.leaders} | {stamp}",
    )

    print_table(
        inflow_rows,
        f"行业主力净流入 TOP{args.top} | 成交额TOP{args.leaders} + 涨幅TOP{args.leaders}",
    )
    print_table(
        outflow_rows,
        f"行业主力净流出 TOP{args.top} | 成交额TOP{args.leaders} + 涨幅TOP{args.leaders}",
    )
    print(f"CSV: {inflow_csv}")
    print(f"CSV: {outflow_csv}")
    print(f"TXT: {inflow_txt}")
    print(f"TXT: {outflow_txt}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
