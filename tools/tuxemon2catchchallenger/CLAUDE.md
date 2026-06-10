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
* Project style here too: no lambdas (use named compare fns), no `auto`, prefer
  `while`, init members in the constructor.
