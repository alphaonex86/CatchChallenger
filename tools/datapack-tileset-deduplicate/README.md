# datapack-tileset-deduplicate

A CLI tool that finds duplicate **16×16 tiles** across every tileset (`.tsx`) of a
datapack and merges them. When tiles are merged it **writes the changes straight to
disk** (there is no Save button): every map (`.tmx`) that referenced a dropped tile
is rewritten to point at the kept one, and the dropped tile is **blanked to
transparent in its tileset `.png`**. Blanking the dropped tile means a later run
re-reads it as empty and never offers it again — so closing and reopening **resumes
where you left off** instead of redoing the work. Every merge is also appended to a
log (see *Merge log*).

> The tool **modifies the datapack it is given** (`.tmx` and `.png`). Point it at a
> working copy, not a pristine source tree.

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
datapack-tileset-deduplicate --reset-skips <path>        # forget all previously SKIP-ped tiles
datapack-tileset-deduplicate --migrate-from a.tsx --migrate-to b.tsx   # replace tileset a with b everywhere
datapack-tileset-deduplicate --stat [<path>]             # list each .tsx with its tile/map usage
datapack-tileset-deduplicate --remove a.tsx              # delete a.tsx (+image) and clear it from all maps
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
their stored RGB.

The 90% similarity threshold is measured over the **relevant pixels only** — the
pixels that are non-transparent on at least one of the two tiles (the visible
footprint of either). Pixels transparent on *both* sides are ignored. So a
mostly-transparent tile is judged on its actual content, not diluted by the large
matching empty background: two small sprites drawn at **different places**, or that
differ across most of their visible pixels, come out DIFFERENT instead of falsely
SIMILAR. Three outcomes:

* **IDENTICAL** — every pixel matches.
* **SIMILAR** — at least **90% of the relevant pixels** match.
* **DIFFERENT** — fewer than that; left untouched.

A **fully-transparent** tile has no visible content and is skipped from comparison
entirely (it is never IDENTICAL or SIMILAR to anything).

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

The window switches to a decision view with **one column per similar tile** — the
source tile plus *every* tile similar to it, however many that is, in a
**horizontally-scrollable** row (no cap). Each column shows the tile **magnified 16×**
with fast (nearest-neighbour, crisp) scaling over a transparency-revealing
checkerboard background. **Maximizing the window scales the previews up** (the
checkerboard keeps 4 squares per real pixel so partial transparency stays readable).

You work by **forming groups**. The candidates in one prompt are not always all the
same tile (90%-similar gathers near-misses too), so instead of one all-or-nothing
choice you pick out the real duplicates a group at a time:

* **Tick the “group” box** on each tile that is the same (or **click its big
  preview** to toggle the tick).
* Click **KEEP THIS TILE** under the tile you want to keep — the master, the one
  *not* replaced. Every other **ticked** tile is merged into it and all maps are
  repointed to it. Any tile can be the kept one.
* The **kept tile stays in the list** (so you can keep grouping more tiles into it)
  together with the untouched tiles; only the just-merged tiles drop out, and the view
  **refreshes**. Form as many groups as you like; when **1 or 0** tiles remain the tool
  resumes scanning. (A lone leftover is kept as-is.)
* **SKIP** leaves the **shown tiles** alone: it records every one of them so none is
  ever offered again — as a source or as a candidate in another tile's cluster — and
  writes a `SKIP` line per tile to the log. So SKIP is **remembered across restarts**
  (reloaded on startup); you won't be re-asked about those tiles next run.
* **BACK** reverts the group decisions of the **current cluster** (click again to step
  further back) — for misclicks. Once a cluster is committed to disk it can't be undone.

`--batch` keeps every SIMILAR cluster (it never auto-merges them — they need a human eye).

## Tileset context, re-picking, and auto-save

Under each magnified tile the column shows that tile's **whole tileset at 1×**
(scrollable) with a **blinking marker** on the tile, auto-scrolled into view, so you
can see where each candidate sits in its sheet. The sheet is shown **with the changes
done**: every tile merged away is cleared to **empty space**, so the view previews the
deduplicated result, not the raw on-disk art.

* **Re-pick a column's tile** — for a tile the auto-compare mis-detected, or to pair a
  tile laid out differently, **click a different tile inside that column's tileset**;
  the column then stands for the tile you clicked. The group merge then uses each
  column's current tile.

There is **no Save button** — changes are written **automatically**: when you finish a
cluster (it drops to ≤ 1 tile, or you click SKIP) the merges made for that source are
committed to disk (maps rewritten, dropped tiles blanked in the `.png`, merges
logged), and **closing the window** commits the cluster you are in. So work is not
lost if you stop partway, and reopening resumes (blanked tiles are no longer offered).

* **BACK** reverts the decisions of the **current, not-yet-committed cluster** and
  re-shows it (for misclicks). Once a cluster is committed it is on disk and can no
  longer be undone from the tool.

Serialization happens on the main thread and the file writes run on a background
thread (joined before each commit returns).

## Migrate one tileset onto another

```sh
datapack-tileset-deduplicate --migrate-from <a.tsx> --migrate-to <b.tsx> [<datapack_path>]
```

Rewrites **every map** that references `a.tsx` to use `b.tsx` instead — each cell keeps
its **local tile id** (so `a` and `b` must share the same tile layout) — then **deletes
`a.tsx` and its image** (`.png`). If a map already uses `b.tsx`, the two are merged
(`a`'s cells move into `b`'s id range). No deduplication is run; this is a one-shot
tileset replacement. With no `<datapack_path>` the maps are searched from the parent of
`a.tsx`'s directory. Maps with a missing/unresolved tileset are skipped (and reported),
as elsewhere.

## Tileset usage report (`--stat`)

```sh
datapack-tileset-deduplicate --stat [<datapack_path>]
```

Prints, for **every `.tsx`** under the path, how many **tile cells** reference it across
all `.tmx` under the path — **counting repeats** (the same tile placed N times counts N)
— and in **how many maps**, e.g. `map/normal1.tsx  953 tiles used into 5 maps`. Sorted
least-used first, so unused (`0 tiles`) and rarely-used tilesets — good candidates for
`--remove`/`--migrate` — are at the top. With no path, the **current directory** is used.

## Remove a tileset (`--remove`)

```sh
datapack-tileset-deduplicate --remove <a.tsx>
```

Deletes `a.tsx` **and its image**, and in every map that uses it **clears every cell**
that referenced it (to empty) and drops its `<tileset>` entry. Unlike `--migrate-*`,
there is no replacement — the tiles are removed. Maps with a missing/unresolved tileset
are skipped (and reported). With no `<datapack_path>`, maps are searched from the parent
of `a.tsx`'s directory.

## Dead tilesets → deleted

When merging away tiles leaves a tileset's `.png` **entirely transparent** (every tile
was merged), that tileset is dead: its `.png` and `.tsx` are **deleted** and its
`<tileset>` reference is **removed from every map** (the cells were already repointed
to the keepers). A tileset that still has any visible tile — a kept tile, or an
animation/property tile — is never deleted.

## Empty (fully-transparent) tiles → transparent space

Fully-transparent tiles are not compared (that would explode the O(n²) pass), but
they are not left referenced either: every **visual** tile-layer cell pointing at a
fully-transparent tile is replaced by transparent space (an empty cell). This is
**not** done on presence-trigger layers — `Collisions`, `Grass`, `OnGrass`,
`Water`, `LedgesDown`/`Left`/`Right` — where a cell's presence means
block/encounter/surf/ledge regardless of pixels, nor on object markers (e.g.
`invisible.tsx` bot/door anchors).

## Merge log

Every decision is recorded in an **append-only** log at
`<datapack_path>/tile-dedup.log`. Merge lines are written when the merge is
**committed to disk** (so the log reflects what is actually persisted); skip lines are
written immediately. Lines are:

```
<ISO-8601 timestamp>  AUTO|USER  <loser .tsx>#<id>  ->  <kept .tsx>#<id>
<ISO-8601 timestamp>  SKIP  <.tsx>#<id>
```

* **AUTO** — an automatic IDENTICAL merge.
* **USER** — a merge you made (a group keep).
* **SKIP** — a tile you left alone.

A per-run header (`# === run <timestamp> ===`) precedes each run's lines. The file is
**only ever appended to** in normal use, so it accumulates a full history. On startup
the **SKIP lines are reloaded**, so skipped tiles are not offered again (merges resume
on their own — the dropped tile is already blanked in the `.png`).

### Reset skipped tiles

Skips live in the `SKIP` lines of `<datapack_path>/tile-dedup.log`. To make the tool
ask about them again, run once with **`--reset-skips`** (it strips the `SKIP` lines and
keeps the merge history), or just delete those lines / the whole `tile-dedup.log` by
hand. (Deleting the log does **not** undo merges — those are already applied to the
maps and `.png` files.)

## Maps with a missing tileset are left untouched

If a map references a `.tsx` whose file is missing (or whose image fails to load),
it is **skipped** and reported: libtiled reads a dangling tileset as an empty
placeholder, and rewriting would shift every later tileset's `firstgid` and corrupt
the map. Fix the dangling reference, then re-run.

## Implementation

C++ with Qt and libtiled. Animated tiles are skipped entirely (merging would drop
their frames) — both libtiled-native ones (`<animation>` frames) and tiles flagged
with a custom **`animation`** property such as `"100ms;6frames"`: that tile is the
first frame and the animation spans it plus the next *frames−1* tile ids, so all of
them are ignored (e.g. `"400ms;2frames"` ignores the tile and the 1 tile after it,
`"100ms;6frames"` the tile and the 5 after it). Other tiles with custom properties
are loaded (shown, hand-mergeable) but never auto-merged or prompted — a pair is
handled automatically (auto-merge when IDENTICAL, ask when SIMILAR) only when
**neither** tile has a custom property; otherwise it is skipped and can be
deduplicated by hand.
