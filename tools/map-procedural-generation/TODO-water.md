# Water world — what is built and what is left

The owner asked for a sailable world: water paths joining coastal towns, walled
so the player cannot get lost, some crossings by boat, islands to break the open
sea, and a guarantee that no map ends up isolated.

**Built** (see the `[water]` section of `settings.ini`):

* `LoadMapAll::detectWaterBodies` — connected components of the `Water` layer
  over the whole world, run right after the terrain is drawn. A body of at least
  `[water] seaMinTiles` cells is a **sea**, the only thing a water path may be
  routed on; anything smaller is a **lake** (a pond in a field is not a shipping
  lane). Reference world: 3 bodies, 2 seas, biggest 668720 tiles.
* `[General] terrainDebug` — Object layer `Terrain` in `all.tmx` with the
  outline polygon of every body, traced on a grid downsampled by
  `[water] bodyDebugStep` so it stays a *general* shape (4-5k points instead of
  1.1 million).

* `LoadMapAll::addWaterPaths` — the ROUTING, called from `addCity` right before
  the road grouping: the seas each town can put to sea on (its chunk plus one
  chunk of margin), every pair of towns sharing a sea, nearest first, one route
  per town, A* over the chunks that are at least `[water] chunkSeaPercent` sea.
  The chunks are OR-ed into `mapPathDirection`, so the grouping, the city links,
  the level, the wild monsters (which come out of the water terrain band by
  construction), the border teleports and the map writing all apply to them
  unchanged; `RoadIndex::isWater` / `isBoat` say which ones they are, and
  `boatCrossings` holds the pairs. Measured with it on: 4 routes over 36 chunks
  out of 315 coastal town pairs.

  **It ships OFF (`[water] pathPercentOfLand=0`)**, because nothing PAINTS a
  water chunk yet: turned on, a route is drawn as an ordinary road over the open
  sea, which is worse than not having the feature. Point 3 below is what turns
  it on.

  Known gap to fix when point 3 lands: the routes are planned on the voronoi
  terrain, which is the terrain BEFORE the city flattening ramps back over
  `flattenFalloff` tiles, so the first sea chunk out of a harbour has more land
  in it than the router assumed. Either route on the post-flatten terrain (the
  water pass then has to register its own chunks instead of riding addCity's
  grouping), or accept it and let the channel painting treat that chunk as the
  coast it now is.

**Left to do**, in the order the pieces depend on each other. Every one of them
touches the WORLD MAP GRAPH (`mapPathDirection` / `roads` / `roadCoordToIndex`),
so a half-finished piece breaks every map, not just the water ones — land them
one at a time, each with `check-generated.py` + `check-determinism.py` green and
a render of the result.

## 1. Which towns can be joined by water

For every city, the set of seas it touches (scan its chunk plus one chunk of
margin for a tile whose `waterBodyOfTile` is a sea). A pair of cities is a
CANDIDATE when they touch the same sea. Sort the candidates by distance and keep
the nearest ones — the owner's rule is "interconnect the 2 nearest cities with
sea touching".

New settings: `[water] pathPercentOfLand` (how many water paths relative to the
number of land roads — "water path should be X fewer than land").

## 2. Routing a water path over the chunk grid

A* from one city's chunk to the other over chunks that hold enough sea tiles,
avoiding chunks already used by a land road or a town. The result is a chunk
list, registered exactly like a land road: `mapPathDirection` gets the
orientation bits of each step, `roads` gets the chunk list, `roadCoordToIndex`
gets a `RoadIndex` with a new `isWater` flag (next to the existing `isCave`).

From there the existing machinery already writes the maps, the border teleports
(`addMapChange`), the zone xml and the minimap.

## 3. Painting a water chunk

Per the owner's sketch (confirmed):

* a water CHANNEL from border to border, walled on both sides by the
  `[water] borderTile` rock;
* the wall is a **continuous chain** — never a gap — whose position WANDERS
  around the travel axis (`[water] wanderAmplitude`), so it does not read as a
  drawn corridor;
* beyond the wall: sea the player can never reach;
* ISLANDS inside the channel: at least `[water] islandMinTiles` (owner: ~20)
  land tiles, mountain core, sometimes ringed by at most 2 tiles of sand, and
  they must never touch the rock line;
* `[water] minFighter` / `maxFighter` trainers, placed like the road ones.

Water is walkable for the engine (`walkOn` on the `Water` layer), so the player
simply swims the channel — no item gate.

## 4. Boat crossings

`[water] boatPercent` of the water paths are CLOSED instead: the chunk is walled
all round by rock, holds a `[water] boatTile`, and a `teleport on push` sits on
that tile, entered from the walkable land cell touching it. A boat crossing is
only valid when BOTH sides are boat chunks, and — like a town — the boat tile
must be reachable from the centre of the chunk's biggest walkable zone on both
sides. `LoadMapAll::checkWalkability` already answers that last question.

## 5. Minimap

`minimap-{1way,2way1,2way2,3way,4way}-water.png` are in the repo and unused.
`MiniMapAll::drawRoad` picks the land variants by orientation; it needs the
water set for a chunk whose `RoadIndex.isWater` is set.

## 6. No isolated map

A final check next to the walkability guard: build the graph of chunks joined by
a border teleport (land or water, boat teleports included) and flood it from the
start city. Every written map must be reachable — report and generate nothing
otherwise. This is what makes "I should be able to go to any map from any map
walking or from water" a guarantee rather than an intention.
