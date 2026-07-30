#!/usr/bin/env python3
"""Create NEW building facades for the city styles, by recolouring the ones that
are already drawn.

A city style folder often holds several variants that share the SAME exterior
(desert-city has 9 variants for 2 facades), so a town shows the same house over
and over. This makes a real new variant out of an existing one: the tiles the
facade uses are extracted into a small tileset of their own, their HUE is
rotated, and a new variant folder is written with that tileset. The shape, the
door, the collisions and the interior are untouched, so the result is coherent
by construction - only the paint changes.

  * every tile stays 16x16, whole tiles are copied, nothing is drawn,
  * grey/white/black pixels (outlines, windows, doors, roofs of slate) keep
    their colour: only a pixel with enough saturation is rotated,
  * the existing variants are NOT modified, the new ones are added after them.

    python3 newfacades.py --list          # what exists today
    python3 newfacades.py --hues 40,150   # 2 new variants per distinct facade
    python3 newfacades.py --contact-sheet out.png   # render them to look at

Run `template-check.py --fix` afterwards: it wires the door/exit of the new
folders exactly like the hand made ones.
"""
import argparse
import base64
import colorsys
import collections
import hashlib
import os
import re
import struct
import sys
import zlib

try:
    import zstandard
except ImportError:
    zstandard = None
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
TEMPLATE = os.path.join(HERE, "template")
TILESET = os.path.join(HERE, "tileset")


def map_header(text):
    header = re.search(r"<map[^>]*>", text).group(0)
    return (int(re.search(r'\swidth="(\d+)"', header).group(1)),
            int(re.search(r'\sheight="(\d+)"', header).group(1)))


def tilesets_of(text):
    """[(firstgid, source)] sorted by firstgid."""
    out = [(int(m.group(1)), m.group(2)) for m in
           re.finditer(r'<tileset firstgid="(\d+)" source="([^"]*)"', text)]
    out.sort()
    return out


def layers_of(text):
    """[(name, attributes, width, height, [gid])] in file order."""
    out = []
    for m in re.finditer(r'<layer([^>]*)name="([^"]*)"([^>]*)>\s*'
                         r'<data encoding="base64"([^>]*)>(.*?)</data>',
                         text, re.S):
        attributes = m.group(1) + m.group(3)
        width = int(re.search(r'\swidth="(\d+)"', attributes).group(1))
        height = int(re.search(r'\sheight="(\d+)"', attributes).group(1))
        raw = base64.b64decode(m.group(5).strip())
        if "zlib" in m.group(4):
            raw = zlib.decompress(raw)
        elif "zstd" in m.group(4):
            raw = zstandard.ZstdDecompressor().decompress(
                raw, max_output_size=width * height * 4)
        out.append((m.group(2), m.group(0), width, height,
                    list(struct.unpack("<%dI" % (width * height),
                                       raw[:4 * width * height]))))
    return out


def tileset_image(tsx_path):
    """(PIL image, columns, tile count) of a tsx."""
    text = open(tsx_path, encoding="utf-8").read()
    source = re.search(r'<image source="([^"]*)"', text).group(1)
    image = Image.open(os.path.join(os.path.dirname(tsx_path),
                                    source)).convert("RGBA")
    tile = int(re.search(r'tilewidth="(\d+)"', text).group(1))
    columns = image.width // tile
    return image, columns, columns * (image.height // tile), tile


def dominant_hue(image):
    """Hue of the paint that covers the building: the roof and the walls."""
    histogram = [0] * 36
    pixels = image.load()
    for y in range(image.height):
        for x in range(image.width):
            r, g, b, a = pixels[x, y]
            if a == 0:
                continue
            high, low = max(r, g, b), min(r, g, b)
            if high == 0 or (high - low) * 255 // high < 60:
                continue
            h, _, _ = colorsys.rgb_to_hsv(r / 255.0, g / 255.0, b / 255.0)
            histogram[int(h * 36) % 36] += 1
    if not any(histogram):
        return None
    return (histogram.index(max(histogram)) + 0.5) / 36.0


def rotate_hue(image, degrees, band):
    """Repaint the building: only the DOMINANT colour (roof + walls) is rotated.

    An unsaturated pixel (outline, white wall, glass reflection) and a colour
    that is not the paint (blue windows, green plants, the wooden door) keep
    their hue, so the house stays the same house in another colour."""
    out = image.copy()
    pixels = out.load()
    paint = dominant_hue(image)
    if paint is None:
        return out
    for y in range(out.height):
        for x in range(out.width):
            r, g, b, a = pixels[x, y]
            if a == 0:
                continue
            high, low = max(r, g, b), min(r, g, b)
            if high == 0 or (high - low) * 255 // high < 60:
                continue  # grey: keep it
            h, s, v = colorsys.rgb_to_hsv(r / 255.0, g / 255.0, b / 255.0)
            distance = abs(h - paint)
            distance = min(distance, 1.0 - distance)
            if distance > band / 360.0:
                continue  # another colour of the sprite: keep it
            h = (h + degrees / 360.0) % 1.0
            nr, ng, nb = colorsys.hsv_to_rgb(h, s, v)
            pixels[x, y] = (int(nr * 255), int(ng * 255), int(nb * 255), a)
    return out


def variants_of(group_path):
    return sorted((d for d in os.listdir(group_path)
                   if os.path.isdir(os.path.join(group_path, d))),
                  key=lambda d: (len(d), d))


def exterior_of(folder):
    for name in sorted(os.listdir(folder)):
        if name.endswith(".tmx") and not name.startswith("floor-"):
            return name
    return None


def build_variant(style, source_variant, degrees, index, band, dry_run):
    """Write template/<style>/<index>/ with a recoloured copy of the facade."""
    source_folder = os.path.join(TEMPLATE, style, source_variant)
    exterior = exterior_of(source_folder)
    text = open(os.path.join(source_folder, exterior), encoding="utf-8").read()
    width, height = map_header(text)
    tilesets = tilesets_of(text)
    layers = layers_of(text)

    # which (tileset, tile id) does this facade use, in a stable order
    used = []
    seen = {}
    for _, _, lw, lh, grid in layers:
        for gid in grid:
            plain = gid & 0x1FFFFFFF
            if plain and plain not in seen:
                seen[plain] = None
                used.append(plain)
    used.sort()
    if not used:
        return None

    def tileset_for(gid):
        chosen = tilesets[0]
        for entry in tilesets:
            if gid >= entry[0]:
                chosen = entry
        return chosen

    # pack the used tiles into one small sheet, 8 per row
    tile_size = None
    columns = 8
    rows = (len(used) + columns - 1) // columns
    images = {}
    for source in set(tileset_for(g)[1] for g in used):
        path = os.path.normpath(os.path.join(source_folder, source))
        images[source] = tileset_image(path)
        tile_size = images[source][3]
    sheet = Image.new("RGBA", (columns * tile_size, rows * tile_size),
                      (0, 0, 0, 0))
    position = {}
    for order, gid in enumerate(used):
        firstgid, source = tileset_for(gid)
        image, sourceColumns, count, tile = images[source]
        local = gid - firstgid
        box = ((local % sourceColumns) * tile, (local // sourceColumns) * tile)
        sheet.paste(image.crop((box[0], box[1], box[0] + tile, box[1] + tile)),
                    ((order % columns) * tile, (order // columns) * tile))
        position[gid] = order
    sheet = rotate_hue(sheet, degrees, band)

    name = "%s-%s-h%d" % (style.replace("-city", ""), source_variant, degrees)
    target_folder = os.path.join(TEMPLATE, style, str(index))
    if dry_run:
        return name, target_folder, len(used)
    if not os.path.isdir(target_folder):
        os.makedirs(target_folder)
    sheet.save(os.path.join(TILESET, name + ".png"))
    open(os.path.join(TILESET, name + ".tsx"), "w", encoding="utf-8").write(
        '<?xml version="1.0" encoding="UTF-8"?>\n'
        '<tileset name="%s" tilewidth="%d" tileheight="%d" tilecount="%d" '
        'columns="%d">\n <image source="%s.png" width="%d" height="%d"/>\n'
        '</tileset>\n' % (name, tile_size, tile_size, columns * rows, columns,
                          name, sheet.width, sheet.height))

    # the new tmx: same layers, gids remapped to the new tileset
    depth = 3  # template/<style>/<variant>/ -> ../../../tileset/
    new_text = text
    first = re.search(r"[ \t]*<tileset firstgid=", new_text)
    end = new_text.rindex("<tileset", 0, new_text.index("<layer"))
    end = new_text.index("/>", end) + 3
    new_text = (new_text[:first.start()] +
                ' <tileset firstgid="1" source="%stileset/%s.tsx"/>\n' %
                ("../" * depth, name) + new_text[end:])
    for _, block, lw, lh, grid in layers:
        remapped = [(position[g & 0x1FFFFFFF] + 1) if (g & 0x1FFFFFFF) else 0
                    for g in grid]
        payload = base64.b64encode(zlib.compress(
            struct.pack("<%dI" % len(remapped), *remapped), 9)).decode()
        newBlock = re.sub(r'<data encoding="base64"[^>]*>.*?</data>',
                          '<data encoding="base64" compression="zlib">\n   ' +
                          payload + "\n  </data>", block, flags=re.S)
        new_text = new_text.replace(block, newBlock)
    # the object markers of the exterior (the door) come from invisible.tsx,
    # which the new sheet does not hold: drop the objects, template-check --fix
    # writes a correct door back
    new_text = re.sub(r'[ \t]*<objectgroup[^>]*name="Moving"[^>]*(?<!/)>.*?'
                      r"</objectgroup>\n", ' <objectgroup name="Moving"/>\n',
                      new_text, flags=re.S)
    new_text = re.sub(r'[ \t]*<objectgroup[^>]*name="Moving"[^>]*/>\n',
                      ' <objectgroup name="Moving"/>\n', new_text)
    open(os.path.join(target_folder, exterior), "w",
         encoding="utf-8").write(new_text)
    # the interior is copied as is: only the facade is new
    for floor in sorted(os.listdir(source_folder)):
        if floor.startswith("floor-"):
            data = open(os.path.join(source_folder, floor), "rb").read()
            open(os.path.join(target_folder, floor), "wb").write(data)
    return name, target_folder, len(used)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--hues", default="40,150",
                        help="hue rotations in degrees, one new variant each")
    parser.add_argument("--band", type=float, default=50,
                        help="degrees around the dominant colour that get "
                             "repainted (the rest of the sprite is kept)")
    parser.add_argument("--styles", default="",
                        help="only these styles (comma separated)")
    parser.add_argument("--list", action="store_true",
                        help="show the distinct facades of every style")
    parser.add_argument("--dry-run", action="store_true")
    arguments = parser.parse_args()

    styles = [d for d in sorted(os.listdir(TEMPLATE))
              if d.endswith("-city") and os.path.isdir(os.path.join(TEMPLATE, d))]
    if arguments.styles:
        styles = [s for s in styles if s in arguments.styles.split(",")]

    created = []
    for style in styles:
        group = os.path.join(TEMPLATE, style)
        facades = collections.OrderedDict()
        for variant in variants_of(group):
            exterior = exterior_of(os.path.join(group, variant))
            if exterior is None:
                continue
            digest = hashlib.md5(open(os.path.join(group, variant, exterior),
                                      "rb").read()).hexdigest()
            facades.setdefault(digest, []).append(variant)
        print("%-16s %2d variants, %d distinct facades" %
              (style, len(variants_of(group)), len(facades)))
        if arguments.list:
            for digest, variants in facades.items():
                print("      " + ", ".join(variants))
            continue
        next_index = max(int(v) for v in variants_of(group) if v.isdigit()) + 1
        for digest, variants in facades.items():
            for degrees in [int(h) for h in arguments.hues.split(",") if h]:
                result = build_variant(style, variants[0], degrees, next_index,
                                       arguments.band, arguments.dry_run)
                if result:
                    print("      + %s/%d  from %s  hue+%d  (%d tiles)" %
                          (style, next_index, variants[0], degrees, result[2]))
                    created.append((style, next_index, variants[0], degrees))
                    next_index += 1
    print("\n%d new variants" % len(created))
    return 0


sys.exit(main())
