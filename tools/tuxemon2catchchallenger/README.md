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

## Verification

The generated datapack loads cleanly through the **real** engine parsers
(`FightLoader::loadTypes/loadMonsterBuff/loadMonsterSkill/loadMonster`,
`DatapackGeneralLoader::loadItems`): 13 types, 35 buffs, 275 skills, 219 items,
28 traps, 393 monsters, **zero warnings**.  XML is well-formed (`xmllint`).

## Not yet converted (next phase)

* **Maps** (`maps/*.tmx` + `gfx/tilesets/*.tsx`).  Tuxemon maps are 16px Tiled
  maps with object-group collisions; CatchChallenger uses layer-name collision
  semantics (`Walkable`/`Collisions`/`Grass`/`Water`/ledges) + `Moving`/`Object`
  groups and warp/bot object contracts.  This is a separate subsystem (see
  `tools/gba2catchchallenger` for the analogous map pipeline).
* **Wild encounters** (`db/encounter/*.yaml`) — belong in the per-map `<map>.xml`
  `<grass>`/`<water>` sections, so they follow the map stage.
* **Crafting / plants / reputation / profiles** — no Tuxemon equivalent; the
  engine treats their absence as empty.
