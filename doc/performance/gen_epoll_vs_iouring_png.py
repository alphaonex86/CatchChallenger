#!/usr/bin/env python3
"""doc/performance/EPOLL-VS-IO_URING.png -- epoll vs io_uring, measured.

Data: scratchpad/ab_geode.py on the real Geode LX800 (i586, 223 MB, NFS root),
14/14 valid runs, 7 per backend, pooled from two sessions of the SAME build
(batched-v4: 3+3, confirm-v5: 4+4). Both backends do the same fixed work with
the same seed, 250 bots, 250 ms action interval, compression=none and the
anti-flood filter compiled out.

Normalised per move because the runs land within ~4 % of each other on move
count; absolute CPU agrees (-9.5 %).

Why the Geode and not the 7950X3D: the fast box could not be loaded past 0.43 %
of one core by any bot count the protocol allows (250 per map), so its absolute
CPU measures nothing. The Geode calibrates to 11.5 %.
"""
import subprocess
import sys

# per-move microseconds, 7 runs each (v4 3 + v5 4)
E = [324.92, 298.87, 330.80, 312.86, 319.89, 317.00, 321.75]
U = [289.43, 294.24, 286.10, 294.44, 273.41, 293.54, 277.17]
E_CPU, U_CPU = sum(E) / len(E), sum(U) / len(U)
E_LO, E_HI, U_LO, U_HI = min(E), max(E), min(U), max(U)
E_RSS, U_RSS = 9.70, 11.69
N = len(E)
BOTS, INTERVAL, LOAD_PCT = 250, 250, 11.5
# the three stages, per move (us). Stage 1 is an EARLIER tree state.
S1_E, S1_U = 318.0, 318.0      # receive side only -> indistinguishable
S2_E, S2_U = 324.3, 331.8      # sends on the ring, submitted per send (2+2 runs)
S3_E, S3_U = E_CPU, U_CPU      # sends on the ring, batched per tick (7+7 runs)

W, H = 1500, 716
INK, GREY = "#1a1a1a", "#8a8a8a"
C_E, C_U, GREEN, RED = "#1f77b4", "#d62728", "#2ca02c", "#b00020"
p = []


def esc(t):
    return str(t).replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def text(x, y, s, size=13, fill=INK, anchor="start", weight="normal"):
    p.append(f'<text x="{x:.1f}" y="{y:.1f}" font-size="{size}" fill="{fill}" '
             f'text-anchor="{anchor}" font-weight="{weight}" '
             f'font-family="DejaVu Sans, sans-serif">{esc(s)}</text>')


def rect(x, y, w, h, fill, stroke="none", op=1.0, rx=0):
    p.append(f'<rect x="{x:.1f}" y="{y:.1f}" width="{w:.1f}" height="{h:.1f}" '
             f'fill="{fill}" stroke="{stroke}" fill-opacity="{op}" rx="{rx}"/>')


def line(x1, y1, x2, y2, stroke=GREY, w=1):
    p.append(f'<line x1="{x1:.1f}" y1="{y1:.1f}" x2="{x2:.1f}" y2="{y2:.1f}" '
             f'stroke="{stroke}" stroke-width="{w}"/>')


p.append(f'<rect width="{W}" height="{H}" fill="#ffffff"/>')
text(W / 2, 40, "epoll vs io_uring - CatchChallenger server", 25, INK,
     anchor="middle", weight="bold")
text(W / 2, 64, f"Geode LX800 - {BOTS} bots @ {INTERVAL} ms - same fixed work, same seed - "
     f"{N} runs each, {N*2}/{N*2} valid", 13, GREY, anchor="middle")

PW, PH, PY = 468, 368, 92


def panel(px, title, tag):
    rect(px, PY, PW, PH, "#fbfbfb", "#dddddd", rx=6)
    text(px + 16, PY + 26, title, 16, INK, weight="bold")
    text(px + PW - 14, PY + 26, tag, 11, GREY, anchor="end")
    return PY + PH - 96, PY + 112


# --- panel 1: CPU per move, with the full observed range ----------------
base, top = panel(30, "CPU for the same work", "MEASURED - schedstat, ns")
span, hi, bw = base - top, max(E_HI, U_HI), 114
for name, val, lo, up, col, x0 in (("epoll", E_CPU, E_LO, E_HI, C_E, 30 + 92),
                                   ("io_uring", U_CPU, U_LO, U_HI, C_U, 30 + 262)):
    bh = span * (val / hi)
    rect(x0, base - bh, bw, bh, col, op=0.85, rx=3)
    text(x0 + bw / 2, base - bh - 14, f"{val:.0f} us", 21, col, anchor="middle",
         weight="bold")
    # the observed min..max, drawn to scale: the claim is that they do not overlap
    ylo, yhi = base - span * (lo / hi), base - span * (up / hi)
    line(x0 + bw / 2, ylo, x0 + bw / 2, yhi, INK, 2)
    line(x0 + bw / 2 - 9, ylo, x0 + bw / 2 + 9, ylo, INK, 2)
    line(x0 + bw / 2 - 9, yhi, x0 + bw / 2 + 9, yhi, INK, 2)
    text(x0 + bw / 2, base + 24, name, 14, INK, anchor="middle", weight="bold")
line(30 + 40, base, 30 + PW - 40, base, INK, 1.5)
text(30 + PW / 2, base + 52, "microseconds of CPU per move packet", 12, GREY,
     anchor="middle")
text(30 + PW / 2, base + 76, f"io_uring is {100*(U_CPU-E_CPU)/E_CPU:+.1f} %", 16,
     GREEN, anchor="middle", weight="bold")
text(30 + PW / 2, base + 96,
     f"ranges do not overlap: worst io_uring {U_HI:.0f} < best epoll {E_LO:.0f}",
     12, GREEN, anchor="middle", weight="bold")

# --- panel 2: memory ----------------------------------------------------
PX2 = 30 + PW + 18
base, top = panel(PX2, "Resident memory", "MEASURED - VmHWM")
span, hi = base - top, max(E_RSS, U_RSS)
for name, val, col, x0 in (("epoll", E_RSS, C_E, PX2 + 92),
                           ("io_uring", U_RSS, C_U, PX2 + 262)):
    bh = span * (val / hi)
    rect(x0, base - bh, 114, bh, col, op=0.85, rx=3)
    text(x0 + 57, base - bh - 14, f"{val:.2f} MB", 21, col, anchor="middle",
         weight="bold")
    text(x0 + 57, base + 24, name, 14, INK, anchor="middle", weight="bold")
line(PX2 + 40, base, PX2 + PW - 40, base, INK, 1.5)
text(PX2 + PW / 2, base + 52, f"io_uring costs {U_RSS-E_RSS:+.2f} MB", 16, RED,
     anchor="middle", weight="bold")
text(PX2 + PW / 2, base + 76, "the provided-buffer ring (sized from max-players)",
     12, GREY, anchor="middle")
text(PX2 + PW / 2, base + 96, "plus one output buffer per client for the queued sends",
     12, GREY, anchor="middle")

# --- panel 3: how it got there ------------------------------------------
PX3 = 30 + 2 * (PW + 18)
base, top = panel(PX3, "Why it did not show before", "MEASURED - 3 tree states")
span = base - top
hi3 = max(S1_U, S2_U, S3_E)
stages = [("recv only", S1_E, S1_U, "0 %"), ("send/ring,\nper send", S2_E, S2_U, "+2.3 %"),
          ("send/ring,\nbatched", S3_E, S3_U, f"{100*(S3_U-S3_E)/S3_E:+.1f} %")]
x0 = PX3 + 46
for label, ev, uv, delta in stages:
    for val, col, dx in ((ev, C_E, 0), (uv, C_U, 46)):
        bh = span * (val / hi3)
        rect(x0 + dx, base - bh, 42, bh, col, op=0.85, rx=2)
    col = GREEN if delta.startswith("-") else (GREY if delta == "0 %" else RED)
    text(x0 + 44, base - span * (max(ev, uv) / hi3) - 10, delta, 13, col,
         anchor="middle", weight="bold")
    for i, ln in enumerate(label.split("\n")):
        text(x0 + 44, base + 22 + i * 14, ln, 11.5, INK, anchor="middle")
    x0 += 132
line(PX3 + 30, base, PX3 + PW - 30, base, INK, 1.5)
text(PX3 + PW / 2, base + 62, "blue epoll / red io_uring - us per move", 12, GREY,
     anchor="middle")
text(PX3 + PW / 2, base + 86, "the win needs the sends batched, not just on the ring",
     12.5, INK, anchor="middle", weight="bold")

BY = PY + PH + 26
rect(30, BY, W - 60, 210, "#fbfbfb", "#dddddd", rx=6)
text(46, BY + 26, "Reading this", 16, INK, weight="bold")
rows = [
    (INK, "With 250 players on ONE map each incoming move is broadcast to the other ~249. "
          "io_uring used to cover the RECEIVE side only, so it saved ~1 syscall out of ~250 "
          "and measured indistinguishable from epoll."),
    (INK, "Putting those sends on the ring is not enough by itself: submitting each one "
          "immediately just swaps ::send() for io_uring_enter() and adds a copy -- measured "
          "SLOWER (+2.3 %)."),
    (GREEN, "The win comes from batching: SQEs accumulate and wait() flushes the whole tick "
            "in ONE io_uring_submit_and_wait, so ~249 sends cost one syscall instead of 249."),
    (GREY, "Ordering is enforced by one per-socket in-flight counter shared by the datapack "
           "send chain and ordinary writes; a write only becomes an SQE when nothing else is "
           "outstanding on that fd, otherwise it is queued."),
    (GREY, f"Measured where the CPU can actually be loaded: the Geode calibrates to "
           f"{LOAD_PCT} % of one core at {BOTS} bots. A 7950X3D could not be pushed past "
           f"0.43 % by any bot count the 8-bit per-map player index allows (250)."),
    (GREY, "Both backends run the SAME fixed work with the same seed -- a fixed TIME window "
           "is not comparable, because the bots act as fast as the server answers."),
    (RED, "Not plotted: requests/s. BENCH packets_in is incremented only on the epoll "
          "readiness path, so io_uring always reads 0 -- not measured, not a result."),
    (GREY, "Earlier figures claiming -24 % or +10 % came from mixing completed runs with "
           "runs truncated at a timeout. Those are withdrawn."),
]
ry = BY + 50
for col, msg in rows:
    text(46, ry, "*", 11, col)
    text(62, ry, msg, 11.5, INK)
    ry += 19

svg = (f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" '
       f'viewBox="0 0 {W} {H}">' + "".join(p) + "</svg>")
out = sys.argv[1] if len(sys.argv) > 1 else "/tmp/out.png"
open(out.rsplit(".", 1)[0] + ".svg", "w").write(svg)
rc = subprocess.run(["rsvg-convert", "-w", str(W), "-o", out,
                     out.rsplit(".", 1)[0] + ".svg"]).returncode
print(("wrote " + out) if rc == 0 else "rsvg-convert failed")
