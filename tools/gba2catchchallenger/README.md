# gba2catchchallenger

Converts a Gen3 GBA ROM into a CatchChallenger datapack region (maps + rendered
tilesets). Output is trademark-free: maps are addressed by numeric group/map
index, no in-game proper names are emitted.

```
gba2catchchallenger --datapack <datapack-root> --gba <rom.gba>
```

`--gda` is accepted as an alias for `--gba`. The ROM's **role is auto-detected
from its header** (`GameInfo::detect`) and decides where the output lands:

* a **base** → `<datapack-root>/map/main/<label>/` (full maps + tilesets), label
  derived from the ROM (`firered`, `ruby`, ...);
* a **sibling sub** (LeafGreen↔FireRed, Sapphire/Emerald↔Ruby — same region, same
  map geometry) → `map/main/<mainCode>/sub/<subCode>/` as a **diff overlay**;
* a **standalone hack** (an off-fingerprint ROM still on a known Gen3 engine, e.g.
  a 32 MiB expanded build) → its own `map/main/<slug>/`, slug from the filename.

## Supported ROMs

| ROM | code/ver | engine | maps | role |
|-----|----------|--------|------|------|
| FireRed   | BPRE v1 | FRLG | 425 | main `firered` |
| LeafGreen | BPGE v1 | FRLG | 425 | sub `firered/sub/leafgreen` |
| Ruby      | AXVE v1 | RSE  | 394 | main `ruby` |
| Sapphire  | AXPE v2 | RSE  | 394 | sub `ruby/sub/sapphire` |
| Emerald   | BPEE v0 | RSE  | 518 | sub `ruby/sub/emerald` |
| *hack*    | any Gen3 code, off-fingerprint | — | — | standalone `<slug>` |

Classification is deterministic from the header, not fuzzy similarity: a canonical
retail ROM is exactly 16 MiB and validates `gMapGroups` at its known offset.
Anything else on a recognised engine becomes a standalone hack and its
`gMapGroups` is **located by a signature scan** (`Decoder::findMapGroups`), so a
ROM that relocated/expanded its tables still decodes (Glazed kept Emerald's table;
HnS relocated it to 0xF39750).

## Sub-datapack overlays

A sibling sub shares the main's geometry, so its overlay writes **no `.tmx` and no
tileset** — only what differs. `CCWriter::writeSubOverlay` diffs the sibling's
per-map wild-encounter sections (`<grass>`/`<water>`, the version-exclusive
Pokémon) against the already-generated main and writes, per changed map, a partial
`<map>.xml` carrying only the changed sections, plus the sub's own
`informations.xml`. This matches the canonical example
`map/main/test/sub/smallchange/` (a water-only override). Maps identical to the
main are not duplicated; sibling-exclusive maps (e.g. Emerald's Battle Frontier)
are skipped by the wild-only overlay. Because a sub is diffed against its main, the
main must be generated first (see `generate-datapack-pkmn.sh`).

## What is decoded

* **Tilesets** — primary+secondary Gen3 tilesets per map: LZ77-decompressed 4bpp
  graphics, BGR555 palettes, 8-subtile (2-layer) metatiles with flips. Each
  metatile becomes one 16×16 CatchChallenger tile, pooled per
  `(primary,secondary)` pointer pair. Each distinct graphic is stored once;
  tiles are packed as rigid 2-D blocks by their **dominant** (most-frequent,
  reciprocated) on-map neighbour, so a whole building/tree reads like the map —
  even when an occasional cell varies (the old rule required the neighbour to be
  identical on *every* map, which fragmented anything that varied). Identical
  blocks merge, and **near-duplicate** tiles fold too — the ROM often stores two
  metatiles that render to visually-identical graphics (a sub-pixel
  shading/palette nuance) which byte-exact dedup would keep as duplicate-looking
  rows/cols; `dedupAdd` folds a tile into an existing one when every RGBA channel
  is within `kNearMaxDiff` and the mean within `kNearMeanDiff` (imperceptible;
  animation frames never fold). So the sheet has almost no duplicate tiles. Sheets
  are **32 tiles wide** (512 px, ~square like the gen2 native blocksets) and hold `kCapacity`
  tiles each. (`kContextSplitMax` can split a tile per-neighbourhood for stricter
  2-D, but default 0 — it bloats the sheets far more than it helps.) `main.cpp`
  wipes the `<label>/` output dir before each run so stale sheets never accumulate.
  **Region subfolders:** when a label has many tilesets (>50 pools) AND the label
  spans ≥2 regions, a pool whose maps all live in one region is tidied into
  `tileset/<region>/` (e.g. `tileset/kanto/`); pools shared across regions stay at
  the `tileset/` root.
  **Global anim tileset:** ALL water/door animations across the whole label go to ONE
  shared tileset `tileset/anim_<n>.png`, **de-duplicated across pools** (the same water
  / flower / door animation is stored once for the entire region) and laid out **1
  animation per row** (width = the animation length). A pool's own tiles are just
  grounds+overs; it records only `metatile → global anim index`. Each map references
  the global anim tileset at `animBase` (right after its primary+secondary pools), and
  the marker follows it. (firered: 62 animations in one `anim_0` instead of ~22 tiny
  per-pool `_anim` sheets.)
  **Layer composite vs decompose:** a metatile that is collidable everywhere with a
  UNIQUE top (a building face) is composited into ONE wall+roof tile (reads merged,
  like the in-game view); a top that's a REUSABLE overlay (a cliff/ledge edge, a
  rock, a tree top — it sits on ≥2 backgrounds or on a standalone terrain) stays a
  separate TRANSPARENT tile over the base terrain, reused over any background
  (fewer tiles).
  **Background/foreground split of BAKED tiles:** the ROM sometimes bakes a
  collidable object (rock, cliff, tree) onto a terrain in ONE image, so the SAME
  object is stored as several near-duplicate tiles that differ only in their
  background. The builder detects "object over a WALKABLE terrain T" — a composited,
  always-collidable, fully-opaque tile that matches some standable terrain T except
  a connected foreground region of ≥12 px (T must be walkable so the split is not
  inverted) — and, when a foreground is reused over ≥2 distinct terrains, splits it
  into `under = T` (terrain, reused) + `over =` the transparent object (ONE shared
  overlay reused over every background). For a collidable cell the terrain draws on
  Collisions and the object on the 2nd Collisions layer (both below the player,
  OR-merged) so it is visually identical and the background never lands on the
  Water/Grass logic layer. (412 baked tiles across the full set split this way.)
  **Pair-based partial match:** when the same object is baked over two *different*
  backgrounds and the full-tile terrain match above can't find the real background
  (its min-foreground pick is wrong — e.g. a cliff over water vs over sand), the
  builder pairs the two baked tiles, takes their **shared connected region as the
  object** (consistent for both), and **partial-matches each tile's background**
  (the object's complement) against walkable terrains only, pre-checking the
  recomposition and requiring the object reused over ≥2 backgrounds.
  A **tiny-tile GUARD** forbids a near-empty tile (1–12 visible px, the rest fully
  transparent — a wasted/degenerate overlay); a fully-transparent tile is fine and a
  few violations are tolerated. Overlay thresholds are ≥13 px so no split produces one.
  A **layer-recompose GUARD** is the test case for ALL layer splits: every tile split
  across two layers — a ROM-sublayer overlay (terrain under + transparent object over),
  a bg/fg split (reused terrain under + shared object over), or a building wall+roof —
  is recomposed from its FINAL stored under + FINAL stored over and compared to the
  metatile re-rendered straight from the ROM (end-to-end — it catches a wrong split, an
  over-aggressive near-dup fold, or a layout/gid slip); bg/fg objects must keep ≥12
  opaque px. (firered: 1534 split tiles verified, 78 of them bg/fg.) Watch for
  `GUARD layer-recompose: PASS`.  **Compact packing:** blocks are placed with a SKYLINE bottom-left
  strip-packer (the rectangle bin-packing used for sprite atlases), free single
  tiles fill every remaining gap — **anchored first** next to a dominant on-map
  neighbour (even an unreciprocated one), so a reused building corner drops into
  its building's empty hole instead of being scattered to a far cell, then any
  leftover singles raster-fill — and the sheet WIDTH is adaptive per pool
  (≈√tiles, 8..32) so a small pool doesn't waste a 32-wide sheet — firered tileset
  free space dropped from ~67% to ~20%.
  **Tiled Wang/terrain sets:** each `.tsx` carries a best-effort `<wangset>` (corner
  type) — every opaque tile's four 8×8 corners are clustered by average colour into
  the dominant terrains (grass/sand/water/rock/path) and each tile gets a per-corner
  `wangid`, so the editor auto-tiles transitions.  Heuristic (corner colour),
  editor-only (libtiled ignores it when rendering); some wangids may need hand-tweaks.
  **Unified human-view layout:** a human reads a building as ONE object, but it is
  tiles on TWO layers — the ground/under wall (Walkable) and the over/WalkBehind
  roof. So the packer lays out the TOP VISIBLE tile of every map cell (its over if
  it has one, else its ground) in ONE 2-D pass, so a roof groups directly ABOVE its
  wall exactly as seen — overs are NOT dumped in a separate region. An over
  pixel-identical to a ground tile folds onto it (no duplicate); the few walls
  HIDDEN behind a roof (cells carrying both) are not in the human view, so they go
  to a small region appended after the visible one. Two **post-build
  GUARDs** (`prepare`): **dup** FAILs if any non-animation graphic repeats inside
  a pool (animation frames exempt); **adjacency** checks every consistent
  immediate map-neighbour stays adjacent in the sheet — it PASSes, reporting only
  *unavoidable cyclic-pattern edges* (a repeating texture / loop that cannot be
  linearised). Watch for `GUARD dup: PASS` and `GUARD adjacency: PASS`.
  A **metatile-routing** GUARD verifies the primary/secondary split: every map
  cell's metatile (id) must decode to real graphics in the array it is routed to
  (primary when `id < metatilesInPrimary`, else secondary `id − metatilesInPrimary`).
  It FAILs when a used metatile is **empty in its routed array but holds graphics
  in the other** at the same id — the signature of a wrong `metatilesInPrimary`,
  which renders that cell as a backdrop "black hole" (a lost building/object).
  Retail ROMs are 0 (PASS).  An off-fingerprint hack that expanded its tilesets to
  the other engine's size (HnS = Emerald maps + FRLG-size 640 tilesets) would
  otherwise route high-id building metatiles to empty slots; `autodetectTilesetSplit`
  (run before tilesets) decodes every map cell under each standard split
  (512/512/6 vs 640/640/7) and picks the one with the fewest empty metatiles,
  overriding `metatilesInPrimary` only on a strict improvement (retail ROMs are
  never touched).  With the right split HnS now passes this guard — and its render
  is clean.  The guard's "graphics in the other array" probe is bounded by the
  primary's defined metatile count `(attrPtr − metaPtr)/16` so an out-of-bounds
  read past the array can't masquerade as recoverable content.
* **Maps** — width×height metatile grid → `Walkable` layer. Collision bit →
  `Collisions`. Metatile behaviour → `Grass` (wild encounters), `Water` (surf),
  `LedgesUp/Down/Left/Right`.
* **Area names** — retail ROMs read the section-name table directly. A hack that
  RELOCATED it (so `detect()` left it unknown) gets it back: `recoverMapNameTable`
  scans for the stride-8 table whose entries best **type-match** the maps (a
  town/city map must name a `…TOWN/CITY`, a route `ROUTE …`, a cave
  `CAVE/TUNNEL/…`), so a wrong base or a generic string pool scores ~0.  This
  recovers HnS's Johto names and Glazed's custom region — every map gets its real
  name **and** grouping (HnS's whole overworld no longer collapses into one
  `area-0`).  A map with no warp-reachable named area falls back to its own named
  section (a detached building keeps its town); a genuinely unnamed group is named
  from its Gen3 map type — `city-N` / `cave-N` / `road-N`.  Every isolated unnamed
  interior (no name and no warp to any area) is gathered under a single
  `building/` parent (`building-N` / `house-N` inside) instead of cluttering the
  region root next to the real towns.
* **Regions** — maps live at `<label>/<region>/<location>/`.  A retail ROM uses its
  engine region (e.g. Kanto + the Sevii Islands).  A multi-region hack (HnS =
  Johto + Kanto) splits per area: a name keyword table (town/landmark, plus the
  route number — Kanto 1–28, Johto 29–48) assigns the obvious ones, an unnamed
  cave/road **inherits** the region of the town it warps to (graph propagation),
  and any leftover isolated cluster of ≥30 maps becomes an undetected region
  `region-1` / `region-2` (a fully-custom region a name table can't place).
  Cross-region warps use the same relative paths (`../../<region>/…`) the engine
  already resolves for the retail Sevii ferry.
* **Warps** → `door` objects (target map + destination warp coordinates).
* **Connections** → `border-top/bottom/left/right` objects.
* **NPCs** → `bot` objects (id, skin, lookAt).
* `informations.xml`, `start.xml`, per-group `zone/*.xml`.

## Layout (one binary)

| file | role |
|------|------|
| `Gba.*` | ROM bytes, per-game constants (`GameInfo`), pointer helpers, LZ77 |
| `Decoder.*` | walk `gMapGroups` → `DecodedMap` (geometry, warps, connections, NPCs) |
| `TilesetBuilder.*` | render metatiles → tileset PNG/`.tsx`; metatile-behaviour queries |
| `CCWriter.*` | write `.tmx` (zstd+base64 layers) + datapack metadata |
| `SkinResolver.*` | reuse/add bot skins by cropped ±10% per-channel pixel match |
| `DatapackBase.*` | load the base datapack via the client datapack loader |

Tile layers use the standard Tiled `base64` + `zstd` encoding (width×height
little-endian u32 GIDs), matching the rest of the datapack.

## Build

```
cmake -S . -B build && cmake --build build -j
```
