#ifndef GENERALTYPE_H
#define GENERALTYPE_H

#include <stdint.h>

#if __cplusplus >= 202302L && __has_include(<flat_map>)
#include <flat_map>
#include <flat_set>
template<typename K, typename V>
using catchchallenger_datapack_map = std::flat_map<K, V>;
template<typename K>
using catchchallenger_datapack_set = std::flat_set<K>;
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
