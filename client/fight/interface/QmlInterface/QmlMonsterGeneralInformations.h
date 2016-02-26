#ifndef QmlMonsterGeneralInformations_H
#define QmlMonsterGeneralInformations_H

#include <QObject>
#include "../../../../general/base/CommonDatapack.h"
#include "../../../base/interface/DatapackClientLoader.h"

class QmlMonsterGeneralInformations : public QObject
{
    Q_OBJECT
public:
    explicit QmlMonsterGeneralInformations(
    const CatchChallenger::Monster &monsterInformations,
    const DatapackClientLoader::MonsterExtra &monsterInformationsExtra,
    QObject *parent = 0);
private:
    const CatchChallenger::Monster &monsterInformations;
    const DatapackClientLoader::MonsterExtra &monsterInformationsExtra;
public slots:
    QString name();
    QString image();
signals:
    void finised();
};

#endif // QmlMonsterGeneralInformations_H
