database/ is where is stored the database, the dynmaic data, when CATCHCHALLENGER_DB_FILE is enabled (that's exclude any other CATCHCHALLENGER_DB_* and that's exclude SQL support/CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
to compil server in this mode, you can use catchchallenger-server-filedb.pro

● database/                                                                                                                                                                                  
    server             global counters (maxClanId, maxAccountId, maxCharacterId, city) - HPS format                                                                                        
    dictionary         to maintain link between id and db id
    login/                                                                                                                                                                                   
      {SHA224_HEX}     login hash → password hash + account_id - HPS format                                                                                                                
    accounts/                                                                                                                                                                                
      {ACCOUNT_ID}     account_id → vector<CharacterEntry> - HPS format                                                                                                                    
    characters/
      {PSEUDO_HEX}     character name hex → Client object without server part - HPS format
    characters_server/
      {PSEUDO_HEX}     character name hex → Client object only server part - HPS format
    clans/             
      {Id}             clan id - HPS format - full Clan object with db id, never internal id
    zone/
      {ZONE_NAME}      zone name hex → clan owner id (uint32_t) - raw binary

  - login/: Keyed by SHA224 hex of the login name. Each file stores the password hash and the numeric account_id it maps to.
  - accounts/: Keyed by numeric account_id. Each file stores the character list (id, pseudo, skin, playtime, etc.) for that account.
  - characters/: Keyed by hex-encoded character pseudo. Each file stores the full serialized character state.
  - zone/: Keyed by zone code name. Each file stores a single uint32_t clan owner id.

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
Eg for map:
id db 1 is mapA
id db 2 is mapB (missing then not loaded into dictionary)
mapC is not into the list, then add id db 3 is now mapB

