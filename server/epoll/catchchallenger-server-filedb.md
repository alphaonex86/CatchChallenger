database/ is where is stored the database, the dynmaic data, when CATCHCHALLENGER_DB_FILE is enabled (that's exclude any other CATCHCHALLENGER_DB_* and that's exclude SQL support/CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
to compil server in this mode, you can use catchchallenger-server-filedb.pro

● database/
    server/server                  global counters (maxClanId, maxAccountId, maxCharacterId, maxCity) - RAW format (allow just seek()+write() at defined offset to override specific value without rewrite the whole file)
    server/dictionary_map          map path (without suffix) ↔ db id - RAW format [(8Bits string size + string content) * count util the end of the file, this allow just append new entry at the end of the file and imply the file will grow]
    common/dictionary_reputation   reputation name ↔ db id - RAW format [8Bits string size + string content) * count, this allow just append new entry at the end of the file and imply the file will grow]
    common/dictionary_skin         skin name ↔ db id - RAW format [8Bits string size + string content) * count, this allow just append new entry at the end of the file and imply the file will grow]
    common/dictionary_starter         skin name ↔ db id - RAW format [8Bits string size + string content) * count, this allow just append new entry at the end of the file and imply the file will grow]
    login/                                                                                                                                                                                   
      {SHA224_HEX}     login hash → password hash + account_id - HPS format                                                                                                                
    common/accounts/                                                                                                                                                                                
      {ACCOUNT_ID}     account_id → vector<CharacterEntry> - HPS format                                                                                                                    
    common/characters/
      {PSEUDO_HEX}     character name hex → Client object without server part - HPS format
                       (non-map-linked: public_informations, cash, monsters, warehouse_monsters,
                        recipes, encyclopedias, items (inventory), reputation, repel_step, clan,
                        fight engine state, position (map_entry/rescue/unvalidated_rescue/current,
                        all using map db_id))
    server/characters/
      {PSEUDO_HEX}     character name hex → Client object only server part - HPS format
                       (map-linked: mapData keyed by map db_id with items/plants/bots_beaten/
                        industriesStatus per map, plus quests)
    server/clans/             
      {Id}             clan id - HPS format - full Clan object without db id (included into the path file), character list db_id
    server/zone/
      {ZONE_NAME}      zone name hex → clan owner id (uint32_t) - raw binary

  - login/: Keyed by SHA224 hex of the login name. Each file stores the password hash and the numeric account_id it maps to.
  - common/accounts/: Keyed by numeric account_id. Each file stores the character list (id, pseudo, skin, playtime, etc.) for that account.
  - common/characters/: Keyed by hex-encoded character pseudo. Non-map-linked character state (inventory, monsters, position, fight engine, …).
  - server/characters/: Same key, map-linked state (mapData per map_db_id + quests). Written/read in lock-step with characters/.
  - server/zone/: Keyed by zone code name. Each file stores a single uint32_t clan owner id.

server/dictionary_map, in format (8Bits string size + string content) * count util the end of the file, if city is at pos 0, then when save character on city is save into 16Bits 0x00 on character_server file.
    if new map is added for later startup, then append to the list (it's just append 8Bits string size + string content to the file and imply the file will grow). EG newmap will be at pos 2 (because had 2 maps)
    if map is remove, then keep the entry into the dictionary because some character_server can have this index, allow don't edit character_server file if map is remove
    server will maintain internal convertion from db_id 16Bits to internal index 16Bits (it's std::vector<CATCHCHALLENGER_TYPE_MAPID> dictionary_map_database_to_internal where index is db_id and CATCHCHALLENGER_TYPE_MAPID it's internal id, 65535 when db_id is not found), EG if maps list into dictionary_map is city (id_db 0), road (id_db 1), newmap (id_db 2) annd road was remove then internal map list is city (internal id 0) and new map (internal id 0), the server will keep only 0 -> 0, 2 -> 1, and then when load from DB convert directly id_db to internal db if found, else log to console output an error
common/dictionary_reputation, in format (8Bits string size + string content) * count util the end of the file, if repEG is at pos 0, then when save character on repEG is save into 8Bits 0x00 on character_server file.
    if new map is added for later startup, then append to the list (it's just append 8Bits string size + string content to the file and imply the file will grow). EG newreputation will be at pos 2 (because had 2 reputations)
    if reputation is remove, then keep the entry into the dictionary because some character_server can have this index, allow don't edit character_server file if reputation is remove
    server will maintain internal convertion from db_id 8Bits to internal index 8Bits (it's std::vector<uint8_t> dictionary_reputation_database_to_internal where index is db_id and uint8_t it's internal id, 255 when db_id is not found), EG if reputations list into dictionary_reputation is repEG (id_db 0), road (id_db 1), newreputation (id_db 2) annd road was remove then internal reputation list is repEG (internal id 0) and new reputation (internal id 0), the server will keep only 0 -> 0, 2 -> 1, and then when load from DB convert directly id_db to internal db if found, else log to console output an error
common/dictionary_skin, in format (8Bits string size + string content) * count util the end of the file, if badboy is at pos 0, then when save character on badboy is save into 8Bits 0x00 on character_server file.
    if new skin is added for later startup, then append to the list (it's just append 8Bits string size + string content to the file and imply the file will grow). EG newskin will be at pos 2 (because had 2 skins)
    if skin is remove, then keep the entry into the dictionary because some character_server can have this index, allow don't edit character_server file if skin is remove
    server will maintain internal convertion from db_id 8Bits to internal index 8Bits (it's std::vector<uint8_t> dictionary_skin_database_to_internal where index is db_id and uint8_t it's internal id, 255 when db_id is not found), EG if skins list into dictionary_skin is badboy (id_db 0), road (id_db 1), newskin (id_db 2) annd road was remove then internal skin list is badboy (internal id 0) and new skin (internal id 0), the server will keep only 0 -> 0, 2 -> 1, and then when load from DB convert directly id_db to internal db if found, else log to console output an error
common/dictionary_starter, in format (8Bits string size + string content) * count util the end of the file, if empire is at pos 0, then when save character on empire is save into 8Bits 0x00 on character_server file.
    if new starter is added for later startup, then append to the list (it's just append 8Bits string size + string content to the file and imply the file will grow). EG newstarter will be at pos 2 (because had 2 starters)
    if starter is remove, then keep the entry into the dictionary because some character_server can have this index, allow don't edit character_server file if starter is remove
    server will maintain internal convertion from db_id 8Bits to internal index 8Bits (it's std::vector<uint8_t> dictionary_starter_database_to_internal where index is db_id and uint8_t it's internal id, 255 when db_id is not found), EG if starters list into dictionary_starter is empire (id_db 0), road (id_db 1), newstarter (id_db 2) annd road was remove then internal starter list is empire (internal id 0) and new starter (internal id 0), the server will keep only 0 -> 0, 2 -> 1, and then when load from DB convert directly id_db to internal db if found, else log to console output an error

Write lifecycle:
  - Character creation (ClientHeavyLoadLogin2.cpp): `Client::saveCharacterFiles()` writes both files with the fresh profile state.
  - Client disconnect (Client::disconnectClient, CharacterSelected branch): `saveCharacterFiles()` is called BEFORE any cleanup/setToDefault wipes fields. This is the only persistence point — in-memory mutations (moves, fights, quest progress, plant harvest, …) are not flushed incrementally; a crash loses everything since last connect.
  - Character selection (ClientHeavyLoadSelectCharCommon.cpp): reads characters/ via `hps::from_stream` then calls `loadCharacterServerFile()` to populate mapData + quests from characters_server/. A missing server-part file is non-fatal and leaves mapData/quests empty.

Server part is all related to map data:
 * map it self
 * zone
 * quests
Character server part is all related to map data specific to character (see class Player_private_and_public_informations_Map):
    std::set<std::pair<COORD_TYPE,COORD_TYPE>> items;
    std::map<std::pair<COORD_TYPE,COORD_TYPE>,PlayerPlant> plants;
    std::unordered_set<CATCHCHALLENGER_TYPE_BOTID> bots_beaten;
    std::vector<IndustryStatus> industriesStatus;

never save internal id because it can change, always use db id to have fixed id

Dictionary files (database/server/dictionary_map, database/common/dictionary_reputation, database/common/dictionary_skin, database/common/dictionary_starter):
  - At startup: loaded from file, resolution table db_id → internal_id built
  - If datapack changed (new map/reputation/skin/starter): new entries added with lastId++, file re-saved
  - At character login: db_id from character file resolved to internal_id via dictionary
  - At character save: internal_id resolved back to db_id via flat_map_list[].id_db (map) or internal_to_database (skin/starter)
  - On the protocol side, never send db_id, always internal id

Eg for map:
id db 1 is mapA
id db 2 is mapB (missing then not loaded into dictionary)
mapC is not into the list, then add id db 3 is now mapC

