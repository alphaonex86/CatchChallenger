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
    explicit CharactersGroup(const char * const db, const char * const host, const char * const login, const char * const pass, const uint8_t &considerDownAfterNumberOfTry, const uint8_t &tryInterval, const QString &name);
    ~CharactersGroup();

    struct InternalGameServer
    {
        QString host;
        uint16_t port;
        void * link;

        //stored into EpollClientLoginMaster
        //CharactersGroup * charactersGroup;
        uint32_t uniqueKey;
        QString metaData;
        uint32_t logicalGroupIndex;

        uint16_t currentPlayer;
        uint16_t maxPlayer;

        QSet<uint32_t> lockedAccount;
    };

    BaseClassSwitch::EpollObjectType getType() const;
    InternalGameServer * addGameServerUniqueKey(void * const link, const uint32_t &uniqueKey, const QString &host, const uint16_t &port,
                                const QString &metaData, const uint32_t &logicalGroupIndex,
                                const uint16_t &currentPlayer, const uint16_t &maxPlayer, const QSet<uint32_t> &lockedAccount);
    void removeGameServerUniqueKey(void * const link);
    bool containsGameServerUniqueKey(const uint32_t &serverUniqueKey) const;
    bool characterIsLocked(const uint32_t &characterId);
    //need check if is already locked before this call
    //don't apply on InternalGameServer
    void lockTheCharacter(const uint32_t &characterId);
    //don't apply on InternalGameServer
    void unlockTheCharacter(const uint32_t &characterId);
    void waitBeforeReconnect(const uint32_t &characterId);
    void purgeTheLockedAccount();

    uint32_t maxClanId;
    uint32_t maxCharacterId;
    uint32_t maxMonsterId;

    QHash<uint32_t/*serverUniqueKey*/,InternalGameServer> gameServers;
    QHash<void * const,uint32_t/*serverUniqueKey*/> gameServersLinkToUniqueKey;

    static int serverWaitedToBeReady;
    static QHash<QString,CharactersGroup *> hash;
    static QList<CharactersGroup *> list;
    QString name;
    uint8_t index;
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
    QHash<uint32_t/*uniqueKey*/,uint64_t/*can reconnect after this time stamps if !=0, else locked*/> lockedAccount;
    QHash<uint32_t/*uniqueKey*/,QSet<uint32_t/*lockedAccount*/> > lockedAccountByDisconnectedServer;
};
}

#endif // CHARACTERSGROUP_H
