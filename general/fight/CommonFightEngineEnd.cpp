#include "CommonFightEngine.hpp"
#include "../base/CommonDatapack.hpp"
#include "../base/CommonDatapackServerSpec.hpp"
#include "../base/CommonSettingsServer.hpp"
#include "../base/CommonSettingsCommon.hpp"
#include "../base/GeneralVariable.hpp"

#include <iostream>

using namespace CatchChallenger;

bool CommonFightEngine::otherMonsterIsKO()
{
    PublicPlayerMonster * publicPlayerMonster=getOtherMonster();
    if(publicPlayerMonster==NULL)
        return true;
    return genericMonsterIsKO(publicPlayerMonster);
}

bool CommonFightEngine::genericMonsterIsKO(const PublicPlayerMonster * publicPlayerMonster) const
{
    if(publicPlayerMonster->hp==0)
        return true;
    return false;
}

bool CommonFightEngine::currentMonsterIsKO()
{
    PlayerMonster * playerMonster=getCurrentMonster();
    if(playerMonster==NULL)
        return true;
    return genericMonsterIsKO(playerMonster);
}

bool CommonFightEngine::dropKOCurrentMonster()
{
    PlayerMonster * playerMonster=getCurrentMonster();
    bool currentPlayerReturn=false;
    if(playerMonster==NULL)
        currentPlayerReturn=true;
    else if(playerMonster->hp==0)
    {
        playerMonster->buffs.clear();
        doTurnIfChangeOfMonster=false;
        ableToFight=false;
        currentPlayerReturn=true;
    }
    return currentPlayerReturn;
}

bool CommonFightEngine::dropKOOtherMonster()
{
    bool otherMonsterReturn=false;
    //Now it's std::vector
    if(!wildMonsters.empty())
    {
        unsigned int index=0;
        while(index<wildMonsters.size())
        {
            const PlayerMonster &playerMonster=wildMonsters.at(index);
            if(playerMonster.hp==0)
            {
                wildMonsters.erase(wildMonsters.cbegin()+index);
                otherMonsterReturn=true;
            }
            index++;
        }
    }
    if(!botFightMonsters.empty())
    {
        unsigned int index=0;
        while(index<botFightMonsters.size())
        {
            const PlayerMonster &playerMonster=botFightMonsters.at(index);
            if(playerMonster.hp==0)
            {
                botFightMonsters.erase(botFightMonsters.cbegin()+index);
                otherMonsterReturn=true;
            }
            index++;
        }
    }
    return otherMonsterReturn;
}

void CommonFightEngine::healAllMonsters()
{
    if(CatchChallenger::CommonDatapack::commonDatapack.get_monsters().empty())
        return;
    unsigned int index=0;
    while(index<get_public_and_private_informations().playerMonster.size())
    {
        PlayerMonster &playerMonster=get_public_and_private_informations().playerMonster[index];
        if(playerMonster.egg_step==0)
        {
            const Monster::Stat &stat=getStat(CatchChallenger::CommonDatapack::commonDatapack.get_monsters().at(playerMonster.monster),playerMonster.level);
            playerMonster.hp=stat.hp;
            playerMonster.buffs.clear();
            unsigned int sub_index=0;
            while(sub_index<playerMonster.skills.size())
            {
                const PlayerMonster::PlayerSkill &playerSkill=playerMonster.skills.at(sub_index);
                const uint16_t &skill=playerSkill.skill;
                const uint8_t &skillIndex=playerSkill.level-1;
                if(CatchChallenger::CommonDatapack::commonDatapack.get_monsterSkills().find(skill)!=CatchChallenger::CommonDatapack::commonDatapack.get_monsterSkills().cend())
                {
                    const Skill &fullSkill=CatchChallenger::CommonDatapack::commonDatapack.get_monsterSkills().at(skill);
                    playerMonster.skills[sub_index].endurance=
                            fullSkill.level.at(skillIndex).endurance;
                }
                else
                    errorFightEngine("Try heal an unknown skill: "+std::to_string(skill)+" into list "+std::to_string(CatchChallenger::CommonDatapack::commonDatapack.get_monsterSkills().size()));
                sub_index++;
            }
        }
        index++;
    }
    if(!isInBattle())
        updateCanDoFight();
}

bool CommonFightEngine::haveBeatBot(const uint16_t &botFightId) const
{
    if(get_public_and_private_informations_ro().bot_already_beaten==NULL)
        abort();
    return get_public_and_private_informations_ro().bot_already_beaten[botFightId/8] & (1<<(7-botFightId%8));
}

bool CommonFightEngine::monsterIsKO(const PlayerMonster &playerMonter)
{
    return playerMonter.hp<=0 || playerMonter.egg_step>0;
}

bool CommonFightEngine::internalTryEscape()
{
    doTurnIfChangeOfMonster=true;
    uint8_t value=getOneSeed(101);
    PlayerMonster * playerMonster=getCurrentMonster();
    if(playerMonster==NULL)
    {
        errorFightEngine("No current monster to try escape");
        return false;
    }
    if(wildMonsters.empty())
    {
        errorFightEngine("Not againts wild monster");
        return false;
    }
    if(wildMonsters.front().level<playerMonster->level && value<75)
        return true;
    if(wildMonsters.front().level==playerMonster->level && value<50)
        return true;
    if(wildMonsters.front().level>playerMonster->level && value<25)
        return true;
    return false;
}

void CommonFightEngine::fightFinished()
{
    unsigned int index=0;
    while(index<get_public_and_private_informations().playerMonster.size())
    {
        unsigned int sub_index=0;
        while(sub_index<get_public_and_private_informations().playerMonster.at(index).buffs.size())
        {
            if(CommonDatapack::commonDatapack.get_monsterBuffs().find(get_public_and_private_informations().playerMonster.at(index).buffs.at(sub_index).buff)!=
                    CommonDatapack::commonDatapack.get_monsterBuffs().cend())
            {
                const PlayerBuff &playerBuff=get_public_and_private_informations().playerMonster.at(index).buffs.at(sub_index);
                if(CommonDatapack::commonDatapack.get_monsterBuffs().at(playerBuff.buff).level.at(playerBuff.level-1).duration!=Buff::Duration_Always)
                    get_public_and_private_informations().playerMonster[index].buffs.erase(get_public_and_private_informations().playerMonster[index].buffs.begin()+sub_index);
                else
                    sub_index++;
            }
            else
                get_public_and_private_informations().playerMonster[index].buffs.erase(get_public_and_private_informations().playerMonster[index].buffs.begin()+sub_index);
        }
        index++;
    }
    wildMonsters.clear();
    botFightMonsters.clear();
    doTurnIfChangeOfMonster=true;
}

bool CommonFightEngine::checkKOOtherMonstersForGain()
{
    messageFightEngine("checkKOOtherMonstersForGain()");
    bool winTheTurn=false;
    if(!wildMonsters.empty())
    {
        const PlayerMonster &wildMonster=wildMonsters.front();
        if(wildMonster.hp==0)
        {
            winTheTurn=true;
            #ifdef CATCHCHALLENGER_DEBUG_FIGHT
            messageFightEngine("The wild monster ("+std::to_string(wildMonster.monster)+") is KO");
            #endif
            //drop the drop item here
            wildDrop(wildMonster.monster);
            //give xp/sp here
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(CommonDatapack::commonDatapack.get_monsters().find(wildMonster.monster)==CommonDatapack::commonDatapack.get_monsters().cend())
            {
                std::cerr << "Error, monster into list not found: " << std::to_string(wildMonster.monster) << std::endl;
                abort();
            }
            if(wildMonster.level<=0)
            {
                std::cerr << "Error, monster into list have level == 0: " << std::to_string(wildMonster.monster) << std::endl;
                abort();
            }
            #endif
            const Monster &wildMonsterInfo=CommonDatapack::commonDatapack.get_monsters().at(wildMonster.monster);
            //multiplicator do at datapack loading
            int sp=wildMonsterInfo.give_sp*wildMonster.level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
            int xp=wildMonsterInfo.give_xp*wildMonster.level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(wildMonsterInfo.give_xp<=0)
            {
                std::cerr << "Error, monster into list have wildmonster.give_xp == 0: " << std::to_string(wildMonster.monster) << std::endl;
                abort();
            }
            if(xp==0)
            {
                std::cerr << "Error, given XP is 0, mostly some wrong value" << std::endl;
                abort();
            }
            #endif
            giveXPSP(xp,sp);
            #ifdef CATCHCHALLENGER_DEBUG_FIGHT
            messageFightEngine("You win "+std::to_string(wildMonsterInfo.give_xp*wildMonster.level/CATCHCHALLENGER_MONSTER_LEVEL_MAX)+
                               " xp and "+std::to_string(wildMonsterInfo.give_sp*wildMonster.level/CATCHCHALLENGER_MONSTER_LEVEL_MAX)+" sp");
            #endif
        }
    }
    else if(!botFightMonsters.empty())
    {
        if(botFightMonsters.front().hp==0)
        {
            winTheTurn=true;
            #ifdef CATCHCHALLENGER_DEBUG_FIGHT
            messageFightEngine("The wild monster ("+std::to_string(botFightMonsters.front().monster)+") is KO");
            #endif
            //don't drop item because it's not a wild fight
            //give xp/sp here
            const Monster &botmonster=CommonDatapack::commonDatapack.get_monsters().at(botFightMonsters.front().monster);
            //multiplicator do at datapack loading
            int sp=botmonster.give_sp*botFightMonsters.front().level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
            int xp=botmonster.give_xp*botFightMonsters.front().level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
            giveXPSP(xp,sp);
            #ifdef CATCHCHALLENGER_DEBUG_FIGHT
            messageFightEngine("You win "+std::to_string(botmonster.give_xp*botFightMonsters.front().level/CATCHCHALLENGER_MONSTER_LEVEL_MAX)+
                               " xp and "+std::to_string(botmonster.give_sp*botFightMonsters.front().level/CATCHCHALLENGER_MONSTER_LEVEL_MAX)+" sp");
            #endif
        }
    }
    else if(isInBattle())
    {
        if(otherMonsterIsKO())
            return true;
        return false;
    }
    else
    {
        errorFightEngine("unknown other monster type");
        return false;
    }
    return winTheTurn;
}

//return true if change level, multiplicator do at datapack loading
bool CommonFightEngine::giveXPSP(int xp,int sp)
{
    bool haveChangeOfLevel=false;
    PlayerMonster * monster=getCurrentMonster();
    const Monster &monsterInformations=CommonDatapack::commonDatapack.get_monsters().at(monster->monster);
    uint32_t remaining_xp=monster->remaining_xp;
    uint8_t level=monster->level;
    if(level>=CATCHCHALLENGER_MONSTER_LEVEL_MAX)
    {
        remaining_xp=0;
        xp=0;
    }
    while((remaining_xp+xp)>=monsterInformations.level_to_xp.at(level-1))
    {
        #ifdef CATCHCHALLENGER_DEBUG_FIGHT
        messageFightEngine("Level up because: ("+std::to_string(remaining_xp)+"+"+std::to_string(xp)+")>=monsterInformations.level_to_xp.at("+std::to_string(level)+"-1): "+std::to_string(monsterInformations.level_to_xp.at(level-1))+"");
        #endif
        const uint32_t &old_max_hp=getStat(monsterInformations,level).hp;
        const uint32_t &new_max_hp=getStat(monsterInformations,level+1).hp;
        xp-=monsterInformations.level_to_xp.at(level-1)-remaining_xp;
        remaining_xp=0;
        level++;
        levelUp(level,getCurrentSelectedMonsterNumber());
        haveChangeOfLevel=true;
        if(new_max_hp>old_max_hp)
            monster->hp+=new_max_hp-old_max_hp;
        #ifdef CATCHCHALLENGER_DEBUG_FIGHT
        messageFightEngine("You pass to the level "+std::to_string(level));
        #endif
        if(level>=CATCHCHALLENGER_MONSTER_LEVEL_MAX)
        {
            remaining_xp=0;
            xp=0;
        }
    }
    remaining_xp+=xp;
    monster->remaining_xp=remaining_xp;
    if(haveChangeOfLevel)
        monster->level=level;
    #ifdef CATCHCHALLENGER_DEBUG_FIGHT
    else
        messageFightEngine("Not level up: ("+std::to_string(remaining_xp)+")<monsterInformations.level_to_xp.at("+std::to_string(level)+"-1): "+std::to_string(monsterInformations.level_to_xp.at(level-1))+"");
    #endif
    monster->sp+=sp;

    return haveChangeOfLevel;
}

bool CommonFightEngine::addLevel(PlayerMonster * monster, const uint8_t &numberOfLevel)
{
    if(monster->level>=CATCHCHALLENGER_MONSTER_LEVEL_MAX)
    {
        errorFightEngine("Monster is already at the level max into addLevel()");
        return false;
    }
    uint8_t monsterIndex=0;
    const int &monsterListSize=static_cast<uint32_t>(get_public_and_private_informations().playerMonster.size());
    while(monsterIndex<monsterListSize)
    {
        if(&get_public_and_private_informations().playerMonster.at(monsterIndex)==monster)
            break;
        monsterIndex++;
    }
    if(monsterIndex==monsterListSize)
    {
        errorFightEngine("Monster index not for to add level");
        return false;
    }
    const Monster &monsterInformations=CommonDatapack::commonDatapack.get_monsters().at(monster->monster);
    const uint8_t &level=get_public_and_private_informations().playerMonster.at(monsterIndex).level;
    const uint32_t &old_max_hp=getStat(monsterInformations,level).hp;
    const uint32_t &new_max_hp=getStat(monsterInformations,level+1).hp;
    if(new_max_hp>old_max_hp)
        monster->hp+=new_max_hp-old_max_hp;
    #ifdef CATCHCHALLENGER_DEBUG_FIGHT
    messageFightEngine("You pass to the level "+std::to_string(level));
    #endif

    if((get_public_and_private_informations().playerMonster.at(monsterIndex).level+numberOfLevel)>=CATCHCHALLENGER_MONSTER_LEVEL_MAX)
        get_public_and_private_informations().playerMonster[monsterIndex].level=CATCHCHALLENGER_MONSTER_LEVEL_MAX;
    else
        get_public_and_private_informations().playerMonster[monsterIndex].level+=numberOfLevel;
    get_public_and_private_informations().playerMonster[monsterIndex].remaining_xp=0;
    levelUp(level,monsterIndex);
    return true;
}

void CommonFightEngine::levelUp(const uint8_t &level,const uint8_t &monsterIndex)//call after done the level
{
    autoLearnSkill(level,monsterIndex);
}
