# gba2catchchallenger

Converts a Gen3 GBA ROM into a CatchChallenger datapack region (maps + rendered
tilesets). Output is trademark-free: maps are addressed by numeric group/map
index, no in-game proper names are emitted.

```
gba2catchchallenger --datapack <datapack-root> --gba <rom.gba>
```

`--gda` is accepted as an alias for `--gba`. Output is written to
`<datapack-root>/map/main/<label>/` with tilesets under `<label>/tileset/`,
where `<label>` is derived from the ROM (`firered`, `ruby`, ...).

## Supported ROMs

| ROM | code/ver | engine | maps |
|-----|----------|--------|------|
| FireRed | BPRE v1 | FRLG | 425 |
| Ruby | AXVE v1 | RSE | 394 |

Adding a sibling (LeafGreen / Emerald / Sapphire) is a one-row edit in
`GameInfo::detect()` — the engines are already handled.

## What is decoded

* **Tilesets** — primary+secondary Gen3 tilesets per map: LZ77-decompressed 4bpp
  graphics, BGR555 palettes, 8-subtile (2-layer) metatiles with flips. Each
  metatile becomes one 16×16 CatchChallenger tile, pooled per
  `(primary,secondary)` pointer pair. Each distinct graphic is stored once;
  tiles with a consistent on-map neighbour are packed as rigid 2-D blocks
  (buildings read like the map) and identical blocks are merged, so the sheet
  has almost no duplicate tiles. Sheets hold `kCapacity` tiles each (16 wide).
  (`kContextSplitMax` can split a tile per-neighbourhood for stricter 2-D, but
  default 0 — it bloats the sheets far more than it helps.) `main.cpp` wipes the
  `<label>/` output dir before each run so stale sheets never accumulate.
  The same 2-D `layout2D` packer is applied to the over (WalkBehind) tiles too,
  so building/tree tops read like the map (not dumped in dedup order); an over
  identical to a ground tile folds onto it (no duplicate). Two **post-build
  GUARDs** (`prepare`): **dup** FAILs if any non-animation graphic repeats inside
  a pool (animation frames exempt); **adjacency** checks every consistent
  immediate map-neighbour stays adjacent in the sheet — it PASSes, reporting only
  *unavoidable cyclic-pattern edges* (a repeating texture / loop that cannot be
  linearised). Watch for `GUARD dup: PASS` and `GUARD adjacency: PASS`.
* **Maps** — width×height metatile grid → `Walkable` layer. Collision bit →
  `Collisions`. Metatile behaviour → `Grass` (wild encounters), `Water` (surf),
  `LedgesUp/Down/Left/Right`.
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
