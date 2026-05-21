"""chart_generator.py — per-node + cross-node session SVG charts.

Two chart levels are generated for each benchmark:

  1. Per-node progression (results/<bench>/<compile>/<exec>/):
     One SVG per (compile, exec) node pair, plotting that node's metrics
     over commit history. Source: append-only history JSONs under
     benchmark/history/<bench>/<compile>/<exec>/.

  2. Cross-node session chart (results/<bench>/):
     A single SVG that groups data by batch_id (one benchmark run across
     ALL platforms = one session). Each metric panel shows one line per
     node, so you can see at a glance whether an optimisation helps or
     hurts every platform.

Champion commit is read from the per-benchmark champion.json
(results/<bench>/champion.json). Decision glyphs: green up-triangle for
KEEP, red down-triangle for DISCARD, yellow diamond for ESCALATE.

No external dependency — the SVG is hand-rolled so the harness keeps
working on minimal-rootfs nodes (matplotlib + numpy are not installed
on every exec target, and the CLAUDE.md "no pip installs" rule blocks
us from pulling them in)."""
import json
import os
import re

import benchmark_helpers as bh
from history_recorder import HISTORY_ROOT, cpu_model_slug

# Decision -> (glyph, fill colour). Glyphs are tiny SVG <polygon>s.
_DECISION_STYLE = {
    "KEEP":     ("triangle-up",   "#2ca02c"),
    "DISCARD":  ("triangle-down", "#d62728"),
    "ESCALATE": ("diamond",       "#dca100"),
}

_LINE_COLOURS = [
    "#1f77b4", "#ff7f0e", "#2ca02c", "#d62728", "#9467bd",
    "#8c564b", "#e377c2", "#7f7f7f", "#bcbd22", "#17becf",
]

_SLUG = re.compile(r"[^a-zA-Z0-9._-]+")


def _slug(s):
    return _SLUG.sub("-", (s or "unknown")).strip("-") or "unknown"


# ---- load history --------------------------------------------------------

def _load_history(benchmark):
    """Return list of (record_dict, path) sorted by started_utc."""
    bdir = os.path.join(HISTORY_ROOT, benchmark)
    if not os.path.isdir(bdir):
        return []
    out = []
    # Layout is nested now: <bench>/<compile>/<exec>/<stamp>.json -- walk
    # the whole subtree (was a flat listdir).
    for root, _dirs, files in os.walk(bdir):
        for name in files:
            if not name.endswith(".json"):
                continue
            p = os.path.join(root, name)
            try:
                with open(p) as f:
                    doc = json.load(f)
            except Exception:
                continue
            out.append((doc, p))
    out.sort(key=lambda r: r[0].get("started_utc", ""))
    return out


def _group_records(records):
    """{ (compile_label, exec_label): [records...] } chronological.
    One chart per compile/exec node pair -- remote nodes included, not
    just the local host."""
    groups = {}
    for doc, _p in records:
        comp, exe = bh.node_path_parts(doc.get("node"))
        groups.setdefault((comp, exe), []).append(doc)
    return groups


def _group_by_batch(records):
    """Group history records by batch_id (== one benchmark session across
    all execution nodes). Returns a list of (batch_id, started_utc,
    commit_short, records_in_batch) sorted by time.

    Each batch contains one record per node that produced data. Groups
    with no records are skipped."""
    batches = {}  # batch_id -> (started_utc, commit_short, [docs])
    for doc, _p in records:
        bid = doc.get("batch_id")
        if not bid:
            continue
        if bid not in batches:
            batches[bid] = (doc.get("started_utc", ""),
                            doc.get("commit_short") or (doc.get("commit") or "")[:7],
                            [])
        batches[bid][2].append(doc)
    out = sorted(batches.values(), key=lambda x: x[0])
    return [(b[0], b[1], b[2]) for b in out]  # (started_utc, commit_short, docs)


def _extract_session_series(batches, champion_commit_short):
    """Cross-node session series: one data point per (batch, node, metric).

    Returns ({metric_label -> [(idx, value, commit_short, decision)]},
             [commit_short...], better_map).

    metric_label = "<node>.<tool>.<metric>" (or
    "<node>.<tool>.<slice>.<metric>" for sub-benchmarks) so each node's
    line is distinct. X-axis is batch index (chronological)."""
    series = {}
    commits = []
    better_map = {}
    for idx, (_ts, commit_short, docs) in enumerate(batches):
        commits.append(commit_short or "")
        # Collect a single decision for this batch: prefer the champion
        # commit if it matches, else pick any non-null decision.
        batch_decision = None
        for d in docs:
            if (d.get("commit_short") or (d.get("commit") or "")[:7]) == champion_commit_short:
                batch_decision = "KEEP"
                break
        if batch_decision is None:
            for d in docs:
                dec = d.get("decision")
                if dec:
                    batch_decision = dec
                    break
        for d in docs:
            node = d.get("node") or "unknown"
            for tool, blk in (d.get("results") or {}).items():
                if not isinstance(blk, dict):
                    continue
                for mname, m in (blk.get("metrics") or {}).items():
                    v = m.get("median") if m.get("median") is not None else m.get("value")
                    if v is None:
                        continue
                    try:
                        v = float(v)
                    except (TypeError, ValueError):
                        continue
                    label = f"{node}.{tool}.{mname}"
                    series.setdefault(label, []).append(
                        (idx, v, commit_short, batch_decision))
                    if label not in better_map and m.get("better"):
                        better_map[label] = m["better"]
                for slice_label, smetrics in (blk.get("subbenchmarks") or {}).items():
                    if not isinstance(smetrics, dict):
                        continue
                    for mname, m in smetrics.items():
                        if not isinstance(m, dict):
                            continue
                        v = m.get("median") if m.get("median") is not None else m.get("value")
                        if v is None:
                            continue
                        try:
                            v = float(v)
                        except (TypeError, ValueError):
                            continue
                        label = f"{node}.{tool}.{slice_label}.{mname}"
                        series.setdefault(label, []).append(
                            (idx, v, commit_short, batch_decision))
                        if label not in better_map and m.get("better"):
                            better_map[label] = m["better"]
    return series, commits, better_map


def _extract_series(records):
    """Return ({metric_label -> [(idx, value, commit_short, decision)]},
              [commit_short...], better_map).

    metric_label is either "<tool>.<metric>" (aggregate metrics) or
    "<tool>.<slice>.<metric>" (per-sub-benchmark slice metrics, e.g.
    "rusage.10-players.cpu_percent"). Sub-benchmark series are the
    only place cpu_percent lives, so per-CLAUDE.md saturation
    detection has to walk both."""
    series = {}
    commits = []
    better_map = {}
    for idx, doc in enumerate(records):
        commits.append(doc.get("commit_short") or (doc.get("commit") or "")[:7])
        decision = doc.get("decision")
        for tool, blk in (doc.get("results") or {}).items():
            if not isinstance(blk, dict):
                continue
            metrics = blk.get("metrics") or {}
            for mname, m in metrics.items():
                v = m.get("median")
                if v is None:
                    v = m.get("value")
                if v is None:
                    continue
                try:
                    v = float(v)
                except (TypeError, ValueError):
                    continue
                label = f"{tool}.{mname}"
                series.setdefault(label, []).append((idx, v, commits[-1], decision))
                if label not in better_map and m.get("better"):
                    better_map[label] = m["better"]
            subs = blk.get("subbenchmarks") or {}
            for slice_label, smetrics in subs.items():
                if not isinstance(smetrics, dict):
                    continue
                for mname, m in smetrics.items():
                    if not isinstance(m, dict):
                        continue
                    v = m.get("median")
                    if v is None:
                        v = m.get("value")
                    if v is None:
                        continue
                    try:
                        v = float(v)
                    except (TypeError, ValueError):
                        continue
                    label = f"{tool}.{slice_label}.{mname}"
                    series.setdefault(label, []).append(
                        (idx, v, commits[-1], decision))
                    if label not in better_map and m.get("better"):
                        better_map[label] = m["better"]
    return series, commits, better_map


# ---- SVG primitives ------------------------------------------------------

def _esc(s):
    return (str(s).replace("&", "&amp;").replace("<", "&lt;")
                  .replace(">", "&gt;").replace('"', "&quot;"))


def _decision_glyph(cx, cy, decision, size=5):
    style = _DECISION_STYLE.get(decision)
    if not style:
        return ""
    shape, fill = style
    s = size
    if shape == "triangle-up":
        pts = f"{cx},{cy-s} {cx-s},{cy+s} {cx+s},{cy+s}"
    elif shape == "triangle-down":
        pts = f"{cx},{cy+s} {cx-s},{cy-s} {cx+s},{cy-s}"
    else:  # diamond
        pts = f"{cx},{cy-s} {cx+s},{cy} {cx},{cy+s} {cx-s},{cy}"
    return (f'<polygon points="{pts}" fill="{fill}" stroke="black" '
            f'stroke-width="0.5"/>')


CPU_SATURATION_THRESHOLD = 90.0


def _plot_panel(x0, y0, w, h, points_by_label, better_map, commits,
                champion_idx, title, panel_metric=None):
    """Render one metric panel. `points_by_label` -> [(idx,val,commit,dec)].

    `panel_metric` is the bare metric name (e.g. "cpu_percent"); when it
    equals "cpu_percent" we overlay a dashed 90% threshold line and
    paint a red halo around every point at or above it (per
    benchmark/CLAUDE.md "mark where the cpu was at 90% or more")."""
    parts = []
    parts.append(f'<g transform="translate({x0},{y0})">')
    # axes box
    parts.append(f'<rect x="0" y="0" width="{w}" height="{h}" '
                 f'fill="white" stroke="#888" stroke-width="0.5"/>')
    parts.append(f'<text x="4" y="-4" font-size="11" '
                 f'font-family="sans-serif">{_esc(title)}</text>')

    # y-range over all series in this panel
    all_vals = [p[1] for pts in points_by_label.values() for p in pts]
    if not all_vals:
        parts.append("</g>")
        return "".join(parts)
    ymin = min(all_vals); ymax = max(all_vals)
    if ymin == ymax:
        ymin -= 1.0; ymax += 1.0
    pad = (ymax - ymin) * 0.08
    ymin -= pad; ymax += pad

    is_cpu_panel = (panel_metric == "cpu_percent")
    if is_cpu_panel:
        # Force the y-range to span [0, 100] (with a hair of headroom)
        # so the 90% threshold is in the same visual place across every
        # chart -- otherwise a run that maxed out at 40% would put the
        # threshold off-screen above ymax.
        ymin = min(ymin, 0.0)
        ymax = max(ymax, 100.0) + 2.0

    n = max(len(commits), 1)
    def xpos(idx):
        if n == 1:
            return w / 2.0
        return (idx / float(n - 1)) * (w - 20) + 10
    def ypos(v):
        return h - ((v - ymin) / (ymax - ymin)) * (h - 10) - 5

    # y-axis tick labels (just min/max)
    parts.append(f'<text x="2" y="10" font-size="8" fill="#444" '
                 f'font-family="sans-serif">{ymax:.3g}</text>')
    parts.append(f'<text x="2" y="{h-2}" font-size="8" fill="#444" '
                 f'font-family="sans-serif">{ymin:.3g}</text>')

    # 90% saturation reference line (only on the cpu_percent panel)
    if is_cpu_panel:
        ty = ypos(CPU_SATURATION_THRESHOLD)
        parts.append(f'<line x1="0" y1="{ty:.2f}" x2="{w}" y2="{ty:.2f}" '
                     f'stroke="#d62728" stroke-width="0.7" '
                     f'stroke-dasharray="4,3" opacity="0.7"/>')
        parts.append(f'<text x="{w-4}" y="{ty-2:.2f}" font-size="8" '
                     f'fill="#d62728" text-anchor="end" '
                     f'font-family="sans-serif">90% saturation</text>')

    # champion vertical line
    if 0 <= champion_idx < n:
        cx = xpos(champion_idx)
        parts.append(f'<line x1="{cx}" y1="0" x2="{cx}" y2="{h}" '
                     f'stroke="#2ca02c" stroke-width="0.7" '
                     f'stroke-dasharray="3,2"/>')

    # one polyline per label
    for ci, (label, pts) in enumerate(sorted(points_by_label.items())):
        col = _LINE_COLOURS[ci % len(_LINE_COLOURS)]
        pts_str = " ".join(f"{xpos(i):.1f},{ypos(v):.2f}" for i, v, _, _ in pts)
        parts.append(f'<polyline points="{pts_str}" fill="none" '
                     f'stroke="{col}" stroke-width="1.2"/>')
        for i, v, _commit, dec in pts:
            cxp = xpos(i); cyp = ypos(v)
            # Saturation halo (drawn before the data dot so the dot
            # stays on top). Triggered on the cpu_percent panel only.
            if is_cpu_panel and v >= CPU_SATURATION_THRESHOLD:
                parts.append(f'<circle cx="{cxp:.1f}" cy="{cyp:.2f}" '
                             f'r="5" fill="none" stroke="#d62728" '
                             f'stroke-width="1.5"/>')
            parts.append(f'<circle cx="{cxp:.1f}" cy="{cyp:.2f}" '
                         f'r="2" fill="{col}"/>')
            parts.append(_decision_glyph(cxp, cyp - 8, dec))
        # legend
        bet = better_map.get(label)
        legend = label + (f" ({bet}=better)" if bet else "")
        parts.append(f'<text x="{w-4}" y="{12 + ci*11}" font-size="9" '
                     f'text-anchor="end" fill="{col}" '
                     f'font-family="sans-serif">{_esc(legend)}</text>')

    parts.append("</g>")
    return "".join(parts)


def _x_axis_labels(x0, y0, w, commits, champion_idx):
    parts = [f'<g transform="translate({x0},{y0})">']
    n = max(len(commits), 1)
    # show every commit if <=20, else thin out
    step = 1 if n <= 20 else max(1, n // 20)
    for i, c in enumerate(commits):
        if i % step != 0 and i != n - 1:
            continue
        x = (i / float(max(n - 1, 1))) * (w - 20) + 10 if n > 1 else w / 2.0
        fill = "#2ca02c" if i == champion_idx else "#222"
        parts.append(f'<text x="{x:.1f}" y="10" font-size="8" fill="{fill}" '
                     f'text-anchor="middle" font-family="monospace" '
                     f'transform="rotate(45 {x:.1f} 10)">{_esc(c)}</text>')
    parts.append("</g>")
    return "".join(parts)


# ---- champion lookup -----------------------------------------------------

def _champion_commit(benchmark):
    """Read the per-benchmark champion.json; return the commit short sha
    or None. The champion is per-benchmark (not per-node), so all charts
    share the same champion commit."""
    p = bh.champion_path(benchmark)
    if not os.path.isfile(p):
        return None
    try:
        with open(p) as f:
            doc = json.load(f)
        return (doc.get("commit") or "")[:7] or None
    except Exception:
        return None


def _find_champion_idx(records, node_label):
    """Index of the champion commit inside the ordered records list.
    Prefer the per-benchmark champion.json; fallback to last KEEP decision."""
    short = _champion_commit(records[0].get("benchmark", "")) if records else None
    if short:
        for i, doc in enumerate(records):
            if (doc.get("commit_short") or (doc.get("commit") or "")[:7]) == short:
                return i
    # fallback: last KEEP
    keep_idx = -1
    for i, doc in enumerate(records):
        if doc.get("decision") == "KEEP":
            keep_idx = i
    return keep_idx


# ---- top-level entry -----------------------------------------------------

PANEL_W = 720
PANEL_H = 120
PANEL_GAP = 18
LEFT_MARGIN = 40
TOP_MARGIN = 28
X_AXIS_H = 50


def _render_group(benchmark, comp, exe, records):
    series, commits, better_map = _extract_series(records)
    if not series:
        return None
    cpu_slug = cpu_model_slug(records[0].get("cpu_model")) if records else "unknown"
    node = records[0].get("node") or exe

    # group series by bare metric name (so one panel = one metric, all
    # tools and all sub-bench slices). For "<tool>.<metric>" the bare
    # name is the last segment; for "<tool>.<slice>.<metric>" we want
    # the trailing "<metric>" so every per-slice cpu_percent lands on
    # the cpu_percent panel.
    panels = {}
    for label, pts in series.items():
        bare = label.rsplit(".", 1)[1] if "." in label else label
        panels.setdefault(bare, {})[label] = pts

    arch = records[0].get("arch") or "unknown"
    champion_idx = _find_champion_idx(records, node)

    npanels = len(panels)
    height = TOP_MARGIN + npanels * (PANEL_H + PANEL_GAP) + X_AXIS_H
    width  = LEFT_MARGIN + PANEL_W + 20
    head = (f'<?xml version="1.0" encoding="UTF-8"?>\n'
            f'<svg xmlns="http://www.w3.org/2000/svg" version="1.1" '
            f'width="{width}" height="{height}" viewBox="0 0 {width} {height}">')
    body = []
    body.append(f'<text x="{LEFT_MARGIN}" y="18" font-size="13" '
                f'font-family="sans-serif" font-weight="bold">'
                f'{_esc(benchmark)} -- {_esc(comp)}/{_esc(exe)} '
                f'({_esc(cpu_slug)}, {_esc(arch)}, n={len(records)})</text>')
    y = TOP_MARGIN
    # Sort so cpu_percent (if present) sits at the top -- it's the
    # saturation tell-tale and the first thing reviewers look for.
    pkeys = sorted(panels.keys(), key=lambda k: (k != "cpu_percent", k))
    for pname in pkeys:
        body.append(_plot_panel(LEFT_MARGIN, y, PANEL_W, PANEL_H,
                                panels[pname], better_map, commits,
                                champion_idx, pname,
                                panel_metric=pname))
        y += PANEL_H + PANEL_GAP
    body.append(_x_axis_labels(LEFT_MARGIN, y, PANEL_W, commits, champion_idx))
    # decision legend (bottom-right)
    body.append(f'<g transform="translate({LEFT_MARGIN + PANEL_W - 220},'
                f'{height - 14})">')
    body.append('<text x="0" y="0" font-size="9" font-family="sans-serif">'
                'decisions:</text>')
    x = 60
    for decision in ("KEEP", "DISCARD", "ESCALATE"):
        body.append(_decision_glyph(x, -4, decision))
        body.append(f'<text x="{x+8}" y="0" font-size="9" '
                    f'font-family="sans-serif">{decision}</text>')
        x += 65
    body.append('</g>')
    return head + "".join(body) + "</svg>\n"


def _render_session_chart(benchmark, batches):
    """Cross-node session chart: plots every node's metrics per batch.

    Returns SVG string or None when no data. Champion commit is read
    from the per-benchmark champion.json."""
    if not batches:
        return None
    champ_short = _champion_commit(benchmark)
    series, commits, better_map = _extract_session_series(batches, champ_short)
    if not series:
        return None

    # Group series by bare metric name (last segment) so related metrics
    # share a panel.
    panels = {}
    for label, pts in series.items():
        bare = label.rsplit(".", 1)[1] if "." in label else label
        panels.setdefault(bare, {})[label] = pts

    # champion index: which batch has the champion commit
    champion_idx = -1
    if champ_short:
        for i, c in enumerate(commits):
            if c == champ_short:
                champion_idx = i
                break

    nbatches = len(batches)
    npanels = len(panels)
    height = TOP_MARGIN + npanels * (PANEL_H + PANEL_GAP) + X_AXIS_H
    width  = LEFT_MARGIN + PANEL_W + 20
    head = (f'<?xml version="1.0" encoding="UTF-8"?>\n'
            f'<svg xmlns="http://www.w3.org/2000/svg" version="1.1" '
            f'width="{width}" height="{height}" viewBox="0 0 {width} {height}">')
    body = []
    body.append(f'<text x="{LEFT_MARGIN}" y="18" font-size="13" '
                f'font-family="sans-serif" font-weight="bold">'
                f'{_esc(benchmark)} -- cross-node session chart '
                f'({nbatches} batches, {len(series)} series)</text>')
    y = TOP_MARGIN
    pkeys = sorted(panels.keys(), key=lambda k: (k != "cpu_percent", k))
    for pname in pkeys:
        body.append(_plot_panel(LEFT_MARGIN, y, PANEL_W, PANEL_H,
                                panels[pname], better_map, commits,
                                champion_idx, pname,
                                panel_metric=pname))
        y += PANEL_H + PANEL_GAP
    body.append(_x_axis_labels(LEFT_MARGIN, y, PANEL_W, commits, champion_idx))
    # decision legend
    body.append(f'<g transform="translate({LEFT_MARGIN + PANEL_W - 220},'
                f'{height - 14})">')
    body.append('<text x="0" y="0" font-size="9" font-family="sans-serif">'
                'decisions:</text>')
    x = 60
    for decision in ("KEEP", "DISCARD", "ESCALATE"):
        body.append(_decision_glyph(x, -4, decision))
        body.append(f'<text x="{x+8}" y="0" font-size="9" '
                    f'font-family="sans-serif">{decision}</text>')
        x += 65
    body.append('</g>')
    return head + "".join(body) + "</svg>\n"


def regenerate(benchmark, stamp=None):
    """Regenerate every chart for `benchmark` from history JSONs.

    Two chart levels are generated:
      1. Per-node progression charts:
           results/<bench>/<compile>/<exec>/champion.svg
           results/<bench>/<compile>/<exec>/candidate-<stamp>.svg
      2. Cross-node session chart (all platforms grouped by batch):
           results/<bench>/champion.svg
           results/<bench>/candidate-<stamp>.svg

    `champion.svg` is the always-current progression chart (champion
    commit marked). When `stamp` is given (a benchmark run passes its
    cand_stamp) the same chart is also frozen as
    `candidate-<stamp>.svg` so the chart at the moment of that run is
    preserved next to its candidate-<stamp>.json. Returns written
    paths."""
    records = _load_history(benchmark)
    if not records:
        return []
    written = []

    # 1) per-node charts (unchanged behaviour)
    groups = _group_records(records)
    for (comp, exe), recs in groups.items():
        svg = _render_group(benchmark, comp, exe, recs)
        if svg is None:
            continue
        outdir = os.path.join(bh.RESULTS, benchmark, comp, exe)
        os.makedirs(outdir, exist_ok=True)
        champ_p = os.path.join(outdir, "champion.svg")
        with open(champ_p, "w") as f:
            f.write(svg)
        written.append(champ_p)
        if stamp:
            cand_p = os.path.join(outdir, f"candidate-{stamp}.svg")
            with open(cand_p, "w") as f:
                f.write(svg)
            written.append(cand_p)

    # 2) cross-node session chart (one chart, all platforms)
    batches = _group_by_batch(records)
    svg = _render_session_chart(benchmark, batches)
    if svg:
        outdir = os.path.join(bh.RESULTS, benchmark)
        os.makedirs(outdir, exist_ok=True)
        champ_p = os.path.join(outdir, "champion.svg")
        with open(champ_p, "w") as f:
            f.write(svg)
        written.append(champ_p)
        if stamp:
            cand_p = os.path.join(outdir, f"candidate-{stamp}.svg")
            with open(cand_p, "w") as f:
                f.write(svg)
            written.append(cand_p)

    return written


# ---- self-test -----------------------------------------------------------

def regenerate_all(stamp=None):
    """Regenerate every benchmark's charts. Returns {bench: [paths]}.
    A benchmark with no history under benchmark/history/<bench>/ is
    silently skipped -- the harness may have created the dir but not
    yet recorded a run."""
    out = {}
    if not os.path.isdir(HISTORY_ROOT):
        return out
    for name in sorted(os.listdir(HISTORY_ROOT)):
        bdir = os.path.join(HISTORY_ROOT, name)
        if not os.path.isdir(bdir):
            continue
        out[name] = regenerate(name, stamp=stamp)
    return out


if __name__ == "__main__":
    import sys
    if len(sys.argv) > 1:
        # explicit benchmark name(s)
        for arg in sys.argv[1:]:
            paths = regenerate(arg)
            for p in paths:
                print(p)
            if not paths:
                print(f"[chart] no history for {arg}", file=sys.stderr)
    else:
        # no arg -> regenerate every benchmark we have history for
        any_written = False
        for bench, paths in regenerate_all().items():
            for p in paths:
                print(p)
                any_written = True
        if not any_written:
            print("[chart] no history found under benchmark/history/",
                  file=sys.stderr)
            sys.exit(1)
