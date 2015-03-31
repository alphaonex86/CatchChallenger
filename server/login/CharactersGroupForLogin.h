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

    std::vector<quint32> maxCharacterId;
    std::vector<quint32> maxMonsterId;
    quint8 index;

    static QHash<QString,CharactersGroupForLogin *> hash;
    static QList<CharactersGroupForLogin *> list;
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
private:
    EpollPostgresql *databaseBaseCommon;
    QList<qint16/*allow -1 to not found*/> dictionary_server_database_to_index;
    QList<quint32> dictionary_server_index_to_database;
    QList<EpollClientLoginSlave * const> clientQueryForReadReturn;
};
}

#endif // CHARACTERSGROUP_H
