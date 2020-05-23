#include "QmlMonsterGeneralInformations.hpp"
#include <QUrl>

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
    return QString::fromStdString(monsterInformationsExtra.name);
}

QString QmlMonsterGeneralInformations::image()
{
    if(stringStartWith(monsterInformationsExtra.frontPath,":/"))
        return "qrc"+QString::fromStdString(monsterInformationsExtra.frontPath);
    else
        return QUrl::fromLocalFile(QString::fromStdString(monsterInformationsExtra.frontPath)).toEncoded();
}
