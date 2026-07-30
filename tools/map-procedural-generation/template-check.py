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


# ---------------------------------------------------------------- geometry
# The generator computes the door/exit cells the same way (LoadMapBuilding.cpp);
# writing them into the template keeps Tiled and the generator in agreement.


def map_size(text):
    header = re.search(r"<map[^>]*>", text).group(0)
    return (int(re.search(r'\swidth="(\d+)"', header).group(1)),
            int(re.search(r'\sheight="(\d+)"', header).group(1)),
            int(re.search(r'\stilewidth="(\d+)"', header).group(1)),
            int(re.search(r'\stileheight="(\d+)"', header).group(1)))


def tile_layers(text):
    """{name: [(w, h, gids)]} of the base64 encoded tile layers."""
    out = {}
    for m in re.finditer(r'<layer[^>]*name="([^"]*)"[^>]*width="(\d+)" '
                         r'height="(\d+)"[^>]*>\s*<data encoding="base64"'
                         r"([^>]*)>(.*?)</data>", text, re.S):
        name, w, h, attributes, payload = (m.group(1), int(m.group(2)),
                                           int(m.group(3)), m.group(4),
                                           m.group(5))
        raw = base64.b64decode(payload.strip())
        if "zlib" in attributes:
            raw = zlib.decompress(raw)
        elif "gzip" in attributes:
            raw = zlib.decompress(raw, 16 + zlib.MAX_WBITS)
        elif "zstd" in attributes:
            if zstandard is None:
                return None
            raw = zstandard.ZstdDecompressor().decompress(
                raw, max_output_size=w * h * 4)
        out.setdefault(name, []).append(
            (w, h, struct.unpack("<%dI" % (w * h), raw[:4 * w * h])))
    return out


def is_collision(layers, width, height, x, y):
    if x < 0 or y < 0 or x >= width or y >= height:
        return True
    for (w, h, grid) in layers.get("Collisions", []):
        if w == width and h == height and grid[x + y * w]:
            return True
    return False


def doorstep_cell(layers, width, height):
    """The free cell under the building body, on its centre column."""
    columns = []
    step = 0
    while step < width:
        offset = (step + 1) // 2
        columns.append(width // 2 - offset if step % 2 == 0
                       else width // 2 + offset)
        step += 1
    for column in columns:
        if 0 <= column < width:
            lowest = -1
            row = 0
            while row < height:
                if is_collision(layers, width, height, column, row):
                    lowest = row
                row += 1
            if lowest >= 0:
                row = lowest + 1
                while row < height and is_collision(layers, width, height,
                                                    column, row):
                    row += 1
                return column, row
    return width // 2, height


def exit_cell(layers, width, height):
    """Bottom row: the doorway gap, else the wall cell under a free one."""
    row = height - 1
    best = -1
    for column in range(width):
        if not is_collision(layers, width, height, column, row):
            if best < 0 or abs(column - width // 2) < abs(best - width // 2):
                best = column
    if best < 0:
        for column in range(width):
            if not is_collision(layers, width, height, column, row - 1):
                if best < 0 or abs(column - width // 2) < abs(best - width // 2):
                    best = column
    if best < 0:
        best = width // 2
    return best, row


def spawn_cell(layers, width, height, exitX, exitY):
    row = exitY - 1
    while row > 0 and is_collision(layers, width, height, exitX, row):
        row -= 1
    return exitX, max(row, 0)


def free_cell_near(layers, width, height, x, y):
    """Closest cell that is free AND has a free neighbour (talkable bot)."""
    best = None
    for row in range(height):
        for column in range(width):
            if is_collision(layers, width, height, column, row):
                continue
            neighbours = 0
            for dx, dy in ((0, 1), (0, -1), (1, 0), (-1, 0)):
                if not is_collision(layers, width, height, column + dx,
                                    row + dy):
                    neighbours += 1
            if neighbours == 0:
                continue
            distance = abs(column - x) + abs(row - y)
            if best is None or distance < best[0]:
                best = (distance, column, row)
    return (best[1], best[2]) if best else None


def invisible_gid(text):
    """gid of the marker tile 3 of the invisible tileset, None when absent."""
    for m in re.finditer(r'<tileset firstgid="(\d+)" source="([^"]*)"', text):
        if os.path.basename(m.group(2)).startswith("invisible"):
            return int(m.group(1)) + 3
    return None


def insert_object(text, group_name, object_xml):
    """Add one object to a group, creating its body when self-closing."""
    identifier = re.search(r'nextobjectid="(\d+)"', text)
    next_id = int(identifier.group(1)) if identifier else 1000
    object_xml = object_xml.replace("@ID@", str(next_id))
    if identifier:
        text = text[:identifier.start(1)] + str(next_id + 1) + \
            text[identifier.end(1):]
    selfClosed = re.search(r'<objectgroup[^>]*name="' + group_name +
                           r'"[^>]*/>', text)
    if selfClosed:
        opening = selfClosed.group(0)[:-2] + ">"
        return (text[:selfClosed.start()] + opening + "\n" + object_xml +
                " </objectgroup>" + text[selfClosed.end():])
    group = re.search(r'<objectgroup[^>]*name="' + group_name +
                      r'"[^>]*(?<!/)>(.*?)</objectgroup>', text, re.S)
    if group:
        return text[:group.end(1)] + object_xml + text[group.end(1):]
    #no such group: add it before the closing tag of the map
    body = (" <objectgroup name=\"" + group_name + "\">\n" + object_xml +
            " </objectgroup>\n")
    return text.replace("</map>", body + "</map>")


def teleport_xml(gid, x, y, tile_width, tile_height, target, targetX, targetY):
    return ("  <object id=\"@ID@\" type=\"teleport on push\"" +
            (" gid=\"%d\"" % gid if gid else "") +
            " x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\">\n"
            "   <properties>\n"
            "    <property name=\"map\" value=\"%s\"/>\n"
            "    <property name=\"x\" value=\"%d\"/>\n"
            "    <property name=\"y\" value=\"%d\"/>\n"
            "   </properties>\n"
            "  </object>\n" % (x * tile_width, y * tile_height, tile_width,
                               tile_height, target, targetX, targetY))


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


# ---------------------------------------------------------------- repairs
# Skins used when the template names one the datapack has not, by bot role.
ROLE_SKIN = {"healer": "nurse", "shopkeeper": "market", "storage": "bankier",
             "trainer": "smith", "villager": "oldman"}


def bot_role(xml_text, bot_id):
    bot = re.search(r'<bot id="' + bot_id + r'"(.*?)</bot>', xml_text, re.S)
    steps = re.findall(r'<step[^>]*type="([a-z]+)"', bot.group(1)) if bot else []
    if "heal" in steps:
        return "healer"
    if "shop" in steps or "sell" in steps:
        return "shopkeeper"
    if "warehouse" in steps:
        return "storage"
    if "fight" in steps:
        return "trainer"
    return "villager"


def move_object(text, block_start, block_end, block, x, y, tile_width,
                tile_height):
    patched = re.sub(r'\sx="[-0-9.]+"', ' x="%d"' % (x * tile_width), block,
                     count=1)
    patched = re.sub(r'\sy="[-0-9.]+"', ' y="%d"' % (y * tile_height), patched,
                     count=1)
    return text[:block_start] + patched + text[block_end:]


def fix_variant(folder, exterior, floors, fixes, warnings, apply_fix):
    """Everything that needs the geometry or the two files of a template."""
    if exterior is None or not floors:
        return
    exterior_path = os.path.join(folder, exterior)
    floor_path = os.path.join(folder, floors[0])
    exterior_text = open(exterior_path, encoding="utf-8").read()
    floor_text = open(floor_path, encoding="utf-8").read()
    exterior_name = exterior[:-4]
    floor_name = floors[0][:-4]
    ew, eh, etw, eth = map_size(exterior_text)
    fw, fh, ftw, fth = map_size(floor_text)
    exterior_layers = tile_layers(exterior_text)
    floor_layers = tile_layers(floor_text)
    if exterior_layers is None or floor_layers is None:
        warnings.append((folder, "zstd layers unreadable (python-zstandard "
                         "missing): geometry not checked"))
        return

    doorX, doorY = doorstep_cell(exterior_layers, ew, eh)
    exitX, exitY = exit_cell(floor_layers, fw, fh)
    spawnX, spawnY = spawn_cell(floor_layers, fw, fh, exitX, exitY)

    #1. the exterior door: create it, or move a hand placed one off a collision
    doors = [(s, e, b) for s, e, b in object_blocks(exterior_text, "Moving") +
             object_blocks(exterior_text, "Object")
             if object_type(b) in ("door", "teleport on push", "teleport on it")]
    if not doors:
        fixes.append((exterior_path, "door added at %d,%d -> %s (%d,%d)" %
                      (doorX, doorY, floor_name, spawnX, spawnY)))
        exterior_text = insert_object(
            exterior_text, "Moving",
            teleport_xml(invisible_gid(exterior_text), doorX, doorY, etw, eth,
                         floor_name, spawnX, spawnY))
    else:
        start, end, block = doors[0]
        props = properties(block)
        x = int(float(re.search(r'\sx="([-0-9.]+)"', block).group(1))) // etw
        y = int(float(re.search(r'\sy="([-0-9.]+)"', block).group(1))) // eth
        if y < eh and is_collision(exterior_layers, ew, eh, x, y):
            #the doorstep is the cell the PLAYER stands on: it cannot be part
            #of the building, else nobody can reach the door. Below the rect is
            #the city ground, always free.
            newY = y
            while newY < eh and is_collision(exterior_layers, ew, eh, x, newY):
                newY += 1
            fixes.append((exterior_path, "door moved off the building: %d,%d "
                          "-> %d,%d" % (x, y, x, newY)))
            exterior_text = move_object(exterior_text, start, end, block, x,
                                        newY, etw, eth)
            doorX, doorY = x, newY
        else:
            doorX, doorY = x, y
        #the target is rewritten below, once the real exit is known
        wanted = {}
        del wanted

    #2. the interior exit
    exits = [(s, e, b) for s, e, b in object_blocks(floor_text, "Moving") +
             object_blocks(floor_text, "Object")
             if object_type(b) in ("door", "teleport on push", "teleport on it")]
    if exits:
        #a hand placed exit wins: the spawn is the free cell above IT
        block = exits[0][2]
        exitX = int(float(re.search(r'\sx="([-0-9.]+)"', block).group(1))) // ftw
        exitY = int(float(re.search(r'\sy="([-0-9.]+)"', block).group(1))) // fth
        spawnX, spawnY = spawn_cell(floor_layers, fw, fh, exitX, exitY - 1)
    if not exits:
        fixes.append((floor_path, "exit added at %d,%d -> %s (%d,%d)" %
                      (exitX, exitY, exterior_name, doorX, doorY)))
        floor_text = insert_object(
            floor_text, "Moving",
            teleport_xml(invisible_gid(floor_text), exitX, exitY + 1, ftw, fth,
                         exterior_name, doorX, doorY))
    else:
        block = exits[0][2]
        props = properties(block)
        wanted = {"map": exterior_name, "x": str(doorX), "y": str(doorY)}
        missing = [k for k in wanted if k not in props]
        if missing or any(props.get(k) != v for k, v in wanted.items()):
            fixes.append((floor_path, "exit target %s -> %s %d,%d" %
                          (props.get("map"), exterior_name, doorX, doorY)))
            if apply_fix:
                start, end = exits[0][0], exits[0][1]
                patched = block
                for key, value in wanted.items():
                    if ('name="' + key + '"') in patched:
                        patched = re.sub(
                            r'(<property name="' + key +
                            r'"(?:\s+type="[^"]*")?\s+value=")[^"]*(")',
                            r"\g<1>" + value + r"\g<2>", patched, count=1)
                    else:
                        insertion = ('    <property name="%s" value="%s"/>\n'
                                     % (key, value))
                        if "<properties>" in patched:
                            patched = patched.replace("<properties>\n",
                                                      "<properties>\n" +
                                                      insertion, 1)
                        else:
                            patched = patched.replace(
                                ">", ">\n   <properties>\n" + insertion +
                                "   </properties>\n", 1)
                floor_text = floor_text[:start] + patched + floor_text[end:]

    #2b. the exterior door target, now that the landing cell inside is known
    doors = [(s, e, b) for s, e, b in object_blocks(exterior_text, "Moving") +
             object_blocks(exterior_text, "Object")
             if object_type(b) in ("door", "teleport on push", "teleport on it")]
    if doors:
        start, end, block = doors[0]
        props = properties(block)
        wanted = {"map": floor_name, "x": str(spawnX), "y": str(spawnY)}
        if any(props.get(k) != v for k, v in wanted.items()):
            fixes.append((exterior_path, "door target %s(%s,%s) -> %s(%d,%d)" %
                          (props.get("map"), props.get("x"), props.get("y"),
                           floor_name, spawnX, spawnY)))
            patched = block
            for key, value in wanted.items():
                if ('name="' + key + '"') in patched:
                    patched = re.sub(
                        r'(<property name="' + key +
                        r'"(?:\s+type="[^"]*")?\s+value=")[^"]*(")',
                        r"\g<1>" + value + r"\g<2>", patched, count=1)
                else:
                    insertion = ('    <property name="%s" value="%s"/>\n'
                                 % (key, value))
                    if "<properties>" in patched:
                        patched = patched.replace("<properties>\n",
                                                  "<properties>\n" + insertion,
                                                  1)
                    else:
                        patched = patched.replace(
                            ">", ">\n   <properties>\n" + insertion +
                            "   </properties>\n", 1)
            exterior_text = exterior_text[:start] + patched + exterior_text[end:]

    #3. bots: in the "Object" group, reachable, with a skin the datapack has
    xml_path = os.path.join(folder, floors[0][:-4] + ".xml")
    xml_text = open(xml_path, encoding="utf-8").read() if \
        os.path.exists(xml_path) else ""
    moved = True
    while moved:
        moved = False
        for start, end, block in object_blocks(floor_text, "Moving"):
            props = properties(block)
            kind = object_type(block)
            if kind == "bot" or ("id" in props and kind not in
                                 ("door", "teleport on push", "teleport on it",
                                  "rescue")):
                fixes.append((floor_path, "bot id " + props.get("id", "?") +
                              " moved to the \"Object\" group"))
                floor_text = floor_text[:start] + floor_text[end:]
                floor_text = insert_object(floor_text, "Object",
                                           "  " + block.strip() + "\n")
                moved = True
                break
    #a duplicated bot id makes two objects share one <bot> definition: give the
    #extra ones a free id (the generator does it at runtime, do it once here)
    changed = True
    while changed:
        changed = False
        used = []
        for start, end, block in object_blocks(floor_text, "Object"):
            props = properties(block)
            if "id" not in props:
                continue
            if props["id"] in used:
                free = 1
                while str(free) in used:
                    free += 1
                fixes.append((floor_path, "duplicated bot id " + props["id"] +
                              " -> " + str(free)))
                patched = re.sub(
                    r'(<property name="id"(?:\s+type="[^"]*")?\s+value=")[^"]*(")',
                    r"\g<1>" + str(free) + r"\g<2>", block, count=1)
                floor_text = floor_text[:start] + patched + floor_text[end:]
                changed = True
                break
            used.append(props["id"])

    seen = set()
    for start, end, block in reversed(object_blocks(floor_text, "Object")):
        props = properties(block)
        if "id" not in props:
            continue
        bot_id = props["id"]
        x = int(float(re.search(r'\sx="([-0-9.]+)"', block).group(1))) // ftw
        y = int(float(re.search(r'\sy="([-0-9.]+)"', block).group(1))) // fth
        tileY = y - 1  #objects carry the engine -1 tile offset
        talkable = any(not is_collision(floor_layers, fw, fh, x + dx, tileY + dy)
                       for dx, dy in ((0, 1), (0, -1), (1, 0), (-1, 0)))
        if is_collision(floor_layers, fw, fh, x, tileY) and not talkable:
            cell = free_cell_near(floor_layers, fw, fh, x, tileY)
            if cell:
                fixes.append((floor_path, "bot id " + bot_id + " was walled in "
                              "at %d,%d -> %d,%d" % (x, tileY, cell[0],
                                                     cell[1])))
                floor_text = move_object(floor_text, start, end, block, cell[0],
                                         cell[1] + 1, ftw, fth)
                continue
        skin = props.get("skin", "")
        if skin and not os.path.isdir(os.path.join(SKIN_DIR, skin)):
            role = bot_role(xml_text, bot_id)
            replacement = ROLE_SKIN.get(role, "oldman")
            fixes.append((floor_path, "bot id " + bot_id + " skin " + skin +
                          " -> " + replacement + " (" + role + ")"))
            floor_text = floor_text[:start] + re.sub(
                r'(<property name="skin"(?:\s+type="[^"]*")?\s+value=")[^"]*(")',
                r"\g<1>" + replacement + r"\g<2>", block, count=1) + \
                floor_text[end:]
        seen.add(bot_id)

    #4. the interior is an indoor map
    if 'value="indoor"' not in floor_text:
        fixes.append((floor_path, "indoor map property added"))
        header = re.search(r"<map[^>]*>", floor_text)
        floor_text = (floor_text[:header.end()] +
                      "\n <properties>\n"
                      "  <property name=\"type\" value=\"indoor\"/>\n"
                      " </properties>" + floor_text[header.end():])

    if apply_fix:
        open(exterior_path, "w", encoding="utf-8").write(exterior_text)
        open(floor_path, "w", encoding="utf-8").write(floor_text)


def fix_skeleton(path, fixes, apply_fix):
    """The floor-N.xml skeleton: only the bot/step STRUCTURE is used, drop what
    is copy/paste debris from another template."""
    tmx = path[:-4] + ".tmx"
    if not os.path.exists(tmx) or not os.path.exists(path):
        return
    text = open(path, encoding="utf-8").read()
    original = text
    tmx_ids = set()
    for _, _, block in object_blocks(open(tmx, encoding="utf-8").read(),
                                     "Object"):
        props = properties(block)
        if "id" in props:
            tmx_ids.add(props["id"])
    for bot in re.finditer(r'[ \t]*<bot id="(\d+)".*?</bot>\n?', text, re.S):
        if bot.group(1) not in tmx_ids:
            fixes.append((path, "bot " + bot.group(1) +
                          " has no object in the tmx, dropped"))
            text = text.replace(bot.group(0), "", 1)
    sound = re.search(r'\s*backgroundsound="([^"]*)"', text)
    if sound:
        found = False
        for music in MUSIC_DIRS:
            candidate = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                     music, os.path.basename(sound.group(1)))
            if os.path.exists(candidate):
                found = True
        if not found:
            fixes.append((path, "backgroundsound " + sound.group(1) +
                          " does not exist, dropped"))
            text = text.replace(sound.group(0), "", 1)
    zone = re.search(r'\s*zone="[^"]*"', text)
    if zone:
        fixes.append((path, "zone= dropped (the generator sets the city zone)"))
        text = text.replace(zone.group(0), "", 1)
    if text != original and apply_fix:
        open(path, "w", encoding="utf-8").write(text)


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
            #geometry and object repairs FIRST: the skeleton clean up below
            #compares the bots of the xml with the objects of the tmx, and a bot
            #object still sitting in the wrong group would look absent
            fix_variant(folder, exteriors[0] if exteriors else None, floors,
                        fixes, warnings, args.fix)
            for tmx in floors:
                check_xml(os.path.join(folder, tmx), problems, warnings)
                fix_skeleton(os.path.join(folder, tmx[:-4] + ".xml"), fixes,
                             args.fix)
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
