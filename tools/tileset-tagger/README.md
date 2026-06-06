# tileset-tagger

A small Qt6 GUI to tag a Tiled `.tsx` tileset with **semantic categories** so the
learn-from-examples map generator can read the same meaning from any tileset
(the hand-made model tilesets *and* the official set). Tags are work data,
so they are stored **out of the datapack** in an XDG sidecar — the datapack stays
the read-only source of truth (so you can tag `CatchChallenger-datapack/` too).
Tag files live in the tool's own app subdir (isolated from the client's data)
`~/.local/share/CatchChallenger/tileset-tagger/datapack-<sha256(abs datapack path)>/tileset-<sha256(abs .tsx path)>.json`.

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
* **You tag the VISUAL identity; the logical role is derived from the maps.**
  Walkable / collision / water are already encoded in the maps (which engine
  layer a tile sits on), so you never tag those by hand — the tool reads them.
  What a map *can't* tell you is what a tile *looks like* (a cliff, a tree-trunk
  and a wall are all just "Collisions"), so that — the **category** — is your job.
* **No free-text input** (error-prone): every field is a fixed combo or a
  checkbox. Tag properties (stored in the sidecar json, never in the `.tsx`):
  | property | source | meaning |
  |---|---|---|
  | `category` | you (combo) | what it **looks like** (tree-canopy, building-roof, table…) |
  | `animated` | auto/you | the tile art animates |
  | `horizontalRepeat` | you (checkbox) | tiles sideways |
  | `horizontalMiddleRepeat` | you (checkbox) | centre repeats, borders fixed |
  | `verticalRepeat` / `verticalMiddleRepeat` | you (checkbox) | same, vertically |
  | `size` | auto | `WxH` of the selected rectangle |
  | `group` | auto | groups the tiles of one placed item |
  | `layer` | **auto from maps** | the layer the tile is *drawn on* (walkable/grass/water/ledge/collision/over) |
  | `walkable` | **auto from maps** | player can stand on it — see below |
* **Pre-filled, you just validate.** Selecting a rectangle reads where those exact
  tiles are used on the real maps: the layer the tile is *drawn on* gives `layer`
  + a soft starting guess for the visual `category` (you set the real one), and
  **same-tile run detection** ticks the repeat flags. You **Tag** to accept or
  adjust the category. Groups used on no map keep their derived info blank.
* **`walkable` respects layer cancellation.** A tile can be drawn on `Walkable`
  yet sit under a `Collisions` tile in the same cell — the engine blocks it
  (`Map_loaderMain.cpp`: `(walkable||zone) && !collisions`). So `walkable` is
  computed from each usage cell's **effective** code with the engine precedence
  **Dirt > Ledges > Collisions-cancels-Walkable > Grass/Water/Lava zone >
  blocked**, not from the drawn-on layer. The read-only line shows it, e.g.
  *"drawn on 'Walkable' · effective BLOCKED (walk 26% / blocked 73% / ledge 0%)"*.
* **Jump to next untagged** walks you through what's left.
* **Save** writes the tags to the sidecar json; the `.tsx` and the rest of the
  datapack are never modified.

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
tileset-tagger --suggest  <x.tsx | tileset-dir>         # auto-tag (terrain + structure)
tileset-tagger --classify <x.tsx>                       # dry-run: print what suggest would tag
```

## How to start

1. **Just open a tileset** — it **auto-tags on open** (no separate step). From the
   maps it best-guesses every used tile (terrain exactly; walls/trees/ground/roofs
   by layer + colour):
   ```
   tileset-tagger /home/user/Desktop/CatchChallenger/datapack-pkmn/map/main/gen2/tileset/normal1.tsx
   ```
2. **FIX the few wrong guesses.** Tiles are coloured: **yellow** = a low-confidence
   guess to review, faint tint = confident, **red** = untagged (tiles never used on
   a map). Skim the yellow ones; where a guess is wrong (a cliff guessed
   `building-wall`, a roof guessed `tree-canopy`), drag a rectangle over the
   cluster, pick the right **category**, **Tag**. The **Map usage** panel shows each
   in situ. **Save** when done (tags go to the sidecar; the datapack is untouched).
3. Do the model tilesets (`gen2/tileset/`) and the target (`map/tileset/`); then
   the generator learns from the model and composes onto the target.

To **batch** the auto-tag for a whole directory up front (optional), use
`tileset-tagger --suggest <tileset-dir>`.

`TagModel` (in `TagModel.{hpp,cpp}`) is the GUI-free, unit-tested core: load the
`.tsx`+image, read/write tags, run the untagged-pixel guard. The GUI is a thin
shell over it.
