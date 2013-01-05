#ifndef FightEngine_H
#define FightEngine_H

#include <QObject>
#include <QByteArray>
#include <QHash>

#include "../base/GeneralStructures.h"

namespace Pokecraft {
class FightEngine : public QObject
{
    Q_OBJECT
public:
    explicit FightEngine();
    ~FightEngine();
    void resetAll();
    const QByteArray randomSeeds();
    bool canDoRandomStep();
    bool canDoRandomFight();
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
private:
    QByteArray m_randomSeeds;
public slots:
    //fight
    void appendRandomSeeds(const QByteArray &data);
};
}

#endif // FightEngine_H
