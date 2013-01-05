#include "FightEngine.h"

using namespace Pokecraft;

FightEngine::FightEngine()
{
}

FightEngine::~FightEngine()
{
}

bool FightEngine::canDoRandomStep()
{
    return false;
}

bool FightEngine::canDoRandomFight()
{
    return false;
}

void FightEngine::resetAll()
{
    m_randomSeeds.clear();
}

void FightEngine::appendRandomSeeds(const QByteArray &data)
{
    m_randomSeeds+=data;
}

const QByteArray FightEngine::randomSeeds()
{
    return m_randomSeeds;
}
