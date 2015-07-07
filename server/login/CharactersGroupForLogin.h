#ifndef CHARACTERSGROUP_H
#define CHARACTERSGROUP_H

#include "../epoll/BaseClassSwitch.h"
#include "../epoll/db/EpollPostgresql.h"
#include "EpollClientLoginSlave.h"
#include <QString>
#include <QHash>
#include <vector>

namespace CatchChallenger {
class CharactersGroupForLogin : public BaseClassSwitch
{
public:
    explicit CharactersGroupForLogin(const char * const db,const char * const host,const char * const login,const char * const pass,const quint8 &considerDownAfterNumberOfTry,const quint8 &tryInterval);
    ~CharactersGroupForLogin();
    BaseClassSwitch::Type getType() const;

    struct InternalGameServer
    {
        QString host;
        quint16 port;
        quint8 indexOnFlatList;
    };

    void clearServerPair();
    void setServerUniqueKey(const quint8 &indexOnFlatList,const quint32 &serverUniqueKey,const char * const hostData,const quint8 &hostDataSize,const quint16 &port);
    bool containsServerUniqueKey(const quint32 &serverUniqueKey) const;
    InternalGameServer getServerInformation(const quint32 &serverUniqueKey) const;

    QList<EpollClientLoginSlave *> clientQueryForReadReturn;
    std::vector<quint32> maxCharacterId;
    std::vector<quint32> maxMonsterId;
    quint8 index;
    struct AddCharacterParam
    {
        void * client;
        quint8 query_id;
        quint8 profileIndex;
        QString pseudo;
        quint8 skinId;
    };
    QList<AddCharacterParam> addCharacterParamList;
    struct RemoveCharacterParam
    {
        void * client;
        quint8 query_id;
        quint32 characterId;
    };
    QList<RemoveCharacterParam> removeCharacterParamList;

    static QHash<QString,CharactersGroupForLogin *> hash;
    static QList<CharactersGroupForLogin *> list;

    void deleteCharacterNow(const quint32 &characterId);
    static void deleteCharacterNow_static(void *object);
    void deleteCharacterNow_object();
    void deleteCharacterNow_return(const quint32 &characterId);
    qint8 addCharacter(void * const client,const quint8 &query_id, const quint8 &profileIndex, const QString &pseudo, const quint8 &skinId);
    static void addCharacterStep1_static(void *object);
    void addCharacterStep1_object();
    void addCharacterStep1_return(EpollClientLoginSlave * const client,const quint8 &query_id,const quint8 &profileIndex,const QString &pseudo,const quint8 &skinId);
    static void addCharacterStep2_static(void *object);
    void addCharacterStep2_object();
    void addCharacterStep2_return(EpollClientLoginSlave * const client,const quint8 &query_id,const quint8 &profileIndex,const QString &pseudo,const quint8 &skinId);
    bool removeCharacter(void * const client,const quint8 &query_id, const quint32 &characterId);
    static void removeCharacter_static(void *object);
    void removeCharacter_object();
    void removeCharacter_return(EpollClientLoginSlave * const client,const quint8 &query_id,const quint32 &characterId);

    void character_list(EpollClientLoginSlave * const client,const quint32 &account_id);
    static void character_list_static(void *object);
    void character_list_object();
    void server_list(EpollClientLoginSlave * const client,const quint32 &account_id);
    static void server_list_static(void *object);
    void server_list_object();
    DatabaseBase::Type databaseType() const;
private:
    void load_character_max_id();
    static void load_character_max_id_static(void *object);
    void load_character_max_id_return();
    void load_monsters_max_id();
    static void load_monsters_max_id_static(void *object);
    void load_monsters_max_id_return();
    void load_clan_max_id();
    static void load_clan_max_id_static(void *object);
    void load_clan_max_id_return();

    void dbQueryWriteCommon(const char * const queryText);
private:
    EpollPostgresql *databaseBaseCommon;
    QHash<quint32,InternalGameServer> servers;
    QList<void * const> clientAddReturnList;
    QList<void * const> clientRemoveReturnList;
    QList<quint32> deleteCharacterNowCharacterIdList;
    QHash<quint32,quint8> uniqueKeyToIndex;

    static char tempBuffer[4096];
};
}

#endif // CHARACTERSGROUP_H
