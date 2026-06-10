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
| `maps/*.tmx` + `gfx/tilesets/*`        | `map/main/tuxemon/<slug>.tmx` + `.xml` + `tileset/` |

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

## Verification

The generated datapack loads cleanly through the **real** engine parsers,
end to end:

* DB (`FightLoader::loadTypes/loadMonsterBuff/loadMonsterSkill/loadMonster`,
  `DatapackGeneralLoader::loadItems`): 13 types, 35 buffs, 275 skills, 219 items,
  28 traps, 393 monsters, **zero warnings**.
* Maps (`Map_loader::tryLoadMap` over every map): **263/263 load, 0 failures**,
  123 318 walkable cells, 51 628 blocked, 890 teleports.  Only `start_tuxemon`
  has no walkable cell (it is an empty 8×8 placeholder in the source).

XML is well-formed; gid fidelity vs the source maps is exact (no tile lost).

## Not yet converted (further phases)

* **Wild encounters** (`db/encounter/*.yaml`).  The `<grass>`/`<water>` monster
  lists in the per-map `.xml` are not emitted yet — Tuxemon keys encounters by a
  separate region table rather than tagging grass cells in the `.tmx`, so this
  needs encounter-zone↔map matching.
* **NPCs / trainers / shops** (`db/npc/*.yaml`, Tuxemon event `act`/`cond`
  dialogue).  Bots are not emitted; only teleport warps are extracted from the
  event system.  Ledges and one-way collision are flattened to plain blocks.
* **Crafting / plants / reputation / profiles** — no Tuxemon equivalent; the
  engine treats their absence as empty.
