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
import shutil
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import benchmark_helpers as bh
import chart_generator as cg
from history_recorder import HISTORY_ROOT


def benchmarks():
    if not os.path.isdir(HISTORY_ROOT):
        return []
    out = []
    for name in sorted(os.listdir(HISTORY_ROOT)):
        if os.path.isdir(os.path.join(HISTORY_ROOT, name)) and name.startswith("benchmark"):
            out.append(name)
    return out


def catalog():
    """[(label, kind, benchmark, comp, exe)] -- every chart that can be drawn.

    kind "session" is the cross-node chart for a benchmark; kind "node" is one
    (compile, exec) pair."""
    entries = []
    for bench in benchmarks():
        records = cg._load_history(bench)
        if not records:
            continue
        entries.append((f"{bench}  [cross-node session]", "session", bench, None, None))
        for (comp, exe) in sorted(cg._group_records(records)):
            entries.append((f"{bench}  {comp}/{exe}", "node", bench, comp, exe))
    return entries


def render(kind, bench, comp, exe):
    """Return SVG text, or None when there is nothing to draw."""
    records = cg._load_history(bench)
    if not records:
        return None
    if kind == "session":
        return cg._render_session_chart(bench, cg._group_by_batch(records))
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
    while True:
        bench = _prompt("benchmark", benches)
        if bench is None:
            return None
        if bench is _BACK:
            if len(benches) == 1:
                return None      # nowhere to go back to
            continue
        has_session = any(e[2] == bench and e[1] == "session" for e in entries)
        platforms = sorted({e[3] for e in entries
                            if e[2] == bench and e[1] == "node"})
        options = ([SESSION_LABEL] if has_session else []) + platforms
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
    svg = render(kind, bench, comp, exe)
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
