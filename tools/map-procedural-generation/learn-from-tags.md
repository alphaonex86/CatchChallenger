# Learn-from-tags: data-driven map generation

**MODEL datapack = `datapack-pkmn/map/main/gen2/`** (owner) — 183 `.tmx` maps
(johto + kanto) over 14 tilesets (`tileset/normal1..15.tsx` + `animations.tsx`),
with `informations.xml` / `zone/` / `quests/`. This is the corpus whose rules we
learn and the target the reproducibility guard round-trips against. The PARTIAL
(target) datapack is the official `CatchChallenger-datapack/`.

Goal (owner): stop hand-coding placement rules. Instead **tag every tile** of both
the hand-made (Pokémon-style) tilesets and the official tileset with a shared
semantic vocabulary, **learn the rules** that the hand-made maps obey *in terms of
those tags*, **translate** the rules onto the official tileset, and **generate**
maps of the same quality.

The tags are the bridge: a rule learned as "category `building-wall` block, 4×3,
with `building-roof` on the row above and `door` at the bottom-centre" is
tileset-independent, so it replays on any tileset that carries those categories.

```
   tagged .tsx (pokemon)          tagged .tsx (official)
            |                              |
            v                              v
   .tmx maps --> [category grid] --> LEARN rules ---> GENERATE --> new .tmx
   (datapack-pkmn)                   (hard+random)    (official tileset)
                                          |
                                          +--> REPRODUCIBILITY GUARD
```

## Stages

1. **Categorise** — `tileset-tagger` writes `category` + attributes
   (`horizontalRepeat`, `horizontalMiddleRepeat`, `verticalRepeat`,
   `verticalMiddleRepeat`, derived `size`/`name`) into each `.tsx` as
   `<property>` entries. Human-driven, one pass per tileset. Guard: no tile that
   draws pixels may stay untagged.

2. **Decode to a category grid** — read each hand-made `.tmx`, resolve every cell's
   gid through its `.tsx` to a **category** (compositing layers top-down to the
   visible category, but keeping the layer role: Grass/Water/Ledge/Collisions/
   WalkBehind). Output: per-map 2-D arrays of categories, not gids. This is the
   only representation the learner sees — tileset-agnostic.

3. **Learn rules** — two kinds, both expressed over categories:
   * **HARD rules** (always hold ⇒ constraints): adjacency that is *never*
     violated (e.g. `building-roof` is *always* directly above `building-wall`;
     `door` is *always* on the bottom edge of a wall block; `ledge-down` is
     *always* walkable-below). Mined as: category-pair adjacency frequencies where
     the forbidden direction count is 0. Also fixed item shapes (a `table` is
     always the same WxH block of its tagged tiles).
   * **RANDOM rules** (distributions ⇒ choices): how many houses per town, where
     grass rectangles go and how big, which decoration tile fills a free ground
     cell, building variant picked, etc. Mined as histograms/probabilities per
     context.

4. **Generate** — a deterministic function `generate(rules, rng) -> category grid`,
   then map categories back to **official-tileset** gids (each needed category +
   size must be tagged there; if missing, **request the owner to tag one** — that
   is the only human gate in generation).

## THE GUARD (owner, stated twice): replay-identical

> the software has to be able to generate the SAME/IDENTICAL pokemon map when the
> correct random numbers are used.

This is the completeness proof of the rule-set. The generator must be a pure
function of `(rules, choice-stream)`. Then add an **analyse/encode** mode:

```
encode(real_map, rules) -> choice_stream      # derive the exact choices that rebuild it
generate(rules, choice_stream) -> map'        # replay
assert map' == real_map                        # byte-for-category identical
```

If for every hand-made map there exists a choice-stream that replays to it
exactly, the grammar is **complete** (it can express every real map) and
**sound** (replay is deterministic). A map that cannot be encoded reveals a
missing rule — that gap is the to-do list. So the guard doubles as the coverage
metric: `% of hand-made maps that round-trip to identity`.

Implementation note: `rng` is a small replayable PRNG whose draw sequence IS the
choice-stream (one draw per decision, in a fixed traversal order). `encode` walks
the same traversal, and at each decision point picks the draw value that makes the
generator emit the category actually present in the real map; if no draw can
(the rule set offers no option matching reality), that decision is logged as an
uncovered case. Generation with the recorded stream then reproduces the map.

## Transfer to a partial datapack (model → partial)

The model datapack (pokemon) is complete; the partial datapack (official) is being
built. After both tilesets are tagged and the detection is confirmed:

1. **Request the missing tiles, proportional to the model.** Count tagged items
   per (category, sub-context) in the model and require the partial set to provide
   at least ~80%: if the model has 5 `table` (2 of them indoor), the partial set
   must provide ≥4 tables (≥1 indoor). Ask the human for each shortfall by
   category / count / size. A category with no partial tile blocks any rule that
   needs it.
2. **Compose the new maps**, gated by the reproducibility guard AND a human
   rating: regenerate the MODEL maps from the MODEL tileset with the recorded
   (non-random) choices and round-trip to identity; then rate ~10 typical
   generated maps — most must score ≥4/5 to ship. The rating notes feed back into
   tuning the rules (a human-noted system).

## Detection problems → report, don't guess

When a tile's role is ambiguous or conflicting (used as two incompatible logical
roles across maps, category vs derived layer mismatch, a 100%-transparent tile, an
item whose partial-set count/size is below the model), the tool flags it for the
human to fix instead of silently picking. 100%-transparent tiles are never tagged
or placed.

## Status

* Stage 1 tool (`tools/tileset-tagger/`) — **built, verified, pushed**.
* Stages 2–4 + guard — to implement once the two tilesets are tagged (need real
  category grids to validate against; building the learner before there is tagged
  data to test it would be untestable guesswork).
