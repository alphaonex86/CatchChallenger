# tileset-tagger

A small Qt6 GUI to tag a Tiled `.tsx` tileset with **semantic categories** so the
learn-from-examples map generator can read the same meaning from any tileset
(the hand-made Pokémon-style tilesets *and* the official set). Tags are written
back into the `.tsx` as standard per-tile `<property>` entries, so Tiled and the
engine still load the file unchanged.

## Why

The generator's job is: *learn the placement rules (hard + random) from the
hand-made maps, then reproduce maps of the same quality on the official tileset.*
A rule like "a house is a 4×3 wall block with a roof on top and a door at the
bottom-centre" only transfers between tilesets if both tilesets are tagged with
the **same vocabulary** (`building-wall`, `building-roof`, `door`, …). This tool
is how a human supplies that vocabulary, once per tileset.

## Build

```
cmake -S tools/tileset-tagger -B /tmp/tagger-build -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/tagger-build -j32
```

## GUI

```
/tmp/tagger-build/tileset-tagger [path/to/tileset.tsx]
```

* The sheet is shown with a tile grid. **Untagged tiles that draw pixels are
  flagged red** — the reminder that nothing is forgotten. The window title and a
  panel label always show the remaining untagged count.
* **No free-text input** (error-prone): a tag is only a **Category** (fixed
  combo) plus **repeat checkboxes** (`horizontalRepeat`,
  `horizontalMiddleRepeat` = the centre repeats while the borders stay fixed,
  `verticalRepeat`, `verticalMiddleRepeat`). The **size** (`WxH`) and an
  item-group **name** are derived automatically from the selected rectangle.
* Drag a rectangle over an item → pick its Category → tick any repeat flag →
  **Tag selection**. **Jump to next untagged** walks you through what's left.
* **Save .tsx** writes the tags back surgically (foreign engine/Tiled properties
  on a tile, e.g. `animation`, are preserved).

### Two-step tagging — see a group in situ before naming it

Tagging is much easier when you can see what a tile actually *is* on a real map:

1. **Select** a rectangle of tiles in the tileset (step 1).
2. The **Map usage** panel (bottom) lists every map that uses that group and, for
   the chosen map, renders it with the usage cells outlined by **animated
   marching-ant rectangles** (step 2). Contiguous placements (a whole house, a
   sign) collapse into one rectangle; scattered ground tiles show many.

The **Used on map** combo is sorted by usage count, so the map where the group is
most prominent comes first. Now you can see "ah, these four tiles are the roof of
a house" and tag them correctly. Map decoding (base64 + zstd/zlib/csv) and
rendering use the vendored libtiled, so every datapack map format works.

When you need a category that isn't in the combo, ask and it gets added to the
curated list in `MainWindow.cpp` (`tagCategories()`).

## Headless CLI (for scripts / CI)

```
tileset-tagger --guard    <x.tsx>                       # report untagged-with-pixels
tileset-tagger --tag      <x.tsx> <cat> <c0> <r0> <c1> <r1> [name] [size]
tileset-tagger --selftest <x.tsx>                       # tag round-trip + guard self-check
tileset-tagger --usage    <x.tsx> <c0> <r0> <c1> <r1>   # list maps using that tile group
```

`TagModel` (in `TagModel.{hpp,cpp}`) is the GUI-free, unit-tested core: load the
`.tsx`+image, read/write tags, run the untagged-pixel guard. The GUI is a thin
shell over it.
