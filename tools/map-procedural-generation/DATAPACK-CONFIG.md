# Per-datapack configuration

The generator's config is datapack-specific: tileset paths, terrain/wall/floor/
furniture tile indices, wild-monster ids, shop item ids and bot skins all depend
on the target datapack. Pick the config with `--config <file>` (defaults to
`settings.ini` next to the binary).

Two configs ship here:

| datapack | config | status |
|---|---|---|
| `CatchChallenger-datapack` | `settings.ini` | complete (native) |
| an external reference datapack | `settings-reference.ini` | **scaffold** — tile indices are best-guess, marked `; TODO tune` |

## Bots / key buildings (`[building]` section)

Bots are emitted **inline** in each map's own `.xml` (the only place the engine
reads them — `Map_loader.cpp`), matched to the `.tmx` bot object by an id that is
**unique per map**. This matches the updated datapack format (`shop`/`fight`
merged into the map file, per-map bot ids). There are no `-bots.xml` / `fight/`
side files any more.

* `shopItems` — numeric item ids that **must exist** in the datapack's
  `items/items.xml`, else the shop is silently empty.
* `botSkins` / `healSkin` / `shopSkin` / `gymTrainerSkin` / `gymLeaderSkin` — a
  skin is a **folder name** under the datapack's `skin/bot` or `skin/fighter`
  (e.g. CC `bankier`; elsewhere `boy`/`girl` or a numbered `skin/bot/<n>`),
  **not** an index.
* `doGym` / `gymTrainers` — a gym (reusing `building-big-1`) is placed in every
  non-small city with that many fight trainers plus one leader. Its monster pool
  is taken from an adjacent road's wild-monster table.

## Run / staging

The generator resolves `settings.ini` tileset paths relative to its CWD and the
bundled `template/*.tmx` reference tilesets as `../dest/map/tileset/<x>.tsx` (or
`../../dest/map/tileset/<x>.tsx` for `building-*/floor-*.tmx`); output is written
to `<binary dir>/dest/map/main/<maincode>/`. So the run dir must contain
`dest/map/tileset/` populated with the `.tsx`+`.png` the chosen config names.
Keep each datapack in its own run dir so their tilesets don't collide. Never
write tilesets back into a source datapack tree (read-only).

* **CatchChallenger-datapack**: copy `CatchChallenger-datapack/map/tileset/*` into
  `<run>/dest/map/tileset/`, then run with `settings.ini`.
* **an external reference datapack**: if its `.tsx` live flat in `map/` (not in
  a `tileset/` subdir), copy `map/*.tsx` + `*.png` into `<run>/dest/map/tileset/`,
  then run with `--config settings-reference.ini`.

## Reference-datapack tile indices — derived from its hand-authored maps

`settings-reference.ini` tile indices were **decoded from that datapack's
hand-authored maps** (after running `datapack-tileset-deduplicate` on its `map/`
to collapse duplicate tiles to a canonical one). The full per-layer decode lives
with the dataset, outside this repo. Data-derived & confident: ground `tileset-1/1`,
tall grass `tileset-1/32`, water `normal18/192`, ledges `tileset-1/740,710,711`,
mountain/collision `tileset-1/961,36-39,70,71`, interior floor `normal1/672-674`,
interior wall `tileset-1/123,155`.

## Remaining TODO for the reference datapack

* A few non-essential biome variants (sand/snow/mountain *ground*) reuse plausible
  `tileset-1` ground tiles, marked `; guess` — that datapack has no distinct
  sand/snow biome, so they affect only rare height/moisture cells.
* The interior furniture + tree overlays (`template/exit.tmx`, `stair-up.tmx`,
  `stair-down.tmx`, `fridge.tmx`, `tree-*.tmx`) still reference CatchChallenger
  `inside-*`/`wood` tilesets it lacks. Add a matching `template/` set or switch
  those entries to inline tiles before a fully clean run.
