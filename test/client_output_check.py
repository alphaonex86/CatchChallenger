#!/usr/bin/env python3
"""Whitelist for what a client prints, so a NEW message cannot hide in the noise.

A client run prints 400-500 lines: timings, SQL traces, datapack warnings, Qt and
audio-stack chatter. Reading that by hand is how a new "Unknown type: ..." or a
fresh warning goes unnoticed for weeks. So every line is normalised (numbers,
paths, pointers, quoted names replaced by placeholders) and looked up in
test/client_output_whitelist.txt; whatever is not in there is reported.

  # what does this run print that we have never seen?
  test/client_output_check.py <client-log>...
  # accept the current output as the new baseline (review the diff!)
  test/client_output_check.py --learn <client-log>...

testingclient.py calls unknown_lines() after each client run and prints what it
gets back. It only FAILS the run with --strict-client-output (or
CC_STRICT_CLIENT_OUTPUT=1): a third-party line (pipewire, ffmpeg, the GL driver)
differs from host to host, and turning every such difference into a red test would
make the fleet permanently red for reasons that are not ours.
"""
import argparse
import os
import re
import sys

WHITELIST = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                         "client_output_whitelist.txt")

#ordered: the most specific first, each one hides a value that legitimately varies
NORMALISERS = [
    (re.compile(r"0x[0-9a-fA-F]+"), "<ptr>"),
    (re.compile(r"(/[^\s\"']+)+"), "<path>"),
    #no word boundary: "31ms", "v2", "id=7" all carry a value that varies per run
    (re.compile(r"[0-9]+(\.[0-9]+)?"), "<n>"),
    (re.compile(r"[ \t]+"), " "),
]


def normalise(line):
    text = line.strip()
    for pattern, replacement in NORMALISERS:
        text = pattern.sub(replacement, text)
    return text.strip()


def load(path=WHITELIST):
    known = set()
    if not os.path.isfile(path):
        return known
    for line in open(path, encoding="utf-8"):
        line = line.rstrip("\n")
        if line.strip() and not line.startswith("#"):
            known.add(line)
    return known


def unknown_lines(lines, known=None):
    """The lines of this run nobody has ever accepted, deduplicated, in order."""
    if known is None:
        known = load()
    seen = set()
    unknown = []
    for line in lines:
        pattern = normalise(line)
        if not pattern or pattern in known or pattern in seen:
            continue
        seen.add(pattern)
        unknown.append(line.strip())
    return unknown


def learn(paths, path=WHITELIST):
    known = load(path)
    added = []
    for source in paths:
        for line in open(source, encoding="utf-8", errors="replace"):
            pattern = normalise(line)
            if pattern and pattern not in known:
                known.add(pattern)
                added.append(pattern)
    header = ("# Normalised lines a client is KNOWN to print (numbers -> <n>,\n"
              "# paths -> <path>, pointers -> <ptr>). Regenerate/extend with:\n"
              "#   test/client_output_check.py --learn <client-log>...\n"
              "# and REVIEW the diff: a new line here means we accepted a new\n"
              "# message, which is exactly the thing this file exists to show.\n")
    with open(path, "w", encoding="utf-8") as handle:
        handle.write(header)
        for pattern in sorted(known):
            handle.write(pattern + "\n")
    return added


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("logs", nargs="+")
    parser.add_argument("--learn", action="store_true",
                        help="accept every line of these logs as known")
    parser.add_argument("--whitelist", default=WHITELIST)
    arguments = parser.parse_args()
    if arguments.learn:
        added = learn(arguments.logs, arguments.whitelist)
        print("%d new pattern(s) accepted into %s"
              % (len(added), arguments.whitelist))
        return 0
    known = load(arguments.whitelist)
    total = 0
    for source in arguments.logs:
        lines = open(source, encoding="utf-8", errors="replace").read().split("\n")
        unknown = unknown_lines(lines, known)
        if unknown:
            print("%s: %d line(s) not in the whitelist" % (source, len(unknown)))
            for line in unknown:
                print("  | " + line)
        total += len(unknown)
    print("%d unknown line(s)" % total)
    return 1 if total else 0


if __name__ == "__main__":
    sys.exit(main())
