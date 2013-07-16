#ifndef CommonFightEngine_H
#define CommonFightEngine_H

#include <QObject>
#include <QByteArray>
#include <QHash>
#include <QString>

#include "../base/GeneralStructures.h"
#include "../../general/base/Map.h"
#include "../base/Api_protocol.h"

namespace CatchChallenger {
//only the logique here, store nothing
class CommonFightEngine : public QObject
{
    Q_OBJECT
public:
    CommonFightEngine();
    virtual void resetAll();
    virtual void setVariable(Player_private_and_public_informations *player_informations);
    virtual bool isInFight();
    virtual bool getAbleToFight();
    bool haveMonsters();
    static Monster::Stat getStat(const Monster &monster, const quint8 &level);
    PlayerMonster *getCurrentMonster();
    virtual PublicPlayerMonster * getOtherMonster();
    Skill::AttackReturn generateOtherAttack();
    quint8 getCurrentSelectedMonsterNumber();
    virtual quint8 getOtherSelectedMonsterNumber();
    bool remainMonstersToFight(const quint32 &monsterId) const;
    virtual bool canDoRandomFight(const Map &map,const quint8 &x,const quint8 &y);
    bool haveRandomFight(const Map &map,const quint8 &x,const quint8 &y);
    void updateCanDoFight();
    bool otherMonsterIsKO();
    bool currentMonsterIsKO();
    bool dropKOCurrentMonster();
    void healAllMonsters();
    bool learnSkill(const quint32 &monsterId,const quint32 &skill);
    void addPlayerMonster(const QList<PlayerMonster> &playerMonster);
    QList<PlayerMonster> getPlayerMonster();
    bool removeMonster(const quint32 &monsterId);
public slots:
    void newRandomNumber(const QByteArray &randomData);
protected:
    virtual bool tryEscape();
    static ApplyOn invertApplyOn(const ApplyOn &applyOn);
    PlayerMonster getRandomMonster(const QList<MapMonster> &monsterList,bool *ok);
    static bool monsterIsKO(const PlayerMonster &playerMonter);
    Skill::LifeEffectReturn applyOtherLifeEffect(const Skill::LifeEffect &effect);
    void applyOtherBuffEffect(const Skill::BuffEffect &effect);
    Skill::LifeEffectReturn applyCurrentLifeEffect(const Skill::LifeEffect &effect);
    void applyCurrentBuffEffect(const Skill::BuffEffect &effect);
    quint8 getOneSeed(const quint8 &max);
signals:
    void error(const QString &error);
    void message(const QString &message);
protected:
    Player_private_and_public_informations *player_informations;
    bool ableToFight;
    QList<PlayerMonster> wildMonsters,botFightMonsters;
    quint8 stepFight_Grass,stepFight_Water,stepFight_Cave;
    QByteArray randomSeeds;
    int selectedMonster;
};
}

#endif // CommonFightEngine_H
