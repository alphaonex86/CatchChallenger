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
	return tr("Monster A evolve into Monster B");
}
