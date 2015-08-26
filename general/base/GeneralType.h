#ifndef GENERALTYPE_H
#define GENERALTYPE_H

typedef signed char qint8;         /* 8 bit signed */
typedef unsigned char uint8_t;      /* 8 bit unsigned */
typedef short int16_t;              /* 16 bit signed */
typedef unsigned short uint16_t;    /* 16 bit unsigned */
typedef int int32_t;                /* 32 bit signed */
typedef unsigned int uint32_t;      /* 32 bit unsigned */
#if defined(Q_OS_WIN) && !defined(Q_CC_GNU)
#  define Q_INT64_C(c) c ## i64    /* signed 64 bit constant */
#  define Q_UINT64_C(c) c ## ui64   /* unsigned 64 bit constant */
typedef __int64 qint64;            /* 64 bit signed */
typedef unsigned __int64 quint64;  /* 64 bit unsigned */
#else
#  define Q_INT64_C(c) static_cast<long long>(c ## LL)     /* signed 64 bit constant */
#  define Q_UINT64_C(c) static_cast<unsigned long long>(c ## ULL) /* unsigned 64 bit constant */
typedef long long qint64;           /* 64 bit signed */
typedef unsigned long long quint64; /* 64 bit unsigned */
#endif

typedef qint64 qlonglong;
typedef quint64 qulonglong;

#define CATCHCHALLENGER_TYPE_ITEM_LIST_SIZE uint8_t
#define CATCHCHALLENGER_TYPE_ITEM_LIST_SIZE_WAREHOUSE uint16_t
#define CATCHCHALLENGER_TYPE_ITEM uint16_t
#define CATCHCHALLENGER_TYPE_ITEM_QUANTITY uint32_t
#define CATCHCHALLENGER_TYPE_MONSTER_LIST_SIZE uint8_t
#define CATCHCHALLENGER_TYPE_MONSTER_LIST_SIZE_WAREHOUSE uint16_t
#define CATCHCHALLENGER_TYPE_MONSTER uint16_t
#define CATCHCHALLENGER_TYPE_SKILL uint16_t
#define CATCHCHALLENGER_TYPE_BUFF uint8_t
#define CATCHCHALLENGER_TYPE_QUEST uint8_t

#endif // GENERALTYPE_H
