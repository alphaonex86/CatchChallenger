# map-procedural-generation — TODO

What is left to make the generated world better, split by who can do it. Every
number here was measured on a full run (107 cities / 617 maps, `settings.ini` of
`build/release`), not estimated.

State of the generator today: 617 maps, 652 teleports, **0 problem** from
`check-generated.py`, every monster / item / music / zone / border / start
reference resolves, two runs are byte identical, and the engine map loader is
silent over a 50 map sample.

---

## 1. Code only (one recompile, uses art that is already drawn)

### 1.1 Town decoration pass
**Why:** a town chunk is 65% walkable ground and **88% of that ground is bare** -
no decoration tile at all. That is what makes a town read as empty.

**What to place**, with the templates that already exist (`template/tree-1.tmx`,
`tree-2.tmx`, `tree-2b.tmx`, `tree-3.tmx`, `flowers.tmx`) and the `signTiles`
already configured in `[city]`:
* tree clusters on the free town ground, as RIGID rectangular blocks (root
  CLAUDE.md rule), never on the avenue and never on a door front or a door path
  — `MapBrush::mapMask` already protects those cells,
* flower beds along the avenue edges and between the street-front lots,
* a sign tile next to the heal / shop / gym entrance (today a sign only sits at
  the town entrances),
* one density knob in `[city]` (0 = nothing, so the current look stays reachable).

**Where:** the city pass of `LoadMapRoad.cpp` (after the buildings are placed and
the avenue is painted, before the vegetation pass).

### 1.2 Road and townsfolk lines in the reviewed bank
**Why:** the interiors take their text from `npc-slots.json` (173 buckets, 1384
reviewed lines), but the road trainers and the town NPCs still take theirs
straight from `dialog.txt`, so they are not part of what a human validates.

**What:** record their slots the same way (`emitRoadBotsForChunk`,
`addCityTownsfolk` / `emitCityBotsForChunk` -> `npc-requests.json` with a bucket
key), then `npcfill.py` covers them too.

### 1.3 Do not ship the debug dump
`dest/map/main/<maincode>/all.tmx` (5.6 MB, the whole world in one file) and
`dest/map/main/tileset/` (run staging only) must not be copied into a datapack.
Either stop writing them next to the real output, or write them under
`dest/debug/`.

---

## 2. Art needed from the owner (no code can do it)

**HARD RULE reminder:** the generator only ASSEMBLES drawn templates. It never
generates, recolours or modifies a tile, a tileset or a facade — a recolour
attempt was made once and reverted (a hue rotation is not architecture, and one
generated sheet per variant is the opposite of what a shared tileset is for).

### 2.1 More building facades
**Why:** a style folder holds several variants that share ONE exterior, so a town
shows the same house again and again:

| style | variants | distinct facades |
|---|---|---|
| brown-city | 3 | 2 |
| desert-city | 9 | 2 |
| flat-roof-city | 9 | 4 |
| med-city | 4 | 4 |
| paris-city | 7 | 3 |
| red-city | 9 | 4 |
| sea-city | 7 | 3 |

**What to draw:** a new `template/<style>-city/<n>/` folder — the code discovers
it on disk, nothing to change. It needs:
* one exterior tmx (any name that is not `floor-*`), 16x16 tiles, layers
  `OnGrass` / `Collisions` / `WalkBehind`, tilesets referenced relatively
  (`../../../tileset/x.tsx`),
* `floor-0.tmx` (the interior) + `floor-0.xml` (the bot/step skeleton: which bot
  runs which step types — every content is regenerated per city),
* run `python3 template-check.py --fix`: it writes the door and the exit, checks
  the skins, the tileset references and the bot objects.

An architecture change is what is wanted here (roof shape, floors, porch, shop
front...), not a colour change.

### 2.2 Town furniture
No template exists for: benches, fences, lamp posts, wells, market stalls,
statues, fountains. Drawn as a small tmx like `flowers.tmx`, the decoration pass
of 1.1 can place them.

### 2.3 Missing datapack assets referenced by the templates
* `music/gym.opus` does not exist (the templates asked for it, the generator
  drops it): draw/record it or the gyms stay silent.
* the numeric skins the templates named (`89`, `153`, `2`, `26`...) do not exist
  in `skin/bot/`: the generator remaps them to the role skin.

---

## 3. Verification still missing

* **Play test.** Nothing has been walked through with the client. Structure,
  references and the engine map loader all pass, but nobody has entered a town,
  talked to an NPC and fought a gym in the generated world.
* **Full engine load.** The map loader was only exercised over a 50 map sample
  (through `map2png`). Loading the whole generated datapack with the server would
  cover the 617 maps at once.
