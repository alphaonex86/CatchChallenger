# datapack-tileset-deduplicate

A CLI tool that finds duplicate **16×16 tiles** across every tileset (`.tsx`) of a
datapack and merges them, rewriting every map (`.tmx`) so each cell/object that
referenced a dropped tile points at the kept one instead. The tileset `.png`/`.tsx`
files are **never modified** — only `.tmx` references are rewritten.

## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## Run

```sh
datapack-tileset-deduplicate <datapack_path>             # interactive window
datapack-tileset-deduplicate --batch <path>              # headless: auto-merge identical, keep similar
datapack-tileset-deduplicate --check-all <path>          # also dedup within each tileset
```

`<datapack_path>` is the datapack folder holding the `.tsx` tilesets and the `.tmx`
maps (searched recursively). With no path and no `--batch`, a folder picker opens.

By default tiles are only compared **across different tilesets** — duplicates
within one `.tsx` are usually intentional and just create noise. Pass
**`--check-all`** to also compare tiles within the same tileset file (this can be
problematic with animations, where the frames inside one tileset are deliberately
very similar to each other).

## Tile comparison

Every tile is 16×16 (256 pixels). Two tiles are compared pixel-by-pixel; a pixel
**matches** when **each RGBA channel is within ±15%** of the other channel
(`|a−b| ≤ 0.15·max(a,b)`; a channel value of 0 requires an exact 0 on the other
side). Two pixels that are both fully transparent (alpha ≈ 0) match regardless of
their stored RGB. Three outcomes:

* **IDENTICAL** — all 256 pixels match.
* **SIMILAR** — at least **90%** of pixels match (≥ 231/256).
* **DIFFERENT** — fewer than 90% match; left untouched.

## Window & progress

A window is shown throughout. While it is comparing and needs no input it shows
`processing… N/TOTAL tiles`. When every tile has been compared it rewrites the maps
and **closes itself**.

## IDENTICAL → merged automatically

No prompt — **but only when neither tile carries a custom property**. The **kept**
tile is the one with the **least transparent area** — the largest sum of pixel
alpha; on a tie, the one **first in alphabetical order** (tileset path, then tile
id). The other tile is dropped and every `.tmx` reference to it is rewritten to the
kept tile.

If **either** tile of an IDENTICAL pair has a custom property, it is **never
auto-merged** (that would silently drop the property's gameplay metadata) **and you
are not prompted about it** — the pair is simply skipped. Property-bearing tiles are
still loaded and shown, so you can deduplicate them **by hand** (manual replacement,
below) whenever you want to.

## SIMILAR → ask the user

The window switches to a decision view showing **both tiles magnified 16×** with
fast (nearest-neighbour, crisp) scaling over a transparency-revealing checkerboard
background. **Maximizing the window scales the previews up** (the checkerboard keeps
4 squares per real pixel so partial transparency stays readable).

* **Click a tile** to **keep** it: the other tile is dropped and all maps are
  repointed to the clicked tile.
* **SKIP** keeps both tiles. The choice is remembered by the **source tile's hash**,
  so the same source tile is never asked about again this run.
* **BACK** reverts your previous decision and re-shows that prompt (click it again to
  step further back through the history) — for when you mis-clicked.

`--batch` keeps every SIMILAR pair (it never auto-merges them — they need a human eye).

## Tileset context, manual replacement, and saving

Under each magnified tile the decision view shows that tile's **whole tileset at
1×** (scrollable) with a **blinking marker** on the tile, auto-scrolled into view,
so you can see where each candidate sits in its sheet. The sheet is shown **with the
changes done**: every tile merged away this session is cleared to **empty space** (its
maps now point at the kept tile, which lives in another sheet), so the view previews
the deduplicated result, not the raw on-disk art. These are **in-memory only** — the
`.tsx`/`.png` files are never modified, and no decision is persisted, so a fresh run
re-reads the clean tilesets and re-evaluates from scratch.

* **Manual replacement** — for a pair the auto-compare misses (e.g. the same tree
  laid out differently in two tilesets), **click a tile in each sheet** (the two that
  match — a green marker shows each pick), then **KEEP LEFT** or **KEEP RIGHT** to
  choose which one survives; the other is dropped and every map repointed to the kept
  tile. **Either tileset can be the one kept** — there is no fixed source/destination.
  Do it tile-by-tile to pair a multi-tile object; revertible with **BACK**.
* **SAVE NOW** flushes the pending merges/replacements to disk immediately (via the
  background writer) without ending the run. It is enabled only when there are
  unsaved changes and disables itself once saved.

Maps are otherwise written when all comparisons finish; serialization happens on the
main thread and the file writes run on a background thread (joined before exit).

## Empty (fully-transparent) tiles → transparent space

Fully-transparent tiles are not compared (that would explode the O(n²) pass), but
they are not left referenced either: every **visual** tile-layer cell pointing at a
fully-transparent tile is replaced by transparent space (an empty cell). This is
**not** done on presence-trigger layers — `Collisions`, `Grass`, `OnGrass`,
`Water`, `LedgesDown`/`Left`/`Right` — where a cell's presence means
block/encounter/surf/ledge regardless of pixels, nor on object markers (e.g.
`invisible.tsx` bot/door anchors).

## Maps with a missing tileset are left untouched

If a map references a `.tsx` whose file is missing (or whose image fails to load),
it is **skipped** and reported: libtiled reads a dangling tileset as an empty
placeholder, and rewriting would shift every later tileset's `firstgid` and corrupt
the map. Fix the dangling reference, then re-run.

## Implementation

C++ with Qt and libtiled. Animated tiles are skipped entirely (merging would drop
their frames). Tiles with custom properties are loaded (shown, hand-mergeable) but
never auto-merged or prompted — a pair is handled automatically (auto-merge when
IDENTICAL, ask when SIMILAR) only when **neither** tile has a custom property;
otherwise it is skipped and can be deduplicated by hand.
