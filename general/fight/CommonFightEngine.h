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
    void newRandomNumber(const QByteArray &randomData);
    void setVariable(Player_private_and_public_informations *player_informations);
    virtual bool isInFight();
    virtual bool getAbleToFight();
protected:
    virtual void getRandomNumberIfNeeded() = 0;
    virtual bool tryEscape();
signals:
    void error(const QString &error);
    void message(const QString &message);
protected:
    Player_private_and_public_informations *player_informations;
    bool ableToFight;
    QList<PlayerMonster> wildMonsters,botFightMonsters;
    quint8 stepFight_Grass,stepFight_Water,stepFight_Cave;
    QByteArray randomSeeds;
};
}

#endif // CommonFightEngine_H
