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
import math
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

# Right-of-panel gutter for the value boxplot + its clustered value labels.
BOX_W = 150
# Values within +-10% are the benchmark noise band (see benchmark/CLAUDE.md
# decision matrix) -- collapse them into one labelled entry on the chart so
# near-equal points don't render as N separate numbers.
VALUE_CLUSTER_TOL = 0.10


def _cluster_values(values, tol=VALUE_CLUSTER_TOL):
    """Collapse near-equal values into +-tol (relative) clusters. Sorts the
    values, then grows a cluster while the next value is within tol of the
    cluster's anchor (first member). Returns a list of
    (median, count, min, max) ascending. Used for the chart value labels and
    the boxplot annotation so a noise-band of points reads as one number."""
    vals = sorted(v for v in values if v is not None)
    if not vals:
        return []
    clusters = []
    cur = [vals[0]]
    i = 1
    while i < len(vals):
        v = vals[i]
        anchor = cur[0]
        denom = abs(anchor) if anchor != 0 else 1.0
        if abs(v - anchor) / denom <= tol:
            cur.append(v)
        else:
            clusters.append(cur)
            cur = [v]
        i += 1
    clusters.append(cur)
    out = []
    j = 0
    while j < len(clusters):
        c = clusters[j]
        out.append((c[len(c) // 2], len(c), c[0], c[-1]))
        j += 1
    return out


def _quantile(sorted_vals, q):
    """Linear-interpolated quantile of an already-sorted non-empty list."""
    if len(sorted_vals) == 1:
        return sorted_vals[0]
    pos = q * (len(sorted_vals) - 1)
    lo = int(pos)
    frac = pos - lo
    if lo + 1 < len(sorted_vals):
        return sorted_vals[lo] * (1 - frac) + sorted_vals[lo + 1] * frac
    return sorted_vals[lo]


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
    # "lower/higher=better" is a property of the METRIC (one per panel), so
    # show it ONCE in the title -- not repeated on every series legend.
    panel_better = None
    for _lbl in points_by_label:
        if better_map.get(_lbl):
            panel_better = better_map[_lbl]
            break
    title_txt = title + (f" ({panel_better}=better)" if panel_better else "")
    parts.append(f'<text x="4" y="-4" font-size="11" '
                 f'font-family="sans-serif">{_esc(title_txt)}</text>')

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
        # legend: just the series label -- the lower/higher=better direction
        # is shown once in the panel title, not per series.
        parts.append(f'<text x="{w-4}" y="{12 + ci*11}" font-size="9" '
                     f'text-anchor="end" fill="{col}" '
                     f'font-family="sans-serif">{_esc(label)}</text>')

    # ---- right-of-panel boxplot over ALL plotted values -----------------
    # Shares this panel's y-scale (ypos). Box = Q1..Q3, line = median,
    # whiskers = min..max. To its right, the value list collapsed into
    # +-10% clusters (the noise band): one "value xN" line per cluster, so
    # near-equal points read as a single number instead of N overlapping
    # labels.
    svals = sorted(all_vals)
    bx = w + 16          # box body left edge (in the right gutter)
    bw = 16              # box body width
    cxb = bx + bw / 2.0
    q1 = _quantile(svals, 0.25)
    med = _quantile(svals, 0.50)
    q3 = _quantile(svals, 0.75)
    vmin = svals[0]; vmax = svals[-1]
    parts.append(f'<text x="{bx}" y="-4" font-size="8" fill="#444" '
                 f'font-family="sans-serif">distribution</text>')
    # whisker min..max
    parts.append(f'<line x1="{cxb:.1f}" y1="{ypos(vmax):.2f}" x2="{cxb:.1f}" '
                 f'y2="{ypos(vmin):.2f}" stroke="#666" stroke-width="0.8"/>')
    # min/max caps
    for cap in (vmin, vmax):
        parts.append(f'<line x1="{bx+3:.1f}" y1="{ypos(cap):.2f}" '
                     f'x2="{bx+bw-3:.1f}" y2="{ypos(cap):.2f}" '
                     f'stroke="#666" stroke-width="0.8"/>')
    # box Q1..Q3
    by = ypos(q3); bh_box = ypos(q1) - ypos(q3)
    parts.append(f'<rect x="{bx:.1f}" y="{by:.2f}" width="{bw}" '
                 f'height="{max(bh_box,0.5):.2f}" fill="#1f77b4" '
                 f'fill-opacity="0.18" stroke="#1f77b4" stroke-width="0.8"/>')
    # median
    parts.append(f'<line x1="{bx:.1f}" y1="{ypos(med):.2f}" '
                 f'x2="{bx+bw:.1f}" y2="{ypos(med):.2f}" '
                 f'stroke="#1f77b4" stroke-width="1.4"/>')
    # clustered value labels (number + count) to the right of the box
    lx = bx + bw + 5
    last_ly = None
    for cval, cnt, _lo, _hi in _cluster_values(all_vals):
        ly = ypos(cval)
        # nudge apart if two clusters would overlap their text vertically
        if last_ly is not None and abs(ly - last_ly) < 9:
            ly = last_ly + 9
        last_ly = ly
        parts.append(f'<line x1="{bx+bw:.1f}" y1="{ypos(cval):.2f}" '
                     f'x2="{lx-1:.1f}" y2="{ly:.2f}" stroke="#999" '
                     f'stroke-width="0.5"/>')
        txt = f"{cval:.3g}" + (f" ×{cnt}" if cnt > 1 else "")
        parts.append(f'<text x="{lx:.1f}" y="{ly+3:.1f}" font-size="8" '
                     f'fill="#333" font-family="sans-serif">{_esc(txt)}</text>')

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
    width  = LEFT_MARGIN + PANEL_W + BOX_W + 20
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
    width  = LEFT_MARGIN + PANEL_W + BOX_W + 20
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


def _by_node_metrics(records):
    """For the by-execution-node comparison: take each node's MOST RECENT
    history record and pull out every metric value. Returns
    ({metric_label -> {node -> value}}, {metric_label -> better}).

    metric_label is the flat metric name (results.<tool>.metrics.<name>, e.g.
    b128_invol_ctx) or "<slice>.<name>" for sub-benchmark metrics -- the same
    data the per-benchmark champion.svg plots, but indexed metric->node so we
    can compare systems against each other on one axis."""
    latest = {}   # node -> (started_utc, doc)
    for doc, _p in records:
        node = doc.get("node") or "unknown"
        ts = doc.get("started_utc", "")
        if node not in latest or ts > latest[node][0]:
            latest[node] = (ts, doc)
    out = {}
    better = {}
    for node, (_ts, doc) in latest.items():
        for tool, blk in (doc.get("results") or {}).items():
            if not isinstance(blk, dict):
                continue
            for mname, m in (blk.get("metrics") or {}).items():
                v = m.get("median")
                if v is None:
                    v = m.get("value")
                if v is None:
                    continue
                try:
                    v = float(v)
                except (TypeError, ValueError):
                    continue
                out.setdefault(mname, {})[node] = v
                if mname not in better and m.get("better"):
                    better[mname] = m["better"]
            for slice_label, smetrics in (blk.get("subbenchmarks") or {}).items():
                if not isinstance(smetrics, dict):
                    continue
                # Normalise the slice to the flat "b<N>_<metric>" naming used
                # by the champion comparison (e.g. "128-bots" -> "b128_"),
                # so a subbench value DEDUPES against the matching flat metric
                # instead of producing a second near-identical panel. Adds
                # only subbench-exclusive metrics (e.g. cpu_percent).
                mnum = re.match(r"^(\d+)", slice_label)
                prefix = ("b" + mnum.group(1)) if mnum else _slug(slice_label)
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
                    key = f"{prefix}_{mname}"
                    slot = out.setdefault(key, {})
                    if node in slot:   # already supplied by the flat metric
                        continue
                    slot[node] = v
                    if key not in better and m.get("better"):
                        better[key] = m["better"]
    return out, better


def _plot_by_node_panel(x0, y0, w, h, node_vals, title, better):
    """One metric panel comparing EXECUTION NODES. x = nodes sorted by value
    (ascending = best-first for a lower=better metric), y = value on a LOG
    scale (linear fallback if any value <= 0, since log10 needs > 0). Each
    node is a dot + short label; a right-of-panel boxplot summarises the
    spread, with the value list collapsed into +-10% clusters."""
    parts = [f'<g transform="translate({x0},{y0})">']
    parts.append(f'<rect x="0" y="0" width="{w}" height="{h}" '
                 f'fill="white" stroke="#888" stroke-width="0.5"/>')
    items = sorted(node_vals.items(), key=lambda kv: kv[1])  # ascending value
    vals = [v for _n, v in items]
    title_txt = title + (f" ({better}=better)" if better else "")
    # log only when every value is strictly positive
    uselog = all(v > 0 for v in vals)
    parts.append(f'<text x="4" y="-4" font-size="11" font-family="sans-serif">'
                 f'{_esc(title_txt)}{"  [log]" if uselog else ""}</text>')
    vmin = min(vals); vmax = max(vals)
    if uselog:
        lmin = math.log10(vmin); lmax = math.log10(vmax)
        if lmin == lmax:
            lmin -= 0.5; lmax += 0.5
        def ypos(v):
            return h - ((math.log10(v) - lmin) / (lmax - lmin)) * (h - 14) - 6
    else:
        if vmin == vmax:
            vmin -= 1.0; vmax += 1.0
        pad = (vmax - vmin) * 0.08
        lo = vmin - pad; hi = vmax + pad
        def ypos(v):
            return h - ((v - lo) / (hi - lo)) * (h - 14) - 6
    # y range labels
    parts.append(f'<text x="2" y="10" font-size="8" fill="#444" '
                 f'font-family="sans-serif">{vmax:.3g}</text>')
    parts.append(f'<text x="2" y="{h-2}" font-size="8" fill="#444" '
                 f'font-family="sans-serif">{vmin:.3g}</text>')
    n = len(items)
    def xpos(i):
        if n == 1:
            return w / 2.0
        return (i / float(n - 1)) * (w - 40) + 20
    # connecting ramp + dots + node labels
    pts_str = " ".join(f"{xpos(i):.1f},{ypos(v):.2f}"
                       for i, (_n, v) in enumerate(items))
    parts.append(f'<polyline points="{pts_str}" fill="none" stroke="#1f77b4" '
                 f'stroke-width="1"/>')
    i = 0
    while i < n:
        node, v = items[i]
        cx = xpos(i); cy = ypos(v)
        parts.append(f'<circle cx="{cx:.1f}" cy="{cy:.2f}" r="2.5" '
                     f'fill="#1f77b4"/>')
        # node label, rotated to avoid overlap
        parts.append(f'<text x="{cx:.1f}" y="{h-1:.1f}" font-size="7" '
                     f'fill="#333" font-family="sans-serif" '
                     f'transform="rotate(-30 {cx:.1f} {h-1:.1f})">'
                     f'{_esc(node)}</text>')
        i += 1
    # right boxplot over node values + +-10% clustered value labels
    svals = sorted(vals)
    bx = w + 16; bw = 16; cxb = bx + bw / 2.0
    q1 = _quantile(svals, 0.25); med = _quantile(svals, 0.50)
    q3 = _quantile(svals, 0.75)
    parts.append(f'<text x="{bx}" y="-4" font-size="8" fill="#444" '
                 f'font-family="sans-serif">spread</text>')
    parts.append(f'<line x1="{cxb:.1f}" y1="{ypos(svals[-1]):.2f}" '
                 f'x2="{cxb:.1f}" y2="{ypos(svals[0]):.2f}" stroke="#666" '
                 f'stroke-width="0.8"/>')
    by = ypos(q3); box_h = ypos(q1) - ypos(q3)
    parts.append(f'<rect x="{bx:.1f}" y="{by:.2f}" width="{bw}" '
                 f'height="{max(box_h,0.5):.2f}" fill="#1f77b4" '
                 f'fill-opacity="0.18" stroke="#1f77b4" stroke-width="0.8"/>')
    parts.append(f'<line x1="{bx:.1f}" y1="{ypos(med):.2f}" x2="{bx+bw:.1f}" '
                 f'y2="{ypos(med):.2f}" stroke="#1f77b4" stroke-width="1.4"/>')
    lx = bx + bw + 5; last_ly = None
    for cval, cnt, _lo, _hi in _cluster_values(vals):
        ly = ypos(cval)
        if last_ly is not None and abs(ly - last_ly) < 9:
            ly = last_ly + 9
        last_ly = ly
        parts.append(f'<line x1="{bx+bw:.1f}" y1="{ypos(cval):.2f}" '
                     f'x2="{lx-1:.1f}" y2="{ly:.2f}" stroke="#999" '
                     f'stroke-width="0.5"/>')
        txt = f"{cval:.3g}" + (f" ×{cnt}" if cnt > 1 else "")
        parts.append(f'<text x="{lx:.1f}" y="{ly+3:.1f}" font-size="8" '
                     f'fill="#333" font-family="sans-serif">{_esc(txt)}</text>')
    parts.append("</g>")
    return "".join(parts)


def _render_by_node_chart(benchmark, records):
    """champion-by-execution-node.svg: one panel per metric (same data list
    as champion.svg), comparing every execution node on a LOG scale, nodes
    sorted by value, with a boxplot of the spread. Lets you see at a glance
    how each system ranks against the others per metric."""
    metrics, better = _by_node_metrics(records)
    if not metrics:
        return None
    mkeys = sorted(metrics.keys())
    npanels = len(mkeys)
    height = TOP_MARGIN + npanels * (PANEL_H + PANEL_GAP) + 10
    width = LEFT_MARGIN + PANEL_W + BOX_W + 20
    nnodes = len({n for mv in metrics.values() for n in mv})
    head = (f'<?xml version="1.0" encoding="UTF-8"?>\n'
            f'<svg xmlns="http://www.w3.org/2000/svg" version="1.1" '
            f'width="{width}" height="{height}" viewBox="0 0 {width} {height}">')
    body = [f'<text x="{LEFT_MARGIN}" y="18" font-size="13" '
            f'font-family="sans-serif" font-weight="bold">'
            f'{_esc(benchmark)} -- by execution node '
            f'(log scale, {nnodes} node(s), sorted)</text>']
    y = TOP_MARGIN
    for mk in mkeys:
        body.append(_plot_by_node_panel(LEFT_MARGIN, y, PANEL_W, PANEL_H,
                                        metrics[mk], mk, better.get(mk)))
        y += PANEL_H + PANEL_GAP
    return head + "".join(body) + "</svg>\n"


def regenerate(benchmark, stamp=None):
    """Regenerate every chart for `benchmark` from history JSONs.

    Three chart levels are generated:
      1. Per-node progression charts:
           results/<bench>/<compile>/<exec>/champion.svg
           results/<bench>/<compile>/<exec>/candidate-<stamp>.svg
      2. Cross-node session chart (all platforms grouped by batch):
           results/<bench>/champion.svg
           results/<bench>/candidate-<stamp>.svg
      3. By-execution-node comparison (log scale, sorted, boxplot):
           results/<bench>/champion-by-execution-node.svg

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

    # 1) per-node charts. Honour the active --node filter: a node-restricted
    # run must only rewrite the charts of the node(s) it actually touched,
    # not churn every other node's champion.svg in git. No filter -> all
    # nodes (default). The cross-node session chart (step 2) is always
    # regenerated so the aggregate stays current.
    groups = _group_records(records)
    for (comp, exe), recs in groups.items():
        arch = recs[0].get("arch") if recs else None
        if not bh.node_allowed(exe, arch):
            continue
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

    # 3) by-execution-node comparison (log scale, sorted, boxplot). Always
    # current (no candidate freeze): it is a cross-node snapshot of the
    # latest value per node, regenerated from the same history JSONs.
    svg = _render_by_node_chart(benchmark, records)
    if svg:
        outdir = os.path.join(bh.RESULTS, benchmark)
        os.makedirs(outdir, exist_ok=True)
        p = os.path.join(outdir, "champion-by-execution-node.svg")
        with open(p, "w") as f:
            f.write(svg)
        written.append(p)

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
    # Pull out --node LABEL|ARCH (repeatable, --node=X too): restrict which
    # per-node charts are rewritten, mirroring the benchmarks' --node. The
    # rest are explicit benchmark names. No --node -> every node (default).
    node_tokens = []
    names = []
    _argv = sys.argv[1:]
    i = 0
    while i < len(_argv):
        a = _argv[i]
        if a == "--node":
            if i + 1 < len(_argv):
                node_tokens.append(_argv[i + 1]); i += 2
            else:
                print("--node requires a value", file=sys.stderr); sys.exit(2)
        elif a.startswith("--node="):
            node_tokens.append(a[len("--node="):]); i += 1
        else:
            names.append(a); i += 1
    bh.set_node_filter(node_tokens or None)

    if names:
        # explicit benchmark name(s)
        for arg in names:
            paths = regenerate(arg)
            for p in paths:
                print(p)
            if not paths:
                print(f"[chart] no history for {arg}", file=sys.stderr)
    else:
        # no benchmark arg -> regenerate every benchmark we have history for
        any_written = False
        for bench, paths in regenerate_all().items():
            for p in paths:
                print(p)
                any_written = True
        if not any_written:
            print("[chart] no history found under benchmark/history/",
                  file=sys.stderr)
            sys.exit(1)
