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

testingclient.py calls unknown_lines() after each client run, prints what it gets
back, and FAILS the run on it. CC_STRICT_CLIENT_OUTPUT=0 downgrades that to a
warning for a host whose third-party stack (pipewire, ffmpeg, the GL driver) prints
something this list has never seen.

Two things keep the list from growing without bound, and both are load bearing: a
datapack checksum is normalised as one <hash> instead of being shredded into
<n>E<n>F<n>FA..., and a line that is the SPLICE of two known ones is accepted --
several threads write to std::cerr with no lock, so messages regularly land on top
of each other and every splice is a one-off string. Without them the list needed
1434 patterns for one suite run and still did not converge; with them, 277.
"""
import argparse
import os
import re
import sys

WHITELIST = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                         "client_output_whitelist.txt")

#ordered: the most specific first, each one hides a value that legitimately varies.
#The hash rule has to run BEFORE the number one: a datapack checksum is half digits,
#so the number rule alone shredded it into <n>E<n>F<n>FA... and every checksum then
#looked like a brand new message nobody could ever whitelist.
NORMALISERS = [
    (re.compile(r"0x[0-9a-fA-F]+"), "<ptr>"),
    (re.compile(r"\b[0-9A-Fa-f]{16,}\b"), "<hash>"),
    (re.compile(r"(\.\.)?(/[^\s\"']+)+"), "<path>"),
    #no word boundary: "31ms", "v2", "id=7" all carry a value that varies per run
    (re.compile(r"[0-9]+(\.[0-9]+)?"), "<n>"),
    (re.compile(r"[ \t]+"), " "),
]


def normalise(line):
    text = line.strip()
    for pattern, replacement in NORMALISERS:
        text = pattern.sub(replacement, text)
    return text.strip()


def _known_or_fragment(part, known):
    """Exactly a line we know, or a torn piece of one."""
    if not part:
        return False
    if part in known:
        return True
    for other in known:
        if len(other) > len(part) and (other.startswith(part) or
                                       other.endswith(part)):
            return True
    return False


def is_known(pattern, known):
    """Known outright, or what unsynchronised threads made of known lines.

    Several threads write to std::cerr with no lock, so messages land on top of each
    other: appended, inserted mid-message, or cut in half across two lines. Every
    such collision is a one-off string, so learning them never converges -- 814
    unknown patterns for one suite run, all unique. Recognising the pieces instead
    keeps the check on what it is for, a message nobody has ever seen. The
    interleaving itself is a logging defect, not a new message.
    """
    if pattern in known:
        return True
    #two messages on one line, in either order
    cut = 1
    while cut < len(pattern):
        if _known_or_fragment(pattern[:cut], known) and \
                _known_or_fragment(pattern[cut:], known):
            return True
        cut += 1
    #one message dropped INSIDE another
    for other in known:
        start = pattern.find(other)
        if start > 0 and len(other) > 20:
            if _known_or_fragment(pattern[:start] +
                                  pattern[start + len(other):], known):
                return True
    return False


def carries_franchise(pattern, forbidden=None):
    """A line quoting a ROM-converted datapack's own creature/place names.

    learn() refuses those on purpose -- committing them is the thing we may not do
    -- so they would fail every run for ever. Reported, never fatal.
    """
    if forbidden is None:
        forbidden = franchise_words()
    lowered = pattern.lower()
    return any(word in lowered for word in forbidden)


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
        if not pattern or pattern in seen or is_known(pattern, known):
            continue
        seen.add(pattern)
        unknown.append(line.strip())
    return unknown


def franchise_words():
    """The operator-local denylist, empty when this machine has none."""
    tools = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                         "tools", "map-procedural-generation")
    if tools not in sys.path:
        sys.path.insert(0, tools)
    try:
        import franchise_denylist
    except ImportError:
        return []
    return franchise_denylist.load(quiet=True)


def learn(paths, path=WHITELIST):
    known = load(path)
    added = []
    #A client run against a ROM-converted datapack prints the creature and place
    #names of that ROM. Accepting those lines verbatim would commit the very words
    #the project may not carry, so they are never learned: the run keeps reporting
    #them, which is correct, they are not ours to whitelist.
    forbidden = franchise_words()
    refused = []
    for source in paths:
        for line in open(source, encoding="utf-8", errors="replace"):
            pattern = normalise(line)
            if not pattern or pattern in known:
                continue
            if is_known(pattern, known):
                continue
            lowered = pattern.lower()
            if any(word in lowered for word in forbidden):
                if pattern not in refused:
                    refused.append(pattern)
                continue
            known.add(pattern)
            added.append(pattern)
    if refused:
        print("%d pattern(s) NOT learned: they carry franchise wording (a "
              "ROM-converted datapack prints it, we may not commit it)"
              % len(refused))
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
