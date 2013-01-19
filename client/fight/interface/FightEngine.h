#ifndef FightEngine_H
#define FightEngine_H

#include <QObject>
#include <QByteArray>
#include <QHash>
#include <QString>
#include <QPixmap>

#include "../base/GeneralStructures.h"
#include "../../general/base/Map.h"
#include "../base/Api_protocol.h"

namespace Pokecraft {
//only the logique here, store nothing
class FightEngine : public QObject
{
    Q_OBJECT
public:
    static FightEngine fightEngine;
    void resetAll();
    const QByteArray randomSeeds();
    bool canDoRandomFight(const Map &map,const quint8 &x,const quint8 &y);
    bool haveRandomFight(const Map &map,const quint8 &x,const quint8 &y);
    bool canDoFight();
    bool isInFight();
    struct MonsterExtra
    {
        QString name;
        QString description;
        QPixmap front;
        QPixmap back;
        QPixmap small;
    };
    struct MonsterBuffExtra
    {
        QString name;
        QString description;
    };
    struct MonsterSkillExtra
    {
        QString name;
        QString description;
    };
    //fight
    QHash<quint32,Monster> monsters;
    QHash<quint32,MonsterSkill> monsterSkills;
    QHash<quint32,MonsterBuff> monsterBuffs;
    QHash<quint32,MonsterExtra> monsterExtra;
    QHash<quint32,MonsterBuffExtra> monsterBuffsExtra;
    QHash<quint32,MonsterSkillExtra> monsterSkillsExtra;
    void setPlayerMonster(const QList<PlayerMonster> &playerMonsterList);
    QList<PlayerMonster> getPlayerMonster();
    PlayerMonster getFightMonster();
    PlayerMonster getOtherMonster();
    bool haveOtherMonster();
    //last step
    QList<quint8> stepFight_Water,stepFight_Grass,stepFight_Cave;
    //current fight
    QList<PlayerMonster> wildMonsters;
    bool tryEscape();//return true if is escaped
    bool canDoFightAction();
private:
    int selectedMonster;
    QByteArray m_randomSeeds;
    QList<PlayerMonster> playerMonsterList;
    bool m_canDoFight;
    PlayerMonster getRandomMonster(const QList<MapMonster> &monsterList, bool *ok);
    inline quint8 getOneSeed();
private:
    void updateCanDoFight();
    explicit FightEngine();
    ~FightEngine();
public slots:
    //fight
    void appendRandomSeeds(const QByteArray &data);
    static Monster::Stat getStat(const Monster &monster,const quint8 &level);
};
}

#endif // FightEngine_H
