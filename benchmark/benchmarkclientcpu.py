#!/usr/bin/env python3
"""Measure how much CPU a client burns while standing on the map.

Both clients are started with --autosolo --closewhenonmapafter=N, which walks the
player for N seconds once the map is loaded and then quits. Running the SAME client
twice with two different N and taking the slope

    cpu_per_second = (cpu(long) - cpu(short)) / (long - short)

cancels the constant start-up cost (datapack parse, savegame creation, asset load),
so what is left is the steady-state cost of being on the map -- the number to
compare between the two front-ends and to watch when optimising.

CPU time comes from /proc/<pid>/stat (utime+stime, the whole process tree of the
client, threads included), not from wall clock, so a busier machine does not move
the result.

Needs an X server: Xvfb is started on a free display (the clients are OpenGL/widget
UIs and the offscreen platform plugin does not exercise the same paint path).

  benchmark/benchmarkclientcpu.py --qtopengl <bin> --qt800 <bin> [--maincode test]
"""
import argparse
import os
import shutil
import signal
import subprocess
import sys
import time

CLOCK_TICKS = os.sysconf("SC_CLK_TCK")


def process_cpu_seconds(pid):
    """utime+stime of the process, in seconds, or None once it is gone."""
    try:
        with open("/proc/%d/stat" % pid, encoding="utf-8") as handle:
            fields = handle.read().rsplit(") ", 1)[1].split()
    except (FileNotFoundError, ProcessLookupError, IndexError):
        return None
    # after the ") " split, field 0 is state, so utime/stime are 11 and 12
    return (int(fields[11]) + int(fields[12])) / CLOCK_TICKS


class Xvfb:
    def __init__(self):
        self.display = None
        self.process = None

    def start(self):
        display = 60
        while display < 100:
            lock = "/tmp/.X%d-lock" % display
            if not os.path.exists(lock):
                self.process = subprocess.Popen(
                    ["Xvfb", ":%d" % display, "-screen", "0", "1024x768x24",
                     "-nolisten", "tcp"],
                    stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                time.sleep(1.0)
                if self.process.poll() is None:
                    self.display = ":%d" % display
                    return True
            display += 1
        return False

    def stop(self):
        if self.process is not None and self.process.poll() is None:
            self.process.terminate()
            try:
                self.process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self.process.kill()


def run_once(binary, seconds, display, maincode, data_home, verbose):
    """CPU seconds the client used for a run that stays `seconds` on the map."""
    # a fresh XDG_DATA_HOME per run: the savegame is recreated every time, so the
    # start-up cost is the same in the short and in the long run and cancels out
    if os.path.isdir(data_home):
        shutil.rmtree(data_home)
    os.makedirs(data_home)
    environment = dict(os.environ)
    environment["DISPLAY"] = display
    environment["XDG_DATA_HOME"] = data_home
    arguments = [binary, "--autosolo", "--closewhenonmapafter=%d" % seconds]
    if maincode:
        arguments.append("--main-datapack-code=" + maincode)
    process = subprocess.Popen(arguments, cwd=os.path.dirname(binary) or ".",
                               env=environment, stdout=subprocess.DEVNULL,
                               stderr=subprocess.DEVNULL,
                               start_new_session=True)
    cpu = 0.0
    deadline = time.monotonic() + seconds + 180
    while time.monotonic() < deadline:
        sample = process_cpu_seconds(process.pid)
        if sample is None:
            break
        cpu = sample
        if process.poll() is not None:
            break
        time.sleep(0.2)
    if process.poll() is None:
        os.killpg(os.getpgid(process.pid), signal.SIGKILL)
        process.wait()
        if verbose:
            print("    (killed: it never quit)")
    if verbose:
        print("    %ds on the map -> %.2f CPU s" % (seconds, cpu))
    return cpu


def measure(name, binary, display, maincode, scratch, short, long_, repeat,
            verbose):
    if not os.path.isfile(binary):
        print("SKIP %s: no binary at %s" % (name, binary))
        return None
    print("%s: %s" % (name, binary))
    slopes = []
    run = 0
    while run < repeat:
        short_cpu = run_once(binary, short, display, maincode,
                             os.path.join(scratch, name + "-short"), verbose)
        long_cpu = run_once(binary, long_, display, maincode,
                            os.path.join(scratch, name + "-long"), verbose)
        slopes.append((long_cpu - short_cpu) / (long_ - short))
        run += 1
    slopes.sort()
    median = slopes[len(slopes) // 2]
    print("  %s: %.3f CPU s per second on the map (%d run(s): %s)"
          % (name, median, repeat,
             ", ".join("%.3f" % value for value in slopes)))
    # A client that draws its map cannot cost almost nothing. Below this the window
    # was not repainting at all -- an X server with no compositor, an occluded or
    # unmapped window, a session that never sends frame callbacks -- and the number
    # measures the idle event loop, not the map. Saying so beats reporting a 1.00x
    # ratio that means nothing.
    if median < 0.01:
        print("  WARNING %s painted (almost) nothing: %.3f CPU s per second is the"
              " idle event loop, not the map. Run this on a session where the"
              " window is really visible." % (name, median))
        return None
    return median


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--qtopengl", default=None, help="qtopengl client binary")
    parser.add_argument("--qt800", default=None, help="qtcpu800x600 client binary")
    parser.add_argument("--maincode", default="test",
                        help="map label to play, empty for the client default")
    parser.add_argument("--short", type=int, default=8, help="short run, seconds")
    parser.add_argument("--long", type=int, default=28, help="long run, seconds")
    parser.add_argument("--repeat", type=int, default=1)
    parser.add_argument("--scratch", default="/tmp/cc-client-cpu")
    parser.add_argument("--quiet", action="store_true")
    arguments = parser.parse_args()

    if arguments.long <= arguments.short:
        print("--long has to be greater than --short")
        return 2
    if arguments.qtopengl is None and arguments.qt800 is None:
        print("nothing to measure: pass --qtopengl and/or --qt800")
        return 2

    xvfb = Xvfb()
    if not xvfb.start():
        print("no free X display for Xvfb")
        return 2
    try:
        results = {}
        for name, binary in (("qtopengl", arguments.qtopengl),
                             ("qtcpu800x600", arguments.qt800)):
            if binary is not None:
                results[name] = measure(name, binary, xvfb.display,
                                        arguments.maincode, arguments.scratch,
                                        arguments.short, arguments.long,
                                        arguments.repeat, not arguments.quiet)
        if results.get("qtopengl") and results.get("qtcpu800x600"):
            ratio = results["qtopengl"] / results["qtcpu800x600"]
            print("\nqtopengl costs %.2fx the CPU of qtcpu800x600 on the map"
                  % ratio)
        else:
            print("\nno usable comparison (see the warnings above)")
    finally:
        xvfb.stop()
    return 0


if __name__ == "__main__":
    sys.exit(main())
