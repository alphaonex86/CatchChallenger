#!/usr/bin/env python3
"""Render a generated .tmx to a .png so a human (or a vision model) can LOOK at
the result — the only way to catch "this town reads as an empty field".

The generated maps are base64 + zstd (the datapack encoding), and each one
references its tilesets by a path RELATIVE to itself, resolved inside the map
label. Both are handled here; zlib/gzip/csv/uncompressed are handled too so the
same script also renders the terrain tool's CSV all.tmx.

    ./render-tmx.py <map.tmx> [-o out.png] [--grid] [--layers Walkable,Collisions]
    ./render-tmx.py <dir> -o out/            # every .tmx of a directory

Layers are drawn in the order the file lists them, except that the above-player
ones (WalkBehind) come last. --layers restricts to a comma separated list, which
is how you check that hiding one layer really changes the picture.
"""

import argparse
import base64
import os
import struct
import sys
import xml.etree.ElementTree as ElementTree
import zlib

from PIL import Image

FLIP_HORIZONTAL = 0x80000000
FLIP_VERTICAL = 0x40000000
FLIP_DIAGONAL = 0x20000000
FLIP_MASK = FLIP_HORIZONTAL | FLIP_VERTICAL | FLIP_DIAGONAL


def decode_layer_data(data_element, width, height):
    """The <data> of a tile layer -> a flat list of width*height raw gids."""
    encoding = data_element.get("encoding")
    compression = data_element.get("compression")
    text = (data_element.text or "").strip()
    if encoding == "csv":
        return [int(value) for value in text.replace("\n", "").split(",") if value != ""]
    if encoding != "base64":
        raise ValueError("unsupported layer encoding %r" % encoding)
    raw = base64.b64decode(text)
    if compression == "zlib":
        raw = zlib.decompress(raw)
    elif compression == "gzip":
        raw = zlib.decompress(raw, 16 + zlib.MAX_WBITS)
    elif compression == "zstd":
        import zstandard
        raw = zstandard.ZstdDecompressor().decompressobj().decompress(raw)
    elif compression:
        raise ValueError("unsupported layer compression %r" % compression)
    return list(struct.unpack("<%dI" % (width * height), raw))


class Tileset(object):
    """One <tileset>: its image, geometry and first gid."""

    def __init__(self, first_gid, name, image, tile_width, tile_height,
                 columns, spacing, margin, tile_count):
        self.first_gid = first_gid
        self.name = name
        self.image = image
        self.tile_width = tile_width
        self.tile_height = tile_height
        self.columns = columns
        self.spacing = spacing
        self.margin = margin
        self.tile_count = tile_count

    def tile_image(self, local_id):
        if self.image is None or local_id >= self.tile_count:
            return None
        column = local_id % self.columns
        row = local_id // self.columns
        left = self.margin + column * (self.tile_width + self.spacing)
        top = self.margin + row * (self.tile_height + self.spacing)
        if left + self.tile_width > self.image.width or top + self.tile_height > self.image.height:
            return None
        return self.image.crop((left, top, left + self.tile_width, top + self.tile_height))


def load_tileset(element, first_gid, base_dir):
    """A <tileset> element, following the "source" indirection when present."""
    source = element.get("source")
    if source is not None:
        path = os.path.normpath(os.path.join(base_dir, source))
        if not os.path.isfile(path):
            sys.stderr.write("missing tileset %s\n" % path)
            return None
        element = ElementTree.parse(path).getroot()
        base_dir = os.path.dirname(path)
    tile_width = int(element.get("tilewidth", 0))
    tile_height = int(element.get("tileheight", 0))
    spacing = int(element.get("spacing", 0))
    margin = int(element.get("margin", 0))
    tile_count = int(element.get("tilecount", 0))
    columns = int(element.get("columns", 0))
    image_element = element.find("image")
    image = None
    if image_element is not None:
        image_path = os.path.normpath(os.path.join(base_dir, image_element.get("source")))
        if os.path.isfile(image_path):
            image = Image.open(image_path).convert("RGBA")
        else:
            sys.stderr.write("missing tileset image %s\n" % image_path)
    if columns == 0 and image is not None and tile_width > 0:
        columns = (image.width - 2 * margin + spacing) // (tile_width + spacing)
    if tile_count == 0 and image is not None and columns > 0:
        rows = (image.height - 2 * margin + spacing) // (tile_height + spacing)
        tile_count = columns * rows
    if columns == 0:
        return None
    return Tileset(first_gid, element.get("name", ""), image, tile_width, tile_height,
                   columns, spacing, margin, tile_count)


def render(tmx_path, wanted_layers=None, grid=False):
    base_dir = os.path.dirname(os.path.abspath(tmx_path))
    root = ElementTree.parse(tmx_path).getroot()
    width = int(root.get("width"))
    height = int(root.get("height"))
    tile_width = int(root.get("tilewidth"))
    tile_height = int(root.get("tileheight"))

    tilesets = []
    for element in root.findall("tileset"):
        tileset = load_tileset(element, int(element.get("firstgid")), base_dir)
        if tileset is not None:
            tilesets.append(tileset)
    tilesets.sort(key=lambda one: one.first_gid)

    def tileset_of(gid):
        found = None
        for tileset in tilesets:
            if tileset.first_gid <= gid:
                found = tileset
            else:
                break
        return found

    # a dark grey background makes an EMPTY cell obvious instead of blending
    # into a black page
    canvas = Image.new("RGBA", (width * tile_width, height * tile_height), (32, 32, 40, 255))

    # tile layers in file order, WalkBehind (above the player) drawn last
    layers = [layer for layer in root.iter("layer")]
    layers.sort(key=lambda layer: layer.get("name") == "WalkBehind")
    for layer in layers:
        name = layer.get("name")
        if wanted_layers is not None and name not in wanted_layers:
            continue
        data_element = layer.find("data")
        if data_element is None:
            continue
        gids = decode_layer_data(data_element, width, height)
        for index, raw_gid in enumerate(gids):
            gid = raw_gid & ~FLIP_MASK
            if gid == 0:
                continue
            tileset = tileset_of(gid)
            if tileset is None:
                continue
            tile = tileset.tile_image(gid - tileset.first_gid)
            if tile is None:
                continue
            if raw_gid & FLIP_HORIZONTAL:
                tile = tile.transpose(Image.FLIP_LEFT_RIGHT)
            if raw_gid & FLIP_VERTICAL:
                tile = tile.transpose(Image.FLIP_TOP_BOTTOM)
            column = index % width
            row = index // width
            # a tileset tile taller than the map grid hangs UPWARD, like Tiled
            position = (column * tile_width,
                        row * tile_height - (tile.height - tile_height))
            canvas.alpha_composite(tile, position)

    if grid:
        from PIL import ImageDraw
        draw = ImageDraw.Draw(canvas)
        for column in range(0, width + 1, 8):
            draw.line([(column * tile_width, 0), (column * tile_width, canvas.height)],
                      fill=(255, 0, 0, 90))
        for row in range(0, height + 1, 8):
            draw.line([(0, row * tile_height), (canvas.width, row * tile_height)],
                      fill=(255, 0, 0, 90))
    return canvas


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("source", help=".tmx file, or a directory of them")
    parser.add_argument("-o", "--output", help="output .png (or output dir for a directory)")
    parser.add_argument("--grid", action="store_true", help="overlay an 8-tile red grid")
    parser.add_argument("--layers", help="comma separated layer names to draw, others skipped")
    arguments = parser.parse_args()

    wanted = None
    if arguments.layers:
        wanted = set(name.strip() for name in arguments.layers.split(","))

    if os.path.isdir(arguments.source):
        output_dir = arguments.output or (arguments.source.rstrip("/") + "-png")
        os.makedirs(output_dir, exist_ok=True)
        count = 0
        for root_dir, _dirs, files in os.walk(arguments.source):
            for name in sorted(files):
                if name.endswith(".tmx"):
                    tmx = os.path.join(root_dir, name)
                    relative = os.path.relpath(tmx, arguments.source)
                    target = os.path.join(output_dir, relative[:-4].replace(os.sep, "_") + ".png")
                    render(tmx, wanted, arguments.grid).save(target)
                    count += 1
        print("%d map(s) rendered into %s" % (count, output_dir))
        return 0

    output = arguments.output or (arguments.source[:-4] + ".png")
    render(arguments.source, wanted, arguments.grid).save(output)
    print("wrote " + output)
    return 0


if __name__ == "__main__":
    sys.exit(main())
