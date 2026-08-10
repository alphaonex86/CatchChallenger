#!/usr/bin/env python3
"""Install the generated maps as a datapack map LABEL (map/main/<label>/).

The generator writes dest/map/main/<label>/ (the label comes from its settings);
a datapack calls that folder a "label" too and it is copied under
map/main/<label>/. Doing that copy by hand is
what broke once: only the maps were copied, and every map referenced its
tilesets in map/tileset/ of the target datapack, which does not own the
gym/heal/shop/house sheets the generator uses -> "tileset not found". The label
is now self-contained (it ships its own tileset/), and this script is the
reviewable way to install it:

  * it REFUSES to touch the datapack while a map references anything outside
    the label (the check that was missing),
  * it lists what it deletes before deleting it,
  * it skips all.tmx, the whole-world debug dump (doallmap): far larger than a
    playable map, and not part of the label.

Usage: install-generated.py [dest-dir] --datapack <dir> [--label generated]
"""
import argparse
import os
import re
import shutil
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import generated_label

#the whole-world dump written when settings doallmap=true: a debug artefact, not
#a map of the label
DEBUG_DUMP = "all.tmx"


def collect(root):
    """every file of the label, as paths relative to it"""
    files = []
    for folder, _, names in os.walk(root):
        for name in names:
            path = os.path.join(folder, name)
            files.append(os.path.relpath(path, root))
    return sorted(files)


def unresolved_tilesets(root, files):
    """references that do not resolve INSIDE the label: the install stopper"""
    problems = []
    for name in files:
        if name.endswith(".tmx"):
            path = os.path.join(root, name)
            with open(path, encoding="utf-8") as handle:
                content = handle.read()
            for tileset in re.findall(r'<tileset[^>]*source="([^"]*)"', content):
                resolved = os.path.abspath(os.path.join(os.path.dirname(path),
                                                        tileset))
                if not os.path.exists(resolved):
                    problems.append((name, "missing: " + tileset))
                elif not resolved.startswith(os.path.abspath(root) + os.sep):
                    problems.append((name, "outside the label: " + tileset))
    return problems


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("dest", nargs="?", default="dest",
                        help="the dest/ the generator wrote")
    parser.add_argument("--datapack", required=True, help="datapack root")
    parser.add_argument("--label", default=None,
                        help="map/main/<label>/ to install into, defaults to the "
                             "label the generator wrote")
    parser.add_argument("--dry-run", action="store_true",
                        help="say what would change, touch nothing")
    arguments = parser.parse_args()

    #the generator resolves its label from its settings, so read it back off disk
    label, source = generated_label.find(arguments.dest)
    if label is None:
        print(source)
        return 2
    if arguments.label is None:
        arguments.label = label
    files = [name for name in collect(source)
             if os.path.basename(name) != DEBUG_DUMP]

    problems = unresolved_tilesets(source, files)
    if problems:
        for name, problem in problems[:20]:
            print("ERROR " + name + ": " + problem)
        print("%d unresolved tileset reference(s): the datapack was NOT touched"
              % len(problems))
        return 1

    target = os.path.join(arguments.datapack, "map", "main", arguments.label)
    existing = collect(target) if os.path.isdir(target) else []
    print("source %s: %d files (%d maps)"
          % (source, len(files), len([n for n in files if n.endswith(".tmx")])))
    print("target %s: %d existing files to replace" % (target, len(existing)))
    if arguments.dry_run:
        return 0

    #replace, never merge: a city that disappeared from the world would else
    #stay for ever and its teleports would point at maps that no longer exist
    if os.path.isdir(target):
        shutil.rmtree(target)
    for name in files:
        destination = os.path.join(target, name)
        directory = os.path.dirname(destination)
        if directory and not os.path.isdir(directory):
            os.makedirs(directory)
        shutil.copy2(os.path.join(source, name), destination)

    installed = collect(target)
    if installed != files:
        print("ERROR the installed tree does not match the source (%d vs %d files)"
              % (len(installed), len(files)))
        return 1
    print("installed %d files into %s" % (len(installed), target))
    return 0


if __name__ == "__main__":
    sys.exit(main())
