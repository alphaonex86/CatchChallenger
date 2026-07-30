#!/usr/bin/env python3
"""Validate (and mechanically repair) the building templates of
map-procedural-generation.

A building template is a folder holding one exterior `building-*.tmx` and its
interior `floor-N.tmx` + `floor-N.xml`.  Groups are `heal-{small,medium,big}`,
`shop-{small,medium,big}`, `gym-building` and the city styles `*-city`; a group
either holds the files directly or one numbered sub-folder per variant.

Checked here (the generator itself aborts on most of them anyway):
  * every `<tileset source=...>` resolves,
  * bot objects carry `type="bot"` (Map_loaderMain.cpp only reads those),
  * bot object ids are unique and match the `<bot id=>` of the sibling xml,
  * skins exist in `skin/`,
  * `backgroundsound` points at an existing music file of the datapack,
  * door / exit objects (informational: the generator wires them itself, but a
    hand-placed door is always kept, so a wrong one stays wrong).

`--fix` repairs only the mechanical ones, with line-level text edits (never an
XML round-trip: libtiled and Tiled do not write the same file back):
  * drop `<tileset>` lines whose source does not resolve WHEN another tileset
    with the same firstgid does resolve (the templates carry the pkmn-dataset
    path next to the good one, at the same firstgid),
  * `source="tileset/houses2"` -> `houses2.tsx`,
  * add `type="bot"` to an object of the `Object` group that has an `id`
    property and no type.
"""
import argparse
import base64
import os
import re
import struct
import sys
import zlib

try:
    import zstandard
except ImportError:
    zstandard = None

TEMPLATE_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                            "template")
SKIN_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "skin")
MUSIC_DIRS = ["../../../CatchChallenger-datapack/music"]


def variants(group_path):
    """[(label, folder)] of a group: numbered sub-folders, else the group."""
    subdirs = sorted(d for d in os.listdir(group_path)
                     if os.path.isdir(os.path.join(group_path, d)))
    if subdirs:
        return [(d, os.path.join(group_path, d)) for d in subdirs]
    return [("", group_path)]


def tileset_lines(text):
    """[(line_index, firstgid, source)] of every external tileset reference."""
    out = []
    index = 0
    for line in text.split("\n"):
        m = re.search(r'<tileset firstgid="(\d+)" source="([^"]*)"', line)
        if m:
            out.append((index, int(m.group(1)), m.group(2)))
        index += 1
    return out


def used_gids(text):
    """Every gid referenced by the tile layers, or None when undecodable."""
    out = set()
    for m in re.finditer(r'<data encoding="base64"([^>]*)>(.*?)</data>',
                         text, re.S):
        raw = base64.b64decode(m.group(2).strip())
        if "zlib" in m.group(1):
            raw = zlib.decompress(raw)
        elif "gzip" in m.group(1):
            raw = zlib.decompress(raw, 16 + zlib.MAX_WBITS)
        elif "zstd" in m.group(1):
            if zstandard is None:
                return None
            raw = zstandard.ZstdDecompressor().decompress(
                raw, max_output_size=1 << 22)
        count = len(raw) // 4
        for gid in struct.unpack("<%dI" % count, raw[:4 * count]):
            gid &= 0x1FFFFFFF
            if gid:
                out.add(gid)
    for m in re.finditer(r'<data encoding="csv">(.*?)</data>', text, re.S):
        for value in m.group(1).replace("\n", "").split(","):
            value = value.strip()
            if value and int(value):
                out.add(int(value) & 0x1FFFFFFF)
    return out


def object_blocks(text, group_name):
    """[(start, end, block)] of the objects of one object group."""
    out = []
    for grp in re.finditer(r'<objectgroup[^>]*name="' + group_name +
                           r'"[^>]*(?<!/)>(.*?)</objectgroup>', text, re.S):
        base = grp.start(1)
        for obj in re.finditer(r'<object\b[^>]*?(?:/>|>.*?</object>)',
                               grp.group(1), re.S):
            out.append((base + obj.start(), base + obj.end(), obj.group(0)))
    return out


def properties(block):
    return dict(re.findall(r'<property name="([^"]*)"(?:\s+type="[^"]*")?'
                           r'\s+value="([^"]*)"', block))


def object_type(block):
    m = re.search(r'<object\b[^>]*?\stype="([^"]*)"', block)
    return m.group(1) if m else None


def gid_range_used(gids, firstgid, resolved):
    """True when a used gid falls between firstgid and the next RESOLVABLE
    tileset above it — i.e. the missing tileset really is needed."""
    above = [g for g in resolved if g > firstgid]
    limit = min(above) if above else None
    for gid in gids:
        if gid >= firstgid and (limit is None or gid < limit):
            return True
    return False


def check_tilesets(path, text, problems, fixes):
    folder = os.path.dirname(path)
    lines = tileset_lines(text)
    resolved = {}
    for _, firstgid, source in lines:
        if os.path.exists(os.path.normpath(os.path.join(folder, source))):
            resolved.setdefault(firstgid, []).append(source)
    gids = used_gids(text)
    drop = []
    for line_index, firstgid, source in lines:
        if not os.path.exists(os.path.normpath(os.path.join(folder, source))):
            if source.endswith("houses2"):
                fixes.append((path, "tileset ref houses2 -> houses2.tsx"))
                text = text.replace('source="' + source + '"',
                                    'source="' + source + '.tsx"')
            elif resolved.get(firstgid):
                fixes.append((path, "drop unresolved tileset " + source +
                              " (firstgid " + str(firstgid) + " also " +
                              resolved[firstgid][0] + ")"))
                drop.append(line_index)
            elif gids is not None and not gid_range_used(gids, firstgid,
                                                         resolved):
                fixes.append((path, "drop unresolved tileset " + source +
                              " (no tile of firstgid " + str(firstgid) +
                              ".. is used)"))
                drop.append(line_index)
            else:
                problems.append((path, "tileset not found and its tiles ARE "
                                 "used: " + source))
    if drop:
        kept = [l for i, l in enumerate(text.split("\n")) if i not in drop]
        text = "\n".join(kept)
    return text


def check_objects(path, text, problems, warnings, fixes):
    #the engine reads bots ONLY from the group named "Object"
    #(Map_loaderMain.cpp); the generator moves the misplaced ones, but the
    #template author should see them in the right group in Tiled
    for group in re.finditer(r'<objectgroup[^>]*name="([^"]*)"[^>]*(?<!/)>'
                             r"(.*?)</objectgroup>", text, re.S):
        if group.group(1) != "Object":
            for obj in re.finditer(r"<object\b[^>]*?(?:/>|>.*?</object>)",
                                   group.group(2), re.S):
                props = properties(obj.group(0))
                kind = object_type(obj.group(0))
                if kind == "bot" or ("id" in props and kind not in
                                     ("door", "teleport on it",
                                      "teleport on push", "rescue")):
                    warnings.append((path, "bot object in the \"" +
                                     group.group(1) + "\" group (id " +
                                     props.get("id", "?") + ") - the engine "
                                     "only reads the \"Object\" group, the "
                                     "generator moves it"))
    seen_ids = {}
    for start, end, block in reversed(object_blocks(text, "Object")):
        props = properties(block)
        if "id" not in props:
            problems.append((path, "object of the Object group without an id "
                             "property"))
        else:
            seen_ids.setdefault(props["id"], 0)
            seen_ids[props["id"]] += 1
        if object_type(block) is None and "id" in props:
            fixes.append((path, 'bot object id=' + props["id"] +
                          ' got type="bot"'))
            patched = re.sub(r'(<object id="\d+")', r'\1 type="bot"', block,
                             count=1)
            text = text[:start] + patched + text[end:]
        if "skin" in props and props["skin"]:
            if not os.path.isdir(os.path.join(SKIN_DIR, props["skin"])):
                warnings.append((path, "skin not in skin/: " + props["skin"] +
                                 " (bot id " + props.get("id", "?") +
                                 ") - the generator remaps it to the role "
                                 "skin of settings.xml"))
    for bot_id, count in seen_ids.items():
        if count > 1:
            warnings.append((path, "bot object id " + bot_id + " used " +
                             str(count) + " times - the generator renumbers "
                             "the duplicates"))
    return text


def check_xml(path, problems, warnings):
    """bot ids of the sibling xml vs the tmx, plus backgroundsound."""
    tmx = path
    xml = path[:-4] + ".xml"
    if not os.path.exists(xml):
        problems.append((path, "no sibling xml"))
        return
    text = open(xml, encoding="utf-8").read()
    xml_ids = set(re.findall(r'<bot id="(\d+)"', text))
    tmx_ids = set()
    for _, _, block in object_blocks(open(tmx, encoding="utf-8").read(),
                                     "Object"):
        props = properties(block)
        if "id" in props:
            tmx_ids.add(props["id"])
    if xml_ids - tmx_ids:
        warnings.append((xml, "bot(s) " + ",".join(sorted(xml_ids - tmx_ids)) +
                         " declared but no object in the tmx - dropped"))
    if tmx_ids - xml_ids:
        warnings.append((xml, "object bot(s) " +
                         ",".join(sorted(tmx_ids - xml_ids)) +
                         " have no <bot> in the xml - a text bot is "
                         "generated for them"))
    sound = re.search(r'backgroundsound="([^"]*)"', text)
    if sound:
        found = False
        for music in MUSIC_DIRS:
            candidate = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                     music, os.path.basename(sound.group(1)))
            if os.path.exists(candidate):
                found = True
        if not found:
            warnings.append((xml, "backgroundsound file not in the datapack: " +
                             sound.group(1) + " - dropped"))


def has_teleport(text):
    for group in ("Moving", "Object"):
        for _, _, block in object_blocks(text, group):
            if object_type(block) in ("door", "teleport on push",
                                      "teleport on it"):
                return True
    return False


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--fix", action="store_true",
                        help="apply the mechanical repairs")
    parser.add_argument("--template-dir", default=TEMPLATE_DIR)
    args = parser.parse_args()

    problems = []
    warnings = []
    fixes = []
    infos = []
    groups = sorted(d for d in os.listdir(args.template_dir)
                    if os.path.isdir(os.path.join(args.template_dir, d)))
    for group in groups:
        for label, folder in variants(os.path.join(args.template_dir, group)):
            name = group + ("/" + label if label else "")
            #the exterior is the tmx that is not a floor (building-house.tmx,
            #building-heal.tmx, gym-building.tmx...)
            exteriors = sorted(f for f in os.listdir(folder)
                               if f.endswith(".tmx") and
                               not f.startswith("floor-"))
            floors = sorted(f for f in os.listdir(folder)
                            if f.startswith("floor-") and f.endswith(".tmx"))
            if not floors:
                problems.append((folder, "no floor-N.tmx interior"))
            if len(exteriors) > 1:
                problems.append((folder, "more than one exterior tmx: " +
                                 str(exteriors)))
            for tmx in exteriors + floors:
                path = os.path.join(folder, tmx)
                text = open(path, encoding="utf-8").read()
                patched = check_tilesets(path, text, problems, fixes)
                patched = check_objects(path, patched, problems, warnings, fixes)
                if not has_teleport(patched):
                    infos.append((path, "no door/exit object (the generator "
                                  "wires one in)"))
                if patched != text and args.fix:
                    open(path, "w", encoding="utf-8").write(patched)
            for tmx in floors:
                check_xml(os.path.join(folder, tmx), problems, warnings)
            if not exteriors:
                problems.append((folder, "no exterior tmx"))
            del name

    for path, message in fixes:
        print(("FIXED " if args.fix else "TOFIX ") +
              os.path.relpath(path, args.template_dir) + ": " + message)
    for path, message in infos:
        print("INFO  " + os.path.relpath(path, args.template_dir) + ": " +
              message)
    for path, message in warnings:
        print("WARN  " + os.path.relpath(path, args.template_dir) + ": " +
              message)
    for path, message in problems:
        print("ERROR " + os.path.relpath(path, args.template_dir) + ": " +
              message)
    print("\n%d repairable, %d informational, %d warnings, %d problems" %
          (len(fixes), len(infos), len(warnings), len(problems)))
    return 1 if problems else 0


sys.exit(main())
