#!/usr/bin/env python3
# Migrate a monster overworld sprite from the "skin" sheet layout
# (skin/animals/<name>/trainer.png) to the monster overworld layout
# (monsters/<id>/overworld.png).
#
# Source layout -- 3 columns x 4 rows, one cell = w/3 x h/4 (16x24 here).
#   Rows are directions, columns are the animation frames of that direction:
#     row 0 = top     cols: step, idle, step   (client: MapControllerMPAPI tileAt 0,1,2)
#     row 1 = right                            (tileAt 3,4,5)
#     row 2 = bottom                           (tileAt 6,7,8)
#     row 3 = left                             (tileAt 9,10,11)
#
# Destination layout -- 2 columns x 4 rows of 32x32, read in this order:
#   top-step  left-step / top-idle  left-idle / bottom-step right-step / bottom-idle right-idle
#   The client (MapVisualiserPlayer::updatePlayerMonsterTile, MapControllerMP.cpp)
#   picks the idle tile per direction: top=2 right=7 bottom=6 left=3, and while
#   walking swaps to baseTile-2 (MapControllerMPMove.cpp) -- so only 2 of the 3
#   source frames fit; the first step frame is kept and the second dropped.
#
# The 16x24 cell is centred horizontally and bottom-aligned inside the 32x32
# cell: the object is placed at (x-0.5, y+1) with a 32-wide tile, so centring is
# what puts the sprite over its map cell.

import os
import re
import sys
import unicodedata
import xml.etree.ElementTree as ET

from PIL import Image

# destination tile index -> (source row, source column)
LAYOUT = {
    0: (0, 0),   # top    walking
    1: (3, 0),   # left   walking
    2: (0, 1),   # top    idle
    3: (3, 1),   # left   idle
    4: (2, 0),   # bottom walking
    5: (1, 0),   # right  walking
    6: (2, 1),   # bottom idle
    7: (1, 1),   # right  idle
}

DEST_TILE = 32


def normalize(name):
    """Monster display name -> skin folder name ("Panthera fira" -> "pantherafira")."""
    name = unicodedata.normalize("NFKD", name).encode("ascii", "ignore").decode()
    return re.sub(r"[^a-z0-9]", "", name.lower())


def monster_names(datapack):
    """id -> default (untranslated) monster name, from monsters/monster.xml."""
    out = {}
    root = ET.parse(os.path.join(datapack, "monsters", "monster.xml")).getroot()
    for monster in root.findall("monster"):
        mid = monster.get("id")
        if mid is not None:
            for node in monster.findall("name"):
                if node.get("lang") is None and node.text:
                    out[mid] = node.text
    return out


def convert(src_path, dst_path):
    src = Image.open(src_path)
    width, height = src.size
    if width % 3 != 0 or height % 4 != 0:
        print("  skip: %s is %dx%d, not a 3x4 sheet" % (src_path, width, height))
        return False
    cw, ch = width // 3, height // 4
    if cw > DEST_TILE or ch > DEST_TILE:
        print("  skip: %s cell %dx%d does not fit in %dx%d"
              % (src_path, cw, ch, DEST_TILE, DEST_TILE))
        return False

    # Keep the source's own pixel format: a paletted sheet stays paletted (the
    # whole destination comes from that one sheet, so the palette still covers
    # every pixel) and nothing is requantised.
    if src.mode == "P":
        dst = Image.new("P", (DEST_TILE * 2, DEST_TILE * 4), 0)
        dst.putpalette(src.getpalette())
        transparency = src.info.get("transparency")
        if transparency is None:
            print("  skip: %s is paletted without transparency" % src_path)
            return False
        save_args = {"transparency": transparency}
        if isinstance(transparency, bytes) and transparency[0] != 0:
            print("  skip: %s palette index 0 is not transparent" % src_path)
            return False
    else:
        src = src.convert("RGBA")
        dst = Image.new("RGBA", (DEST_TILE * 2, DEST_TILE * 4), (0, 0, 0, 0))
        save_args = {}

    dx = (DEST_TILE - cw) // 2          # centre horizontally
    dy = DEST_TILE - ch                 # stand on the bottom edge
    for index, (row, col) in LAYOUT.items():
        cell = src.crop((col * cw, row * ch, (col + 1) * cw, (row + 1) * ch))
        dst.paste(cell, ((index % 2) * DEST_TILE + dx, (index // 2) * DEST_TILE + dy))

    dst.save(dst_path, optimize=True, **save_args)
    return True


def main():
    if len(sys.argv) < 2:
        print("usage: %s <datapack-root> [output-root]" % sys.argv[0])
        return 1
    datapack = sys.argv[1]
    out_root = sys.argv[2] if len(sys.argv) > 2 else datapack

    names = monster_names(datapack)
    skin_dir = os.path.join(datapack, "skin", "animals")
    skins = set(os.listdir(skin_dir)) if os.path.isdir(skin_dir) else set()

    done = 0
    for mid in sorted(names, key=lambda k: int(k)):
        folder = normalize(names[mid])
        if folder in skins:
            src_path = os.path.join(skin_dir, folder, "trainer.png")
            if os.path.isfile(src_path):
                dst_dir = os.path.join(out_root, "monsters", mid)
                os.makedirs(dst_dir, exist_ok=True)
                dst_path = os.path.join(dst_dir, "overworld.png")
                print("monster %s (%s) <- skin/animals/%s/trainer.png"
                      % (mid, names[mid], folder))
                if convert(src_path, dst_path):
                    done += 1
            else:
                print("monster %s (%s): skin/animals/%s has no trainer.png"
                      % (mid, names[mid], folder))
    missing = [mid for mid in names if normalize(names[mid]) not in skins]
    print("%d written, %d monster(s) have no skin/animals source: %s"
          % (done, len(missing), ", ".join(sorted(missing, key=lambda k: int(k)))))
    return 0


if __name__ == "__main__":
    sys.exit(main())
