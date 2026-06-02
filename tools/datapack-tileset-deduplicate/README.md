# datapack-tileset-deduplicate

Finds duplicate **16×16 tiles** across every tileset (`.tsx`) of a datapack,
merges them, and rewrites every `.tmx` map so cells/objects that referenced a
merged-away tile point at the kept tile instead. Pure reference rewriting — the
tileset `.png`/`.tsx` files themselves are **not** modified.

## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## Usage

```sh
./datapack-tileset-deduplicate <datapack_path>     # interactive window
./datapack-tileset-deduplicate --batch <path>      # headless, no window
```

With no path a folder picker is shown. The window shows `processing... X/Y
tiles` while it works and closes itself when every tile has been compared.

## Comparison

Every pair of non-empty tiles is compared per pixel, per RGBA channel, with the
project's relative **±10%** tolerance (`|a−b| ≤ 0.10·max(a,b)`; a 0 needs an
exact 0). Fully-transparent pixels (alpha ≈ 0 on both sides) match regardless of
their stored RGB. Three outcomes:

* **IDENTICAL** — every pixel within ±10%. Merged automatically.
* **SIMILAR** — at least 90% of pixels within ±10%. In the window the two tiles
  are shown magnified 16× over a checkerboard (so transparency is visible) with
  **SKIP** / **MERGE** buttons. Choosing SKIP remembers that source tile and
  never asks about it again. `--batch` keeps similar tiles (never auto-merges
  them — they need a human eye).
* **DIFFERENT** — left alone.

Fully-transparent tiles, and tiles carrying an animation or other property, are
skipped (merging them would drop their behaviour).

## Which tile is kept on a merge

The **less transparent** tile (higher sum of pixel alpha). On a tie, the tile
first in alphabetical order (tileset path, then tile id). All `.tmx` references
to the other tile are rewritten to the kept one.

## Maps with a missing tileset are left untouched

If a map references a `.tsx` whose file is missing (or whose image fails to
load), it is **skipped** and reported. libtiled reads such a dangling tileset as
an empty placeholder; rewriting the map would recompute and shift every later
tileset's `firstgid`, corrupting it. Fix the dangling reference, then re-run to
deduplicate those maps too.
