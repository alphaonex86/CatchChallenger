# tuxemon2catchchallenger — rules

Combine with the root CLAUDE.md.  Converts Tuxemon (`old/Tuxemon/mods/tuxemon`)
into a CatchChallenger datapack (`Tuxemon-datapack/`).  One binary, build dir
out of tree (`/tmp/tux2cc-build`).

* **Input is READ-ONLY.**  Never write into `old/Tuxemon/`.  Output dir is
  created/overwritten; nothing else is touched.
* **System libs only.**  Qt6 Core+Gui + system `yaml-cpp`.  Never vendor a YAML
  parser, never add a Qt module beyond Core/Gui, never auto-install.
* **The loader is the spec.**  Output must parse through `general/fight/FightLoader*`
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
  base64+zlib.  Engine collision: a cell with a `Walkable` tile AND no
  `Collisions` tile is passable; a `Collisions` tile blocks.  Handle BOTH
  external `<tileset source>` and INLINE `<tileset><image></tileset>` (37 maps
  use inline — materialise them as `.tsx`).  Warp object Y must be
  `(tile_y+1)*16` (loader does `y/16 - 1`).  Verify maps by loading EVERY one
  through `Map_loader::tryLoadMap` (extend `/tmp/tuxverify`); the loader aborts
  unless `CommonDatapack`'s item+monster name tables are non-empty
  (`set_tempNameToItemId/MonsterId`) — populate them first.  Require
  263/263 load, 0 failures.
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
* **Verify with the FULL singleton + buffers.**  `/tmp/tuxverify` calls
  `CommonDatapack::commonDatapack.parseDatapack(dp + "/")` (TRAILING SLASH — the
  engine concatenates `dp+"skin/"`), then `tryLoadMap(..., &buf)` with a
  `MapLoadBuffers` (else text/sell bot steps emit benign "step not found"
  warnings; with a buffer they go to `unknownBotStepBuffer`).  Require 0 warnings,
  263/263 maps, profiles=1, encounterCells>0.
* Project style here too: no lambdas (use named compare fns), no `auto`, prefer
  `while`, init members in the constructor.
