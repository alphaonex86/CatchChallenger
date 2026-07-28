#!/usr/bin/env python3
"""svg.py -- render a benchmark chart ON DEMAND and open it.

A chart is a VIEW of the history JSONs, not data. Keeping 100+ champion SVGs on
disk (and in git) spends megabytes storing something that regenerates from those
JSONs in well under a second -- so this renders only the one you asked for, into
a temp file, and opens it. The JSONs stay the single source of truth.

    python3 svg.py                    # menu of every available chart
    python3 svg.py --list             # just print the menu, don't open
    python3 svg.py mapmanager         # first chart matching a substring
    python3 svg.py mapmanager fitxlan # narrow by node too
    python3 svg.py --out /tmp/x.svg mapmanager   # write there, don't open

Rendering reuses chart_generator's own functions, so a chart opened here is
byte-identical to the one it would have written.
"""
import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import benchmark_helpers as bh
import chart_generator as cg
from history_recorder import HISTORY_ROOT


def benchmarks():
    """Benchmarks that HAVE recorded history (the only ones a chart can be
    drawn from)."""
    if not os.path.isdir(HISTORY_ROOT):
        return []
    out = []
    for name in sorted(os.listdir(HISTORY_ROOT)):
        if os.path.isdir(os.path.join(HISTORY_ROOT, name)) and name.startswith("benchmark"):
            out.append(name)
    return out


def benchmarks_without_data():
    """Benchmark scripts that exist but have recorded nothing yet.

    Listed so a missing entry reads as "never run since the history was
    reset" instead of looking like the viewer lost it - benchmarks() can only
    see what is on disk, so a benchmark that has not run is simply absent."""
    here = os.path.dirname(os.path.abspath(__file__))
    known = set()
    for name in sorted(os.listdir(here)):
        if (name.startswith("benchmark") and name.endswith(".py")
                and name not in ("benchmark_helpers.py", "benchmark_remote.py")):
            known.add(os.path.splitext(name)[0].replace("_", ""))
    return sorted(known - set(benchmarks()))


def catalog():
    """[(label, kind, benchmark, comp, exe)] -- every chart that can be drawn.

    kind "session" is the cross-node chart for a benchmark (one line per node
    over the batches); kind "bynode" is the by-execution-node comparison (one
    boxplot per node per metric category, log scale, sorted by median - what
    used to be written as champion-by-execution-node.svg); kind "node" is one
    (compile, exec) pair."""
    entries = []
    for bench in benchmarks():
        records = cg._load_history(bench)
        if not records:
            continue
        entries.append((f"{bench}  [cross-node session]", "session", bench, None, None))
        entries.append((f"{bench}  [by execution node]", "bynode", bench, None, None))
        for (comp, exe) in sorted(cg._group_records(records)):
            entries.append((f"{bench}  {comp}/{exe}", "node", bench, comp, exe))
    return entries



# ---------------------------------------------------------------------------
# "simple" mode: WHICH panels a chart draws. VIEW-ONLY - series.json keeps every
# number, --full brings them all back. Each benchmark gets the metrics that
# answer the question it exists to answer, kept under SIMPLE_MAX panels so the
# chart stays readable (the full mapmanager chart is 42 panels / 7864px tall,
# botactions has 520 metric categories).
SIMPLE_MAX = 15

# mapmanager: tick latency at a WHITELIST of player counts - p5/p20/p50/p100/p200
# is enough to read the scaling curve, and the tail (p95) is where a stall shows.
# _median_tick_ns, _ticks, _ticks_per_s and _bytes_sent stay in --full.
_MAP_SIMPLE = re.compile(r"^p(5|20|50|100|200)_p95_tick_ns$")
# serversave: how long a save takes, full stop. Everything else (user_s, sys_s,
# cpu_percent, max_rss_kb, cache_bytes, binary_size_bytes, perf_*) is in --full.
_SAVE_SIMPLE = frozenset(("wall_s",))
# Benchmarks whose simple view wants the AGGREGATE only: serversave's single
# "default" slice mirrors the aggregate, so keeping both would draw the same
# number twice (default_wall_s next to wall_s).
_SIMPLE_AGGREGATE_ONLY = frozenset(("benchmarkserversave",))
# botactions: the latency TAIL per bot count, on the default (uncompressed,
# no-io_uring-variant) sweep - what a player actually feels. The io_uring and
# zstd6 sweeps are config experiments and stay in --full.
# botactions: the tick/latency picture, 5 panels.
#   * latency_p50_ns / latency_p95_ns  - median + tail of the whole run
#   * none-300-bots_latency_p50/p95_ns - the same under the heaviest load
#   * b300_user_s                      - CPU actually burned at 300 bots
# b<N>_wall_s is deliberately NOT here: the bot run is a fixed window, so it
# reads 30.01-30.05s on all 16 nodes and can never show a regression. Note the
# latency/loop families are recorded on the HOST only - no remote node reports
# one - so the by-execution-node chart falls back to the cumulated CPU panel.
_BOT_SIMPLE = re.compile(r"^(none-300-bots_latency_p(50|95)_ns|b300_user_s)$")
_BOT_SIMPLE_EXTRA = frozenset(("latency_p50_ns", "latency_p95_ns"))

# clientlatency: client-visible round-trip percentiles + jitter.
_CLIENT_SIMPLE = re.compile(r"latency_(p50|p95|p99|jitter)_ns$")

# Fallback for a benchmark with no explicit target: drop the bulky detail.
_DEFAULT_DROP_SUFFIXES = ("_bytes_sent", "_ticks", "_median_tick_ns")


def _simple_keep(bench, name):
    """True when `name` (the bare metric, i.e. the panel title) belongs in the
    simple view of `bench`."""
    n = str(name)
    if bench == "benchmarkmapmanager":
        return bool(_MAP_SIMPLE.search(n))
    if bench == "benchmarkserversave":
        return n in _SAVE_SIMPLE
    if bench == "benchmarkbotactions":
        return bool(_BOT_SIMPLE.match(n)) or n in _BOT_SIMPLE_EXTRA
    if bench == "benchmarkclientlatency":
        return bool(_CLIENT_SIMPLE.search(n))
    return not n.endswith(_DEFAULT_DROP_SUFFIXES)


def _drop_all_zero(records):
    """Copy of `records` without the metrics that are zero in EVERY run.

    A metric that only ever reads 0 is not a result - it is a workload that did
    not run (io_uring's 0 req/s) or a counter nothing touched. Plotted, it is a
    flat line on the axis that steals a panel and reads like a measurement. A
    metric that is zero in SOME runs keeps all its points: there the zero is
    information."""
    seen = {}
    for doc, _p in records:
        for blk in (doc.get("results") or {}).values():
            if not isinstance(blk, dict):
                continue
            for k, m in (blk.get("metrics") or {}).items():
                v = m.get("median") if m.get("median") is not None else m.get("value")
                seen[k] = seen.get(k, False) or bool(v)
            for sm in (blk.get("subbenchmarks") or {}).values():
                for k, m in (sm or {}).items():
                    v = m.get("median") if m.get("median") is not None else m.get("value")
                    seen[k] = seen.get(k, False) or bool(v)
    dead = {k for k, alive in seen.items() if not alive}
    if not dead:
        return records
    out = []
    for doc, path in records:
        res = {}
        for tool, blk in (doc.get("results") or {}).items():
            if not isinstance(blk, dict):
                continue
            nblk = {}
            met = {k: v for k, v in (blk.get("metrics") or {}).items() if k not in dead}
            if met:
                nblk["metrics"] = met
            subs = {}
            for sl, sm in (blk.get("subbenchmarks") or {}).items():
                kept = {k: v for k, v in (sm or {}).items() if k not in dead}
                if kept:
                    subs[sl] = kept
            if subs:
                nblk["subbenchmarks"] = subs
            if nblk:
                res[tool] = nblk
        if res:
            ndoc = dict(doc)
            ndoc["results"] = res
            out.append((ndoc, path))
    return out


def _prune_simple(records, bench):
    """Copy of `records` holding only the simple-view metrics of `bench`.

    Records are the per-run dicts rebuilt from the series, so this changes the
    chart only - nothing on disk is touched. When the target still yields more
    than SIMPLE_MAX panels the extra ones are dropped smallest-scale-first and
    REPORTED, never silently."""
    kept_names = set()
    out = []
    for doc, path in records:
        res = {}
        for tool, blk in (doc.get("results") or {}).items():
            if not isinstance(blk, dict):
                continue
            nblk = {}
            metrics = {k: v for k, v in (blk.get("metrics") or {}).items()
                       if _simple_keep(bench, k)}
            if metrics:
                nblk["metrics"] = metrics
            subs = {}
            for slice_label, smetrics in ({} if bench in _SIMPLE_AGGREGATE_ONLY
                                          else (blk.get("subbenchmarks") or {})).items():
                kept = {k: v for k, v in (smetrics or {}).items()
                        if _simple_keep(bench, k)}
                if kept:
                    subs[slice_label] = kept
            if subs:
                nblk["subbenchmarks"] = subs
            if nblk:
                res[tool] = nblk
                kept_names.update(nblk.get("metrics", {}))
                for sm in nblk.get("subbenchmarks", {}).values():
                    kept_names.update(sm)
        if res:
            ndoc = dict(doc)
            ndoc["results"] = res
            out.append((ndoc, path))
    if len(kept_names) > SIMPLE_MAX:
        # Sort by the numeric scale in the name (p5 < p10 < p300, 2-bots <
        # 300-bots) and drop from the small end: the big configurations are
        # where a regression shows.
        def scale(n):
            # A name with no number is an AGGREGATE (requests_per_s,
            # latency_p95_ns): it summarises the whole run, so it must be the
            # last thing dropped, not the first.
            m = re.search(r"(\d+)", n)
            return int(m.group(1)) if m else 10 ** 9
        ranked = sorted(kept_names, key=scale)
        drop = set(ranked[:len(kept_names) - SIMPLE_MAX])
        print("[svg] simple: %d panels > %d, dropping the smallest-scale %d: %s"
              % (len(kept_names), SIMPLE_MAX, len(drop), ", ".join(sorted(drop))),
              file=sys.stderr)
        for doc, _p in out:
            for blk in doc["results"].values():
                if "metrics" in blk:
                    blk["metrics"] = {k: v for k, v in blk["metrics"].items()
                                      if k not in drop}
                for sl, sm in list(blk.get("subbenchmarks", {}).items()):
                    blk["subbenchmarks"][sl] = {k: v for k, v in sm.items()
                                                if k not in drop}
    return out



def _drop_m32(records):
    """Records without the -m32 nodes.

    Every 64-bit box also runs a 32-bit sibling (atom-n455-m32, rpi-4-m32, ...),
    which doubles the node count on every cross-node chart for a comparison that
    is only occasionally interesting (32- vs 64-bit codegen). Off by default;
    "full with m32" brings them back. A chart of ONE node is never filtered - if
    you picked an m32 node explicitly, you get it."""
    return [(d, p) for d, p in records
            if not str(d.get("node") or "").endswith("-m32")]


def render(kind, bench, comp, exe, simple=False, with_m32=False):
    """Return SVG text, or None when there is nothing to draw."""
    records = cg._load_history(bench)
    if not records:
        return None
    # -m32 siblings only clutter the charts that aggregate nodes; a per-node
    # chart is a node the caller named, so leave it alone.
    # A metric that reads 0 in every run is hidden everywhere: it is not a result.
    records = _drop_all_zero(records) or records
    if not with_m32 and kind in ("session", "bynode"):
        filtered = _drop_m32(records)
        if filtered:
            records = filtered
    if simple:
        records = _prune_simple(records, bench)
        if not records:
            return None
    if kind == "session":
        return cg._render_session_chart(bench, cg._group_by_batch(records))
    if kind == "bynode":
        return cg._render_by_node_chart(bench, records)
    recs = cg._group_records(records).get((comp, exe))
    if not recs:
        return None
    return cg._render_group(bench, comp, exe, recs)


def open_file(path):
    """Best-effort open in the desktop viewer. On a headless box there is no
    opener, so just say where it is -- never fail the command over it."""
    for opener in ("xdg-open", "open"):
        if shutil.which(opener):
            try:
                subprocess.Popen([opener, path],
                                 stdout=subprocess.DEVNULL,
                                 stderr=subprocess.DEVNULL)
                return True
            except OSError:
                pass
    return False


# Sentinel returned by _prompt() when the user asks to step back a level.
_BACK = object()
SESSION_LABEL = "[cross-node session]"
BYNODE_LABEL = "[by execution node]"


def _prompt(title, options):
    """Numbered menu. Returns the chosen option, _BACK, or None to quit.

    A level with a single option auto-selects: there is nothing to decide, and
    several platforms hold exactly one node."""
    if len(options) == 1:
        print(f"{title}: {options[0]}   (only choice)")
        return options[0]
    print(f"\n{title}:")
    for i, option in enumerate(options, 1):
        print(f"{i:3d}) {option}")
    try:
        raw = input(f"[1-{len(options)}, b=back, q=quit]: ").strip().lower()
    except (EOFError, KeyboardInterrupt):
        print()
        return None
    if raw in ("q", "quit"):
        return None
    if raw in ("b", "back", ""):
        return _BACK
    if raw.isdigit() and 1 <= int(raw) <= len(options):
        return options[int(raw) - 1]
    print("[svg] not a valid choice")
    return _BACK


def interactive(entries):
    """Drill down benchmark -> platform -> node.

    86 charts in one flat list is unreadable; this asks the three questions
    that actually narrow it. The cross-node session chart sits at the platform
    level because that is what it is: the whole benchmark rather than one
    platform."""
    benches = sorted({e[2] for e in entries})
    # Say which benchmarks exist but have no history yet, so a short menu reads
    # as "these have not run since the reset" rather than "the viewer lost one".
    empty = benchmarks_without_data()
    if empty:
        print("\n(no data yet, never run since the history reset: %s)"
              % ", ".join(empty))
    while True:
        bench = _prompt("benchmark", benches)
        if bench is None:
            return None
        if bench is _BACK:
            if len(benches) == 1:
                return None      # nowhere to go back to
            continue
        has_session = any(e[2] == bench and e[1] == "session" for e in entries)
        has_bynode = any(e[2] == bench and e[1] == "bynode" for e in entries)
        platforms = sorted({e[3] for e in entries
                            if e[2] == bench and e[1] == "node"})
        # Both whole-benchmark charts sit at the platform level: they ARE the
        # benchmark across platforms, not one platform.
        options = (([SESSION_LABEL] if has_session else [])
                   + ([BYNODE_LABEL] if has_bynode else []) + platforms)
        if not options:
            print(f"[svg] nothing recorded for {bench}")
            continue
        while True:
            platform = _prompt(f"{bench}  ->  platform", options)
            if platform is None:
                return None
            if platform is _BACK:
                break
            if platform == SESSION_LABEL:
                return next(e for e in entries
                            if e[2] == bench and e[1] == "session")
            if platform == BYNODE_LABEL:
                return next(e for e in entries
                            if e[2] == bench and e[1] == "bynode")
            nodes = sorted({e[4] for e in entries
                            if e[2] == bench and e[3] == platform})
            node = _prompt(f"{bench} / {platform}  ->  node", nodes)
            if node is None:
                return None
            if node is _BACK:
                continue
            return next(e for e in entries if e[2] == bench
                        and e[3] == platform and e[4] == node)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("pattern", nargs="*",
                    help="substring(s) to match a chart; all must match")
    ap.add_argument("--list", action="store_true", help="print the menu and exit")
    ap.add_argument("--out", help="write the SVG here instead of a temp file")
    ap.add_argument("--simple", action="store_true",
                    help="only the panels that answer what this benchmark is "
                         "for, capped at %d; the JSON keeps everything "
                         "(DEFAULT when there is no terminal to ask on)"
                         % SIMPLE_MAX)
    ap.add_argument("--full", action="store_true",
                    help="every panel (overrides the non-interactive simple default)")
    ap.add_argument("--with-m32", action="store_true",
                    help="also plot the 32-bit (-m32) sibling nodes; off by "
                         "default, they double the node count for a 32-vs-64-bit "
                         "comparison that is rarely what you are looking at")
    args = ap.parse_args()

    entries = catalog()
    if not entries:
        print(f"[svg] no history under {HISTORY_ROOT} -- run a benchmark first",
              file=sys.stderr)
        return 1

    if args.list:
        for i, (label, _k, _b, _c, _e) in enumerate(entries, 1):
            print(f"{i:3d}) {label}")
        return 0

    chosen = None
    if args.pattern:
        matches = [e for e in entries
                   if all(p.lower() in e[0].lower() for p in args.pattern)]
        if not matches:
            print("[svg] nothing matches " + " ".join(args.pattern), file=sys.stderr)
            return 1
        if len(matches) > 1 and not sys.stdin.isatty():
            print(f"[svg] {len(matches)} charts match; first is {matches[0][0]}")
        chosen = matches[0]
    else:
        if not sys.stdin.isatty():
            print("[svg] no pattern and no terminal to prompt on; "
                  "use --list or pass a pattern", file=sys.stderr)
            return 2
        chosen = interactive(entries)
        if chosen is None:
            return 130

    label, kind, bench, comp, exe = chosen
    # Detail level LAST: it is the one question that does not narrow WHICH chart,
    # only how much of it is drawn. Explicit flags win; otherwise ask when there
    # is a terminal, and default to full when there is not.
    with_m32 = args.with_m32
    if args.simple:
        simple = True
    elif args.full:
        simple = False
    elif not sys.stdin.isatty():
        # No terminal to ask on (script, pipe, --out in a job): give the
        # readable chart, not the 42-panel one. --full overrides.
        simple = True
    else:
        pick = _prompt("chart", [
            "simple        (the benchmark's target, <=%d panels, no -m32)" % SIMPLE_MAX,
            "full          (every panel, no -m32)",
            "full with m32 (every panel + the 32-bit sibling nodes)"])
        if pick is None:
            return 130
        if pick is _BACK:
            simple = False
        else:
            simple = pick.startswith("simple")
            if pick.startswith("full with m32"):
                with_m32 = True
    svg = render(kind, bench, comp, exe, simple=simple, with_m32=with_m32)
    if svg is None:
        print(f"[svg] nothing to draw for {label}", file=sys.stderr)
        return 1

    if args.out:
        path = args.out
        with open(path, "w") as handle:
            handle.write(svg)
    else:
        fd, path = tempfile.mkstemp(prefix="ccbench-", suffix=".svg")
        with os.fdopen(fd, "w") as handle:
            handle.write(svg)

    print(f"[svg] {label} -> {path} ({len(svg)} bytes)")
    if not args.out and not open_file(path):
        print("[svg] no desktop opener found; open the path above yourself")
    return 0


if __name__ == "__main__":
    sys.exit(main())
