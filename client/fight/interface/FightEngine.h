#ifndef FightEngine_H
#define FightEngine_H

#include <QObject>
#include <QByteArray>
#include <QHash>

#include "../base/GeneralStructures.h"
#include "../../general/base/Map.h"

namespace Pokecraft {
//only the logique here, store nothing
class FightEngine : public QObject
{
    Q_OBJECT
public:
    explicit FightEngine();
    ~FightEngine();
    void resetAll();
    const QByteArray randomSeeds();
    bool canDoRandomFight(const Map &map,const quint8 &x,const quint8 &y);
    bool haveRandomFight(const Map &map,const quint8 &x,const quint8 &y);
    bool canDoFight();
    enum landFight
    {
        landFight_Water,
        landFight_Grass,
        landFight_Cave
    };
    //fight
    QHash<quint32,Monster> monsters;
    QHash<quint32,MonsterSkill> monsterSkills;
    QHash<quint32,MonsterBuff> monsterBuffs;
    void setPlayerMonster(const QList<PlayerMonster> &playerMonster);
    QList<PlayerMonster> getPlayerMonster();
private:
    QByteArray m_randomSeeds;
    QList<PlayerMonster> playerMonster;
    bool m_canDoFight;
private:
    void updateCanDoFight();
public slots:
    //fight
    void appendRandomSeeds(const QByteArray &data);
    static Monster::Stat getStat(const Monster &monster,const quint8 &level);
};
}

#endif // FightEngine_H
