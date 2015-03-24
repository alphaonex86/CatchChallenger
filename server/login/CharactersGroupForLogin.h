#ifndef CHARACTERSGROUP_H
#define CHARACTERSGROUP_H

#include "../epoll/BaseClassSwitch.h"
#include "../epoll/db/EpollPostgresql.h"
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

    std::vector<quint32> maxClanId;
    std::vector<quint32> maxCharacterId;
    std::vector<quint32> maxMonsterId;

    static int serverWaitedToBeReady;
    static QHash<QString,CharactersGroupForLogin *> databaseBaseCommonList;
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
