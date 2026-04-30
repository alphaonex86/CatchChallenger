#ifndef CATCHCHALLENGER_GENERALVARIABLE_H
#define CATCHCHALLENGER_GENERALVARIABLE_H

#include "CatchChallenger_Hash.hpp"

#ifndef CATCHCHALLENGER_RELEASE
#ifndef CATCHCHALLENGER_HARDENED
#define CATCHCHALLENGER_HARDENED
#endif
#endif

/*
#define PROTOCOLPARSINGDEBUG
#define PROTOCOLPARSINGINPUTOUTPUTDEBUG
#define PROTOCOLPARSINGINPUTDEBUG
//*/

//#define DEBUG_PROTOCOLPARSING_RAW_NETWORK
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
#define CATCHCHALLENGER_DEBUG_FIGHT
//*/

// 16-byte random token used both for the client auth handshake and for
// the per-character connect-to-game-server hop. Not a blake3 digest
// (blake3 would be 32 bytes); this is a plain random nonce, 16 bytes
// is enough to cover ~300 years of brute-force at 2 billion connect/sec.
// Legacy aliases (CATCHCHALLENGER_TOKENSIZE, TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT)
// were merged into this single name.
#define CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER 16
#define CATCHCHALLENGER_MONSTER_LEVEL_MAX 100
#define CATCHCHALLENGER_MONSTER_WILD_SKILL_NUMBER 5
//32Bits for the monster, 8Bits for the gender, 8Bits for level, 8Bits for the next step
#define CATCHCHALLENGER_MIN_RANDOM_TO_FIGHT 7
//if need more than this size and receive less, bug with the preivous message, reset
#define CATCHCHALLENGER_MIN_PACKET_SIZE 128
#ifdef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
#define CATCHCHALLENGER_MAX_PACKET_SIZE 256*1024
#define CATCHCHALLENGER_MAX_FILE_SIZE 16*1024
#define CATCHCHALLENGER_MAX_UNCOMPRESSED_FILE_SIZE 256*1024*1024
#else
#define CATCHCHALLENGER_MAX_PACKET_SIZE 1024*1024
#define CATCHCHALLENGER_MAX_FILE_SIZE 8*1024*1024
#define CATCHCHALLENGER_MAX_UNCOMPRESSED_FILE_SIZE 256*1024*1024
#endif
//here to ProtocolParsingGeneral
#define TOKEN_SIZE_FOR_MASTERAUTH 32
// TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT removed: merged into
// CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER (same 16-byte size,
// same client-auth handshake semantics).
#define CATCHCHALLENGER_MAXPROTOCOLQUERY 16

#define RANDOM_FLOAT_PART_DIVIDER 10000

#define CATCHCHALLENGER_EXTENSION_ALLOWED "tmx;xml;tsx;js;png;jpg;gif;ogg;opus"

#define CATCHCHALLENGER_CHECK_MAINDATAPACKCODE "^[a-z0-9]+$" //can't be "-" because the separator is used for http download
#define CATCHCHALLENGER_CHECK_SUBDATAPACKCODE "^[a-z0-9]+$" //can't be "-" because the separator is used for http download
#define DATAPACK_FILE_REGEX "^[0-9a-z_./-]*[0-9a-z]\\.[a-z]{2,4}$" //Not work: "^[-0-9/a-z\\._]*[0-9a-z]\\.[a-z]{2,4}$", Partial work: "^[-0-9a-z\\._/]*[0-9a-z]\\.[a-z]{2,4}$", Work: "^[0-9a-z/.-]*[0-9a-z]\\.[a-z]{2,4}$"
#define DATAPACK_BASE_PATH_MAPBASE "map/"
#define DATAPACK_BASE_PATH_MAPMAIN "map/main/"
#define DATAPACK_BASE_PATH_MAPSUB1 "map/main/"
#define DATAPACK_BASE_PATH_MAPSUB2 "/sub/"
#define DATAPACK_BASE_PATH_ZONE1 "map/main/"
#define DATAPACK_BASE_PATH_ZONE2 "/zone/"
#define DATAPACK_BASE_PATH_SKIN "skin/fighter/"
#define DATAPACK_BASE_PATH_SKINBASE "skin/"
#define DATAPACK_BASE_PATH_ANIMATION "animation/"
#define DATAPACK_BASE_PATH_ITEM "items/"
#define DATAPACK_BASE_PATH_INDUSTRIESBASE "industries/"
#define DATAPACK_BASE_PATH_INDUSTRIESSPEC1 "map/main/"
#define DATAPACK_BASE_PATH_INDUSTRIESSPEC2 "/industries/"
#define DATAPACK_BASE_PATH_PLANTS "plants/"
#define DATAPACK_BASE_PATH_MONSTERS "monsters/"
#define DATAPACK_BASE_PATH_BUFF "monsters/buff/"
#define DATAPACK_BASE_PATH_BUFFICON "monsters/buff/"
#define DATAPACK_BASE_PATH_SKILL "monsters/skill/"
#define DATAPACK_BASE_PATH_SKILLANIMATION "monsters/skill-animation/"
#define DATAPACK_BASE_PATH_FIGHT1 "map/main/"
#define DATAPACK_BASE_PATH_FIGHT2 "/fight/"
#define DATAPACK_BASE_PATH_PLAYERBASE "player/"
#define DATAPACK_BASE_PATH_PLAYERSPEC "map/main/"
#define DATAPACK_BASE_PATH_QUESTS1 "map/main/"
#define DATAPACK_BASE_PATH_QUESTS2 "/quests/"
#define DATAPACK_BASE_PATH_CRAFTING "crafting/"
#define DATAPACK_BASE_PATH_SHOP1 "map/main/"
#define DATAPACK_BASE_PATH_SHOP2 "/shop/"

#if defined(__WIN32__)
    #include <winsock2.h>
    #include <sys/param.h>

    #	if BYTE_ORDER == LITTLE_ENDIAN

    #define htobe16(x) htons(x)
    #define htole16(x) (x)
    #define be16toh(x) ntohs(x)
    #define le16toh(x) (x)

    #define htobe32(x) htonl(x)
    #define htole32(x) (x)
    #define be32toh(x) ntohl(x)
    #define le32toh(x) (x)

    #define htobe64(x) htonll(x)
    #define htole64(x) (x)
    #define be64toh(x) ntohll(x)
    #define le64toh(x) (x)

    #elif BYTE_ORDER == BIG_ENDIAN

    /* that would be xbox 360 */
    #define htobe16(x) (x)
    #define htole16(x) __builtin_bswap16(x)
    #define be16toh(x) (x)
    #define le16toh(x) __builtin_bswap16(x)

    #define htobe32(x) (x)
    #define htole32(x) __builtin_bswap32(x)
    #define be32toh(x) (x)
    #define le32toh(x) __builtin_bswap32(x)

    #define htobe64(x) (x)
    #define htole64(x) __builtin_bswap64(x)
    #define be64toh(x) (x)
    #define le64toh(x) __builtin_bswap64(x)

    #else

    #error byte order not supported

    #endif
#else
    #if defined(__linux__) || defined(__CYGWIN__)
        #include <endian.h>
        #ifdef __BYTE_ORDER
        #if __BYTE_ORDER == __BIG_ENDIAN //if both are not defined it is TRUE!
            /* already set on debian 10 mips big endian
             * #define be16toh(a) a
            #define be32toh(a) a
            #define be64toh(a) a
            #define htobe16(a) a
            #define htobe32(a) a
            #define htobe64(a) a*/
        #elif __BYTE_ORDER == __LITTLE_ENDIAN
        #elif __BYTE_ORDER == __PDP_ENDIAN
        #else
            #error "Endian determination failed"
        #endif
        #else
        #error "Endian determination failed"
        #endif
    #else
        #if defined(__APPLE__)
            #include <machine/endian.h>
            #include <libkern/OSByteOrder.h>

            #define htobe16(x) OSSwapHostToBigInt16(x)
            #define htole16(x) OSSwapHostToLittleInt16(x)
            #define be16toh(x) OSSwapBigToHostInt16(x)
            #define le16toh(x) OSSwapLittleToHostInt16(x)

            #define htobe32(x) OSSwapHostToBigInt32(x)
            #define htole32(x) OSSwapHostToLittleInt32(x)
            #define be32toh(x) OSSwapBigToHostInt32(x)
            #define le32toh(x) OSSwapLittleToHostInt32(x)

            #define htobe64(x) OSSwapHostToBigInt64(x)
            #define htole64(x) OSSwapHostToLittleInt64(x)
            #define be64toh(x) OSSwapBigToHostInt64(x)
            #define le64toh(x) OSSwapLittleToHostInt64(x)
        #else
            #include <endian.h>
        #endif
    #endif
#endif

//#define CATCHCHALLENGER_GITCOMMIT "GITCOMMITXXXXXXXXXX"

//server cluster
#define CATCHCHALLENGER_SERVER_MINIDBLOCK 20
#define CATCHCHALLENGER_SERVER_MAXIDBLOCK 50
#define CATCHCHALLENGER_SERVER_MINCLANIDBLOCK 1
#define CATCHCHALLENGER_SERVER_MAXCLANIDBLOCK 5

#endif // GENERALVARIABLE_H
