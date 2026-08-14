#!/usr/bin/env python3
"""Regression guard for the map generator: two runs of the same binary with the
same settings must produce a BYTE IDENTICAL world.

The generator draws with customRand("<what for>"): one stream per reason, seeded
from [General] seed plus, per chunk, a salt made of (seed, chunk x, chunk y,
pass). Any accidental dependency on process state (an unordered_map iteration
order, an uninitialised value, a filesystem listing order) shows up here as a
differing file.

    ./check-determinism.py <build dir> [-- <extra generator args>]

Exit code 0 when the two runs match, 1 otherwise (differing files are listed).
"""

import argparse
import filecmp
import os
import shutil
import subprocess
import sys


def run_generator(build_dir, extra_args):
    binary = os.path.join(build_dir, "map-procedural-generation")
    if not os.path.isfile(binary):
        sys.exit("No %s — build first" % binary)
    # -platform offscreen: the tool links Qt Widgets but draws nothing
    command = [binary, "-platform", "offscreen"] + extra_args
    completed = subprocess.run(command, cwd=build_dir,
                               stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if completed.returncode != 0:
        sys.stdout.write(completed.stdout.decode("utf-8", "replace"))
        sys.exit("Generator failed with %d" % completed.returncode)
    return completed.stdout.decode("utf-8", "replace")


def label_dir(build_dir):
    """dest/map/main/<label>/ — the one label a run produces."""
    main = os.path.join(build_dir, "dest", "map", "main")
    labels = [name for name in sorted(os.listdir(main))
              if os.path.isdir(os.path.join(main, name)) and name != "tileset"]
    if len(labels) != 1:
        sys.exit("Expected exactly one label under %s, found %r" % (main, labels))
    return os.path.join(main, labels[0])


def compare_trees(left, right):
    """Every file of both trees, compared by content. Returns the mismatches."""
    differences = []
    left_files = set()
    for root, _dirs, files in os.walk(left):
        for name in files:
            left_files.add(os.path.relpath(os.path.join(root, name), left))
    right_files = set()
    for root, _dirs, files in os.walk(right):
        for name in files:
            right_files.add(os.path.relpath(os.path.join(root, name), right))
    for missing in sorted(left_files - right_files):
        differences.append("only in run 1: " + missing)
    for missing in sorted(right_files - left_files):
        differences.append("only in run 2: " + missing)
    for common in sorted(left_files & right_files):
        if not filecmp.cmp(os.path.join(left, common),
                           os.path.join(right, common), shallow=False):
            differences.append("differs: " + common)
    return differences


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("build_dir", help="directory holding map-procedural-generation")
    parser.add_argument("generator_args", nargs="*",
                        help="extra arguments forwarded to the generator")
    arguments = parser.parse_args()
    build_dir = os.path.abspath(arguments.build_dir)

    run_generator(build_dir, arguments.generator_args)
    first = label_dir(build_dir)
    # outside dest/map/main/, else the copy itself reads as a second map label
    keep = os.path.join(build_dir, "determinism-run1")
    if os.path.isdir(keep):
        shutil.rmtree(keep)
    shutil.copytree(first, keep)
    try:
        run_generator(build_dir, arguments.generator_args)
        second = label_dir(build_dir)
        differences = compare_trees(keep, second)
    finally:
        shutil.rmtree(keep)

    if differences:
        print("%d difference(s) between two runs:" % len(differences))
        for difference in differences[:40]:
            print("  " + difference)
        if len(differences) > 40:
            print("  ... and %d more" % (len(differences) - 40))
        return 1
    print("two runs are byte identical")
    return 0


if __name__ == "__main__":
    sys.exit(main())
