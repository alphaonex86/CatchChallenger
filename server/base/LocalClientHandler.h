#include "ClientMapManagement/MapBasicMove.h"

#ifndef POKECRAFT_LOCALPLAYERHANDLER_H
#define POKECRAFT_LOCALPLAYERHANDLER_H

#include <QObject>
#include <QList>
#include <QString>
#include <QHash>
#include <QHashIterator>
#include <QSqlQuery>

#include "../../general/base/DebugClass.h"
#include "ServerStructures.h"
#include "EventThreader.h"
#include "../VariableServer.h"
#include "MapServer.h"
#include "SqlFunction.h"

/** \brief Do here only the calcule local to the client
 * That's mean map collision, monster event into grass, fight, object usage, ...
 * no access to other client -> broadcast, no file/db access, no visibility calcule, ...
 * Only here you need use the random list */

namespace Pokecraft {
class LocalClientHandler : public MapBasicMove
{
    Q_OBJECT
public:
    explicit LocalClientHandler();
    virtual ~LocalClientHandler();
private:
    bool checkCollision();
    void getRandomNumberIfNeeded();

    //info linked
    static Direction	temp_direction;
    static QHash<QString,LocalClientHandler *> playerByPseudo;
    QByteArray randomSeeds;

    //fight
    quint8 selectedMonster;
    bool ableToFight;
    QList<PlayerMonster> wildMonsters;
    quint8 stepFight_Grass,stepFight_Water,stepFight_Cave;
    inline quint8 getOneSeed(const quint8 &max=0);
    PlayerMonster getRandomMonster(const QList<MapMonster> &monsterList,bool *ok);
    Monster::Stat getStat(const Monster &monster, const quint8 &level);
    void applyOtherBuffEffect(const Monster::Skill::BuffEffect &effect);
    void applyOtherLifeEffect(const Monster::Skill::LifeEffect &effect);
    void generateOtherAttack();
    void updateCanDoFight();
    bool tryEscapeInternal();
    void checkKOMonsters();
    void saveCurrentMonsterStat();

    //map move
    bool singleMove(const Direction &direction);
public slots:
    void put_on_the_map(Map *map,const COORD_TYPE &x,const COORD_TYPE &y,const Orientation &orientation);
    bool moveThePlayer(const quint8 &previousMovedUnit,const Direction &direction);
    //random linked signals
    void newRandomNumber(const QByteArray &randomData);
    //seed
    void useSeed(const quint8 &plant_id);
    //crafting
    void useRecipe(const quint8 &query_id,const quint32 &recipe_id);
    //inventory
    void addObjectAndSend(const quint32 &item,const quint32 &quantity=1);
    void addObject(const quint32 &item,const quint32 &quantity=1);
    quint32 removeObject(const quint32 &item,const quint32 &quantity=1);
    void sendRemoveObject(const quint32 &item,const quint32 &quantity=1);
    quint32 objectQuantity(const quint32 &item);
    void addCash(const quint64 &cash);
    void removeCash(const quint64 &cash);

    void sendHandlerCommand(const QString &command,const QString &extraText);
    //inventory
    void destroyObject(const quint32 &itemId,const quint32 &quantity);
    void useObject(const quint8 &query_id,const quint32 &itemId);
    //teleportation
    void receiveTeleportTo(Map *map,const /*COORD_TYPE*/quint8 &x,const /*COORD_TYPE*/quint8 &y,const Orientation &orientation);
    virtual void teleportValidatedTo(Map *map,const /*COORD_TYPE*/quint8 &x,const /*COORD_TYPE*/quint8 &y,const Orientation &orientation);
    //shop
    void getShopList(const quint32 &query_id,const quint32 &shopId);
    void buyObject(const quint32 &query_id,const quint32 &shopId,const quint32 &objectId,const quint32 &quantity,const quint32 &price);
    void sellObject(const quint32 &query_id,const quint32 &shopId,const quint32 &objectId,const quint32 &quantity,const quint32 &price);
    //fight
    void tryEscape();
    bool checkFightCollision(Map *map,const COORD_TYPE &x,const COORD_TYPE &y);
private slots:
    virtual void extraStop();
    void savePosition();
signals:
    void dbQuery(const QString &sqlQuery);
    void askRandomNumber();
    void receiveSystemText(const QString &text,const bool &important=false);
    void postReply(const quint8 &queryNumber,const QByteArray &data);

    void seedValidated();
    void teleportTo(Map *map,const /*COORD_TYPE*/quint8 &x,const /*COORD_TYPE*/quint8 &y,const Orientation &orientation);
};
}

#endif // POKECRAFT_LOCALPLAYERHANDLER_H
