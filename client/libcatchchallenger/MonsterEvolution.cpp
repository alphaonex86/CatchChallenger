#include "MonsterEvolution.hpp"

using namespace CatchChallenger;

const uint32_t MonsterEvolution::animationMs;

bool MonsterEvolution::evolveTo(const Monster &monster,const uint8_t &level,const bool &traded,CATCHCHALLENGER_TYPE_MONSTER &to)
{
    unsigned int index=0;
    while(index<monster.evolutions.size())
    {
        const Monster::Evolution &evolution=monster.evolutions.at(index);
        if((evolution.type==Monster::EvolutionType_Level && evolution.data.level<=level) ||
                (evolution.type==Monster::EvolutionType_Trade && traded))
        {
            to=evolution.evolveTo;
            return true;
        }
        index++;
    }
    return false;
}

bool MonsterEvolution::showEvolved(const uint32_t &elapsedMs)
{
    if(elapsedMs>=animationMs)
        return true;
    //flip count grows with the square of the time: 0 at the start, 50 at the end
    const uint64_t flips=static_cast<uint64_t>(elapsedMs)*elapsedMs/(static_cast<uint64_t>(animationMs)*80);
    return flips%2==1;
}
