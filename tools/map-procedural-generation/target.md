# Procedural Map Generator — Datapack Specification & Rules

This document is a complete, vendor‑neutral specification of the on‑disk format and the
generation rules needed to procedurally build a tile‑based, creature‑collecting RPG world
("datapack"). It is written so an independent program (or another agent) can **generate a
brand‑new world from scratch** that loads in the same engine, **without copying any
existing place names, creature names, art, music, or storyline**.

The generator writes everything under one **generation root** (referred to below as
`<root>/`). All cross‑references inside the files are **relative paths** — relative to the
file that contains them, or to `<root>`. Never write an absolute path.

Shared art (the tileset `.tsx` files and the marker tilesets) lives **two directories above
`<root>`** and is reached with `../` segments (see §3.3). The generator does not create or
edit those `.tsx` files; it only references them and places tile ids from them.

### Generic vocabulary

Throughout this document the following neutral terms are used. They map onto whatever the
engine's underlying systems are called; pick your own original proper nouns when you emit
content.

| Generic term | Meaning |
|---|---|
| **creature / monster** | A catchable/battleable entity referenced by id in encounter and battle tables |
| **region** | A top‑level grouping folder of locations |
| **location** | A named place — town, road, cave, building cluster (a folder) |
| **map** | One screen/area the player stands on (one `.tmx` + one `.xml`) |
| **overworld map** | A `city`, `outdoor`, or `cave` map (has the outside world, encounters, borders) |
| **interior** | An `indoor` map entered through a door (house, shop, healing center, lab…) |
| **healing center** | An interior whose NPC fully restores the player's party |
| **shop** | An NPC/interior that sells and/or buys items |
| **bot** | Any scripted NPC, sign, trainer, vendor, or interactive marker |
| **faction / team** | Optional allegiance tag on a bot |
| **variant** | An alternate edition that only swaps spawn tables |

---

## 1. What "correct output" means

The generator succeeds when it emits a set of files that together satisfy:

1. **Paired files.** Every playable map is a matched pair: a `.tmx` (geometry + objects in
   the Tiled map format) and a sibling `.xml` (metadata: name, encounters, NPC scripts),
   sharing the same basename in the same folder.
2. **Fixed grid.** All geometry uses an orthogonal grid of **16×16‑pixel tiles**.
3. **Connected world.** From the start spawn (`start.xml`), every map is reachable by
   walking through **edge borders** and **doors**; both come in matched pairs.
4. **Resolvable references.** Every border/door target, every bot id/file, every creature
   id, item id, zone, music file, and quest target points at something that exists.
5. **Shared tile semantics.** Tile graphics come from external tilesets referenced by
   relative path with `firstgid` offsets; logic comes from a small fixed set of **layers**
   and **object types** defined below.

---

## 2. Datapack tree under `<root>/`

```
<root>/
  informations.xml            # world identity: name, theme color, initial letter
  start.xml                   # spawn profiles (where new players begin)

  <region>/                   # one folder per region (kebab-case slug, invented)
    <location>/               # one folder per location
      <location>.tmx          # main overworld map geometry for this location
      <location>.xml          # main map metadata
      <interior>.tmx          # zero+ interiors entered from this location
      <interior>.xml
      bots.xml                # optional: shared NPC script library for this location
      <name>-bots.xml         # optional: external NPC bundles referenced by maps

  zone/                       # per-zone presentation (music) keyed by zone slug
    <zone>.xml

  music/                      # audio assets referenced by maps and zones
    <track>.opus

  quests/                     # optional quests
    <n>/
      definition.xml
      text.xml

  sub/                        # optional alternate editions
    <variant>/
      informations.xml        # variant identity
      <region>/.../<map>.xml  # sparse overrides: spawn tables only
```

Rules:

- **Folder/file names** are lowercase `kebab-case` slugs invented by the generator. The
  engine resolves maps by **path**, not by display name, so slugs are free — never reuse
  any pre‑existing place name.
- A **location folder** holds its overworld map plus every interior reachable from it. Keep
  interiors in the same folder so a door target can be written as a **bare filename** (no
  directory part).
- The main overworld map conventionally shares the folder name
  (`town-a/town-a.tmx`), but only the path in border/door targets actually matters.

### 2.1 Region & location naming taxonomy

A readable world names every folder for *what it is*. Derive names in this order (a
real ROM-import does the same — see the reference worlds under `datapack-pkmn/map/main/`,
which split a two-region world into `<region>/` trees with named locations and typed
fallbacks):

1. **Real name when known.** If the source gives a place a name, slugify it
   (`new-bark-town`, `route-29`, `ice-path`). The overworld map repeats the folder name.
2. **Type fallback for an unnamed location.** With no name, name it by its map *type*:
   a `city`/town cluster → `city-N`; interconnected `cave` → `cave-N`; an outdoor route →
   `road-N`. Never a bare `area-N`.
3. **One shared `building/` for orphan interiors.** Every interior with no name **and**
   no warp to any named place (disconnected in the warp graph) collects under a single
   `building/` parent (`building-N` / `house-N` inside), so it never clutters the region
   root beside the real towns.
4. **A map keeps its own named section** when it has no warp-reachable named neighbour but
   the source still tags it (a detached town building stays under its town, not `building/`).

**Regions (the top folder level).** A single-region world uses one region folder. A
**multi-region** world assigns each location a region:

- **By name/affinity** when recognisable (a known place → its region).
- **By graph propagation** — an unnamed cave/road/city inherits the region of the place it
  warps or borders to (build the *map-of-maps*: nodes = locations, edges = warps+borders).
- **By isolation** — a leftover cluster densely linked internally but weakly to the rest
  (e.g. ≥~30 maps reachable among themselves) is its own region; if it can't be named,
  call it `region-1`, `region-2`, … A tiny stray cluster joins the dominant region rather
  than spawning a one-map "region".

Cross-region borders/doors use ordinary relative paths (`../../<region>/<location>/<map>`);
the engine resolves them the same as same-region links.

---

## 3. Coordinate system, units, and paths

### 3.1 Units
- Grid is **orthogonal**, render order **right‑down**, tile size **16×16 px**,
  `infinite="0"`.
- A map has integer `width`×`height` **in tiles**.
- **Layer cells** are indexed in tiles, row‑major from the top‑left, `width*height` cells
  total.
- **Object geometry** (`x`, `y`, `width`, `height` on a `<object>`) is in **pixels**. On a
  16‑px grid every gameplay object is `16×16` and its `x`,`y` are multiples of 16.
  Tile `(col,row)` ⇄ pixel `(col*16, row*16)`.
- **Warp destination coordinates** carried as object *properties* (a door's `x`/`y` saying
  where to place the player in the target map) are in **tiles**, not pixels.

> Keep this distinction strict and never mix it up: *object placement = pixels*;
> *warp destination property = tiles*; *layer cells = tiles*.

### 3.2 Object Y baseline
Tiled places tile‑objects by their **bottom‑left** corner in some versions. In the existing
assets, gameplay objects are emitted with `y` equal to the top‑left tile pixel (i.e. the
object occupies the cell `(x/16, y/16)`). Generate consistently and verify a known door
lands on the intended tile.

### 3.3 Relative paths to shared tilesets
Shared `.tsx` files live **two directories above `<root>`**. The number of `../` segments
depends on how deep the referencing `.tmx` sits:

- A map at `<root>/<region>/<location>/map.tmx` is **four** directories below the tileset
  directory → reference tilesets as `../../../../tileset-1.tsx`.
- A map nested one level deeper uses **five** `../` (`../../../../../tileset-1.tsx`).

Compute the prefix as: *(depth of the `.tmx` below the tileset directory)* repetitions of
`../`. The marker tilesets (`invisible.tsx`, `animations.tsx`) are reached the same way.

---

## 4. The `.tmx` map file (geometry + interactivity) — extensive

### 4.1 Document skeleton

```xml
<?xml version="1.0" encoding="UTF-8"?>
<map version="1.4" tiledversion="1.4.1"
     orientation="orthogonal" renderorder="right-down"
     width="W" height="H" tilewidth="16" tileheight="16"
     infinite="0" nextlayerid="N" nextobjectid="M">

    <!-- tilesets: cumulative firstgid, relative source paths -->
    <tileset firstgid="1"    source="<prefix>tileset-1.tsx"/>
    <tileset firstgid="1025" source="<prefix>tileset-2.tsx"/>
    <!-- … any visual tilesets this map actually uses … -->
    <tileset firstgid="4097" source="<prefix>invisible.tsx"/>   <!-- logic markers -->
    <tileset firstgid="4113" source="<prefix>normalN.tsx"/>     <!-- interior/extra art -->
    <tileset firstgid="5137" source="<prefix>animations.tsx"/>  <!-- animated tiles, doors -->

    <!-- layers, bottom-to-top -->
    <layer id="1" name="Walkable" width="W" height="H">
        <data encoding="base64" compression="zstd">…</data>
    </layer>
    <!-- … more layers … -->

    <!-- interactivity -->
    <objectgroup id="..." name="Moving"> … </objectgroup>
    <objectgroup id="..." name="Object"> … </objectgroup>
</map>
```

Top‑level `<map>` attributes the generator must set:

- `orientation="orthogonal"`, `renderorder="right-down"`, `tilewidth="16"`,
  `tileheight="16"`, `infinite="0"` — constant for every map.
- `width`, `height` — the map size in tiles.
- `version`/`tiledversion` — the Tiled format version (any recent value is accepted by the
  engine; keep it constant across the world).
- `nextlayerid` / `nextobjectid` — one greater than the highest layer/object id used. Keep
  them correct so ids stay unique.

### 4.2 Tilesets and the global‑id (gid) system — in depth

- Each `<tileset>` is included **once per map** with a `firstgid` and a relative `source`.
- A layer cell stores a **global id (gid)**. To resolve which tileset/tile a gid belongs
  to: find the tileset with the **largest `firstgid` ≤ gid**; the local tile index is
  `gid - firstgid`. `gid == 0` means **empty cell**.
- **`firstgid` is cumulative, per map.** The first tileset starts at `1`; each subsequent
  `firstgid` = previous `firstgid` + (number of tiles in the previous tileset). Because
  maps include different subsets of tilesets, the same tileset can receive **different**
  `firstgid` values in different maps — that is expected and fine, as long as it is
  internally consistent within each file.
  - In practice large tilesets occupy ~1024‑tile bands, giving the familiar ladder
    `1, 1025, 2049, 3073, 4097, …`. Small marker/animation tilesets (e.g. 16 tiles) produce
    `+16` offsets such as `4097 → 4113`. Either way, just sum tile counts.
- **Top‑bit flip flags.** Tiled reserves the high 3 bits of each 32‑bit gid for
  horizontal/vertical/diagonal flips. They are normally 0 here; preserve/clear them
  deliberately when emitting cells.
- **Tileset roles** (the generator chooses which to include per map):
  - *visual tilesets* — ground, water, cliffs, buildings, foliage, town/route themes, etc.
  - *the "invisible" marker tileset* — its tiles are not drawn; they tag objects/cells with
    meaning (border markers, bot/rescue anchors, collision helpers). Object types like
    `bot`, `rescue`, and `border-*` use marker gids from this set for their `gid`.
  - *the "animations" tileset* — animated tiles such as **doors** (open/close frames) and
    water shimmer. Door objects draw with a gid from this set.
- Always include the marker tileset on any map that places objects, and the animations
  tileset on any map that has doors/animated tiles.

### 4.3 Layers — the full set, semantics, and ordering

Layers are written **bottom‑to‑top**; later layers render over earlier ones and over the
player unless noted. A `<layer>` carries `id`, `name`, `width`, `height` and a single
`<data>` child. Use this semantic vocabulary (emit only the layers a map needs — a tiny
interior may have just `Walkable` + `Collisions`):

| Layer name | Role and generation guidance |
|---|---|
| `Walkable` | **Base ground** the avatar stands on. Almost every map has it. May appear more than once when ground art is composited in passes. |
| `Grass` / `OnGrass` | **Tall‑grass** tiles. Standing on a non‑empty cell here triggers **land random encounters** (`grass` / `grassNight` tables). Scatter patches where you want wild battles. |
| `Dirt` | Cosmetic path/dirt overlay. No special behavior. |
| `Water` | **Water** tiles. Triggers **surf/fishing encounters** (`water`, `waterOldRod`, `waterGoodRod`, `waterSuperRod`) and gates water traversal. |
| `LedgesDown` / `LedgesLeft` / `LedgesRight` | **One‑way jump ledges**; the player may hop across only in the named direction. Use to enforce routing/shortcuts. |
| `Collisions` | **Blocking mask.** Any non‑empty cell is impassable. This is the authoritative walkability layer; compute it last from all solid features. |
| `WalkBehind` | Tiles drawn **on top of** the avatar (tree canopies, roof eaves, sign tops) so the player can pass "behind" them. Pair tall objects: solid base in `Collisions`, upper part in `WalkBehind`. |
| `Back` / `Over` / `Random`/`random` | Optional extra art / decoration / parallax layers; no gameplay effect. |

Generation rule of thumb for solids: a building/tree/cliff places its footprint in
`Collisions`; any pixels that should overlap the player from above go in `WalkBehind`; the
visible body goes in ordinary visual layers below.

### 4.4 Layer `<data>` encoding — exact procedure

Preferred encoding for parity and size:

```xml
<data encoding="base64" compression="zstd">…base64 text…</data>
```

To produce it:
1. Build the cell array: `width*height` entries, **row‑major**, each a **little‑endian
   unsigned 32‑bit** gid (with flip bits in the top 3 bits, usually 0; `0` = empty).
2. Serialize to a raw byte buffer (`4 * width * height` bytes).
3. Compress the buffer with **zstd**.
4. **Base64‑encode** the compressed bytes; that text is the `<data>` body.

To read existing maps, reverse the steps. (Plain `encoding="csv"` — comma‑separated gids,
row‑major — is also valid Tiled and is far easier to emit while iterating; switch to
base64+zstd for final output to match the rest of the world and shrink files.)

### 4.5 Object group `Moving` — transitions between maps

`<objectgroup name="Moving">` holds everything that moves the player to another map. Each
child is an `<object>` with a category (`type="…"` in some editor versions, `class="…"` in
others — accept both; emit one consistently), a marker/animation `gid`, pixel geometry, and
a `<properties>` block.

**1) Edge borders** — `border-left`, `border-right`, `border-top`, `border-bottom`.
Seamlessly scroll the player into the neighboring map when they step off that edge.

```xml
<object type="border-left" gid="<marker>" x="0" y="272" width="16" height="16">
  <properties><property name="map" value="../west-town/west-town"/></properties>
</object>
```
- `map` property = relative path to the neighbor's main map (same region:
  `../<location>/<map>`; other region: `../../<region>/<location>/<map>`). The basename form
  without extension is used.
- Place the object on the matching edge: `left` at column 0 (`x=0`), `right` at the last
  column (`x=(W-1)*16`), `top` at row 0 (`y=0`), `bottom` at the last row (`y=(H-1)*16`).
- Span the connecting edge with as many border objects as there are connecting tiles, or
  one per crossing row/column, aligned with the neighbor.

**2) Doors / warps** — `door`. A point that teleports the player.

```xml
<object type="door" gid="<door-anim>" x="144" y="224" width="16" height="16">
  <properties>
    <property name="map" value="lab"/>          <!-- bare filename = same folder -->
    <property name="x" value="5"/>               <!-- destination TILE x -->
    <property name="y" value="10"/>              <!-- destination TILE y -->
  </properties>
</object>
```
- `map` = target map (bare filename for an interior in the same folder; otherwise a
  relative path). `x`,`y` = destination **tile** coordinates (must be walkable).
- Draw the door with an animated door gid from the animations tileset.

**3) Rescue / respawn anchor** — `rescue`. Where the player is returned on faint/rescue
(commonly a healing center or town entrance). Uses an invisible marker gid; usually no
properties.

```xml
<object type="rescue" gid="<marker>" x="144" y="240" width="16" height="16"/>
```

### 4.6 Object group `Object` — entities placed in the map

`<objectgroup name="Object">` holds NPCs and pickups.

**1) Bots (NPCs / signs / trainers / vendors)** — `bot`. Anchored to a tile.

```xml
<object type="bot" gid="<marker>" x="112" y="256" width="16" height="16">
  <properties>
    <property name="id" value="3" type="int"/>      <!-- links to <bot id="3"> in this .xml -->
    <property name="lookAt" value="left"/>           <!-- facing: left|right|up|down -->
    <property name="skin" value="<sprite-slug>"/>     <!-- which character sprite -->
  </properties>
</object>
```
- Provide **either** `id` (int, linking to a `<bot id="…">` in this map's companion `.xml`)
  **or** `file` (the basename of an external bot bundle, e.g. `route-a-bots`, letting many
  maps share scripts).
- Optional cosmetics: `lookAt` (facing direction), `skin` (sprite to render), `visible`
  (set false for invisible trigger bots).

**2) Ground items / pickups** — `object`. A collectible tile.

```xml
<object type="object" gid="<item-tile>" x="240" y="448" width="16" height="16">
  <properties><property name="item" type="int" value="35"/></properties>
</object>
```
- `item` (int) = the item id granted on pickup. The drawn `gid` is the item's visual tile.

**Property typing.** Numeric properties carry `type="int"`. String properties omit the
`type` attribute. Keep this consistent.

---

## 5. The `.xml` map metadata (sibling of each `.tmx`) — extensive

Every playable map has a companion `<map>` XML describing presentation, wild encounters,
and NPC behavior. The two files share a basename; the engine pairs them automatically.

### 5.1 Root element and attributes

```xml
<map type="city|outdoor|cave|indoor"
     zone="<zone-slug>"
     backgroundsound="music/<track>.opus"
     color="#000000" alpha="60">
  …
</map>
```

- `type` (**required**) classifies the map and chooses default behavior + music category:
  - `city` — town overworld: doors to interiors, signs/NPCs, optional water encounters.
  - `outdoor` — road/route: land encounters in grass, optional water encounters, borders.
  - `cave` — enclosed area: cave encounters, optional water; often darkened.
  - `indoor` — interior: **no wild encounters**; services and flavor NPCs live here.
- `zone` (optional but recommended) — links to `zone/<zone>.xml` for shared presentation.
- `backgroundsound` (optional) — overrides the zone music for this single map; relative
  `music/…` path.
- `color` + `alpha` (optional, mainly on `cave`) — a darkness/tint overlay (e.g.
  `color="#000000" alpha="60"` for a dim cave). Higher alpha = darker.

### 5.2 Names, descriptions, and localization

```xml
<name>Display Name</name>
<name lang="fr">Nom traduit</name>
<shortdescription>One-line tagline</shortdescription>
<shortdescription lang="fr">…</shortdescription>
<description>A sentence or two of flavor text.</description>
<description lang="fr">…</description>
```

- Any human‑readable element may be repeated with a `lang="…"` attribute to add a
  translation; the element **without** `lang` is the default/base language.
- `shortdescription`/`description` are mainly used by towns; roads often carry only `name`.
- **Generate original names and flavor** — do not reproduce existing world text.

### 5.3 Wild‑encounter tables

Encounters are named groups of weighted `<monster>` entries. Emit only the groups the map
type and its layers support (land groups need a grass layer; water groups need a water
layer; cave groups apply to `cave` maps).

| Group element | Triggered by |
|---|---|
| `<grass>` | Walking in tall grass (day / default). |
| `<grassNight>` | Tall grass at night (optional day/night variation). |
| `<cave>` / `<caveNight>` | Walking inside a `cave` map (day / night). |
| `<water>` | Surfing on water tiles. |
| `<waterOldRod>` / `<waterGoodRod>` / `<waterSuperRod>` | Fishing, by rod tier (progressively rarer & higher level). |

Each entry:

```xml
<monster id="<creatureId>" luck="<weight>" minLevel="<lo>" maxLevel="<hi>"/>
<!-- or a single fixed level: -->
<monster id="<creatureId>" luck="<weight>" level="<n>"/>
```

Rules and guidance:

- `id` references a species. In the **base** world it may be a readable species key; in
  **variant overrides** (§8) it is the **numeric** species id. Use the form the file you're
  writing expects.
- `luck` is a **relative weight**; within one group the weights should sum to ~`100` so each
  reads like a percent. The engine normalizes by the group total regardless.
- Provide either `level` (fixed) or a `minLevel`/`maxLevel` range.
- Design heuristics: 2–4 common species (high weight) + 1–2 rare ones (`luck="1"–"5"`);
  raise levels the farther a map is from the start; make higher rod tiers / night tables
  rarer and stronger than lower ones; keep day/night sets overlapping but distinct.

### 5.4 Bots — the NPC step machine (full step catalog)

NPC behavior is a small state machine. Each `<bot>` has an `id` that matches the `id`
property of a `bot` object in the `.tmx` (or lives in a shared bundle), plus an ordered list
of `<step>`s. Text steps branch by linking to other step ids.

```xml
<bot id="3" team="<faction-slug>">
  <name>NPC Name</name>
  <name lang="fr">…</name>
  <step id="1" type="text"> … </step>
  <step id="2" type="shop"> … </step>
  …
</bot>
```

- `id` (**required**) — unique within the file/bundle; matches the `.tmx` bot object.
- `team` (optional) — faction tag for themed NPCs (invent your own slugs).
- `name` — localizable display name.

**Step `type` catalog:**

| `type` | Children / attributes | Effect |
|---|---|---|
| `text` | `<text><![CDATA[…]]></text>` (localizable) | Show a dialog box. Body allows simple markup: `<br />` line breaks, `<b>…</b>` emphasis, and `<a href="<stepId>">label</a>` links that jump to another step. A target like `href="3;2"` **runs step 3 then continues at step 2** (chain a service, then a follow‑up line). |
| `shop` | `<product item="<key|id>" [overridePrice="N"]/>` × many | Open a **buy** menu. Omit `overridePrice` to use the item's default price. |
| `sell` | attribute `shop="<id>"` (optional) | Open a **sell** menu (often paired with a `shop` step on the same vendor). |
| `heal` | — | Fully **restore the player's party** (healing center / lab service). |
| `warehouse` | — | Open the player's **storage** system. |
| `fight` | `<start>…</start>`, `<win>…</win>` (CDATA), `<monster id level/>` × team, `<gain cash="N"/>` | A **battle** against this NPC: `start` line before battle, `win` line after defeat, the opposing team, and the prize money. |
| `quests` | — | Open the **quest** dialog/board for quests bound to this bot. |
| `learn` | — | Open a **move/skill‑learning** service. |
| `industry` | attribute `industry="<id>"` | Open a **crafting/production** facility by id. |

Worked examples:

```xml
<!-- A sign: one text step -->
<bot id="1">
  <name><![CDATA[Sign]]></name>
  <step id="1" type="text">
    <text><![CDATA[Town entrance — travelers welcome.]]></text>
  </step>
</bot>

<!-- A healing-center attendant: greet, then heal, then sign off -->
<bot id="1">
  <name>Attendant</name>
  <step id="1" type="text">
    <text><![CDATA[Let me restore your team.<br /><a href="3;2">[Heal]</a>]]></text>
  </step>
  <step id="2" type="text"><text><![CDATA[Good luck out there!]]></text></step>
  <step id="3" type="heal"/>
</bot>

<!-- A vendor: dialog branches to buy and sell -->
<bot id="1">
  <name>Seller</name>
  <step id="1" type="text">
    <text><![CDATA[Welcome!<br /><a href="2">Buy</a><br /><a href="3">Sell</a>]]></text>
  </step>
  <step id="2" type="shop">
    <product item="<basic-ball>"/>
    <product item="<potion>" overridePrice="120"/>
  </step>
  <step id="3" type="sell" shop="1"/>
</bot>

<!-- A trainer battle -->
<bot id="4" team="<faction>">
  <name>Wanderer</name>
  <step id="1" type="fight">
    <start><![CDATA[You look strong — let's battle!]]></start>
    <win><![CDATA[Well fought.]]></win>
    <monster id="<speciesA>" level="31"/>
    <monster id="<speciesB>" level="33"/>
    <gain cash="1488"/>
  </step>
</bot>
```

Conventions the generator should follow:
- A **sign** = a bot with a single `text` step.
- A **healing center** interior = one attendant bot whose flow reaches a `heal` step.
- A **shop** interior = a vendor with a `shop` step (and usually a `sell` step).
- **Trainers** placed on roads/caves use `fight` steps with a small team and `cash` prize.
- Bots may live in a shared `bots.xml` / `<name>-bots.xml` wrapped in `<bots>…</bots>`,
  typically with higher id numbers to avoid clashing with per‑map ids; the `.tmx` bot
  object then references them via the `file` property instead of `id`.

---

## 6. Zone presentation (`zone/<zone>.xml`)

A zone groups maps that share presentation (chiefly music by map type). Each map's
`zone="…"` points here.

```xml
<zone>
  <name>Zone Display Name</name>
  <name lang="fr">…</name>
  <music type="indoor" backgroundsound="music/<track>.opus"/>
  <music type="city"   backgroundsound="music/<track>.opus"/>
  <!-- one <music> per map type the zone uses -->
</zone>
```

- Emit a `<music>` for each `type` (`city`/`outdoor`/`cave`/`indoor`) used by maps in the
  zone, so the engine always has a track. A map's own `backgroundsound` overrides it.
- Reference audio under `music/` by relative path. The generator should ship (or point at)
  real `.opus` files — do not reuse copyrighted tracks; use original/royalty‑free audio.

---

## 7. Quests (optional)

Each quest is a numbered folder under `quests/` with two files.

`definition.xml` — structure, steps, requirements, rewards:

```xml
<quest map="<region>/<location>/<map>" bot="<botId>" repeatable="no">
  <name>Quest Title</name>
  <name lang="fr">…</name>
  <step id="1">
    <text><![CDATA[Do the thing.]]></text>
    <item id="<itemId>" quantity="1"/>   <!-- item required to advance this step -->
  </step>
  <rewards show="true">
    <item id="<itemId>"/>
  </rewards>
</quest>
```

`text.xml` — the giver's conditional lines, keyed by state:

```xml
<text>
  <client_logic id="1">
    <condition queststep="1"/>
    <condition haverequirements="true"/>
    <text><![CDATA[Do you have it?<br /><a href="next_quest_step;52">Yes</a>]]></text>
  </client_logic>
  <client_logic id="52">
    <text><![CDATA[Thank you!]]></text>
  </client_logic>
</text>
```

- `map` + `bot` bind the quest to a specific NPC on a specific map.
- `<client_logic>` blocks are selected by their `<condition>`s (current `queststep`, whether
  requirements are met, …); `href="next_quest_step;<id>"` advances the quest and jumps to a
  result block.
- Use plain incrementing folder numbers (`1`, `2`, …).

---

## 8. Variant editions (`sub/<variant>/`)

A variant is an alternate edition that **only changes which creatures spawn** — different
feel, no duplicated geometry.

```
sub/<variant>/
  informations.xml                 # variant identity (own name, color, initial)
  <region>/<location>/<map>.xml    # SPARSE overrides — encounter tables only
```

Override file shape (note: **no** `type`/`zone`/names; **numeric** creature ids):

```xml
<map>
  <grass>
    <monster id="16" luck="55" minLevel="2" maxLevel="4"/>
    <monster id="19" luck="5"  minLevel="2" maxLevel="4"/>
  </grass>
  <grassNight> … </grassNight>
</map>
```

Rules:
- Mirror the **same relative path** under `sub/<variant>/` that the base map has under
  `<root>/`. The engine merges the override onto the base map.
- Include only the groups you want to replace; the rest fall back to the base map.
- Each variant has its own `informations.xml` with a distinct name/color/initial (§9).

---

## 9. Root configuration

`informations.xml` — world identity:

```xml
<?xml version='1.0'?>
<informations color="#RRGGBB">
  <name>World Name</name>
  <name lang="fr">…</name>
  <initial>X</initial>   <!-- single identifying letter -->
</informations>
```

`start.xml` — spawn profiles for new players:

```xml
<profile>
  <start id="normal">
    <map file="<region>/<location>/<map>" x="26" y="27"/>
  </start>
  <start id="<other-profile>">
    <map file="<region>/<location>/<map>" x="13" y="19"/>
  </start>
</profile>
```

- `file` = start map path relative to `<root>` (no extension). `x`,`y` = **tile**
  coordinates of the spawn; the tile must be walkable (empty in `Collisions`).
- Provide at least one profile (`id="normal"`); extra profiles can start the player
  elsewhere (e.g. tied to a faction).

---

## 10. World‑graph connectivity rules

Build an explicit graph first, then emit geometry that realizes it.

1. **Nodes** = maps. **Edges** = a border‑pair between two overworld maps, or a door‑pair
   between an overworld map and an interior.
2. **Borders are reciprocal.** If A has a `border-right` to B, B must have a `border-left`
   back to A, aligned on the same rows so the scroll is continuous. Emit both sides
   together with matching offsets.
3. **Doors are reciprocal.** A door in A → interior I at tile `(x,y)` needs a return door in
   I → A at a walkable tile in front of A's door. Destination tiles must be walkable.
4. **Reachability.** From the `start.xml` spawn, every map must be reachable via
   borders + doors. Run a traversal after generation; repair or fail on any orphan.
5. **No dangling references.** Every border/door `map`, every bot `id`/`file`, every
   creature/item/quest id, every `zone`, and every `backgroundsound` must resolve.
6. **Spatial sanity.** Borders sit on the correct edge; connected maps have compatible
   extents along the shared edge; rescue points exist where the player should respawn.

---

## 11. Suggested generation pipeline

1. **Macro graph.** Choose regions; per region choose locations (towns, roads, caves) and
   how they connect. A spanning tree guarantees reachability; add a few extra edges for
   loops. Assign each location a `zone` and music.
2. **Abstract layout.** Place locations on a grid and size each map (tiles). This fixes
   which edge of each map carries which border and at what offset, and where roads meet
   towns and caves branch off roads.
3. **Overworld geometry, per map:**
   - Lay `Walkable`; carve `Dirt` paths; place `Water`; scatter `Grass`/`OnGrass`.
   - Add `Ledges*` to shape one‑way routing.
   - Place buildings; for each enterable one add a `door` and create its interior map.
   - Derive `Collisions` from all solids; route pass‑behind tops into `WalkBehind`.
   - Emit `border-*` objects on the right edges with planned neighbor paths.
   - Place `bot` and `object` (item) entities; allocate ids; set `skin`/`lookAt`.
4. **Interiors:** small `Walkable`+`Collisions` rooms with a return `door` and the service
   bot — `heal` (centers/labs), `shop`/`sell` (vendors), `warehouse` (storage), `learn`,
   `industry`, or plain `text` (signs/flavor). Trainers (`fight`) go on roads/caves.
5. **Companion `.xml` per map:** type/zone/music; original names & flavor; encounter tables
   matching the map type and distance from start; bot scripts matching the placed bot ids.
6. **Zone files** with per‑type music; **quests** and **variant overrides** if desired.
7. **Root config** (`informations.xml`, `start.xml`); pick a walkable spawn tile.
8. **Validate and repair** (§12).

---

## 12. Validation checklist (run before finishing)

- [ ] Every playable map has both `.tmx` and `.xml` with the same basename.
- [ ] Every layer's decoded cell count equals `width*height`; every non‑zero gid resolves
      to a declared tileset band.
- [ ] `firstgid` values are cumulative and consistent within each file.
- [ ] Each `border-*` has a reciprocal border on the neighbor, aligned on the shared edge
      and on the correct map edge.
- [ ] Each `door` target map exists; its destination `(x,y)` tile is walkable; interiors
      have a return door.
- [ ] Every `bot` object's `id`/`file` resolves to a `<bot>` definition; every `<step>`
      `href` (and `next_quest_step` target) points to a real step.
- [ ] Every `object` item id, encounter creature id, quest item id, `fight` species id, and
      `industry` id is valid.
- [ ] Every map's `zone` exists and provides music for that map's `type`; every
      `backgroundsound` file exists under `music/`.
- [ ] Encounter `luck` weights are positive and sum to roughly 100 per group.
- [ ] Variant override paths mirror real base‑map paths and use numeric creature ids.
- [ ] From the `start.xml` spawn, a walk over borders+doors reaches every map; the spawn
      tile is walkable.
- [ ] No file contains an absolute path; all references are relative.

---

## 13. Quick field reference

**Map `type`:** `city` · `outdoor` · `cave` · `indoor`
(cave may add `color` + `alpha` for darkness)

**TMX object types (group `Moving`):** `border-left` · `border-right` · `border-top` ·
`border-bottom` · `door` · `rescue`
**TMX object types (group `Object`):** `bot` · `object`

**Object properties:** `map` · `x` · `y` (warp dest, tiles) · `id` (int) · `file` ·
`item` (int) · `skin` · `lookAt` (`left`/`right`/`up`/`down`) · `visible`

**TMX layer names:** `Walkable` · `Grass`/`OnGrass` · `Dirt` · `Water` ·
`LedgesDown`/`LedgesLeft`/`LedgesRight` · `Collisions` · `WalkBehind` ·
`Back`/`Over`/`Random`

**Encounter groups:** `grass` · `grassNight` · `cave` · `caveNight` · `water` ·
`waterOldRod` · `waterGoodRod` · `waterSuperRod`
(`<monster id luck (level | minLevel/maxLevel)/>`, weights ≈100 per group)

**Bot step `type`:** `text` · `shop` · `sell` · `heal` · `warehouse` · `fight` · `quests` ·
`learn` · `industry`

**Text markup (inside `text`/`start`/`win` CDATA):** `<br />` · `<b>…</b>` ·
`<a href="<stepId>">…</a>` · `<a href="3;2">…</a>` (run step 3, continue at 2) ·
`<a href="next_quest_step;<id>">…</a>` (quests)

**Units:** tile = 16 px; layer cells & warp‑destination properties in **tiles**; object
`x/y/width/height` in **pixels**.

**Layer data:** `encoding="base64" compression="zstd"` of a row‑major little‑endian
uint32 gid array (`0` = empty; top 3 bits = flip flags). `csv` encoding is also accepted.
