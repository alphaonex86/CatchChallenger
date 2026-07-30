#!/usr/bin/env python3
"""doc/performance/EPOLL-VS-IO_URING.png -- epoll vs io_uring, measured.

Produced by benchmark/benchmarkepolliouring.py: server/cli built TWICE from the
same tree (differing only by -DCATCHCHALLENGER_IO_URING), both run on the SAME
exec node with the measuring client (tools/bot-bench, Qt-free) beside them on
127.0.0.1. Requests/s at saturation, 60 bots, 30 s windows, --reps 8, --tuned.

PER NODE ON PURPOSE. The previous version of this figure published a single
"-9.8 %" headline. That number never reproduced -- under two independent
methodologies it came back either overlapping or reversed -- and one number is
the wrong SHAPE for this result: the answer depends on the machine, and on one
machine there is no answer at all. Swapping one contested figure for another
would repeat the mistake, so the boundary condition is drawn rather than
implied.

Two traps this figure records, both paid for:
  * min..max separation at n=3 is NOT evidence. Ranges widen with sample count;
    a clean non-overlapping geode separation at n=3 dissolved at n=4. Every bar
    here is n=8.
  * the io_uring sub-options are NOT optional. Untuned, rpi-4 reported epoll
    +2.5 % -- the WRONG SIGN. COOP_TASKRUN + TASKRUN_FLAG + NO_SQARRAY (no
    extra threads, no extra CPU) turn that into io_uring +7.7 %.
"""
import os
import subprocess
import sys
import tempfile

# (label, cores, epoll (median,min,max), iouring (median,min,max), verdict)
NODES = [
    ("odroid-n2", 6, (95324.5, 95176.8, 95508.8),
     (100552.3, 100129.4, 100894.1), "io_uring +5.5 %"),
    ("rpi-4", 4, (70336.8, 70057.1, 70539.6),
     (75748.6, 73720.7, 75979.2), "io_uring +7.7 %"),
    ("Geode LX800", 1, (17442.9, 17287.4, 17530.0),
     (17560.7, 17422.3, 17652.1), "no separation"),
]
REPS, BOTS, WINDOW_S = 8, 60, 30

W, H = 1500, 800
INK, GREY = "#1a1a1a", "#8a8a8a"
C_E, C_U, GREEN, RED, AMBER = "#1f77b4", "#d62728", "#2ca02c", "#b00020", "#a06800"
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
text(W / 2, 65, f"requests/s at saturation - higher is better - {BOTS} bots, "
     f"{WINDOW_S}s window, n={REPS} per backend, io_uring sub-options ON",
     13, GREY, anchor="middle")
text(W / 2, 86, "the measuring client runs ON the node beside the server "
     "(loopback): no orchestrating host and no physical link in the "
     "measurement path", 12, GREY, anchor="middle")

PW, PH, PY = 468, 400, 108


def panel(px, label, cores, e, u, verdict):
    rect(px, PY, PW, PH, "#fbfbfb", "#dddddd", rx=6)
    text(px + 16, PY + 27, label, 17, INK, weight="bold")
    text(px + PW - 14, PY + 27,
         f"{cores} core" + ("s" if cores > 1 else ""), 11, GREY, anchor="end")
    base, top = PY + PH - 112, PY + 122
    span, hi = base - top, max(e[2], u[2])
    bw = 114
    for name, v, col, x0 in (("epoll", e, C_E, px + 92),
                             ("io_uring", u, C_U, px + 262)):
        bh = span * (v[0] / hi)
        rect(x0, base - bh, bw, bh, col, op=0.85, rx=3)
        text(x0 + bw / 2, base - bh - 14, f"{v[0]/1000:.1f}k", 21, col,
             anchor="middle", weight="bold")
        # the observed min..max drawn to scale: whether they OVERLAP is the
        # entire claim, so it has to be visible rather than asserted
        ylo, yhi = base - span * (v[1] / hi), base - span * (v[2] / hi)
        line(x0 + bw / 2, ylo, x0 + bw / 2, yhi, INK, 2)
        line(x0 + bw / 2 - 9, ylo, x0 + bw / 2 + 9, ylo, INK, 2)
        line(x0 + bw / 2 - 9, yhi, x0 + bw / 2 + 9, yhi, INK, 2)
        text(x0 + bw / 2, base + 24, name, 14, INK, anchor="middle",
             weight="bold")
    line(px + 40, base, px + PW - 40, base, INK, 1.5)
    text(px + PW / 2, base + 50, "requests/s", 12, GREY, anchor="middle")
    sep = verdict != "no separation"
    text(px + PW / 2, base + 78, verdict, 17, GREEN if sep else AMBER,
         anchor="middle", weight="bold")
    if sep:
        msg = (f"ranges do not overlap: {u[1]/1000:.1f}k > {e[2]/1000:.1f}k")
    else:
        msg = "ranges OVERLAP - a better median alone is not a result"
    text(px + PW / 2, base + 98, msg, 11.5, GREEN if sep else AMBER,
         anchor="middle")


x = 30
for label, cores, e, u, verdict in NODES:
    panel(x, label, cores, e, u, verdict)
    x += PW + 18

BY = PY + PH + 24
BOX_H = 250
rect(30, BY, W - 60, BOX_H, "#fbfbfb", "#dddddd", rx=6)
text(46, BY + 26, "Reading this", 16, INK, weight="bold")
rows = [
    (GREEN, "io_uring is worth 5-8 % on multi-core boards, and gets it at LOWER server "
            "CPU (rpi-4 94.7 -> 92.5 %, odroid-n2 78.8 -> 76.9 %) for about +0.8 MB RSS."),
    (AMBER, "On the single-core Geode there is NO measurable difference. With the load "
            "generator beside it the server gets only 42 % of the core and schedstat "
            "run_delay matches its on-CPU time: it is scheduler-bound, not "
            "syscall-bound, so there is nothing for io_uring to remove."),
    (INK, "The win is on the SEND side: ~249 broadcast ::send() calls collapse into ONE "
          "io_uring_enter per tick. Putting the sends on the ring WITHOUT batching them "
          "measured slower."),
    (INK, "NOT on the receive side. epoll drains 62 moves per read wakeup on odroid-n2 "
          "against io_uring's 2.5 (packets_in vs moves), so multishot recv issues 25x "
          "MORE dispatches and still wins overall. Provided-buffer recv is not "
          "zero-copy either -- both paths do exactly one skb->userspace copy, and both "
          "decode inline out of that buffer."),
    (RED, "SQPOLL measured HARMFUL: -8.1 % on rpi-4, -0.4 % on odroid-n2. It spins a "
          "kernel thread costing a core it cannot repay, this server is not "
          "submission-bound, and it is not like-for-like against single-threaded epoll."),
    (GREY, "Why not the >2x reported for echo servers: this epoll path already drains up "
           "to 62 moves per read syscall and per-move application work is ~2.6 us, so "
           "syscalls are a small fraction of the total. The published multipliers are "
           "syscall-throughput microbenchmarks against unbatched baselines, usually with "
           "SQPOLL and/or zero-copy receive -- impossible here: it needs NIC header/data "
           "split, and a 3-byte move packet is far below the size where it pays."),
    (RED, "WITHDRAWN: this figure's earlier -9.8 % headline. It did not reproduce under "
          "two independent methodologies. Those runs also mixed instrumented binaries "
          "with an SSH-sampled CPU window; both are gone."),
]
#wrap: at 11.5px DejaVu Sans roughly 6.05px per char, and the box is
#W-60 wide starting at x=62, so cut well inside that rather than letting a
#line run off the canvas (which it silently did before)
MAX_CH = int((W - 60 - 46) / 6.05)


def wrap(msg, width):
    out, cur = [], ""
    words = msg.split(" ")
    i = 0
    while i < len(words):
        cand = words[i] if cur == "" else cur + " " + words[i]
        if len(cand) <= width:
            cur = cand
        else:
            out.append(cur)
            cur = words[i]
        i += 1
    if cur != "":
        out.append(cur)
    return out


ry = BY + 50
for col, msg in rows:
    lines = wrap(msg, MAX_CH)
    text(46, ry, "*", 11, col)
    j = 0
    while j < len(lines):
        text(62, ry + j * 17, lines[j], 11.5, INK)
        j += 1
    ry += 17 * len(lines) + 5

svg = (f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" '
       f'viewBox="0 0 {W} {H}">' + "".join(p) + "</svg>")
out = sys.argv[1] if len(sys.argv) > 1 else "/tmp/out.png"
#the SVG is a build intermediate: keep it OUT of the source tree, next to the
#PNG it would be an untracked artefact sitting beside a checked-in file
with tempfile.NamedTemporaryFile("w", suffix=".svg", delete=False) as fh:
    fh.write(svg)
    tmp = fh.name
rc = subprocess.run(["rsvg-convert", "-w", str(W), "-o", out, tmp]).returncode
os.unlink(tmp)
print(("wrote " + out) if rc == 0 else "rsvg-convert failed")
sys.exit(0 if rc == 0 else 1)
