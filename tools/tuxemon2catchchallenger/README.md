# tuxemon2catchchallenger

Convert the [Tuxemon](https://www.tuxemon.org/) game data (the YAML `db/`, the
battle sprite sheets and the gettext catalogues) into a **CatchChallenger
datapack**: `monsters/type.xml`, `monsters/buff/buff.xml`,
`monsters/skill/skill.xml`, `monsters/monster.xml`, per-monster sprite folders,
and `items/items.xml` (+ icons).

Self-contained, one binary.  Reads YAML with the **system** `yaml-cpp`
(system-lib-first; nothing vendored), writes the datapack XML by hand and
extracts sprites with `QImage` (Qt6 Gui).

## Build

```sh
cmake -S tools/tuxemon2catchchallenger -B build/tuxemon2catchchallenger
cmake --build build/tuxemon2catchchallenger -j
```

Needs Qt6 (Core, Gui) and `yaml-cpp` (`libyaml-cpp-dev`).  Both are picked up
from the system; the converter never installs anything.

## Run

```sh
tuxemon2catchchallenger <mods/tuxemon dir> <output datapack dir>
# e.g.
tuxemon2catchchallenger \
    /home/user/Desktop/CatchChallenger/old/Tuxemon/mods/tuxemon \
    /home/user/Desktop/CatchChallenger/Tuxemon-datapack
```

The output directory is created/overwritten.  The Tuxemon source tree is read
only.

## What is converted

| Tuxemon (`db/…`)        | CatchChallenger                              |
|-------------------------|----------------------------------------------|
| `element/*.yaml`        | `monsters/type.xml` (13 types + matchup chart) |
| `status/*.yaml`         | `monsters/buff/buff.xml`                      |
| `technique/*.yaml`      | `monsters/skill/skill.xml` (+ mandatory id 0)|
| `monster/*.yaml` + `shape/shapes.yaml` | `monsters/monster.xml`        |
| `gfx/sprites/battle/<slug>-sheet.png`  | `monsters/<id>/{front,back,small}.png` |
| `item/*.yaml` + `gfx/items/*.png`      | `items/items.xml` + `items/tuxemon/*.png` |
| `l18n/{en_US,fr_FR}/…/base.po`         | `<name>`/`<description>` (en + `lang="fr"`) |
| `maps/*.tmx` + `gfx/tilesets/*`        | `map/main/tuxemon/<region>/<location>/<slug>.tmx` + `.xml` + shared `tileset/` |
| `db/encounter/*` + `random_encounter` events | map `.xml` `<grass>`/`<grassNight>` + a `Grass` layer |
| `db/npc/*` + `create_npc`/`add_monster`/`set_economy` events | map `.tmx` `bot` objects + `.xml` `<bot>` (text/fight/shop/sell) |
| `db/economy/*`                         | shop `<product>` lists                       |
| `sprites/*` + `gfx/sprites/player/*`   | `skin/fighter/player/` + `skin/bot/<npc>/`   |
| `music/*.ogg`/`*.mp3` (ffmpeg)         | `music/*.opus` + `map/music.xml`             |
| `mod.yaml`                             | `player/start.xml` + `map/main/tuxemon/start.xml` (startable profile) |

The whole raw Tuxemon record for each monster/skill/item/buff/type is *also*
emitted verbatim under an engine-ignored `<tuxemon>` element (a generic
recursive YAML→XML dump), so any Tuxemon field — including ones added in future
Tuxemon versions — is preserved automatically for later support.

### Mapping decisions

* **Ids.**  Monsters keep their Tuxemon `txmn_id`.  Techniques get sequential
  skill ids from 1 (id 0 is the engine's mandatory "Last luck" fallback).
  Statuses → buff ids from 1, items → item ids from 1.  Duplicate slugs/ids in
  the source are dropped (Tuxemon ships 225 item files for 219 unique slugs).
* **Types.**  Tuxemon's own 13 elements and its full effectiveness matrix are
  emitted verbatim (grouped into `2 / 0.5 / 0` multiplicators, default `1`
  omitted) — the datapack is faithfully Tuxemon, not re-skinned onto the
  official CatchChallenger types.
* **Stats.**  Tuxemon has no per-monster stat numbers; they come from the body
  **shape** (6 attributes on a 0..10 scale).  Mapped to the 6 CatchChallenger
  level-100 stats: hp→hp, melee→attack, ranged→special_attack, armour→defense,
  dodge→special_defense, speed→speed, then scaled by evolution stage
  (basic ×1.0, stage1 ×1.18, stage2 ×1.35).  Heuristic; tune in
  `DatapackWriter::computeStats`.
* **Skills.**  Tuxemon `power` (a multiplier) → flat damage `round(power×60)`;
  `accuracy` → `success%`; `power` → PP; a `give` effect → a `<buff>` whose id
  is the converted status, with `potency` → `success%`.
* **Sprites.**  Battle sheets are 2-column (idle bob); the left column is taken
  as the front pose and alpha-cropped.  `back.png` is the front mirrored
  (Tuxemon has no back sprite); `small.png` is an icon-sized copy.
* **Items.**  `category: capture` → `<trap>` (rate from cost), `heal` effect →
  `<hp add>`, a remove/cure effect → `<buff remove="all">`, otherwise a plain
  item (stones, key items, berries) with no effect element.
* **Maps.**  Tuxemon already ships proper external/inline `.tsx` tilesets, so
  the tile gids are kept verbatim and the tilesets are copied once into
  `map/main/tuxemon/tileset/`.  The work is re-homing each cell's tile onto the
  layer-name collision model: the `Collisions` object-group rectangles are
  rasterised to a blocked grid, and each cell's tile goes to a `Walkable` layer
  (passable) or a `Collisions` layer (blocked), over-tiles to `WalkBehind`.
  Tile layers are re-emitted as base64+zlib.  `transition_teleport` events
  become `teleport on it` warp objects (with the loader's `-1` object-Y offset).
  Inline tilesets are materialised as external `.tsx`.
  Maps are organised the official-datapack way, `<region>/<location>/<slug>`:
  region = the map's `scenario` property (else the nearest one through the warp
  graph, else `other`), location = the nearest *outdoor* map in the warp graph,
  so a town and its interiors share one folder.  Warp `map` properties are
  emitted relative to the source map's folder.  The engine marker tileset
  `map/invisible.png`/`.tsx` (byte-identical to the official one) is installed
  and every warp/bot object carries a `gid` into it (`firstgid+2` teleport,
  `+0` bot) so they are visible in Tiled, like the gen2 reference maps.

## Verification

The full datapack loads cleanly through the **real** engine, end to end —
`CommonDatapack::parseDatapack` (skins, reputation, plants, crafting, profiles,
layers + the whole DB) then `Map_loader::tryLoadMap` over every map, with
**zero warnings**:

* DB: 13 types, 35 buffs, 275 skills, 219 items (28 traps), 393 monsters,
  1 skin, 1 reputation, **1 startable profile** (validated starter monster +
  capture item + skin), 3 monster-collision layers.
* Maps: **263/263 load, 0 failures**, 890 teleports, ~120 000 walkable cells,
  ~52 000 blocked, **1 954 wild-encounter cells** over 42 maps, **606 bots**.
  Only `start_tuxemon` has no walkable cell (an empty 8×8 placeholder in source).

XML is well-formed; map gid fidelity vs the source is exact (no tile lost); maps
render correctly with libtiled (including materialised inline tilesets and the
grass encounter layer).

## Known simplifications

* **Ledges / one-way collision** are flattened to plain blocks.
* **Dialogue** is the concatenated Tuxemon `translated_dialog` text; the richer
  branching event logic (`cond`/`act` graphs) is not modelled — but the raw data
  is preserved in the maps for later use.
* **Trainer skins** reuse the NPC overworld sprite; battle front/back are a
  best-effort frame extraction (Tuxemon sprite-sheet layouts vary).
* **Crafting / plants** have no Tuxemon equivalent and are emitted empty.
