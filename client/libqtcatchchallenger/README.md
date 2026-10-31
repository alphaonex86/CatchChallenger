# libqtcatchchallenger — the shared Qt layer of every client

Static lib `catchchallenger_qt_lib` (`libqt.cmake`), on top of
`client/libcatchchallenger/` (Qt-free protocol + datapack, see its README),
`general/` and the vendored `libtiled/` (+ `libogg`/`libopus`/`libopusfile`
when audio is on).

## The rule: what goes here, what does not

**Here: everything that behaves the same whatever the screen looks like.**
Qt transport, datapack download, datapack images decoded in RAM, audio, settings,
language, CLI options, automation channel, map scene + movement + pathfinding,
and the client-side game rules (quest/shop/factory/fight decisions).

**Not here: how a client draws its UI.** Each front-end picks its own idiom and
does it as it likes:

* `qtcpu800x600/` — Qt widgets + `.ui`, fixed 800x600, CPU painting.
* `qtopengl/` — responsive (phones), KISS: everything painted in draw calls.
* any future one — pixel art at a fixed base resolution, a complex widget UI...

Whatever the idiom, every datapack datum and every game decision must already be
**one function call away** in this lib. Test before writing code in a client:
*"would a third client have to rewrite this to get the same behaviour?"* — yes →
it goes here, the client only displays the result.

## What is here

| Area | Files |
|---|---|
| Transport (TCP, SSL, WebSocket, solo fake socket) | `ConnectedSocket`, `Api_protocol_Qt` (Qt signals for every `Api_protocol` notification) |
| Multiplayer client + datapack download (internal protocol, HTTP mirror `tar.zst`) | `Api_client_real*.cpp`, `QZstdDecodeThread`, `QtDatapackChecksum` |
| Solo client (in-process server, `CATCHCHALLENGER_SOLO`) | `Api_client_virtual`, `SoloDatabaseInit` |
| Datapack in RAM (texts + decoded images + tilesets) | `QtDatapackClientLoader`, `QtDatapackClientLoaderThread` |
| Audio (opus decode, ambiance loop, volume; dropped by `CATCHCHALLENGER_NOAUDIO`) | `Audio`, `QInfiniteBuffer`, `ClientVariableAudio.hpp`, `audio.cmake` |
| Map scene, player/bot/door/plant on map, fight trigger, pathfinding, map scale | `maprender/` (`MapVisualiser*` = `QGraphicsView`, `MapControllerMP`, `PathFinding`, `MonsterSheet`) |
| CLI options, automation channel | `CliClientOptions`, `LocalListener` (+ `MapControllerMP::remoteActionExecute()`) |
| Settings, language | `Settings` (`QSettings`), `Language` (`QTranslator`) |
| Server discovery, news, updater, paid key | `LanBroadcastWatcher`, `FeedNews`, `InternetUpdater`, `Ultimate` |
| Thread-safe log lines | `CerrLineAtomic` |
| Misc | `ExtraSocket` (local socket path), `PlatformMacro.hpp`, `DisplayStructures.hpp` |

## Datapack data: one call away

All through the singleton `QtDatapackClientLoader::datapackLoader`, after its
`datapackParsed()` / `datapackParsedMainSub()` signals. All images are decoded
**once, at load, in RAM** (frame-perfect, no load time during play; `QPixmap` only
exists on the GUI thread, so it is all converted up front).

| Need | Call |
|---|---|
| Monster name/description/kind/habitat | `get_monsterExtra(id)` |
| Monster front/back/thumb image | `getMonsterExtra(id).front/.back/.thumb` (`QPixmap`) |
| Monster overworld sheet frame | `maprender/MonsterSheet` (both sheet layouts) |
| Item name/description, image | `get_itemExtra(id)`, `getItemExtra(id).image` |
| Skill, buff texts; buff icon | `get_monsterSkillExtra(id)`, `get_monsterBuffExtra(id)`, `getMonsterBuffExtra(id).icon` |
| Plant tileset; seed → plant | `getPlantExtra(id).tileset`, `get_itemToPlant(item)` |
| Type name/colour | `get_typeExtra(id)` |
| Quest texts; quests started by a bot | `get_questExtra(id)`, `get_botToQuestStartForBot(map,bot)` |
| Reputation names/levels | `get_reputationExtra(name)` |
| Zone name + ambiance; general ambiance | `get_zoneExtra(zone)`, `get_audioAmbiance(type)` |
| Skins, player front image path | `get_skins()`, `getFrontSkinPath(skinId)` |
| Maps, bot at a tile, new-game profiles | `get_maps()`, `get_mapToId(path)`, `getBot(map,x,y)`, `get_profileText(i)` |
| Game rules (items, monsters, recipes, shops...) | `CatchChallenger::CommonDatapack::commonDatapack`, `CommonDatapackServerSpec::commonDatapackServerSpec` |
| Player state (cash, inventory, monsters, quests) | `Api_protocol::get_player_informations_ro()` |

## What every client must implement

Feature parity, not visual parity: same information and same actions in every
client, each in its own idiom.

* **Server selection CLI** — exactly one of `--server` / `--host`+`--port` /
  `--url`, reject a combination (`client/CLAUDE.md`); stored in `CliClientOptions`.
* **Test/automation flags** — `--autosolo[=DX,DY]`, `--autologin`, `--character`,
  `--closewhenonmap[after]`, `--dropsenddataafteronmap`, `--take-screenshot`,
  `--main-datapack-code`, `--remote-control`, `--test-*` (`client/dev.md`).
* **Automation channel** — create `LocalListener` when `CliClientOptions::remoteControl`,
  wire it to `MapControllerMP::remoteAction()`; never add test code in the client.
* **Map scale** — always `MapControllerMP::scaleFactor()`; the server mirrors it.
* **Bundled UI font** — loaded from resources (no system font fallback).
* **Local datapack check** — `DatapackClientLoader::datapackProblem()` before
  handing a local datapack to the engine; show the reason, never core-dump.
* **Datapack cache coherent** — one cache dir per server, hash kept in sync
  (`client/CLAUDE.md`), else every connect re-downloads.
* **Errors visible** — every error reaches the user (simple message) and the
  console (full detail); no silent failure, no abort.
* **Game features** — login + account creation, server list (+ LAN servers,
  news), character create/select/delete, solo, map move (keys + click/pathfinding),
  bot dialog + quests, shop, factory, warehouse, crafting, plants, inventory +
  item use, wild/bot/player fight, catch, evolution, learn skill, trade, clan,
  city capture, chat, options (volume, language).

## Shared (done 2026-10-30, tested by `test/testingclientlogic.py`)

| Logic | Now in |
|---|---|
| CLI flag parsing + conflict check | `CliClientOptions::parse()` (both `main.cpp`, before `QApplication`) |
| Chat HTML, entities, smilies; bot dialog HTML | `ChatParsing` (`dimSystemText` = client look choice), `ChatParsing::botTextToDisplay()` |
| `timeToString`, `rectTouch`, `reputationRequirementsToText` | `FacilityLibClient` (qtopengl superset kept) |
| Language pick | `Language::pickLanguage()` |
| Chat input checks + anti-flood | `ChatInputGuard` |
| Map ambiance choice (map → zone → general, main then base path) | `DatapackClientLoader::ambianceFile()` (libcatchchallenger) |
| `haveStartQuestRequirement`, `haveNextStepQuestRequirements`, `haveBeatBot` | `Api_protocol` only (matches the server) |

## To move here

Logic still written twice — once in each client. A third client would have to
write it a third time.

| Logic | qtcpu800x600 | qtopengl | State |
|---|---|---|---|
| Slow-server detection (`detectSlowDown`) | `base/interface/BaseWindow.cpp` | `foreground/OverMapLogic.cpp` | near identical |
| Quest rules (`botHaveQuest`, `tryValidateQuestStep`) | `base/interface/BaseWindowBot.cpp` | `foreground/OverMapLogicBot.cpp` | diverged |
| Bot step dispatch (`goToBotStep`: text/shop/heal/learn/quest/clan/fight/zone...) | `base/interface/BaseWindowBot.cpp` | `foreground/OverMapLogicBot.cpp` | diverged (322 vs 287 lines) |
| Factory price/production (`updateFactoryStatProduction`, `factoryToProductItem`) | `base/interface/BaseWindowFactory.cpp` | `foreground/OverMapLogicFactory.cpp`, `above/Factory.cpp` | near identical |
| Reputation reward text (`appendReputationRewards`) | `base/interface/BaseWindow.cpp` | `foreground/OverMapLogic.cpp` | near identical |
| Evolution check after fight (`checkEvolution`) | `fight/interface/BaseWindowFight.cpp` | `foreground/Battle.cpp` | diverged (58 vs 123 lines) |
| Server list load/save (`loadXmlConnexionInfoList`, `loadConfigConnexionInfoList`, `saveConnexionInfoList`) | `ultimate/mainwindow.cpp` | `foreground/Multi.cpp` | same format, two codes |
| Datapack cache dir per server (`serverToDatapachPath`) | `ultimate/mainwindow.cpp` | `ConnexionManager.cpp` | diverged — risk on cache/hash coherence |
| New-game profiles (`getProfileCount`, `getProfileIndex`) | `base/interface/NewProfile.cpp` | `above/AddCharacter.cpp` | same logic |

**Why it matters, concrete case:** the three client copies of
`haveStartQuestRequirement()` had drifted from the server: a quest with a
non-inverse prerequisite never showed as startable, and `haveBeatBot()` always
said yes on qtopengl and the bots. Nobody saw it because no test covered it.

## Build

```cmake
include(${CC_REPO_ROOT}/general/general.cmake)
include(${CC_REPO_ROOT}/client/libcatchchallenger/lib.cmake)
include(${CC_REPO_ROOT}/client/libqtcatchchallenger/libqt.cmake)
target_link_libraries(<client> PRIVATE catchchallenger_qt_lib)
```

Qt6 modules: Core Gui Network Widgets OpenGL OpenGLWidgets Xml (+ WebSockets if
present, + Multimedia unless `CATCHCHALLENGER_NOAUDIO`, + Sql with
`CATCHCHALLENGER_SOLO`). No QML/QtQuick.
