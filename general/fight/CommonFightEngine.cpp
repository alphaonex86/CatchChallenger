#include "CommonFightEngine.h"

using namespace CatchChallenger;

CommonFightEngine::CommonFightEngine()
{
    stepFight_Grass=0;
    stepFight_Water=0;
    stepFight_Cave=0;
}

bool CommonFightEngine::tryEscape()
{
    if(!isInFight())//check if is in fight
    {
        emit error(QString("error: tryEscape() when is not in fight"));
        return false;
    }
    if(wildMonsters.isEmpty())
    {
        emit message("You can't escape because it's not a wild monster");
        return false;
    }
    return true;
}

bool CommonFightEngine::isInFight()
{
    return !wildMonsters.empty() || !botFightMonsters.isEmpty();
}

void CommonFightEngine::newRandomNumber(const QByteArray &randomData)
{
    randomSeeds+=randomData;
}

void CommonFightEngine::setVariable(Player_private_and_public_informations *player_informations)
{
    this->player_informations=player_informations;
}

bool CommonFightEngine::getAbleToFight()
{
    return ableToFight;
}
