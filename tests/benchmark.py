#!/usr/bin/env python3
"""FlowScope 4-architecture benchmark.

Drives each sub-server (server1-4) with a fixed number of concurrent
connections and measures, for that server in isolation:

    throughput    successful requests per second
    p95 latency   95th-percentile request round-trip time (ms)
    memory        peak resident set size of the server process (MB)
    cpu           average CPU utilisation of the server process (%)

Each server is started alone, benchmarked, then stopped — so the memory and
CPU figures belong to that architecture only.

Results are printed as a comparison table and written to
benchmark_results.md and benchmark_results.csv next to this script.

Usage:
    python3 benchmark.py                       # 100 connections, 5s load
    python3 benchmark.py --connections 50 --duration 3 --warmup 1
    python3 benchmark.py --servers server1,server4
"""
from __future__ import annotations

import argparse
import csv
import os
import pathlib
import statistics
import threading
import time

import protocol as proto
import serverctl
from client import FlowScopeClient

HERE = pathlib.Path(__file__).resolve().parent
_CLK_TCK = os.sysconf("SC_CLK_TCK")


# ---------------------------------------------------------------------------
# /proc sampling
# ---------------------------------------------------------------------------
def _read_rss_mb(pid: int) -> float:
    try:
        with open(f"/proc/{pid}/status") as f:
            for line in f:
                if line.startswith("VmRSS:"):
                    return int(line.split()[1]) / 1024.0      # kB -> MB
    except (FileNotFoundError, ProcessLookupError, ValueError):
        pass
    return 0.0


def _read_cpu_ticks(pid: int) -> int:
    """Total CPU ticks (utime + stime) for the whole process, all threads."""
    try:
        with open(f"/proc/{pid}/stat") as f:
            content = f.read()
        # the comm field can contain spaces/parens, so split after the last ')'
        tail = content.rsplit(")", 1)[1].split()
        # tail[0] is field 3 (state); utime is field 14 -> tail[11], stime -> tail[12]
        return int(tail[11]) + int(tail[12])
    except (FileNotFoundError, ProcessLookupError, IndexError, ValueError):
        return 0


def _rss_sampler(pid: int, stop_evt: threading.Event, out: dict) -> None:
    peak = 0.0
    while not stop_evt.is_set():
        peak = max(peak, _read_rss_mb(pid))
        time.sleep(0.05)
    out["peak_rss_mb"] = max(peak, _read_rss_mb(pid))


# ---------------------------------------------------------------------------
# load generation
# ---------------------------------------------------------------------------
def _load_worker(port: int, stop_evt: threading.Event,
                 latencies: list, counters: list, idx: int) -> None:
    """One persistent connection sending echo requests back-to-back until stopped."""
    samples: list[float] = []
    count = 0
    try:
        client = FlowScopeClient(port, timeout=10).connect()
    except Exception:                       # noqa: BLE001
        counters[idx] = 0
        latencies[idx] = samples
        return
    try:
        while not stop_evt.is_set():
            t0 = time.perf_counter()
            try:
                client.request({"cmd": "echo", "data": {"n": count}})
            except Exception:               # noqa: BLE001
                break
            samples.append((time.perf_counter() - t0) * 1000.0)   # ms
            count += 1
    finally:
        client.close()
    latencies[idx] = samples
    counters[idx] = count


def _run_load(port: int, connections: int, duration: float):
    latencies: list = [None] * connections
    counters: list = [0] * connections
    stop_evt = threading.Event()
    workers = [
        threading.Thread(target=_load_worker,
                         args=(port, stop_evt, latencies, counters, i))
        for i in range(connections)
    ]
    start = time.perf_counter()
    for w in workers:
        w.start()
    time.sleep(duration)
    stop_evt.set()
    for w in workers:
        w.join()
    elapsed = time.perf_counter() - start
    return latencies, counters, elapsed


def benchmark_server(name: str, port: int, pid: int,
                     connections: int, duration: float, warmup: float) -> dict:
    if warmup > 0:
        _run_load(port, min(connections, 20), warmup)

    rss_out: dict = {}
    rss_stop = threading.Event()
    rss_thread = threading.Thread(target=_rss_sampler, args=(pid, rss_stop, rss_out))

    cpu_before = _read_cpu_ticks(pid)
    rss_thread.start()
    latencies, counters, elapsed = _run_load(port, connections, duration)
    rss_stop.set()
    rss_thread.join()
    cpu_after = _read_cpu_ticks(pid)

    all_latencies = [x for sub in latencies if sub for x in sub]
    total_requests = sum(counters)

    throughput = total_requests / elapsed if elapsed > 0 else 0.0
    if len(all_latencies) >= 20:
        p95 = statistics.quantiles(all_latencies, n=20)[18]
    elif all_latencies:
        p95 = max(all_latencies)
    else:
        p95 = 0.0
    cpu_pct = (cpu_after - cpu_before) / _CLK_TCK / elapsed * 100.0 if elapsed > 0 else 0.0
    mem_mb = rss_out.get("peak_rss_mb", _read_rss_mb(pid))

    return {
        "throughput": throughput,
        "p95_ms": p95,
        "mem_mb": mem_mb,
        "cpu_pct": cpu_pct,
        "requests": total_requests,
        "elapsed": elapsed,
    }


# ---------------------------------------------------------------------------
# rendering
# ---------------------------------------------------------------------------
_METRICS = ("throughput", "p95_ms", "mem_mb", "cpu_pct")


def _labels(connections: int) -> dict:
    return {
        "throughput": f"처리량 (req/s) @ {connections} connections",
        "p95_ms": "p95 지연 (ms)",
        "mem_mb": "메모리 (MB)",
        "cpu_pct": "CPU (%)",
    }


def _fmt(metric: str, value: float) -> str:
    if metric == "throughput":
        return f"{value:,.0f}"
    if metric == "p95_ms":
        return f"{value:.2f}"
    return f"{value:.1f}"


def _col_title(name: str) -> str:
    # "server1" -> "Server 1", "central" -> "Central"
    return f"Server {name[-1]}" if name.startswith("server") else name.capitalize()


def render(results: dict, order: list, args) -> None:
    labels = _labels(args.connections)

    # ---- console table ----
    width = 36 + 14 * len(order)
    print("\n" + "=" * width)
    print("FlowScope 4-Architecture Benchmark")
    print(f"({args.connections} connections, {args.duration:.0f}s load, "
          f"{args.warmup:.0f}s warmup)")
    print("=" * width)
    header = f"{'지표':<36}" + "".join(f"{_col_title(n):>14}" for n in order)
    print(header)
    print("-" * width)
    for metric in _METRICS:
        row = f"{labels[metric]:<36}"
        for name in order:
            cell = _fmt(metric, results[name][metric]) if name in results else "-"
            row += f"{cell:>14}"
        print(row)
    print("=" * width)

    # ---- markdown ----
    md = [
        "# FlowScope 4-Architecture Benchmark",
        "",
        f"- connections: **{args.connections}**",
        f"- load: **{args.duration:.0f}s** (warmup {args.warmup:.0f}s)",
        f"- generated: {time.strftime('%Y-%m-%d %H:%M:%S')}",
        "",
        "테스트 통과/실패가 아닌 수치 표로 정리:",
        "",
        "| 지표 | " + " | ".join(
            f"{_col_title(n)}<br/>{proto.SERVERS[n]['arch']}" for n in order) + " |",
        "|" + "---|" * (len(order) + 1),
    ]
    for metric in _METRICS:
        cells = " | ".join(
            _fmt(metric, results[name][metric]) if name in results else "-"
            for name in order
        )
        md.append(f"| {labels[metric]} | {cells} |")
    md.append("")
    (HERE / "benchmark_results.md").write_text("\n".join(md) + "\n", encoding="utf-8")

    # ---- csv ----
    with open(HERE / "benchmark_results.csv", "w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(["metric"] + order)
        for metric in _METRICS:
            writer.writerow(
                [metric] + [f"{results[name][metric]:.4f}" if name in results else ""
                            for name in order]
            )

    print(f"\nWrote {HERE / 'benchmark_results.md'}")
    print(f"Wrote {HERE / 'benchmark_results.csv'}")


# ---------------------------------------------------------------------------
# main
# ---------------------------------------------------------------------------
def main() -> None:
    parser = argparse.ArgumentParser(description="FlowScope architecture benchmark")
    parser.add_argument("--connections", type=int, default=100,
                        help="concurrent connections (default: 100)")
    parser.add_argument("--duration", type=float, default=5.0,
                        help="load duration in seconds (default: 5)")
    parser.add_argument("--warmup", type=float, default=1.0,
                        help="warmup duration in seconds (default: 1)")
    parser.add_argument("--servers", default=",".join(proto.SUB_SERVERS),
                        help="comma-separated server names (default: server1..4)")
    args = parser.parse_args()

    order = [s.strip() for s in args.servers.split(",") if s.strip()]
    for name in order:
        if name not in proto.SERVERS:
            parser.error(f"unknown server '{name}' (choose from {list(proto.SERVERS)})")

    print("Building servers...")
    binaries = serverctl.build_all()

    results: dict = {}
    for name in order:
        port = proto.SERVERS[name]["port"]
        print(f"\nBenchmarking {name}  [{proto.SERVERS[name]['arch']}]  :{port}")
        handle = serverctl.ServerHandle(name, port, binaries[name]).start()
        try:
            res = benchmark_server(name, port, handle.pid,
                                   args.connections, args.duration, args.warmup)
            results[name] = res
            print(f"  {res['requests']:,} requests in {res['elapsed']:.1f}s  ->  "
                  f"{res['throughput']:,.0f} req/s | p95 {res['p95_ms']:.2f} ms | "
                  f"{res['mem_mb']:.1f} MB | {res['cpu_pct']:.1f}% CPU")
        finally:
            handle.stop()

    render(results, order, args)


if __name__ == "__main__":
    main()
