#ifndef CHARACTERSGROUP_H
#define CHARACTERSGROUP_H

#include "../epoll/BaseClassSwitch.h"
#include "../epoll/db/EpollPostgresql.h"
#include <QString>
#include <QHash>
#include <QSet>

namespace CatchChallenger {
class CharactersGroup : public BaseClassSwitch
{
public:
    explicit CharactersGroup(const char * const db,const char * const host,const char * const login,const char * const pass,const quint8 &considerDownAfterNumberOfTry,const quint8 &tryInterval,const QString &name);
    ~CharactersGroup();

    struct InternalGameServer
    {
        QString host;
        quint16 port;
        void * link;

        //stored into EpollClientLoginMaster
        //CharactersGroup * charactersGroup;
        quint32 uniqueKey;
        QString metaData;
        QString logicalGroup;

        quint16 currentPlayer;
        quint16 maxPlayer;
    };

    BaseClassSwitch::Type getType() const;
    InternalGameServer * addGameServerUniqueKey(void * const link,const quint32 &uniqueKey,const QString &host,const quint16 &port,
                                const QString &metaData,const QString &logicalGroup,
                                const quint16 &currentPlayer,const quint16 &maxPlayer);
    void removeGameServerUniqueKey(void * const link);
    bool containsGameServerUniqueKey(const quint32 &serverUniqueKey) const;
    void broadcastGameServerChange();

    quint32 maxClanId;
    quint32 maxCharacterId;
    quint32 maxMonsterId;

    QHash<quint32/*serverUniqueKey*/,InternalGameServer> gameServers;
    QHash<void * const,quint32/*serverUniqueKey*/> gameServersLinkToUniqueKey;

    static int serverWaitedToBeReady;
    static QHash<QString,CharactersGroup *> hash;
    static QList<CharactersGroup *> list;
    QSet<quint32> lockedAccount;
    QString name;
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
};
}

#endif // CHARACTERSGROUP_H
