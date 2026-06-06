# tileset-tagger + learn-from-tags — rules

Combine with the root CLAUDE.md. The tagger feeds a data-driven map generator:
tag the MODEL datapack and the PARTIAL datapack tilesets with ONE visual
vocabulary, learn the model maps' rules in terms of those tags, then compose
equivalent maps on the partial tileset. Design: `../map-procedural-generation/learn-from-tags.md`.
* MODEL map datapack = `datapack-pkmn/map/main/gen2/` (100+ .tmx over 14 tilesets `tileset/normal1..15.tsx`+`animations.tsx`, johto+kanto). PARTIAL (target) datapack = official `CatchChallenger-datapack/`: target tileset (read-only input) = `map/tileset/` (12 .tsx); generated maps OUTPUT = `map/main/generated/` (only writable path there, owner-authorized; ref tilesets by relative path).
* `datapack-pkmn/` is a LOCAL-only external reference dataset — never commit/redistribute/ship it or its contents; learn only abstract STRUCTURE/RULES from it; generated output + the target datapack must be ORIGINAL. (See root CLAUDE.md.)

## Human role — keep it MINIMAL
The pipeline AUTO-detects/tags/learns/generates; the human's WHOLE job is only:
1. MODEL tileset: FIX the FEW tag-detection ERRORS (auto-tag first, correct only the wrong ones — never hand-tag from scratch).
2. TARGET tileset: ADD the missing CONTENT (items the inventory-diff is short on) AND fix the FEW tag-detection errors.
3. RATE the generated maps — the tool shows MULTIPLE maps at once (a grid) so several are rated in one shot; the developer ADAPTS the algorithm from that feedback until generation is correct.
Everything else is automatic. The bar: make auto-detection good enough that only a FEW fixes remain — EXTEND the auto-tagger/heuristics, never push bulk work onto the human.

## Workflow (end to end)
1. AUTO-TAG both tileset sets (`--suggest`, tags → sidecar): model `gen2/tileset/` (14 .tsx) + target `CatchChallenger-datapack/map/tileset/` (12 .tsx). The human then FIXES only the few detection errors (not hand-tag from scratch).
2. INVENTORY-DIFF the tagged sets per category: list what the target is MISSING vs the model (proportional, ≥~80%) and REQUEST those from the human (count/size). Names differ (model `normal*` vs target `t1/t2/t3/building-small/inside-*`) so match by TAG, not filename.
3. DECODE the 100+ model maps to tileset-agnostic CATEGORY grids (resolve gids through the tagged tilesets, keep the engine layer role).
4. LEARN hard + random rules + structure from the grids.
5. GENERATE onto the target tileset → `map/main/generated/`. GATE: reproducibility (a real gen2 map round-trips to identity from recorded non-random choices) + human rating (≥4/5; show ~10 typical maps MULTIPLE-at-once in a grid for fast batch rating).

## Tag model: LOGICAL (auto) vs VISUAL (auto-detected, human fixes errors)
* LOGICAL info is DERIVED from the model maps, NEVER hand-tagged: which layer a tile is drawn on, the layer ORDER, and that some layers CANCEL others (Collisions cancels Walkable). Authoritative files: draw/render order = `client/libqtcatchchallenger/maprender/MapVisualiserOrder.cpp`; walkability precedence = `general/base/Map_loaderMain.cpp` (per-cell else-if: Dirt(249) > LedgesLeft/Right/Top/Bottom(250-253) > `(Walkable||monsterZone)&&!Collisions`(0) > blocked(254); same-named layers OR-merge; Grass/Water/Lava are monster-collision zones = walkable). `walkable`/`layer` come from the EFFECTIVE per-cell code, not the drawn-on layer.
* VISUAL info (`category` = what the tile LOOKS like) is AUTO-DETECTED and applied best-effort by `--suggest`/`MapUsageIndex::suggestCategory` (dominant drawn-on layer + effective walkability + green-vs-not colour: terrain exact = high-confidence; Collisions→tree-trunk/building-wall, WalkBehind→tree-canopy/building-roof, Walkable→grass-short/ground/path, OnGrass→bush/flower = low-confidence, marked `auto=guess`). The human only FIXES the wrong guesses (shown YELLOW in the GUI). Keep improving the classifier (blob size, neighbour structure, near-dup clustering) so the fix-list shrinks. Categories like "table"; attributes `horizontalRepeat`, `horizontalMiddleRepeat` (centre repeats, borders fixed), vertical*. (Tiles never used on any map have no signal → stay untagged/red.)
* SUPPORT mis-detected / mis-labelled tags: the human can always override the pre-fill, and when detection is uncertain or CONFLICTS (same tile used as two incompatible roles, category vs layer mismatch, item count/size below the model) the tool REPORTS the problem to the human to fix — never silently guess.
* A 100%-transparent tile is NEVER tagged or placed.
* Tags (and any derived/work data) are stored OUT of the datapack in the XDG sidecar `~/.local/share/CatchChallenger/tileset-tagger/datapack-<sha256(abs datapack root path)>/tileset-<sha256(abs .tsx path)>.json` (own app subdir — isolated from the client's `~/.local/share/CatchChallenger/client*/`) — the `.tsx`/datapack is read-only (per root CLAUDE.md), so `CatchChallenger-datapack/` can be tagged too.

## Learn the model structure (from the tagged model datapack)
Compose the PRECISE map structure from the model maps, expressed in tags: which tile is RANDOM among N visual equivalents (e.g. grass = random of 3), the average building SIZE and how it is STRUCTURED (wall+roof+door layout), terrain detection (biome zones, edges, corners), and which rules are HARD (always hold) vs RANDOM (distributions).

## Transfer to the partial datapack
After tagging confirms correct detection on the partial tileset:
1. REQUEST the missing objects/tiles, PROPORTIONAL to the model: if the model has 5 tables (2 indoor), the partial datapack must provide ≥4 tables (≥1 indoor). Ask the human per category / count / size for what is missing.
2. COMPOSE the new map datapack on the partial tileset. GATE (reproducibility): this is valid ONLY if the generator can regenerate the MODEL maps from the MODEL tileset using the DEFINED (recorded, non-random) values — i.e. a real model map round-trips to identity. TUNE the algorithm with a human RATING: present ~10 typical maps MULTIPLE-at-once (a grid, several rated in one shot); most must score ≥4/5 to ship.
