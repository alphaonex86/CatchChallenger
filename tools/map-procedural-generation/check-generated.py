#!/usr/bin/env python3
"""Validate a GENERATED datapack tree (the dest/ of map-procedural-generation).

Checks what the engine needs and what a player would notice:
  * every door/teleport resolves to an existing map, and its x/y lands INSIDE
    that map on a cell that is not a collision (else the player is stuck),
  * every interior has a way back out,
  * the bots of a map xml and the bot objects of its tmx match, and every
    <a href="N"> points at a step the bot really has,
  * skins exist in the datapack, tilesets referenced by a map exist,
  * NO franchise wording anywhere in the generated text (original content only).

Usage: check-generated.py [dest-dir] [--datapack <dir>]
"""
import argparse
import base64
import os
import re
import struct
import sys
import zlib

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import franchise_denylist
import generated_label

try:
    import zstandard
except ImportError:
    zstandard = None

# Trademarked / franchise wording must never reach the datapack: the generated
# world is original. Checked on the generated text AND on dialog.txt. The words
# themselves are what we must not carry, so the list is operator-local and stays
# out of git, see franchise_denylist.py.
FORBIDDEN = franchise_denylist.load()


def read_map(path):
    """(width, height, {layer: [grid]}, [objects]) of a tmx."""
    data = open(path, encoding="utf-8").read()
    header = re.search(r"<map[^>]*>", data).group(0)
    width = int(re.search(r'\swidth="(\d+)"', header).group(1))
    height = int(re.search(r'\sheight="(\d+)"', header).group(1))
    layers = {}
    for m in re.finditer(r'<layer[^>]*name="([^"]*)"[^>]*>\s*<data([^>]*)>'
                         r"(.*?)</data>", data, re.S):
        name, attributes, payload = m.group(1), m.group(2), m.group(3)
        if 'encoding="base64"' not in attributes:
            continue
        raw = base64.b64decode(payload.strip())
        if "zlib" in attributes:
            raw = zlib.decompress(raw)
        elif "gzip" in attributes:
            raw = zlib.decompress(raw, 16 + zlib.MAX_WBITS)
        elif "zstd" in attributes:
            if zstandard is None:
                return width, height, None, objects_of(data)
            raw = zstandard.ZstdDecompressor().decompress(
                raw, max_output_size=width * height * 4)
        grid = struct.unpack("<%dI" % (width * height), raw[:4 * width * height])
        layers.setdefault(name, []).append(grid)
    return width, height, layers, objects_of(data)


def objects_of(data):
    out = []
    for m in re.finditer(r"<object\b[^>]*?(?:/>|>.*?</object>)", data, re.S):
        block = m.group(0)
        kind = re.search(r'\stype="([^"]*)"', block)
        x = re.search(r'\sx="([-0-9.]+)"', block)
        y = re.search(r'\sy="([-0-9.]+)"', block)
        properties = dict(re.findall(
            r'<property name="([^"]*)"(?:\s+type="[^"]*")?\s+value="([^"]*)"',
            block))
        out.append({"type": kind.group(1) if kind else None,
                    "x": int(float(x.group(1))) // 16 if x else None,
                    "y": int(float(y.group(1))) // 16 if y else None,
                    "properties": properties})
    return out


def blocked(layers, width, height, x, y):
    if layers is None:
        return False
    if x < 0 or y < 0 or x >= width or y >= height:
        return True
    for grid in layers.get("Collisions", []):
        if grid[x + y * width]:
            return True
    return False


def border_offset(objectType, x, y):
    """What the engine shifts a crossing by, from the border object itself.

    Map_loaderMain.cpp reads a border-* object as the WHOLE side of the map plus
    an offset: y_offset=objectTileY-1 for left/right (the -1 is the Tiled object
    convention), x_offset=objectTileX for top/bottom. MoveOnTheMap.hpp then lands
    the player at y+int8(y_offset) / x+int8(x_offset) on the other map. So the
    object is NOT a teleport on one cell, and putting it anywhere but row 0 /
    column 0 shifts every crossing of that side.
    """
    if objectType in ("border-left", "border-right"):
        raw = (y - 1) & 0xFF
    else:
        raw = x & 0xFF
    return raw - 256 if raw > 127 else raw


def covered(layers, width, height, x, y):
    """A tile is drawn ABOVE the player on that cell (tree canopy, roof)."""
    if layers is None:
        return False
    if x < 0 or y < 0 or x >= width or y >= height:
        return False
    for grid in layers.get("WalkBehind", []):
        if grid[x + y * width]:
            return True
    return False


#Map_loader::loadExtraXml default when the <fight> step carries no fightRange
FIGHT_RANGE = 5
FIGHT_DIRECTIONS = {"bottom": (0, 1), "top": (0, -1),
                    "left": (-1, 0), "right": (1, 0)}


def water_at(layers, width, height, x, y):
    """the Water layer holds a tile there: walkable with the swim item, a WALL
    for anyone who does not own it — which is exactly who takes the ferry"""
    if x < 0 or y < 0 or x >= width or y >= height:
        return False
    for grid in layers.get("Water", []):
        if grid[x + y * width]:
            return True
    return False


def ground_at(layers, width, height, x, y):
    """the Walkable layer holds a tile there: the quay of a boat has to be real
    ground, not a hole the far side lands in"""
    if x < 0 or y < 0 or x >= width or y >= height:
        return False
    for grid in layers.get("Walkable", []):
        if grid[x + y * width]:
            return True
    return False


def foot_reachable_from_border(layers, objects, width, height, targets):
    """can the player WALK (no swimming) from a border of this map to one of
    `targets`? The quay of a moored boat has to be, else the ferry is a picture:
    a sea map is mostly water, so "reachable" measured with the water walkable
    is always true and says nothing."""
    starts = []
    for obj in objects:
        if obj["type"] == "border-left":
            starts += [(0, y) for y in range(height)]
        elif obj["type"] == "border-right":
            starts += [(width - 1, y) for y in range(height)]
        elif obj["type"] == "border-top":
            starts += [(x, 0) for x in range(width)]
        elif obj["type"] == "border-bottom":
            starts += [(x, height - 1) for x in range(width)]
    seen = set()
    queue = []
    for (x, y) in starts:
        if (x, y) not in seen and not blocked(layers, width, height, x, y) \
                and not water_at(layers, width, height, x, y):
            seen.add((x, y))
            queue.append((x, y))
    if not queue:
        return True  # entered by water anyway, nothing better to ask for
    index = 0
    while index < len(queue):
        (x, y) = queue[index]
        index += 1
        if (x, y) in targets:
            return True
        for (stepX, stepY) in ((-1, 0), (1, 0), (0, -1), (0, 1)):
            nx, ny = x + stepX, y + stepY
            if 0 <= nx < width and 0 <= ny < height and (nx, ny) not in seen:
                if not blocked(layers, width, height, nx, ny) \
                        and not water_at(layers, width, height, nx, ny):
                    seen.add((nx, ny))
                    queue.append((nx, ny))
    return False


def check_fight_lines(path, xmlPath, objects, layers, width, height, problems):
    """No two trainers may claim the same line-of-sight cell.

    CommonMap::botsFightTrigger keeps ONE fight per (x,y), so the second trainer
    reaching an already claimed cell is dropped there and never triggers.
    """
    text = open(xmlPath, encoding="utf-8").read()
    fightIds = set()
    for bot in re.finditer(r'<bot id="(\d+)"(.*?)</bot>', text, re.S):
        if re.search(r'<step[^>]*type="fight"', bot.group(2)):
            fightIds.add(bot.group(1))
    claimed = {}
    for obj in objects:
        if obj["type"] != "bot":
            continue
        botId = obj["properties"].get("id")
        if botId not in fightIds:
            continue
        step = FIGHT_DIRECTIONS.get(obj["properties"].get("lookAt"))
        if step is None:
            continue
        #the engine reads the object y one tile above the stored one
        x, y = obj["x"], obj["y"] - 1
        walked = 0
        while walked < FIGHT_RANGE:
            x += step[0]
            y += step[1]
            if blocked(layers, width, height, x, y):
                walked = FIGHT_RANGE
            else:
                if (x, y) in claimed:
                    problems.append((path, "bot " + botId + " crosses the line "
                                     "of sight of bot " + claimed[(x, y)] +
                                     " at %d,%d" % (x, y)))
                else:
                    claimed[(x, y)] = botId
                walked += 1


def check_text(path, problems):
    text = open(path, encoding="utf-8").read()
    lowered = text.lower()
    for word in FORBIDDEN:
        if word in lowered:
            problems.append((path, 'FRANCHISE WORDING "' + word + '"'))
    # <a href="2;3"> jumps to steps 2 then 3: every target must exist
    for bot in re.finditer(r'<bot id="(\d+)"(.*?)</bot>', text, re.S):
        steps = set(re.findall(r'<step[^>]*id="(\d+)"', bot.group(2)))
        for href in re.findall(r'<a href="([^"]*)"', bot.group(2)):
            for target in href.split(";"):
                if target.strip() and target.strip() not in steps:
                    problems.append((path, "bot " + bot.group(1) +
                                     " links to the unknown step " + target))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("dest", nargs="?", default="dest")
    parser.add_argument("--datapack", default=None,
                        help="datapack root the skins are taken from")
    arguments = parser.parse_args()
    #the generator resolves its map label from its settings, so read it back off disk
    label, root = generated_label.find(arguments.dest)
    if label is None:
        print(root)
        return 2
    skins = set()
    if arguments.datapack:
        skinDir = os.path.join(arguments.datapack, "skin", "bot")
        if os.path.isdir(skinDir):
            skins = set(os.listdir(skinDir))

    problems = []
    doors = 0
    maps = {}
    for folder, _, files in os.walk(root):
        for name in files:
            #all.tmx is the whole world in one file, a debug dump: its doors
            #point at the per city files that live in the sub folders
            if name.endswith(".tmx") and name != "all.tmx":
                path = os.path.join(folder, name)
                maps[os.path.normpath(path)] = read_map(path)
    for path in sorted(maps):
        width, height, layers, objects = maps[path]
        folder = os.path.dirname(path)
        exits = 0
        botCells = {}
        for tileset in re.findall(r'<tileset[^>]*source="([^"]*)"',
                                  open(path, encoding="utf-8").read()):
            resolved = os.path.normpath(os.path.join(folder, tileset))
            if not os.path.exists(resolved):
                problems.append((path, "tileset not shipped: " + tileset))
            #the generated tree is ONE map label, copied alone into a datapack
            #(map/main/<label>/): a reference climbing above it resolves to a
            #tileset the target datapack is not required to own, and the map
            #then loads with no tileset at all
            elif not os.path.abspath(resolved).startswith(
                    os.path.abspath(root) + os.sep):
                problems.append((path, "tileset OUTSIDE the generated map "
                                 "label: " + tileset))
        for obj in objects:
            #A BORDER is the whole side of the map plus an offset, so what has to
            #be checked is that SOME row (or column) of it is walkable on BOTH
            #maps once the offset is applied. A border object left in the middle
            #of its side shifts every crossing by 21 rows and the link is dead:
            #612 of 804 links of a world were unusable that way.
            if obj["type"] in ("border-left", "border-right",
                               "border-top", "border-bottom"):
                target = obj["properties"].get("map")
                if target is None:
                    problems.append((path, obj["type"] + " without a map property"))
                else:
                    if not target.endswith(".tmx"):
                        target += ".tmx"
                    targetPath = os.path.normpath(os.path.join(folder, target))
                    if targetPath not in maps:
                        problems.append((path, obj["type"] + " to a missing map: " +
                                         target))
                    else:
                        tw, th, tlayers, _ = maps[targetPath]
                        offset = border_offset(obj["type"], obj["x"], obj["y"])
                        usable = 0
                        if obj["type"] in ("border-left", "border-right"):
                            fromX = 0 if obj["type"] == "border-left" else width - 1
                            toX = tw - 1 if obj["type"] == "border-left" else 0
                            for y in range(height):
                                if not blocked(layers, width, height, fromX, y):
                                    if not blocked(tlayers, tw, th, toX, y + offset):
                                        usable += 1
                        else:
                            fromY = 0 if obj["type"] == "border-top" else height - 1
                            toY = th - 1 if obj["type"] == "border-top" else 0
                            for x in range(width):
                                if not blocked(layers, width, height, x, fromY):
                                    if not blocked(tlayers, tw, th, x + offset, toY):
                                        usable += 1
                        if usable == 0:
                            problems.append((path, obj["type"] + " to " + target +
                                             " has NO crossable row/column"
                                             " (offset %d)" % offset))
            if obj["type"] in ("door", "teleport on it", "teleport on push"):
                doors += 1
                exits += 1
                target = obj["properties"].get("map")
                if target is None:
                    problems.append((path, "teleport without a map property"))
                else:
                    if not target.endswith(".tmx"):
                        target += ".tmx"
                    targetPath = os.path.normpath(os.path.join(folder, target))
                    if targetPath not in maps:
                        problems.append((path, "teleport to a missing map: " +
                                         target))
                    else:
                        tw, th, tlayers, _ = maps[targetPath]
                        tx = obj["properties"].get("x")
                        ty = obj["properties"].get("y")
                        if tx is None or ty is None:
                            problems.append((path, "teleport to " + target +
                                             " without x/y"))
                        elif int(tx) >= tw or int(ty) >= th:
                            problems.append((path, "teleport lands OUTSIDE " +
                                             target + " at " + tx + "," + ty +
                                             " (map is %dx%d)" % (tw, th)))
                        elif blocked(tlayers, tw, th, int(tx), int(ty)):
                            problems.append((path, "teleport lands on a "
                                             "COLLISION of " + target + " at " +
                                             tx + "," + ty))
                #THE BOAT OF A CROSSING: a push-teleport standing ON a collision
                #is the ship, and the player pushes it from the quay beside it.
                #That quay has to be walkable ON FOOT from a border of the map —
                #the ferry is what you take when you cannot swim.
                #(a push teleport on a collision that stands ON THE WATER is a
                #boat; the same object on dry land is the exit of an interior)
                if obj["type"] == "teleport on push" \
                        and blocked(layers, width, height, obj["x"], obj["y"]) \
                        and water_at(layers, width, height, obj["x"], obj["y"]):
                    #the cell beside it must be GROUND THE PLAYER STANDS ON:
                    #no water, a Walkable tile, no collision — and a SIDE of the
                    #tile, never a corner
                    quays = set()
                    for (stepX, stepY) in ((-1, 0), (1, 0), (0, -1), (0, 1)):
                        qx, qy = obj["x"] + stepX, obj["y"] + stepY
                        if 0 <= qx < width and 0 <= qy < height \
                                and not blocked(layers, width, height, qx, qy) \
                                and not water_at(layers, width, height, qx, qy) \
                                and ground_at(layers, width, height, qx, qy):
                            quays.add((qx, qy))
                    if not quays:
                        problems.append((path, "the boat at %d,%d has no quay beside"
                                         " it (walkable ground, no water, no"
                                         " collision)" % (obj["x"], obj["y"])))
                    elif not foot_reachable_from_border(layers, objects, width,
                                                        height, quays):
                        problems.append((path, "the boat at %d,%d cannot be "
                                         "walked to from a border of the map"
                                         % (obj["x"], obj["y"])))
            if obj["type"] == "bot":
                skin = obj["properties"].get("skin")
                if skins and skin and skin not in skins:
                    problems.append((path, "skin not in the datapack: " + skin))
                #a bot WITH a skin draws a CHARACTER: its cell must be free
                #ground, else the NPC is painted inside a tree/wall or hidden
                #under a canopy.  A bot without a skin is only a marker on a
                #decor tile (city sign, heal machine): that tile IS the visual
                #and is a collision on purpose.
                if skin:
                    botId = obj["properties"].get("id", "?")
                    #the engine reads the object y one tile above the stored one
                    x, y = obj["x"], obj["y"] - 1
                    if blocked(layers, width, height, x, y):
                        problems.append((path, "bot " + botId + " stands ON a "
                                         "COLLISION at %d,%d" % (x, y)))
                    elif covered(layers, width, height, x, y):
                        problems.append((path, "bot " + botId + " is hidden "
                                         "under a WalkBehind tile at %d,%d"
                                         % (x, y)))
                    if (x, y) in botCells:
                        problems.append((path, "bot " + botId + " shares the "
                                         "cell %d,%d with bot " % (x, y) +
                                         botCells[(x, y)]))
                    else:
                        botCells[(x, y)] = botId
        # an interior (a map that is not the chunk of its folder) must let the
        # player out again
        base = os.path.basename(path)[:-4]
        isChunk = (base == os.path.basename(os.path.dirname(path)) or
                   re.match(r"^\d+$", base) or base.startswith("road-"))
        if not isChunk and exits == 0:
            problems.append((path, "interior without any way out"))
        xmlPath = path[:-4] + ".xml"
        if os.path.exists(xmlPath):
            check_text(xmlPath, problems)
            check_fight_lines(path, xmlPath, objects, layers, width, height,
                              problems)
            xmlIds = set(re.findall(r'<bot id="(\d+)"',
                                    open(xmlPath, encoding="utf-8").read()))
            tmxIds = set(o["properties"]["id"] for o in objects
                         if o["type"] == "bot" and "id" in o["properties"])
            if xmlIds != tmxIds:
                problems.append((xmlPath, "bots of the xml " +
                                 str(sorted(xmlIds)) + " != objects of the tmx " +
                                 str(sorted(tmxIds))))
        elif not isChunk:
            problems.append((path, "no sibling xml"))

    for path, message in problems:
        print("ERROR " + os.path.relpath(path, arguments.dest) + ": " + message)
    print("\n%d maps, %d teleports, %d problems" %
          (len(maps), doors, len(problems)))
    return 1 if problems else 0


sys.exit(main())
