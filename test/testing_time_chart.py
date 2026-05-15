#!/usr/bin/env python3
"""Render a donut chart of per-script wall time from
`testing-individual-time.json` (written by all.sh).

Output: `<tmpfs_root>/testing-time-donut.svg`. Hand-rolled SVG so the
host needs no extra Python packages (matplotlib is not installed).

Invoked at the tail of a full all.sh run; the operator opens the .svg
in any browser to see where the wall went.

Sorting: largest slice first, smallest last (clockwise from 12
o'clock). Slices < 1% of the total fold into a single "other" wedge so
the legend stays readable.
"""

import json
import math
import os
import sys


C_LEGEND_W = 320
INNER_RATIO = 0.55
CHART_SIZE = 520
LEGEND_ROW_H = 18
LEGEND_SWATCH_W = 14
TITLE_H = 50
FOOTER_H = 30
OTHER_THRESHOLD_PCT = 1.0

PALETTE = [
    "#e6194b", "#3cb44b", "#ffe119", "#4363d8", "#f58231",
    "#911eb4", "#46f0f0", "#f032e6", "#bcf60c", "#fabebe",
    "#008080", "#e6beff", "#9a6324", "#fffac8", "#800000",
    "#aaffc3", "#808000", "#ffd8b1", "#000075", "#808080",
    "#a52a2a", "#5f9ea0", "#7fff00", "#d2691e", "#ff7f50",
]


def _fmt_dur(seconds):
    s = int(round(seconds))
    if s < 60:
        return f"{s}s"
    m, sec = divmod(s, 60)
    if m < 60:
        return f"{m}m{sec:02d}s"
    h, m = divmod(m, 60)
    return f"{h}h{m:02d}m{sec:02d}s"


def _arc_path(cx, cy, r_out, r_in, theta0, theta1):
    """Build an SVG path for a donut slice between two angles (radians,
    measured clockwise from 12 o'clock). Always emits valid SVG even
    for full-circle slices (single-entry pie)."""
    sweep = theta1 - theta0
    if sweep <= 0:
        return ""
    large = 1 if sweep > math.pi else 0
    full_circle = abs(sweep - 2 * math.pi) < 1e-9

    def pt(r, theta):
        x = cx + r * math.sin(theta)
        y = cy - r * math.cos(theta)
        return x, y

    if full_circle:
        # Two-arc trick — a single 360° arc has identical start/end
        # points and SVG would draw nothing.
        x1o, y1o = pt(r_out, 0.0)
        x2o, y2o = pt(r_out, math.pi)
        x1i, y1i = pt(r_in, 0.0)
        x2i, y2i = pt(r_in, math.pi)
        return (
            f"M {x1o:.2f} {y1o:.2f} "
            f"A {r_out:.2f} {r_out:.2f} 0 1 1 {x2o:.2f} {y2o:.2f} "
            f"A {r_out:.2f} {r_out:.2f} 0 1 1 {x1o:.2f} {y1o:.2f} "
            f"M {x1i:.2f} {y1i:.2f} "
            f"A {r_in:.2f} {r_in:.2f} 0 1 0 {x2i:.2f} {y2i:.2f} "
            f"A {r_in:.2f} {r_in:.2f} 0 1 0 {x1i:.2f} {y1i:.2f} Z"
        )

    x0o, y0o = pt(r_out, theta0)
    x1o, y1o = pt(r_out, theta1)
    x0i, y0i = pt(r_in,  theta0)
    x1i, y1i = pt(r_in,  theta1)
    return (
        f"M {x0o:.2f} {y0o:.2f} "
        f"A {r_out:.2f} {r_out:.2f} 0 {large} 1 {x1o:.2f} {y1o:.2f} "
        f"L {x1i:.2f} {y1i:.2f} "
        f"A {r_in:.2f} {r_in:.2f} 0 {large} 0 {x0i:.2f} {y0i:.2f} Z"
    )


def render(time_json_path, out_svg_path):
    if not os.path.isfile(time_json_path):
        print(f"[time-chart] no input {time_json_path} — skipping",
              file=sys.stderr)
        return False
    with open(time_json_path, "r") as f:
        try:
            entries = json.load(f)
        except json.JSONDecodeError as exc:
            print(f"[time-chart] {time_json_path} is not valid JSON: {exc}",
                  file=sys.stderr)
            return False
    if not isinstance(entries, list) or not entries:
        print(f"[time-chart] {time_json_path} is empty — skipping",
              file=sys.stderr)
        return False

    # Merge duplicate script entries (a script that ran twice in a
    # single all.sh run, e.g. testinggateway.py listed twice, should
    # show as one combined wedge).
    merged = {}
    order = []
    i = 0
    while i < len(entries):
        e = entries[i]
        i += 1
        name = e.get("script", "?")
        dur = float(e.get("duration_s", 0))
        rc = int(e.get("rc", 0))
        if name in merged:
            merged[name]["dur"] += dur
            if rc != 0:
                merged[name]["rc"] = rc
        else:
            merged[name] = {"dur": dur, "rc": rc}
            order.append(name)
    items = []
    j = 0
    while j < len(order):
        n = order[j]
        items.append((n, merged[n]["dur"], merged[n]["rc"]))
        j += 1
    items.sort(key=lambda x: -x[1])

    total = sum(x[1] for x in items)
    if total <= 0:
        print(f"[time-chart] total duration is 0 — skipping", file=sys.stderr)
        return False

    threshold = total * OTHER_THRESHOLD_PCT / 100.0
    big = []
    small = []
    k = 0
    while k < len(items):
        name, dur, rc = items[k]
        if dur < threshold:
            small.append((name, dur, rc))
        else:
            big.append((name, dur, rc))
        k += 1
    if small:
        other_dur = sum(s[1] for s in small)
        other_rc  = 0
        si = 0
        while si < len(small):
            if small[si][2] != 0:
                other_rc = small[si][2]
            si += 1
        big.append((f"other ({len(small)} scripts)", other_dur, other_rc))

    n = len(big)
    width = CHART_SIZE + C_LEGEND_W
    height = max(CHART_SIZE + TITLE_H + FOOTER_H,
                 TITLE_H + (n + 2) * LEGEND_ROW_H + FOOTER_H)
    cx = CHART_SIZE / 2
    cy = TITLE_H + CHART_SIZE / 2
    r_out = (CHART_SIZE / 2) - 30
    r_in  = r_out * INNER_RATIO

    parts = [
        f'<?xml version="1.0" encoding="UTF-8"?>',
        f'<svg xmlns="http://www.w3.org/2000/svg" '
        f'viewBox="0 0 {width} {height}" '
        f'font-family="DejaVu Sans, Verdana, sans-serif" '
        f'font-size="12">',
        f'<rect width="{width}" height="{height}" fill="#ffffff"/>',
        f'<text x="{width/2:.1f}" y="28" text-anchor="middle" '
        f'font-size="18" font-weight="bold" fill="#202020">'
        f'all.sh wall-time per testing*.py (total {_fmt_dur(total)})'
        f'</text>',
    ]

    theta = 0.0
    legend_x = CHART_SIZE + 20
    legend_y = TITLE_H + 10
    idx = 0
    while idx < len(big):
        name, dur, rc = big[idx]
        frac = dur / total
        sweep = frac * 2 * math.pi
        color = PALETTE[idx % len(PALETTE)]
        path = _arc_path(cx, cy, r_out, r_in, theta, theta + sweep)
        if path:
            stroke = "#ffffff"
            parts.append(
                f'<path d="{path}" fill="{color}" stroke="{stroke}" '
                f'stroke-width="1.5"><title>'
                f'{name} — {_fmt_dur(dur)} ({frac*100:.1f}%)'
                f'</title></path>'
            )
        # mid-slice label, only when the slice is wide enough to fit text
        if frac >= 0.06:
            mid = theta + sweep / 2
            lx = cx + (r_out + r_in) / 2 * math.sin(mid)
            ly = cy - (r_out + r_in) / 2 * math.cos(mid)
            parts.append(
                f'<text x="{lx:.1f}" y="{ly:.1f}" text-anchor="middle" '
                f'dominant-baseline="middle" font-size="11" '
                f'fill="#202020">{_fmt_dur(dur)}</text>'
            )
        # legend row
        ly = legend_y + idx * LEGEND_ROW_H
        parts.append(
            f'<rect x="{legend_x}" y="{ly}" '
            f'width="{LEGEND_SWATCH_W}" height="{LEGEND_SWATCH_W}" '
            f'fill="{color}" stroke="#404040" stroke-width="0.5"/>'
        )
        label = name
        if rc != 0:
            label = f"{name} (rc={rc})"
        parts.append(
            f'<text x="{legend_x + LEGEND_SWATCH_W + 6}" '
            f'y="{ly + LEGEND_SWATCH_W - 2}" fill="#202020">'
            f'{label}'
            f'</text>'
        )
        parts.append(
            f'<text x="{width - 10}" y="{ly + LEGEND_SWATCH_W - 2}" '
            f'text-anchor="end" fill="#404040">'
            f'{_fmt_dur(dur)} · {frac*100:.1f}%'
            f'</text>'
        )
        theta += sweep
        idx += 1

    # center label = total
    parts.append(
        f'<text x="{cx:.1f}" y="{cy - 6:.1f}" text-anchor="middle" '
        f'font-size="16" font-weight="bold" fill="#202020">'
        f'{_fmt_dur(total)}</text>'
    )
    parts.append(
        f'<text x="{cx:.1f}" y="{cy + 12:.1f}" text-anchor="middle" '
        f'font-size="11" fill="#606060">total wall</text>'
    )

    parts.append(
        f'<text x="{width/2:.1f}" y="{height - 10}" '
        f'text-anchor="middle" font-size="10" fill="#808080">'
        f'source: {os.path.basename(time_json_path)} · '
        f'{len(entries)} entries, {len(big)} slices'
        f'</text>'
    )
    parts.append('</svg>')
    with open(out_svg_path, "w") as f:
        f.write("\n".join(parts))
    print(f"[time-chart] wrote {out_svg_path} "
          f"({len(big)} slices, total {_fmt_dur(total)})")
    return True


def main():
    import test_config
    # all.sh writes per-script timings to testing-individual-time.json
    # — separate from time.json (per-phase log).
    tmpfs_root = test_config.TMPFS_ROOT
    time_json = os.path.join(tmpfs_root, "testing-individual-time.json")
    out_svg   = os.path.join(tmpfs_root, "testing-time-donut.svg")
    ok = render(time_json, out_svg)
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
