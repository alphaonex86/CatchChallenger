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

    void clearServerPair();
    void setServerPair(const quint8 &index,const quint16 &databaseId);
    void setServerUniqueKey(const quint32 &serverUniqueKey,const char * const hostData,const quint8 &hostDataSize,const quint16 &port);
    bool containsServerUniqueKey(const quint32 &serverUniqueKey) const;

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
    bool addCharacter(void * const client,const quint8 &query_id, const quint8 &profileIndex, const QString &pseudo, const quint8 &skinId);
    static void addCharacter_static(void *object);
    void addCharacter_object();
    void addCharacter_return(const quint8 &query_id,const quint8 &profileIndex,const QString &pseudo,const quint8 &skinId);
    bool removeCharacter(void * const client,const quint8 &query_id, const quint32 &characterId);
    static void removeCharacter_static(void *object);
    void removeCharacter_object();
    void removeCharacter_return(const quint8 &query_id,const quint32 &characterId);

    void character_list(EpollClientLoginSlave * const client,const quint32 &account_id);
    static void character_list_static(void *object);
    void character_list_object();
    void server_list(EpollClientLoginSlave * const client,const quint32 &account_id);
    static void server_list_static(void *object);
    void server_list_object();
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

    struct InternalLoginServer
    {
        QString host;
        quint16 port;
    };
private:
    EpollPostgresql *databaseBaseCommon;
    QList<qint16/*allow -1 to not found*/> dictionary_server_database_to_index;
    QList<quint32> dictionary_server_index_to_database;
    QHash<quint32,InternalLoginServer> servers;
    QList<void * const> clientAddReturnList;
    QList<void * const> clientRemoveReturnList;
    QList<quint32> deleteCharacterNowCharacterIdList;
};
}

#endif // CHARACTERSGROUP_H
