#include "FightEngine.h"
#include "../base/GeneralVariable.h"

using namespace Pokecraft;

FightEngine::FightEngine()
{
    resetAll();
}

FightEngine::~FightEngine()
{
}

//return is have random seed to do random step
bool FightEngine::canDoRandomFight(const Map &map,const quint8 &x,const quint8 &y)
{
    return false;
}

bool FightEngine::haveRandomFight(const Map &map,const quint8 &x,const quint8 &y)
{
    return false;
}

bool FightEngine::canDoFight()
{
    return m_canDoFight;
}

void FightEngine::setPlayerMonster(const QList<PlayerMonster> &playerMonster)
{
    this->playerMonster=playerMonster;
    updateCanDoFight();
}

void FightEngine::updateCanDoFight()
{
    m_canDoFight=false;
    int index=0;
    while(index<playerMonster.size())
    {
        const PlayerMonster &playerMonsterEntry=playerMonster.at(index);
        if(playerMonsterEntry.hp>0 && playerMonsterEntry.egg_step==0)
        {
            m_canDoFight=true;
            return;
        }
        index++;
    }
}

QList<PlayerMonster> FightEngine::getPlayerMonster()
{
    return playerMonster;
}

void FightEngine::resetAll()
{
    monsters.clear();
    monsterSkills.clear();
    monsterBuffs.clear();
    m_randomSeeds.clear();
    playerMonster.clear();
    m_canDoFight=false;
}

void FightEngine::appendRandomSeeds(const QByteArray &data)
{
    m_randomSeeds+=data;
}

/** \warning you need check before the input data */
Monster::Stat FightEngine::getStat(const Monster &monster, const quint8 &level)
{
    Monster::Stat stat=monster.stat;
    stat.attack=stat.attack*level/POKECRAFT_MONSTER_LEVEL_MAX;
    stat.defense=stat.defense*level/POKECRAFT_MONSTER_LEVEL_MAX;
    stat.hp=stat.hp*level/POKECRAFT_MONSTER_LEVEL_MAX;
    stat.special_attack=stat.special_attack*level/POKECRAFT_MONSTER_LEVEL_MAX;
    stat.special_defense=stat.special_defense*level/POKECRAFT_MONSTER_LEVEL_MAX;
    stat.speed=stat.speed*level/POKECRAFT_MONSTER_LEVEL_MAX;
    return stat;
}

const QByteArray FightEngine::randomSeeds()
{
    return m_randomSeeds;
}
