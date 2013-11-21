#include "QmlMonsterGeneralInformations.h"

QmlMonsterGeneralInformations::QmlMonsterGeneralInformations(const CatchChallenger::Monster &monsterInformations,
                                     const DatapackClientLoader::MonsterExtra &monsterInformationsExtra,
                                     QObject *parent) :
    QObject(parent),
    monsterInformations(monsterInformations),
    monsterInformationsExtra(monsterInformationsExtra)
{
}

QString QmlMonsterGeneralInformations::name()
{
    return monsterInformationsExtra.name;
}

QString QmlMonsterGeneralInformations::image()
{
    if(monsterInformationsExtra.frontPath.startsWith(":/"))
        return "qrc"+monsterInformationsExtra.frontPath;
    else
        return "file://"+monsterInformationsExtra.frontPath;
}
