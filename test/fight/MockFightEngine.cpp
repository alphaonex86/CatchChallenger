#include "MockFightEngine.hpp"

#include <iostream>
#include <cstring>

using namespace CatchChallenger;

MockFightEngine::MockFightEngine() :
    catchInvoked(false),
    lastCaughtToStorage(false),
    seedIndex_(0)
{
    lastCaught.monster=0;
    lastCaught.level=0;
    lastCaught.hp=0;
    lastCaught.catched_with=0;
    lastCaught.gender=Gender_Unknown;
    lastCaught.remaining_xp=0;
    lastCaught.egg_step=0;
    lastCaught.id=0;
    lastCaught.character_origin=0;

    // Allocate the bitmap members the engine never reads but Player_private_
    // and_public_informations holds as raw pointers. Sized large enough for
    // the test datapack (a few KB is plenty).
    player_.recipes=new char[256];
    std::memset(player_.recipes,0,256);
    player_.encyclopedia_monster=new char[256];
    std::memset(player_.encyclopedia_monster,0,256);
    player_.encyclopedia_item=new char[256];
    std::memset(player_.encyclopedia_item,0,256);
    player_.cash=0;
    player_.clan=0;
    player_.repel_step=0;
    player_.clan_leader=false;
    player_.allow_create_clan=false;
    ableToFight=true;
    stepFight=0;
    selectedMonster=0;
    doTurnIfChangeOfMonster=false;
}

MockFightEngine::~MockFightEngine()
{
    delete[] player_.recipes;
    delete[] player_.encyclopedia_monster;
    delete[] player_.encyclopedia_item;
}

bool MockFightEngine::isInBattle() const
{
    return !wildMonsters.empty() || !botFightMonsters.empty();
}

uint8_t MockFightEngine::getOneSeed(const uint8_t &max)
{
    if(seedIndex_>=seeds_.size())
        return 0;
    const uint8_t v=seeds_.at(seedIndex_);
    seedIndex_++;
    if(max==0)
        return 0;
    // Engine's contract: result in [0, max). Mirror by modulo to keep
    // tests deterministic — every queued seed maps unambiguously.
    return static_cast<uint8_t>(v%max);
}

uint32_t MockFightEngine::randomSeedsSize() const
{
    if(seedIndex_>=seeds_.size())
        return 0;
    return static_cast<uint32_t>(seeds_.size()-seedIndex_);
}

uint32_t MockFightEngine::catchAWild(const bool &toStorage, const PlayerMonster &newMonster)
{
    catchInvoked=true;
    lastCaught=newMonster;
    lastCaughtToStorage=toStorage;
    return 1;
}

Player_private_and_public_informations &MockFightEngine::get_public_and_private_informations()
{
    return player_;
}

const Player_private_and_public_informations &MockFightEngine::get_public_and_private_informations_ro() const
{
    return player_;
}

void MockFightEngine::errorFightEngine(const std::string &error)
{
    errors.push_back(error);
    std::cerr << "[engine-error] " << error << std::endl;
}

void MockFightEngine::messageFightEngine(const std::string &message) const
{
    const_cast<MockFightEngine*>(this)->messages.push_back(message);
    std::cout << "[engine-msg] " << message << std::endl;
}

void MockFightEngine::mockPushWildMonster(const PlayerMonster &m)
{
    wildMonsters.push_back(m);
}

void MockFightEngine::mockClearWild() { wildMonsters.clear(); }
void MockFightEngine::mockSetAbleToFight(bool v) { ableToFight=v; }
void MockFightEngine::mockSetStepFight(uint8_t v) { stepFight=v; }
uint8_t MockFightEngine::mockStepFight() const { return stepFight; }
void MockFightEngine::mockResetCounters()
{
    catchInvoked=false;
    messages.clear();
    errors.clear();
}
void MockFightEngine::mockSetSelectedMonster(uint8_t v) { selectedMonster=v; }

void MockFightEngine::mockPushSeed(uint8_t s) { seeds_.push_back(s); }
void MockFightEngine::mockPushSeeds(const std::vector<uint8_t> &v)
{
    seeds_.insert(seeds_.end(),v.begin(),v.end());
}
void MockFightEngine::mockClearSeeds()
{
    seeds_.clear();
    seedIndex_=0;
}
