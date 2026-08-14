#!/usr/bin/env python3
"""Seam guard for the map generator: adding a TEMPLATE must change the map where
that template lands, and NOWHERE else.

The generator draws with customRand("<reason>"), one stream per reason, and every
how-use.ini driven template draws on a stream named after its OWN
how-use.ini path (LoadMapAll::TemplateUse::reason). That is what keeps the blast
radius of a content change small: with a single shared stream, one more optional
template scanned before the others consumed one more number per chunk and moved
every map of the world.

This script measures it. It runs the generator, drops ONE extra decoration next
to template/on-walkable/ (a copy of an existing variant -- no art is created --
set to a low mapPercent), runs it again and counts the differing files. Only the
maps where the extra decoration really landed may differ.

    ./check-seam.py <build dir> [--map-percent 2] [--max-diff-percent 20]

Exit code 0 when the change stayed local, 1 otherwise.
"""

import argparse
import os
import shutil
import subprocess
import sys

# sorts FIRST on purpose: the templates are scanned in name order, so a probe
# added at the end of the list would have nothing left to shift and the test
# would pass whatever the code does.
PROBE = "aa-seam-probe"


def run_generator(build_dir):
    binary = os.path.join(build_dir, "map-procedural-generation")
    if not os.path.isfile(binary):
        sys.exit("No %s -- build first" % binary)
    # -platform offscreen: the tool links Qt Widgets but draws nothing
    completed = subprocess.run([binary, "-platform", "offscreen"], cwd=build_dir,
                               stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if completed.returncode != 0:
        sys.stdout.write(completed.stdout.decode("utf-8", "replace"))
        sys.exit("Generator failed with %d" % completed.returncode)


def label_dir(build_dir):
    """dest/map/main/<label>/ -- the one label a run produces."""
    main = os.path.join(build_dir, "dest", "map", "main")
    labels = [name for name in sorted(os.listdir(main))
              if os.path.isdir(os.path.join(main, name)) and name != "tileset"]
    if len(labels) != 1:
        sys.exit("Expected exactly one label under %s, found %r" % (main, labels))
    return os.path.join(main, labels[0])


def read_tree(root):
    """relative path -> bytes, for every file of the tree."""
    content = {}
    for directory, _subdirectories, names in os.walk(root):
        for name in names:
            path = os.path.join(directory, name)
            with open(path, "rb") as handle:
                content[os.path.relpath(path, root)] = handle.read()
    return content


def materialise_templates(build_dir):
    """The build directory LINKS template/ to the sources. Replace the link with a
    real copy so the probe is never written into the repository. Returns what has
    to be put back."""
    templates = os.path.join(build_dir, "template")
    if not os.path.islink(templates):
        return None
    target = os.readlink(templates)
    os.remove(templates)
    shutil.copytree(target, templates, symlinks=False)
    return target


def restore_templates(build_dir, link_target):
    templates = os.path.join(build_dir, "template")
    if link_target is None:
        shutil.rmtree(os.path.join(templates, "on-walkable", PROBE), ignore_errors=True)
    else:
        shutil.rmtree(templates)
        os.symlink(link_target, templates)


def add_probe(build_dir, map_percent):
    """Copy an existing on-walkable variant under a new name. No art is created:
    the tmx and its tileset reference are the ones already drawn, and the copy is
    a sibling so its relative tileset path still resolves."""
    walkable = os.path.join(build_dir, "template", "on-walkable")
    sources = [name for name in sorted(os.listdir(walkable))
               if os.path.isfile(os.path.join(walkable, name, "how-use.ini"))]
    if not sources:
        sys.exit("No template/on-walkable/<variant>/how-use.ini to copy")
    probe = os.path.join(walkable, PROBE)
    shutil.rmtree(probe, ignore_errors=True)
    shutil.copytree(os.path.join(walkable, sources[0]), probe)
    with open(os.path.join(probe, "how-use.ini"), "w", encoding="utf-8") as handle:
        handle.write("; written by check-seam.py, not part of the datapack\n"
                     "[use]\nmapPercent=%d\nmin=1\nmax=1\n" % map_percent)
    return sources[0]


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("build_dir", help="directory holding map-procedural-generation")
    parser.add_argument("--map-percent", type=int, default=2,
                        help="how often the probe decoration is used (default 2%%)")
    # measured: the per-template streams leave 0.7% of the files differing (the
    # maps the probe really landed on, plus the all.tmx dump), the single shared
    # stream this replaced left 17.5%.
    parser.add_argument("--max-diff-percent", type=int, default=5,
                        help="share of the maps allowed to differ (default 5%%)")
    arguments = parser.parse_args()
    build_dir = os.path.abspath(arguments.build_dir)

    run_generator(build_dir)
    before = read_tree(label_dir(build_dir))

    link_target = materialise_templates(build_dir)
    try:
        copied = add_probe(build_dir, arguments.map_percent)
        print("probe: template/on-walkable/%s copied to %s at mapPercent=%d"
              % (copied, PROBE, arguments.map_percent))
        run_generator(build_dir)
        after = read_tree(label_dir(build_dir))
    finally:
        restore_templates(build_dir, link_target)

    names = sorted(set(before) | set(after))
    differing = [name for name in names
                 if before.get(name) != after.get(name)]
    total = len(names)
    if total == 0:
        sys.exit("The generator wrote no file at all")
    share = 100 * len(differing) / total
    print("%d of %d file(s) differ (%.1f%%) after adding ONE template"
          % (len(differing), total, share))
    for name in differing[:20]:
        print("  " + name)
    if len(differing) > 20:
        print("  ... and %d more" % (len(differing) - 20))
    if share > arguments.max_diff_percent:
        print("FAIL: one added template moved more than %d%% of the world -- a draw "
              "is sharing a customRand() stream across templates"
              % arguments.max_diff_percent)
        return 1
    print("the change stayed local")
    return 0


if __name__ == "__main__":
    sys.exit(main())
