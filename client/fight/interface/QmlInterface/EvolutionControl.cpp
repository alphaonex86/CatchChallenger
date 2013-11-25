#include "EvolutionControl.h"

EvolutionControl::EvolutionControl(const CatchChallenger::Monster &fromMonsterInformations,const DatapackClientLoader::MonsterExtra &frontMonsterInformationsExtra,const CatchChallenger::Monster &toMonsterInformations,const DatapackClientLoader::MonsterExtra &toMonsterInformationsExtra) :
    fromMonsterInformations(fromMonsterInformations),
    frontMonsterInformationsExtra(frontMonsterInformationsExtra),
    toMonsterInformations(toMonsterInformations),
    toMonsterInformationsExtra(toMonsterInformationsExtra)
{
}

QString EvolutionControl::evolutionText()
{
    return tr("Your %1 will evolve into %1").arg(frontMonsterInformationsExtra.name).arg(toMonsterInformationsExtra.name);
}
