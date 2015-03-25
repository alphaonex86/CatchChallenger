#ifndef GENERALTYPE_H
#define GENERALTYPE_H

typedef signed char qint8;         /* 8 bit signed */
typedef unsigned char quint8;      /* 8 bit unsigned */
typedef short qint16;              /* 16 bit signed */
typedef unsigned short quint16;    /* 16 bit unsigned */
typedef int qint32;                /* 32 bit signed */
typedef unsigned int quint32;      /* 32 bit unsigned */
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

#define CATCHCHALLENGER_TYPE_ITEM_LIST_SIZE quint8
#define CATCHCHALLENGER_TYPE_ITEM_LIST_SIZE_WAREHOUSE quint16
#define CATCHCHALLENGER_TYPE_ITEM quint16
#define CATCHCHALLENGER_TYPE_ITEM_QUANTITY quint32
#define CATCHCHALLENGER_TYPE_MONSTER_LIST_SIZE quint8
#define CATCHCHALLENGER_TYPE_MONSTER_LIST_SIZE_WAREHOUSE quint16
#define CATCHCHALLENGER_TYPE_MONSTER quint16
#define CATCHCHALLENGER_TYPE_SKILL quint16
#define CATCHCHALLENGER_TYPE_BUFF quint8
#define CATCHCHALLENGER_TYPE_QUEST quint8

#endif // GENERALTYPE_H
