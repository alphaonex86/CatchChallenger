#ifndef CATCHCHALLENGER_GENERALVARIABLE_H
#define CATCHCHALLENGER_GENERALVARIABLE_H

#define PROTOCOL_HEADER_VERSION 0x02
#define PROTOCOL_HEADER {0x9c,0xd6,0x49,0x8d,PROTOCOL_HEADER_VERSION}
#define CATCHCHALLENGER_VERSION "0.7.0.1"
#define CATCHCHALLENGER_SAVEGAME_VERSION "0.6"
#define CATCHCHALLENGER_SERVER_LIST_URL "http://catchchallenger.first-world.info/server_list.xml"
#define CATCHCHALLENGER_UPDATER_URL "http://catchchallenger.first-world.info/updater.txt"
#define CATCHCHALLENGER_RSS_URL "http://catchchallenger.first-world.info/rss_global.xml"

#define CATCHCHALLENGER_FIRSTLOGINPASSHASHSIZE 28

#define CATCHCHALLENGER_EXTRA_CHECK

/*
#define FAKESOCKETDEBUG
#define PROTOCOLPARSINGINPUTDEBUG
#define PROTOCOLPARSINGDEBUG
#define DEBUG_PROTOCOLPARSING_RAW_NETWORK
//*/

/*
#define DEBUG_MESSAGE_MOVEONTHEMAP
#define DEBUG_MESSAGE_MAP
#define DEBUG_MESSAGE_MAP_TP
#define DEBUG_MESSAGE_MAP_RAW
#define DEBUG_MESSAGE_MAP_BORDER
#define DEBUG_MESSAGE_MAP_OBJECT
#define DEBUG_MESSAGE_MONSTER_LOAD
#define DEBUG_CLIENT_PLANTS
#define DEBUG_MESSAGE_FIGHT_LOAD
//*/
#define CATCHCHALLENGER_DEBUG_FIGHT

#define CATCHCHALLENGER_TOKENSIZE 16//can't be more than 250 due to server conception, and useless, with 8bytes, at 2 billions connexion rate, it's 300years to crack
#define CATCHCHALLENGER_MONSTER_LEVEL_MAX 100
#define CATCHCHALLENGER_MONSTER_WILD_SKILL_NUMBER 5
//32Bits for the monster, 8Bits for the gender, 8Bits for level, 8Bits for the next step
#define CATCHCHALLENGER_MIN_RANDOM_TO_FIGHT 7
//if need more than this size and receive less, bug with the preivous message, reset
#define CATCHCHALLENGER_MIN_PACKET_SIZE 128
#define CATCHCHALLENGER_MAX_PACKET_SIZE 1024*1024

#define RANDOM_FLOAT_PART_DIVIDER 10000

#define CATCHCHALLENGER_EXTENSION_ALLOWED "tmx;xml;tsx;js;png;jpg;gif;ogg;qml;qm;ts;txt"

#define DATAPACK_FILE_REGEX "^[0-9/a-z\\.\\- _]*[0-9a-z]\\.[a-z]{2,4}$"
#define DATAPACK_BASE_PATH_MAP "map/"
#define DATAPACK_BASE_PATH_ZONE "map/zone/"
#define DATAPACK_BASE_PATH_SKIN "skin/fighter/"
#define DATAPACK_BASE_PATH_SKINBASE "skin/"
#define DATAPACK_BASE_PATH_ANIMATION "animation/"
#define DATAPACK_BASE_PATH_ITEM "items/"
#define DATAPACK_BASE_PATH_INDUSTRIES "industries/"
#define DATAPACK_BASE_PATH_PLANTS "plants/"
#define DATAPACK_BASE_PATH_MONSTERS "monsters/"
#define DATAPACK_BASE_PATH_BUFF "monsters/buff/"
#define DATAPACK_BASE_PATH_BUFFICON "monsters/buff/"
#define DATAPACK_BASE_PATH_SKILL "monsters/skill/"
#define DATAPACK_BASE_PATH_SKILLANIMATION "monsters/skill-animation/"
#define DATAPACK_BASE_PATH_FIGHT "fight/"
#define DATAPACK_BASE_PATH_PLAYER "player/"
#define DATAPACK_BASE_PATH_QUESTS "quests/"
#define DATAPACK_BASE_PATH_CRAFTING "crafting/"
#define DATAPACK_BASE_PATH_SHOP "shop/"

#if defined(__linux__) || defined(__CYGWIN__)
    #include <byteswap.h>
    #include <endian.h>
    #ifdef __BYTE_ORDER
      #if __BYTE_ORDER == __BIG_ENDIAN //if both are not defined it is TRUE!
        #define be16toh(a) a
        #define be32toh(a) a
        #define be64toh(a) a
        #define htobe16(a) a
        #define htobe32(a) a
        #define htobe64(a) a
      #elif __BYTE_ORDER == __LITTLE_ENDIAN
      #elif __BYTE_ORDER == __PDP_ENDIAN
      #else
        #error "Endian determination failed"
      #endif
    #else
      #error "Endian determination failed"
    #endif
#else
    #include "PortableEndian.h"
#endif

#endif // GENERALVARIABLE_H
