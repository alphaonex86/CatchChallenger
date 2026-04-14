Dump of map array for test/road.tmx:
1988B HPSflat_simplified_map.dump
318B HPSflat_simplified_map.dump.zst
1262B RAWflat_simplified_map.dump
251B RAWflat_simplified_map.dump.gz
350B RAWflat_simplified_map.dump.lz4
260B RAWflat_simplified_map.dump.xz
284B RAWflat_simplified_map.dump.zst


● database/                                                                                                                                                                                  
    server             global counters (maxClanId, maxAccountId, maxCharacterId, city) - HPS format                                                                                        
    login/                                                                                                                                                                                   
      {SHA224_HEX}     login hash → password hash + account_id - HPS format                                                                                                                
    accounts/                                                                                                                                                                                
      {ACCOUNT_ID}     account_id → vector<CharacterEntry> - HPS format                                                                                                                    
    characters/
      {PSEUDO_HEX}     character name hex → full Client object - HPS format
    clans/             (reserved, unused)
    zone/
      {ZONE_NAME}      zone name → clan owner id (uint32_t) - raw binary

  - login/: Keyed by SHA224 hex of the login name. Each file stores the password hash and the numeric account_id it maps to.
  - accounts/: Keyed by numeric account_id. Each file stores the character list (id, pseudo, skin, playtime, etc.) for that account.
  - characters/: Keyed by hex-encoded character pseudo. Each file stores the full serialized character state.
  - zone/: Keyed by zone code name. Each file stores a single uint32_t clan owner id.
