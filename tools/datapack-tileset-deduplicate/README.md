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
datapack-tileset-deduplicate --move-from a.tsx --move-to dir/a.tsx [--move-image]  # relocate a.tsx (and its .png with --move-image), fix every map's relative path
datapack-tileset-deduplicate --merge-used --merge-out dir/t.tsx [--merge-keep a.tsx,b.tsx] [<path>]  # pack the used tiles of all maps into ONE tileset
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
`a.tsx`'s directory.

A map that carries an **unresolved tileset** (e.g. an intentional `missing.tsx` left in to
test server robustness) can't go through libtiled — re-serializing it would shift every
later `firstgid` and corrupt the cells. Instead such a map is **repointed in text**: only
`a.tsx`'s `<tileset source>` string is swapped to `b.tsx` (same `firstgid`, so the cells'
gids stay valid); the dangling entry and every base64/zstd layer is left byte-for-byte
untouched. The one case still skipped (and reported) is when the dangling map *already*
uses `b.tsx` — merging the two id ranges would need a gid remap that a text edit can't do.

## Move (relocate) a tileset

```sh
datapack-tileset-deduplicate --move-from <a.tsx> --move-to <dir/a.tsx> [<datapack_path>]
```

**Physically relocates** `a.tsx` to the new path (which may add new sub-folders — they
are created) **without changing its tiles**, then rewrites **every `.tmx`** under the
path that references it so its `<tileset source>` uses the **new relative path** (computed
per map, so each map's own depth is respected). Unlike `--migrate-*` this keeps the
*same* tileset — it only moves the file and fixes the links.

The tileset's **image is not moved**: the relocated `.tsx` keeps the same `.png`, with its
`<image source>` rewritten relative to the new `.tsx` location (the rest of the `.tsx` is
copied byte-for-byte, so `trans`/animations/properties survive untouched). `--move-to` is
resolved relative to the current directory when not absolute; it must end in `.tsx` and
must not already exist. With no `<datapack_path>` the **current directory** (and below) is
the scan root.

A map carrying an **unresolved tileset** (an intentional `missing.tsx`, etc.) is **repointed
in text** — only the moved tileset's `<tileset source>` is rewritten to the new relative
path; the dangling entry and all encoded layer data stay byte-for-byte identical (a move
never merges tilesets, so this is always safe). The original `.tsx` is deleted once every
referencing map (resolvable or dangling) has been repointed.

By default only the `.tsx` moves and its `<image source>` is rewritten to keep pointing at
the image where it sits. Pass **`--move-image`** to relocate the `.png` alongside the `.tsx`
(the new `.tsx` then references it by bare filename) for a self-contained tileset folder.

## Merge the used tiles of every map into one tileset (`--merge-used`)

```sh
datapack-tileset-deduplicate --merge-used --merge-out <dir/t.tsx> [--merge-keep a.tsx,b.tsx] [<path>]
```

Scans every `.tmx` under the path, gathers the tiles **actually used** (tile-layer cells +
object gids) from each **resolvable** tileset, collapses identical 16×16 tiles, and packs the
unique ones into **one** new tileset (`.tsx` + `.png` at `--merge-out`). Every map is then
rewritten to that single tileset: the merged-away `<tileset>` entries are dropped, the new one
is appended (at a `firstgid` above all others), and each cell/object gid is remapped — flip
flags preserved.

`--merge-keep` lists tileset **basenames** to leave as **separate** references (never merged) —
e.g. the engine's `invisible.tsx` markers or an animated `animations.tsx`. **Unresolved**
tilesets (e.g. an intentional `missing.tsx`) are always kept untouched. It works directly on
the gid / base64+zstd layer data, so **maps with a dangling tileset are handled too** (unlike
the libtiled-based passes) and every untouched layer stays byte-for-byte identical. With no
`<path>` the **current directory** (and below) is scanned. Verify with `map2png`: a map renders
pixel-for-pixel identically before and after.

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

## Maps with an unresolved tileset

If a map references a `.tsx` whose file is missing (or whose image fails to load),
libtiled reads it as an empty placeholder, so **re-serializing** the map would shift
every later tileset's `firstgid` and corrupt the cells. How each operation copes:

* **`--migrate` / `--move`** — repoint the affected `<tileset source>` **in text**
  (no re-serialization), so the map *is* updated while the dangling entry (e.g. an
  intentional `missing.tsx`, kept on purpose to test server robustness) and all encoded
  layer data stay byte-for-byte intact. `--migrate` still skips a dangling map only when
  it already uses the migrate-to tileset (that merge needs a gid remap).
* **`--remove` and the dedup pass** — still **skip** dangling maps (and report them):
  clearing/merging cells needs the gid surgery a text edit can't do safely.

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
