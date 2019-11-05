#include "EvolutionControl.h"

EvolutionControl::EvolutionControl(const CatchChallenger::Monster &fromMonsterInformations,const DatapackClientLoader::MonsterExtra &fromMonsterInformationsExtra,const CatchChallenger::Monster &toMonsterInformations,const DatapackClientLoader::MonsterExtra &toMonsterInformationsExtra) :
    fromMonsterInformations(fromMonsterInformations),
    fromMonsterInformationsExtra(fromMonsterInformationsExtra),
    toMonsterInformations(toMonsterInformations),
    toMonsterInformationsExtra(toMonsterInformationsExtra)
{
}

QString EvolutionControl::evolutionText()
{
    return tr("Your %1 will evolve into %2")
            .arg(QString::fromStdString(fromMonsterInformationsExtra.name))
            .arg(QString::fromStdString(toMonsterInformationsExtra.name));
}
