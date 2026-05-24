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

# Right-of-panel gutter for the value boxplot + its clustered value labels
# (single-boxplot charts: the by-execution-node comparison).
BOX_W = 150
# Per-series boxplot slot width: _plot_panel draws one mini-boxplot per
# series side-by-side, so the gutter scales with the series count.
BOX_SLOT_W = 16


def _series_gutter_w(max_series):
    """Right-gutter width needed for `max_series` side-by-side mini-boxplots."""
    return max(max_series, 1) * BOX_SLOT_W + 16


# Legend draws one line per series at 11px steps from y=12; grow the panel so
# a many-series legend (and the per-series boxplots) never overflow into the
# next panel.
LEGEND_STEP = 11


def _panel_h(max_series):
    """Panel height: the base, or tall enough for `max_series` legend lines."""
    return max(PANEL_H, max_series * LEGEND_STEP + 16)
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


def _num(x):
    """Format a mantissa without scientific notation: 1010, 95.5, 8.5, 0.13."""
    a = abs(x)
    if a >= 100:
        return f"{x:.0f}"
    if a >= 10:
        return f"{x:.1f}".rstrip("0").rstrip(".")
    return f"{x:.2f}".rstrip("0").rstrip(".")


def _fmt_bytes(b):
    for thr, suf in ((1024**4, "TB"), (1024**3, "GB"), (1024**2, "MB"),
                     (1024, "KB")):
        if abs(b) >= thr:
            return f"{_num(b/thr)}{suf}"
    return f"{_num(b)}B"


def _fmt_time(s):
    """Human time from a value in SECONDS (ns/µs/ms/s/min/h)."""
    a = abs(s)
    if a == 0:
        return "0s"
    if a < 1e-6:
        return f"{_num(s*1e9)}ns"
    if a < 1e-3:
        return f"{_num(s*1e6)}us"
    if a < 1:
        return f"{_num(s*1e3)}ms"
    if a < 120:
        return f"{_num(s)}s"
    if a < 7200:
        return f"{_num(s/60)}min"
    return f"{_num(s/3600)}h"


def _fmt_si(n):
    """Human plain count: 1, 56K, 1.2M, 3.4G (no unit)."""
    a = abs(n)
    for thr, suf in ((1e12, "T"), (1e9, "G"), (1e6, "M"), (1e3, "K")):
        if a >= thr:
            return f"{_num(n/thr)}{suf}"
    if n == int(n):
        return str(int(n))
    return _num(n)


def _unit_from_name(name):
    """Infer a value's unit from its metric name suffix (the chart doesn't
    carry the JSON unit field through). Order matters: '_per_s' before '_s'."""
    n = (name or "").lower()
    if "percent" in n:
        return "%"
    if n.endswith("_kb"):
        return "kb"
    if "bytes" in n:
        return "bytes"
    if n.endswith("_ns"):
        return "ns"
    if n.endswith("_us") or n.endswith("_µs"):
        return "us"
    if n.endswith("_ms"):
        return "ms"
    if n.endswith("_per_s") or n.endswith("per_s"):
        return "/s"
    if n.endswith("_s"):
        return "s"
    return ""


def _humanize(v, unit):
    """Human-readable value+unit: 8.5KB, 9MB, 20ms, 82s, 7.5min, 56K, 1."""
    unit = (unit or "").lower()
    if unit == "%":
        return f"{_num(v)}%"
    if unit == "bytes":
        return _fmt_bytes(v)
    if unit == "kb":
        return _fmt_bytes(v * 1024)
    if unit == "ns":
        return _fmt_time(v / 1e9)
    if unit == "us":
        return _fmt_time(v / 1e6)
    if unit == "ms":
        return _fmt_time(v / 1e3)
    if unit == "s":
        return _fmt_time(v)
    if unit == "/s":
        return _fmt_si(v) + "/s"
    return _fmt_si(v)


def _humanize_name(v, name):
    """_humanize using the unit inferred from a metric name."""
    return _humanize(v, _unit_from_name(name))


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

    # y-axis tick labels (just min/max), human-readable units
    parts.append(f'<text x="2" y="10" font-size="8" fill="#444" '
                 f'font-family="sans-serif">{_esc(_humanize_name(ymax, panel_metric))}</text>')
    parts.append(f'<text x="2" y="{h-2}" font-size="8" fill="#444" '
                 f'font-family="sans-serif">{_esc(_humanize_name(ymin, panel_metric))}</text>')

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

    # Legend labels: strip the dot-segments common to EVERY series in this
    # panel -- they just repeat the panel metric (e.g. every cross-node
    # series ends ".rusage.b128_invol_ctx", so only the node "cb2" is
    # informative). Keeps the distinguishing prefix (node, or node.tool /
    # tool.slice).
    _labels = list(points_by_label.keys())
    _splits = [l.split(".") for l in _labels]
    _common = 0
    if _splits:
        _minlen = min(len(s) for s in _splits)
        while _common < _minlen and len({s[-(_common + 1)] for s in _splits}) == 1:
            _common += 1
    def _short_label(l):
        s = l.split(".")
        keep = s[:len(s) - _common] if _common < len(s) else s
        return ".".join(keep) if keep else l

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
        # legend: distinguishing part only (panel-common suffix stripped);
        # lower/higher=better is in the title, not per series.
        parts.append(f'<text x="{w-4}" y="{12 + ci*11}" font-size="9" '
                     f'text-anchor="end" fill="{col}" '
                     f'font-family="sans-serif">{_esc(_short_label(label))}</text>')

    # ---- per-series boxplots in the right gutter ------------------------
    # ONE mini-boxplot per series (e.g. one per execution node on the
    # cross-node chart), coloured like its line and sharing this panel's
    # y-scale + full height, laid out side-by-side so each series'
    # distribution can be compared at a glance. The top-right legend maps
    # colour -> series. Box = Q1..Q3, line = median, whiskers = min..max.
    parts.append(f'<text x="{w+10}" y="-4" font-size="8" fill="#444" '
                 f'font-family="sans-serif">per-node spread</text>')
    gx = w + 10
    for ci, (label, pts) in enumerate(sorted(points_by_label.items())):
        col = _LINE_COLOURS[ci % len(_LINE_COLOURS)]
        svals = sorted(v for _i, v, _c, _d in pts)
        if not svals:
            continue
        slot = BOX_SLOT_W
        cxs = gx + ci * slot + slot / 2.0
        bw = slot * 0.55
        bx = cxs - bw / 2.0
        q1 = _quantile(svals, 0.25)
        med = _quantile(svals, 0.50)
        q3 = _quantile(svals, 0.75)
        vmn = svals[0]; vmx = svals[-1]
        # whisker + caps
        parts.append(f'<line x1="{cxs:.1f}" y1="{ypos(vmx):.2f}" '
                     f'x2="{cxs:.1f}" y2="{ypos(vmn):.2f}" stroke="{col}" '
                     f'stroke-width="0.6"/>')
        for cap in (vmn, vmx):
            parts.append(f'<line x1="{bx+1:.1f}" y1="{ypos(cap):.2f}" '
                         f'x2="{bx+bw-1:.1f}" y2="{ypos(cap):.2f}" '
                         f'stroke="{col}" stroke-width="0.6"/>')
        # box Q1..Q3
        by = ypos(q3); box_h = ypos(q1) - ypos(q3)
        parts.append(f'<rect x="{bx:.1f}" y="{by:.2f}" width="{bw:.1f}" '
                     f'height="{max(box_h,0.5):.2f}" fill="{col}" '
                     f'fill-opacity="0.22" stroke="{col}" '
                     f'stroke-width="0.7"/>')
        # median
        parts.append(f'<line x1="{bx:.1f}" y1="{ypos(med):.2f}" '
                     f'x2="{bx+bw:.1f}" y2="{ypos(med):.2f}" '
                     f'stroke="{col}" stroke-width="1.2"/>')

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
    max_series = max((len(p) for p in panels.values()), default=1)
    panel_h = _panel_h(max_series)
    height = TOP_MARGIN + npanels * (panel_h + PANEL_GAP) + X_AXIS_H
    width  = LEFT_MARGIN + PANEL_W + _series_gutter_w(max_series) + 20
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
        body.append(_plot_panel(LEFT_MARGIN, y, PANEL_W, panel_h,
                                panels[pname], better_map, commits,
                                champion_idx, pname,
                                panel_metric=pname))
        y += panel_h + PANEL_GAP
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
    # A cross-node chart with a single node carries no cross-node signal
    # (can't compare platforms, can't drive KEEP/DISCARD) and only eats
    # visual space -- skip it. Count distinct nodes across the batches.
    nodes = {d.get("node") or "unknown"
             for _ts, _cs, docs in batches for d in docs}
    if len(nodes) < 2:
        return None
    champ_short = _champion_commit(benchmark)
    series, commits, better_map = _extract_session_series(batches, champ_short)
    # A per-node line is only a trend worth charting if it has >=3 points;
    # a node with <3 values (e.g. only "local" reported b128_net_rx_bytes
    # once or twice) is a lone dot that can't drive a decision -- drop it.
    series = {lbl: pts for lbl, pts in series.items() if len(pts) >= 3}
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
    max_series = max((len(p) for p in panels.values()), default=1)
    panel_h = _panel_h(max_series)
    height = TOP_MARGIN + npanels * (panel_h + PANEL_GAP) + X_AXIS_H
    width  = LEFT_MARGIN + PANEL_W + _series_gutter_w(max_series) + 20
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
        body.append(_plot_panel(LEFT_MARGIN, y, PANEL_W, panel_h,
                                panels[pname], better_map, commits,
                                champion_idx, pname,
                                panel_metric=pname))
        y += panel_h + PANEL_GAP
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
    """For the by-execution-node comparison: collect, per metric category and
    per node, the LIST of every value across the node's history (so each node
    gets a real distribution to box-plot). Returns
    ({metric -> {node -> [values]}}, {metric -> better}).

    metric is the flat name (results.<tool>.metrics.<name>, e.g.
    b128_invol_ctx) or the slice-normalised "b<N>_<name>" for sub-benchmark
    metrics -- same data list as champion.svg, indexed metric->node->values."""
    out = {}
    better = {}
    for doc, _p in records:
        node = doc.get("node") or "unknown"
        seen = set()   # flat metric keys already recorded for THIS doc
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
                out.setdefault(mname, {}).setdefault(node, []).append(v)
                seen.add(mname)
                if mname not in better and m.get("better"):
                    better[mname] = m["better"]
            for slice_label, smetrics in (blk.get("subbenchmarks") or {}).items():
                if not isinstance(smetrics, dict):
                    continue
                # Normalise slice to the flat "b<N>_<metric>" naming so a
                # subbench value DEDUPES against the matching flat metric in
                # this same doc; adds only subbench-exclusive metrics (e.g.
                # cpu_percent).
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
                    if key in seen:   # flat already recorded it this doc
                        continue
                    out.setdefault(key, {}).setdefault(node, []).append(v)
                    if key not in better and m.get("better"):
                        better[key] = m["better"]
    return out, better


def _plot_by_node_panel(x0, y0, w, h, node_vals, title, better):
    """One metric CATEGORY panel: ONE boxplot per execution node (the
    distribution of that node's history values), boxes laid out side-by-side
    and SORTED by median (best-first for lower=better), each labelled with the
    node name, on a LOG y-scale (linear fallback when any value <= 0). Lets
    you compare systems within the category at a glance."""
    parts = [f'<g transform="translate({x0},{y0})">']
    parts.append(f'<rect x="0" y="0" width="{w}" height="{h}" '
                 f'fill="white" stroke="#888" stroke-width="0.5"/>')
    # one (node -> sorted values) entry per node, sorted by median
    items = []
    for node, vals in node_vals.items():
        sv = sorted(vals)
        if sv:
            items.append((node, sv, _quantile(sv, 0.50)))
    items.sort(key=lambda t: t[2])
    if not items:
        parts.append("</g>")
        return "".join(parts)
    allv = [v for _n, sv, _m in items for v in sv]
    title_txt = title + (f" ({better}=better)" if better else "")
    uselog = all(v > 0 for v in allv)
    parts.append(f'<text x="4" y="-4" font-size="11" font-family="sans-serif">'
                 f'{_esc(title_txt)}{"  [log]" if uselog else ""}</text>')
    vmin = min(allv); vmax = max(allv)
    top = 14; bot = h - 38     # reserve a bottom band for node labels
    if uselog:
        lmin = math.log10(vmin); lmax = math.log10(vmax)
        if lmin == lmax:
            lmin -= 0.5; lmax += 0.5
        def ypos(v):
            return bot - ((math.log10(v) - lmin) / (lmax - lmin)) * (bot - top)
    else:
        if vmin == vmax:
            vmin -= 1.0; vmax += 1.0
        pad = (vmax - vmin) * 0.08
        lo = vmin - pad; hi = vmax + pad
        def ypos(v):
            return bot - ((v - lo) / (hi - lo)) * (bot - top)
    # y range labels
    parts.append(f'<text x="2" y="{top+4}" font-size="8" fill="#444" '
                 f'font-family="sans-serif">{_esc(_humanize_name(vmax, title))}</text>')
    parts.append(f'<text x="2" y="{bot}" font-size="8" fill="#444" '
                 f'font-family="sans-serif">{_esc(_humanize_name(vmin, title))}</text>')
    n = len(items)
    padx = 14
    slot = (w - 2 * padx) / float(n)
    bw = min(slot * 0.6, 26)
    i = 0
    while i < n:
        node, sv, _med = items[i]
        col = _LINE_COLOURS[i % len(_LINE_COLOURS)]
        cx = padx + (i + 0.5) * slot
        bx = cx - bw / 2.0
        q1 = _quantile(sv, 0.25); med = _quantile(sv, 0.50)
        q3 = _quantile(sv, 0.75)
        vmn = sv[0]; vmx = sv[-1]
        # whisker + caps
        parts.append(f'<line x1="{cx:.1f}" y1="{ypos(vmx):.2f}" x2="{cx:.1f}" '
                     f'y2="{ypos(vmn):.2f}" stroke="{col}" stroke-width="0.7"/>')
        for cap in (vmn, vmx):
            parts.append(f'<line x1="{bx+2:.1f}" y1="{ypos(cap):.2f}" '
                         f'x2="{bx+bw-2:.1f}" y2="{ypos(cap):.2f}" '
                         f'stroke="{col}" stroke-width="0.7"/>')
        # box Q1..Q3 + median
        by = ypos(q3); box_h = ypos(q1) - ypos(q3)
        parts.append(f'<rect x="{bx:.1f}" y="{by:.2f}" width="{bw:.1f}" '
                     f'height="{max(box_h,0.5):.2f}" fill="{col}" '
                     f'fill-opacity="0.22" stroke="{col}" stroke-width="0.8"/>')
        parts.append(f'<line x1="{bx:.1f}" y1="{ypos(med):.2f}" '
                     f'x2="{bx+bw:.1f}" y2="{ypos(med):.2f}" stroke="{col}" '
                     f'stroke-width="1.3"/>')
        # human-readable median value just above the box's top whisker
        parts.append(f'<text x="{cx:.1f}" y="{ypos(vmx)-2:.2f}" font-size="6.5" '
                     f'fill="{col}" font-family="sans-serif" '
                     f'text-anchor="middle">{_esc(_humanize_name(med, title))}</text>')
        # node label below, rotated to avoid overlap
        ly = h - 4
        parts.append(f'<text x="{cx:.1f}" y="{ly:.1f}" font-size="7.5" '
                     f'fill="#333" font-family="sans-serif" text-anchor="end" '
                     f'transform="rotate(-35 {cx:.1f} {ly:.1f})">'
                     f'{_esc(node)}</text>')
        i += 1
    parts.append("</g>")
    return "".join(parts)


def _render_by_node_chart(benchmark, records):
    """champion-by-execution-node.svg: one panel per metric CATEGORY (same
    data list as champion.svg, e.g. requests_per_s / cpu_percent /
    b128_invol_ctx), each holding one boxplot PER execution node, sorted by
    median on a log scale and labelled by node -- compare systems by
    category."""
    metrics, better = _by_node_metrics(records)
    if not metrics:
        return None
    nnodes = len({n for mv in metrics.values() for n in mv})
    # A by-execution-node comparison with a single node is just one boxplot
    # per panel -- nothing to compare against, and it wastes chart space.
    # Skip it until at least two nodes have history.
    if nnodes < 2:
        return None
    # Drop any metric panel whose data covers <50% of the nodes: a category
    # reported by only "local" (or only "rpi4", ...) can't drive a cross-node
    # decision and just eats vertical space. Keep panels seen on at least half
    # the nodes.
    min_nodes = (nnodes + 1) // 2   # ceil(nnodes/2): >=50% of the nodes
    mkeys = sorted(mk for mk in metrics if len(metrics[mk]) >= min_nodes)
    if not mkeys:
        return None
    npanels = len(mkeys)
    # base height + bottom band for the rotated node labels
    panel_h = PANEL_H + 24
    height = TOP_MARGIN + npanels * (panel_h + PANEL_GAP) + 10
    width = LEFT_MARGIN + PANEL_W + 20
    head = (f'<?xml version="1.0" encoding="UTF-8"?>\n'
            f'<svg xmlns="http://www.w3.org/2000/svg" version="1.1" '
            f'width="{width}" height="{height}" viewBox="0 0 {width} {height}">')
    body = [f'<text x="{LEFT_MARGIN}" y="18" font-size="13" '
            f'font-family="sans-serif" font-weight="bold">'
            f'{_esc(benchmark)} -- by execution node, by category '
            f'(boxplot per node, log scale, sorted; {nnodes} node(s))</text>']
    y = TOP_MARGIN
    for mk in mkeys:
        body.append(_plot_by_node_panel(LEFT_MARGIN, y, PANEL_W, panel_h,
                                        metrics[mk], mk, better.get(mk)))
        y += panel_h + PANEL_GAP
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

    # 2) cross-node session chart (one chart, all platforms). Returns None
    # when fewer than 2 nodes have history -- a single-node chart carries no
    # cross-node signal. Remove any stale file so it doesn't linger.
    batches = _group_by_batch(records)
    svg = _render_session_chart(benchmark, batches)
    outdir = os.path.join(bh.RESULTS, benchmark)
    champ_p = os.path.join(outdir, "champion.svg")
    if svg:
        os.makedirs(outdir, exist_ok=True)
        with open(champ_p, "w") as f:
            f.write(svg)
        written.append(champ_p)
        if stamp:
            cand_p = os.path.join(outdir, f"candidate-{stamp}.svg")
            with open(cand_p, "w") as f:
                f.write(svg)
            written.append(cand_p)
    elif os.path.isfile(champ_p):
        os.remove(champ_p)

    # 3) by-execution-node comparison (log scale, sorted, boxplot). Always
    # current (no candidate freeze): it is a cross-node snapshot of the
    # latest value per node, regenerated from the same history JSONs.
    # Returns None for a single node (nothing to compare); drop stale file.
    svg = _render_by_node_chart(benchmark, records)
    bynode_p = os.path.join(outdir, "champion-by-execution-node.svg")
    if svg:
        os.makedirs(outdir, exist_ok=True)
        with open(bynode_p, "w") as f:
            f.write(svg)
        written.append(bynode_p)
    elif os.path.isfile(bynode_p):
        os.remove(bynode_p)

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
