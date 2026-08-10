#!/usr/bin/env python3
# HEADLESS: yes -- runs under QT_QPA_PLATFORM=offscreen, no display server needed.
# Measures: nothing on its own. This is a PROFILER driver, not a benchmark: it emits a
# perf report of the client steady on-map frame loop, no metric and no champion, so it
# feeds no decision matrix.
# Sampled (perf, 499 Hz): two runs never give byte-identical output.
"""Attach perf to a client ONLY while it stands on the map.

Launches the client the way the harness does (--autosolo --closewhenonmapafter),
watches its stdout for the map marker, and only then runs `perf record -p <pid>`
for a few seconds. Nothing is added to the client: the profiler attaches from the
outside, so the production code is untouched and the profile covers the steady
on-map frame loop instead of the datapack parse and the asset load.
"""
import argparse
import os
import shutil
import subprocess
import sys
import threading
import time

MARKER = "MapVisualiserPlayer::mapDisplayedSlot()"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("binary")
    parser.add_argument("--seconds", type=int, default=15)
    parser.add_argument("--maincode", default="test")
    parser.add_argument("--out", default="/tmp/cc-perf")
    arguments = parser.parse_args()

    os.makedirs(arguments.out, exist_ok=True)
    data_home = os.path.join(arguments.out, "xdg")
    if os.path.isdir(data_home):
        shutil.rmtree(data_home)
    os.makedirs(data_home)
    environment = dict(os.environ)
    environment["XDG_DATA_HOME"] = data_home
    #offscreen paints deterministically: it does not depend on a visible window,
    #a compositor or frame callbacks, which is what made every on-display run
    #measure an idle event loop instead of the map
    environment["QT_QPA_PLATFORM"] = "offscreen"

    process = subprocess.Popen(
        [arguments.binary, "--autosolo",
         "--main-datapack-code=" + arguments.maincode,
         "--closewhenonmapafter=%d" % (arguments.seconds + 20)],
        cwd=os.path.dirname(arguments.binary) or ".", env=environment,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

    on_map = threading.Event()

    def reader():
        for raw in iter(process.stdout.readline, b""):
            if MARKER in raw.decode(errors="replace"):
                on_map.set()

    thread = threading.Thread(target=reader, daemon=True)
    thread.start()

    if not on_map.wait(timeout=180):
        print("the client never reached the map")
        process.kill()
        return 2
    #let the first frames settle: the map fade and the first tile uploads are not
    #the steady state we want to profile
    time.sleep(2.0)
    data = os.path.join(arguments.out, "perf.data")
    print("on the map, recording %ds..." % arguments.seconds)
    record = subprocess.run(
        ["perf", "record", "-g", "--call-graph", "dwarf", "-F", "499",
         "-o", data, "-p", str(process.pid), "--", "sleep",
         str(arguments.seconds)],
        capture_output=True, text=True)
    if record.returncode != 0:
        print("perf record failed: " + record.stderr.strip()[:400])
        process.kill()
        return 2
    process.kill()
    process.wait()
    report = subprocess.run(
        ["perf", "report", "-i", data, "--stdio", "--no-children",
         "--percent-limit", "0.8"],
        capture_output=True, text=True)
    print(report.stdout[:8000])
    return 0


if __name__ == "__main__":
    sys.exit(main())
