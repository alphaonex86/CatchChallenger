# tuxemon2catchchallenger — rules

Combine with the root CLAUDE.md.  Converts Tuxemon (`old/Tuxemon/mods/tuxemon`)
into a CatchChallenger datapack (`Tuxemon-datapack/`).  One binary, build dir
out of tree (`/tmp/tux2cc-build`).

* **Input is READ-ONLY.**  Never write into `old/Tuxemon/`.  Output dir is
  created/overwritten; nothing else is touched.
* **System libs only.**  Qt6 Core+Gui + system `yaml-cpp`.  Never vendor a YAML
  parser, never add a Qt module beyond Core/Gui, never auto-install.
* **The loader is the spec.**  Output must parse through `general/base/fight/FightLoader*`
  and `general/base/DatapackGeneralLoader*`.  Schema facts that bit us:
  `xp_max` is accepted (aliased to `xp_for_max_level`); `ratio_gender` keeps the
  `%`; skill **id 0** must exist with one `<life success="100%" … applyOn="aloneEnemy">`
  negative-quantity level; a monster learns each skill at most once **per
  attack_level** (the engine keys on (skill, attack_level), NOT monster level —
  Tuxemon relearns the same move at two levels, so dedup by skill id); only one
  `type="level"` evolution per monster.
* **Verify by loading, not by eyeballing.**  After any change, regenerate then
  load through the real parsers and require **0 stderr warnings**.  The throwaway
  harness (link `catchchallenger_general`, `-DCATCHCHALLENGER_SOLO`, call
  loadTypes/loadMonsterBuff/loadMonsterSkill/loadItems/loadMonster) lives at
  `/tmp/tuxverify`; it must set `CommonSettingsServer::commonSettingsServer
  .rates_xp/_xp_pow` (non-zero) before `loadMonster` or the monster loader
  aborts (a server runtime setting, not a datapack field).
* **Dedup defensively.**  Tuxemon ships duplicate slugs (225 item files / 219
  slugs) — every loader drops repeat slugs/ids, else the engine rejects the
  collision (`id number already set`).
* **Stats are a heuristic** (shape 0..10 → level-100 stats × stage).  If asked to
  rebalance, edit `DatapackWriter::computeStats` only; keep it documented.
* **Maps** (`MapConverter`): keep Tuxemon gids verbatim (fidelity is checked) —
  only re-home each cell onto `Walkable`/`Collisions`/`WalkBehind` and re-encode
  base64+zlib.  Output layout is gen2-style `map/main/tuxemon/<region>/<location>/<base>`
  (region = `scenario` map property else nearest via warp graph else `other`;
  location = nearest outdoor map via warp graph, so a town + its interiors share
  one folder; file base = slug minus the region+location token prefixes, dashed,
  per-folder dedup — ALL derived from source data, nothing hardcoded, so new
  upstream maps/scenarios lay out automatically); warp `map` props are
  folder-relative; sidecar `<name>` = the Tuxemon catalogue name.  NOTE: short
  dashed names exposed an ENGINE bug ('-'<'.', so the with-`.tmx` sort diverged
  from the stripped sort and `loadAllMapsAndLink` linked teleports to wrong map
  ids) — fixed in `Map_loader` (`compareMapPathStripTmx`); the converter installs
  `map/invisible.png|.tsx` (embedded, byte-identical to the official one;
  `InvisibleAsset.hpp` = `xxd -i invisible.png`) and gives warp/bot objects a
  `gid` into it (+2 teleport, +0 bot) so they are visible in Tiled.  Engine collision: a cell with a `Walkable` tile AND no
  `Collisions` tile is passable; a `Collisions` tile blocks.  Handle BOTH
  external `<tileset source>` and INLINE `<tileset><image></tileset>` (37 maps
  use inline — materialise them as `.tsx`).  Warp object Y must be
  `(tile_y+1)*16` (loader does `y/16 - 1`).  Verify maps by loading EVERY one
  through `Map_loader::tryLoadMap` (extend `/tmp/tuxverify`); the loader aborts
  unless `CommonDatapack`'s item+monster name tables are non-empty
  (`set_tempNameToItemId/MonsterId`) — populate them first.  Require
  263/263 load, 0 failures.
* **Teleports are PER-TILE:** an event rect spanning several tiles (2-wide
  stairs, whole-edge transitions — ~118 events) emits one warp per covered
  tile; additionally a 1x1 warp EXTENDS onto the neighbour tile when it
  continues the same multi-tile graphic (same layer, consecutive gid, bounded
  run of exactly two, overlay above a non-empty lower layer, walkable, not
  already a warp) — the author often marks only one tile of a 2-tile stair.
* **Events live in TWO encodings:** TMX object properties AND a sidecar
  `<map>.yaml` (`events:` -> x/y/width/height in TILES + actions/conditions/
  behav lists — 62 maps, e.g. eclipse banks).  Both feed the SAME interpreter
  (`processEventProps`/`loadYamlEvents`); the layout pre-pass reads yaml warps
  too (location grouping follows the warp graph).  Monster battle sheets are
  COMBINED (tuxemon/db.py MonsterSpritesModel): front (0,0,64,64), back
  (64,0,64,64) — a REAL pose, not a mirror — menu1 (0,64,24,24) -> small.png
  (32x32); rects per-monster overridable via the raw `sprites:` node.  NPC walk
  sheets: 3 cols (walk1, idle, walk2) x 4 rows (front,left,right,back) of
  16x32; CC trainer.png rows are up,right,down,left with idle in the middle
  column (tiles 1/4/7/10).  Combat sheets: LEFT=back, RIGHT=front.
* **Dynamic data preservation.**  The full raw Tuxemon YAML per entity is dumped
  generically under an engine-ignored `<tuxemon>` element (`writeTuxemon`/`dumpYaml`
  in DatapackWriter) — NEVER hand-pick fields; new Tuxemon attributes must carry
  automatically.  The structs keep a `YAML::Node raw` for this.
* **Full datapack** (WorldWriter + MapConverter): skins (SkinGen, from Tuxemon
  sprites), player/start.xml (startable profile — starter monster + capture item
  + reputation must all resolve or loadProfileList rejects it), reputation/event,
  map/layers.xml (grass/cave/water), music (ffmpeg ogg→opus), region info/start,
  empty plants/crafting.  Maps also emit wild encounters (`random_encounter`
  event region → a `Grass` layer + `<grass>`/`<grassNight>`, luck normalised to
  sum 100) and bots (`create_npc`/`add_monster`/`set_economy`/`open_shop`/
  `translated_dialog` events → `bot` objects + `<bot>` text/fight/shop/sell steps).
* **Datapack filenames MUST be `[a-z0-9._/-]` only** (no uppercase/space/punct).
  `FacilityLibGeneral::getSuffixAndValidatePathFromFS` (used by the gateway +
  ClientHeavyLoad SYNC paths) silently DROPS other names, so a tileset called
  `My_Sheet (BW).png` loads locally but never reaches a networked client →
  broken map.  `MapConverter::uniqueTilesetFile`/`sanitizeFsBase` rename every
  copied/materialised tileset (.tsx + image) and rewrite the references.  Tuxemon
  map slugs are already valid; tileset names are NOT.
* **Verify with the FULL singleton + buffers + map-link.**  Also run
  `Map_loader::loadAllMapsAndLink(flat, mapdir+"/", semi, mapPathToId, NULL, &bufs)`
  (real server init: border/warp linking + map-id assignment) — it flags
  out-of-range teleport destinations and duplicate teleport triggers; the
  converter clamps warp dests to the target map size (pre-pass `mapDims_`) and
  dedups warps by source tile.  Also run the CLIENT loader (`/tmp/tuxclient`,
  subclass `DatapackClientLoader`) — it wants `map/visualcategory.xml`, a
  reputation `<name>`, and the `map/fight/<terrain>/` backgrounds layers.xml
  references.  `/tmp/tuxverify` calls
  `CommonDatapack::commonDatapack.parseDatapack(dp + "/")` (TRAILING SLASH — the
  engine concatenates `dp+"skin/"`), then `tryLoadMap(..., &buf)` with a
  `MapLoadBuffers` (else text/sell bot steps emit benign "step not found"
  warnings; with a buffer they go to `unknownBotStepBuffer`).  Require 0 warnings,
  263/263 maps, profiles=1, encounterCells>0.
* Project style here too: no lambdas (use named compare fns), no `auto`, prefer
  `while`, init members in the constructor.
