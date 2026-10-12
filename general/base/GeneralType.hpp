#ifndef GENERALTYPE_H
#define GENERALTYPE_H

#include <stdint.h>

// Small-RAM profile (< 8 MB: ESP32, i486): the lower-RAM choice where RAM and
// speed trade off, the fastest one elsewhere (benchmark/benchmarkcontainers,
// measured on the target archs). -DCATCHCHALLENGER_LOWRAM forces it.
#if !defined(CATCHCHALLENGER_LOWRAM) && (defined(CC_TARGET_ESP32) || defined(__i486__))
#define CATCHCHALLENGER_LOWRAM
#endif

// The datapack's key -> value structures, reached at random for every player
// action: one sorted array on LOWRAM (~1/3 of the heap), a hash table elsewhere
// (the fastest lookups). Both serialize to the same bytes (FlatMap.hpp).
#ifdef CATCHCHALLENGER_LOWRAM
#include "FlatMap.hpp"
template<typename K, typename V>
using catchchallenger_datapack_map = CatchChallenger::FlatMap<K, V>;
template<typename K>
using catchchallenger_datapack_set = CatchChallenger::FlatSet<K>;
#else
#include <unordered_map>
#include <unordered_set>
template<typename K, typename V>
using catchchallenger_datapack_map = std::unordered_map<K, V>;
template<typename K>
using catchchallenger_datapack_set = std::unordered_set<K>;
#endif

#define CATCHCHALLENGER_TYPE_ITEM uint16_t
#define CATCHCHALLENGER_TYPE_ITEM_QUANTITY uint32_t
#define CATCHCHALLENGER_TYPE_MONSTER_LIST_SIZE uint8_t
#define CATCHCHALLENGER_TYPE_MONSTER_LIST_SIZE_WAREHOUSE uint16_t
#define CATCHCHALLENGER_TYPE_MONSTER uint16_t
#define CATCHCHALLENGER_TYPE_SKILL uint16_t
#define CATCHCHALLENGER_TYPE_BUFF uint8_t
#define CATCHCHALLENGER_TYPE_QUEST uint8_t
#define CATCHCHALLENGER_TYPE_MAPID uint16_t
#define CATCHCHALLENGER_TYPE_TELEPORTERID uint16_t
#define CATCHCHALLENGER_TYPE_BOTID uint8_t
#define CATCHCHALLENGER_TYPE_PLANT uint8_t
#define CATCHCHALLENGER_TYPE_REPUTATION uint8_t
#define CATCHCHALLENGER_TYPE_CRAFTINGRECIPE uint8_t
#define COORD_TYPE uint8_t
#define SIMPLIFIED_PLAYER_ID_FOR_MAP uint8_t
#define CLAN_ID_TYPE uint32_t
#define ZONE_TYPE uint8_t
#define ZONE_TYPE_MAX 255
#define SHOP_TYPE uint8_t
#define FACTORY_TYPE uint16_t
#define DATAPACK_FILE_NUMBER uint16_t

#define PLAYER_INDEX_FOR_CONNECTED uint16_t
#define PLAYER_INDEX_FOR_CONNECTED_MAX 65535

#endif // GENERALTYPE_H
