# tileset-tagger + learn-from-tags — rules

Combine with the root CLAUDE.md. The tagger feeds a data-driven map generator:
tag the MODEL datapack and the PARTIAL datapack tilesets with ONE visual
vocabulary, learn the model maps' rules in terms of those tags, then compose
equivalent maps on the partial tileset. Design: `../map-procedural-generation/learn-from-tags.md`.
* MODEL map datapack = `datapack-pkmn/map/main/gen2/` (183 .tmx over 14 tilesets `tileset/normal1..15.tsx`+`animations.tsx`, johto+kanto). PARTIAL (target) datapack = official `CatchChallenger-datapack/`: target tileset (read-only input) = `map/tileset/` (12 .tsx); generated maps OUTPUT = `map/main/generated/` (only writable path there, owner-authorized; ref tilesets by relative path).

## Workflow (end to end)
1. TAG both tileset sets (tags → sidecar): model `gen2/tileset/` (14 .tsx) + target `CatchChallenger-datapack/map/tileset/` (12 .tsx). Drive each tileset's untagged-with-pixels count to 0.
2. INVENTORY-DIFF the tagged sets per category: list what the target is MISSING vs the model (proportional, ≥~80%) and REQUEST those from the human (count/size). Names differ (model `normal*` vs target `t1/t2/t3/building-small/inside-*`) so match by TAG, not filename.
3. DECODE the 183 model maps to tileset-agnostic CATEGORY grids (resolve gids through the tagged tilesets, keep the engine layer role).
4. LEARN hard + random rules + structure from the grids.
5. GENERATE onto the target tileset → `map/main/generated/`. GATE: reproducibility (a real gen2 map round-trips to identity from recorded non-random choices) + human rating (≥4/5 on ~10 typical maps).

## Tag model: LOGICAL (auto) vs VISUAL (human)
* LOGICAL info is DERIVED from the model maps, NEVER hand-tagged: which layer a tile is drawn on, the layer ORDER, and that some layers CANCEL others (Collisions cancels Walkable). Authoritative files: draw/render order = `client/libqtcatchchallenger/maprender/MapVisualiserOrder.cpp`; walkability precedence = `general/base/Map_loaderMain.cpp` (per-cell else-if: Dirt(249) > LedgesLeft/Right/Top/Bottom(250-253) > `(Walkable||monsterZone)&&!Collisions`(0) > blocked(254); same-named layers OR-merge; Grass/Water/Lava are monster-collision zones = walkable). `walkable`/`layer` come from the EFFECTIVE per-cell code, not the drawn-on layer.
* VISUAL info (`category` = what the tile LOOKS like) is tagged by the human, but PRE-FILLED from how the tile is used on the maps (a soft guess to confirm or edit). Categories like "table"; attributes like `horizontalRepeat`, `horizontalMiddleRepeat` (centre repeats, borders fixed), vertical*.
* SUPPORT mis-detected / mis-labelled tags: the human can always override the pre-fill, and when detection is uncertain or CONFLICTS (same tile used as two incompatible roles, category vs layer mismatch, item count/size below the model) the tool REPORTS the problem to the human to fix — never silently guess.
* A 100%-transparent tile is NEVER tagged or placed.
* Tags (and any derived/work data) are stored OUT of the datapack in the XDG sidecar `~/.local/share/catchchallenger/datapack-<sha256(abs datapack root path)>/tileset-<sha256(abs .tsx path)>.json` — the `.tsx`/datapack is read-only (per root CLAUDE.md), so `CatchChallenger-datapack/` can be tagged too.

## Learn the model structure (from the tagged model datapack)
Compose the PRECISE map structure from the model maps, expressed in tags: which tile is RANDOM among N visual equivalents (e.g. grass = random of 3), the average building SIZE and how it is STRUCTURED (wall+roof+door layout), terrain detection (biome zones, edges, corners), and which rules are HARD (always hold) vs RANDOM (distributions).

## Transfer to the partial datapack
After tagging confirms correct detection on the partial tileset:
1. REQUEST the missing objects/tiles, PROPORTIONAL to the model: if the model has 5 tables (2 indoor), the partial datapack must provide ≥4 tables (≥1 indoor). Ask the human per category / count / size for what is missing.
2. COMPOSE the new map datapack on the partial tileset. GATE (reproducibility): this is valid ONLY if the generator can regenerate the MODEL maps from the MODEL tileset using the DEFINED (recorded, non-random) values — i.e. a real model map round-trips to identity. TUNE the algorithm with a human RATING: test ~10 typical maps; most must score ≥4/5 to ship.
