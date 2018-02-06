#include "CommonFightEngine.h"
#include "../base/CommonDatapack.h"
#include "../base/CommonDatapackServerSpec.h"
#include "../base/CommonSettingsServer.h"
#include "../base/CommonSettingsCommon.h"
#include "../base/GeneralVariable.h"

#include <iostream>

using namespace CatchChallenger;

CommonFightEngine::CommonFightEngine()
{
    resetAll();
}

bool CommonFightEngine::canEscape()
{
    if(!isInFight())//check if is in fight
    {
        errorFightEngine("error: tryEscape() when is not in fight");
        return false;
    }
    if(wildMonsters.empty())
    {
        messageFightEngine("You can't escape because it's not a wild monster");
        return false;
    }
    return true;
}

void CommonFightEngine::resetAll()
{
    stepFight=0;
    ableToFight=false;
    wildMonsters.clear();
    botFightMonsters.clear();
    selectedMonster=0;
    doTurnIfChangeOfMonster=true;
}

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
    /*if(!wildMonsters.empty())
    {
        auto it=wildMonsters.cbegin();
        while(it!=wildMonsters.cend())
        {

            if((*it).hp==0)
            {
                wildMonsters.erase(it);
                otherMonsterReturn=true;
            }
            ++it;
        }
    }
    if(!botFightMonsters.empty())
    {
        auto it=botFightMonsters.cbegin();
        while(it!=botFightMonsters.cend())
        {
            if((*it).hp==0)
            {
                botFightMonsters.erase(it);
                otherMonsterReturn=true;
            }
            ++it;
        }
    }*/
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
    if(CatchChallenger::CommonDatapack::commonDatapack.monsters.empty())
        return;
    unsigned int index=0;
    while(index<public_and_private_informations.playerMonster.size())
    {
        PlayerMonster &playerMonster=public_and_private_informations.playerMonster[index];
        if(playerMonster.egg_step==0)
        {
            const Monster::Stat &stat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(playerMonster.monster),playerMonster.level);
            playerMonster.hp=stat.hp;
            playerMonster.buffs.clear();
            unsigned int sub_index=0;
            while(sub_index<playerMonster.skills.size())
            {
                const PlayerMonster::PlayerSkill &playerSkill=playerMonster.skills.at(sub_index);
                const int &skill=playerSkill.skill;
                const int &skillIndex=playerSkill.level-1;
                if(CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.find(skill)!=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.cend())
                {
                    const Skill &fullSkill=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.at(skill);
                    playerMonster.skills[sub_index].endurance=
                            fullSkill.level.at(skillIndex).endurance;
                }
                else
                    errorFightEngine("Try heal an unknown skill: "+std::to_string(skill)+" into list "+std::to_string(CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.size()));
                sub_index++;
            }
        }
        index++;
    }
    if(!isInBattle())
        updateCanDoFight();
}

bool CommonFightEngine::isInFight() const
{
    return !wildMonsters.empty() || !botFightMonsters.empty();
}

bool CommonFightEngine::getAbleToFight() const
{
    return ableToFight;
}

bool CommonFightEngine::haveMonsters() const
{
    return !public_and_private_informations.playerMonster.empty();
}

//return is have random seed to do random step
bool CommonFightEngine::canDoRandomFight(const CommonMap &map,const uint8_t &x,const uint8_t &y) const
{
    if(isInFight())
    {
        messageFightEngine("map: "+map.map_file+" ("+std::to_string(x)+","+std::to_string(y)+"), is in fight");
        return false;
    }
    if(map.parsed_layer.monstersCollisionMap==NULL)
        return true;

    uint8_t zoneCode=map.parsed_layer.monstersCollisionMap[x+y*map.width];
    const MonstersCollisionValue &monstersCollisionValue=map.parsed_layer.monstersCollisionList.at(zoneCode);
    if(monstersCollisionValue.walkOn.empty())
        return true;
    else
        return randomSeedsSize()>=CATCHCHALLENGER_MIN_RANDOM_TO_FIGHT;

    /// no fight in this zone
    messageFightEngine("map: "+map.map_file+" ("+std::to_string(x)+","+std::to_string(y)+"), no fight in this zone");
    return true;
}

bool CommonFightEngine::haveBeatBot(const uint16_t &botFightId) const
{
    if(public_and_private_informations.bot_already_beaten==NULL)
        abort();
    return public_and_private_informations.bot_already_beaten[botFightId/8] & (1<<(7-botFightId%8));
}

void CommonFightEngine::updateCanDoFight()
{
    if(isInBattle())
    {
        errorFightEngine("Can't auto select the next monster when is in battle");
        return;
    }
    ableToFight=false;
    unsigned int index=0;
    while(index<public_and_private_informations.playerMonster.size())
    {
        const PlayerMonster &playerMonsterEntry=public_and_private_informations.playerMonster.at(index);
        if(!monsterIsKO(playerMonsterEntry))
        {
            selectedMonster=index;
            ableToFight=true;
            return;
        }
        index++;
    }
}

bool CommonFightEngine::haveAnotherMonsterOnThePlayerToFight() const
{
    unsigned int index=0;
    while(index<public_and_private_informations.playerMonster.size())
    {
        const PlayerMonster &playerMonsterEntry=public_and_private_informations.playerMonster.at(index);
        if(!monsterIsKO(playerMonsterEntry))
            return true;
        index++;
    }
    return false;
}

bool CommonFightEngine::haveAnotherEnnemyMonsterToFight()
{
    if(!wildMonsters.empty())
        return false;
    if(!botFightMonsters.empty())
        return botFightMonsters.size()>1;
    errorFightEngine("Unable to locate the other monster");
    return false;
}

PlayerMonster * CommonFightEngine::getCurrentMonster()//no const due to error message
{
    const int &playerMonsterSize=public_and_private_informations.playerMonster.size();
    if(selectedMonster>=0 && selectedMonster<playerMonsterSize)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(CommonDatapack::commonDatapack.monsters.find(public_and_private_informations.playerMonster.at(selectedMonster).monster)==CommonDatapack::commonDatapack.monsters.cend())
        {
            errorFightEngine("Current monster don't exists: "+std::to_string(public_and_private_informations.playerMonster.at(selectedMonster).monster));
            return NULL;
        }
        #endif
        return &public_and_private_informations.playerMonster[selectedMonster];
    }
    else
    {
        errorFightEngine("selectedMonster is out of range, max: "+std::to_string(playerMonsterSize));
        return NULL;
    }
}

uint8_t CommonFightEngine::getCurrentSelectedMonsterNumber() const
{
    return selectedMonster;
}

uint8_t CommonFightEngine::getOtherSelectedMonsterNumber() const
{
    return 0;
}

PublicPlayerMonster *CommonFightEngine::getOtherMonster()
{
    if(!wildMonsters.empty())
        return (PublicPlayerMonster *)&wildMonsters.front();
    else if(!botFightMonsters.empty())
        return (PublicPlayerMonster *)&botFightMonsters.front();
    else
        return NULL;
}

bool CommonFightEngine::remainMonstersToFightWithoutThisMonster(const uint8_t &monsterPosition) const
{
    unsigned int index=0;
    while(index<public_and_private_informations.playerMonster.size())
    {
        const PlayerMonster &playerMonsterEntry=public_and_private_informations.playerMonster.at(index);
        if(index==monsterPosition)
        {
            //the current monster can't fight, echange it will do nothing
            if(monsterIsKO(playerMonsterEntry))
                return true;
        }
        else
        {
            //other monster can fight, can continue to fight
            if(!monsterIsKO(playerMonsterEntry))
                return true;
        }
        index++;
    }
    return false;
}

bool CommonFightEngine::monsterIsKO(const PlayerMonster &playerMonter)
{
    return playerMonter.hp<=0 || playerMonter.egg_step>0;
}

/// \note Object quantity verified in LocalClientHandler::useObjectOnMonster()
bool CommonFightEngine::useObjectOnMonsterByPosition(const uint16_t &object, const uint8_t &monsterPosition)
{
    if(!haveThisMonsterByPosition(monsterPosition))
    {
        errorFightEngine("have not this monster: "+std::to_string(monsterPosition));
        return false;
    }
    PlayerMonster * playerMonster=monsterByPosition(monsterPosition);
    if(playerMonster==NULL)
    {
        errorFightEngine("have not this monster pointer: "+std::to_string(monsterPosition));
        return false;
    }
    if(genericMonsterIsKO(playerMonster))
    {
        errorFightEngine("can't be applyied on KO monster: "+std::to_string(object));
        return false;
    }
    if(CatchChallenger::CommonDatapack::commonDatapack.items.evolutionItem.find(object)!=CatchChallenger::CommonDatapack::commonDatapack.items.evolutionItem.cend())
    {
        if(isInFight())
        {
            errorFightEngine("this item "+std::to_string(object)+" can't be used in fight");
            return false;
        }
        if(CatchChallenger::CommonDatapack::commonDatapack.items.evolutionItem.at(object).find(playerMonster->monster)==CatchChallenger::CommonDatapack::commonDatapack.items.evolutionItem.at(object).cend())
        {
            errorFightEngine("this item "+std::to_string(object)+" can't be applied on this monster "+std::to_string(playerMonster->monster));
            return false;
        }
        confirmEvolutionTo(playerMonster,CatchChallenger::CommonDatapack::commonDatapack.items.evolutionItem.at(object).at(playerMonster->monster));
        return true;
    }
    else if(CommonDatapack::commonDatapack.items.monsterItemEffect.find(object)!=CommonDatapack::commonDatapack.items.monsterItemEffect.cend()
            || CommonDatapack::commonDatapack.items.monsterItemEffectOutOfFight.find(object)!=CommonDatapack::commonDatapack.items.monsterItemEffectOutOfFight.cend())
    {
        if(CommonDatapack::commonDatapack.items.monsterItemEffect.find(object)!=CommonDatapack::commonDatapack.items.monsterItemEffect.cend())
        {
            const Monster::Stat &playerMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(playerMonster->monster),playerMonster->level);
            const std::vector<MonsterItemEffect> monsterItemEffect = CommonDatapack::commonDatapack.items.monsterItemEffect.at(object);
            unsigned int index=0;
            while(index<monsterItemEffect.size())
            {
                const MonsterItemEffect &effect=monsterItemEffect.at(index);
                switch(effect.type)
                {
                    case MonsterItemEffectType_AddHp:
                        if(effect.value>0 && (playerMonsterStat.hp-playerMonster->hp)>(uint32_t)effect.value)
                            hpChange(playerMonster,playerMonster->hp+effect.value);
                        else if(playerMonsterStat.hp!=playerMonster->hp)
                            hpChange(playerMonster,playerMonsterStat.hp);
                        else if(monsterItemEffect.size()==1)
                            return false;
                    break;
                    case MonsterItemEffectType_RemoveBuff:
                        if(effect.value>0)
                            removeBuffOnMonster(playerMonster,effect.value);
                        else
                            removeAllBuffOnMonster(playerMonster);
                    break;
                    default:
                        messageFightEngine("Item "+std::to_string(object)+" have unknown effect");
                    break;
                }
                index++;
            }
            if(isInFight())
                doTheOtherMonsterTurn();
        }
        else if(CommonDatapack::commonDatapack.items.monsterItemEffectOutOfFight.find(object)!=CommonDatapack::commonDatapack.items.monsterItemEffectOutOfFight.cend())
        {
            if(isInFight())
            {
                errorFightEngine("this item "+std::to_string(object)+" can't be used in fight");
                return false;
            }
            const std::vector<MonsterItemEffectOutOfFight> monsterItemEffectOutOfFight = CommonDatapack::commonDatapack.items.monsterItemEffectOutOfFight.at(object);
            unsigned int index=0;
            while(index<monsterItemEffectOutOfFight.size())
            {
                const MonsterItemEffectOutOfFight &effect=monsterItemEffectOutOfFight.at(index);
                switch(effect.type)
                {
                    case MonsterItemEffectTypeOutOfFight_AddLevel:
                        if(playerMonster->level>=CATCHCHALLENGER_MONSTER_LEVEL_MAX)
                        {
                            errorFightEngine("this item "+std::to_string(object)+" can't be used on monster at level max");
                            return false;
                        }
                        addLevel(playerMonster);
                    break;
                    default:
                        messageFightEngine("Item "+std::to_string(object)+" have unknown effect");
                    break;
                }
                index++;
            }
        }
    }
    else if(CommonDatapack::commonDatapack.items.itemToLearn.find(object)!=CommonDatapack::commonDatapack.items.itemToLearn.cend())
    {
        if(isInFight())
        {
            errorFightEngine("this item "+std::to_string(object)+" can't be used in fight");
            return false;
        }
        if(CatchChallenger::CommonDatapack::commonDatapack.items.itemToLearn.at(object).find(playerMonster->monster)==CatchChallenger::CommonDatapack::commonDatapack.items.itemToLearn.at(object).cend())
        {
            errorFightEngine("this item "+std::to_string(object)+" can't be applied on this monster "+std::to_string(playerMonster->monster));
            return false;
        }
        return learnSkillByItem(playerMonster,object);
    }
    else
    {
        errorFightEngine("This object can't be applied on monster: "+std::to_string(object));
        return false;
    }

    return true;
}

void CommonFightEngine::hpChange(PlayerMonster * currentMonster,const uint32_t &newHpValue)
{
    currentMonster->hp=newHpValue;
}

bool CommonFightEngine::changeOfMonsterInFight(const uint8_t &monsterPosition)
{
    if(monsterPosition>=public_and_private_informations.playerMonster.size())
    {
        errorFightEngine("The monster is not found: "+std::to_string(monsterPosition));
        return false;
    }
    if(!isInFight())
        return false;
    if((uint8_t)selectedMonster==monsterPosition)
    {
        errorFightEngine("try change monster but is already on the current monster");
        return false;
    }
    const PlayerMonster &playerMonsterEntry=public_and_private_informations.playerMonster.at(monsterPosition);
    if(!monsterIsKO(playerMonsterEntry))
    {
        selectedMonster=monsterPosition;
        ableToFight=true;
        if(doTurnIfChangeOfMonster)
            doTheOtherMonsterTurn();
        else
            doTurnIfChangeOfMonster=true;
        return true;
    }
    else
        return false;
    errorFightEngine("unable to locate the new monster to change");
    return false;
}

ApplyOn CommonFightEngine::invertApplyOn(const ApplyOn &applyOn)
{
    switch(applyOn)
    {
        case ApplyOn_AloneEnemy:
            return ApplyOn_Themself;
        case ApplyOn_AllEnemy:
            return ApplyOn_AllAlly;
        case ApplyOn_Themself:
            return ApplyOn_AloneEnemy;
        case ApplyOn_AllAlly:
            return ApplyOn_AllEnemy;
        default:
            return ApplyOn_Themself;
        break;
    }
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
    while(index<public_and_private_informations.playerMonster.size())
    {
        unsigned int sub_index=0;
        while(sub_index<public_and_private_informations.playerMonster.at(index).buffs.size())
        {
            if(CommonDatapack::commonDatapack.monsterBuffs.find(public_and_private_informations.playerMonster.at(index).buffs.at(sub_index).buff)!=CommonDatapack::commonDatapack.monsterBuffs.cend())
            {
                const PlayerBuff &playerBuff=public_and_private_informations.playerMonster.at(index).buffs.at(sub_index);
                if(CommonDatapack::commonDatapack.monsterBuffs.at(playerBuff.buff).level.at(playerBuff.level-1).duration!=Buff::Duration_Always)
                    public_and_private_informations.playerMonster[index].buffs.erase(public_and_private_informations.playerMonster[index].buffs.begin()+sub_index);
                else
                    sub_index++;
            }
            else
                public_and_private_informations.playerMonster[index].buffs.erase(public_and_private_informations.playerMonster[index].buffs.begin()+sub_index);
        }
        index++;
    }
    wildMonsters.clear();
    botFightMonsters.clear();
    doTurnIfChangeOfMonster=true;
}

std::vector<uint8_t> CommonFightEngine::addPlayerMonster(const std::vector<PlayerMonster> &playerMonster)
{
    std::vector<uint8_t> positionList;
    {
        unsigned int index=0;
        while(index<playerMonster.size())
        {
            positionList.push_back(public_and_private_informations.playerMonster.size()+index);
            index++;
        }
    }
    public_and_private_informations.playerMonster.insert(public_and_private_informations.playerMonster.cend(),playerMonster.cbegin(),playerMonster.cend());
    updateCanDoFight();
    return positionList;
}

std::vector<uint8_t> CommonFightEngine::addPlayerMonster(const PlayerMonster &playerMonster)
{
    std::vector<uint8_t> positionList;
    positionList.push_back(public_and_private_informations.playerMonster.size());
    public_and_private_informations.playerMonster.push_back(playerMonster);
    updateCanDoFight();
    return positionList;
}

void CommonFightEngine::insertPlayerMonster(const uint8_t &place,const PlayerMonster &playerMonster)
{
    public_and_private_informations.playerMonster.insert(public_and_private_informations.playerMonster.cbegin()+place,playerMonster);
    updateCanDoFight();
}

std::vector<PlayerMonster> CommonFightEngine::getPlayerMonster() const
{
    return public_and_private_informations.playerMonster;
}

bool CommonFightEngine::moveUpMonster(const uint8_t &number)
{
    if(isInFight())
        return false;
    if(number==0)
        return false;
    if(number>=(public_and_private_informations.playerMonster.size()))
        return false;
    std::swap(public_and_private_informations.playerMonster[number],public_and_private_informations.playerMonster[number-1]);
    updateCanDoFight();
    return true;
}

bool CommonFightEngine::moveDownMonster(const uint8_t &number)
{
    if(isInFight())
        return false;
    if(number>=(public_and_private_informations.playerMonster.size()-1))
        return false;
    std::swap(public_and_private_informations.playerMonster[number],public_and_private_informations.playerMonster[number+1]);
    updateCanDoFight();
    return true;
}

bool CommonFightEngine::removeMonsterByPosition(const uint8_t &monsterPosition)
{
    if(monsterPosition>=public_and_private_informations.playerMonster.size())
        return false;
    else
    {
        public_and_private_informations.playerMonster.erase(public_and_private_informations.playerMonster.cbegin()+monsterPosition);
        updateCanDoFight();
        return true;
    }
}

bool CommonFightEngine::haveThisMonsterByPosition(const uint8_t &monsterPosition) const
{
    if(monsterPosition>=public_and_private_informations.playerMonster.size())
        return false;
    else
        return true;
}

PlayerMonster * CommonFightEngine::monsterByPosition(const uint8_t &monsterPosition)
{
    if(monsterPosition>=public_and_private_informations.playerMonster.size())
        return NULL;
    else
        return &public_and_private_informations.playerMonster[monsterPosition];
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
            if(CommonDatapack::commonDatapack.monsters.find(wildMonster.monster)==CommonDatapack::commonDatapack.monsters.cend())
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
            const Monster &wildMonsterInfo=CommonDatapack::commonDatapack.monsters.at(wildMonster.monster);
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
            const Monster &botmonster=CommonDatapack::commonDatapack.monsters.at(botFightMonsters.front().monster);
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
    const Monster &monsterInformations=CommonDatapack::commonDatapack.monsters.at(monster->monster);
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
    int monsterIndex=0;
    const int &monsterListSize=public_and_private_informations.playerMonster.size();
    while(monsterIndex<monsterListSize)
    {
        if(&public_and_private_informations.playerMonster.at(monsterIndex)==monster)
            break;
        monsterIndex++;
    }
    if(monsterIndex==monsterListSize)
    {
        errorFightEngine("Monster index not for to add level");
        return false;
    }
    const Monster &monsterInformations=CommonDatapack::commonDatapack.monsters.at(monster->monster);
    const uint8_t &level=public_and_private_informations.playerMonster.at(monsterIndex).level;
    const uint32_t &old_max_hp=getStat(monsterInformations,level).hp;
    const uint32_t &new_max_hp=getStat(monsterInformations,level+1).hp;
    if(new_max_hp>old_max_hp)
        monster->hp+=new_max_hp-old_max_hp;
    #ifdef CATCHCHALLENGER_DEBUG_FIGHT
    messageFightEngine("You pass to the level "+std::to_string(level));
    #endif

    if((public_and_private_informations.playerMonster.at(monsterIndex).level+numberOfLevel)>=CATCHCHALLENGER_MONSTER_LEVEL_MAX)
        public_and_private_informations.playerMonster[monsterIndex].level=CATCHCHALLENGER_MONSTER_LEVEL_MAX;
    else
        public_and_private_informations.playerMonster[monsterIndex].level+=numberOfLevel;
    public_and_private_informations.playerMonster[monsterIndex].remaining_xp=0;
    levelUp(level,monsterIndex);
    return true;
}

void CommonFightEngine::levelUp(const uint8_t &level,const uint8_t &monsterIndex)//call after done the level
{
    autoLearnSkill(level,monsterIndex);
}

bool CommonFightEngine::canDoFightAction()
{
    if(randomSeedsSize()>5)
        return true;
    else
        return false;
}

bool CommonFightEngine::haveMoreEndurance()
{
    PlayerMonster * currentMonster=getCurrentMonster();
    if(currentMonster==NULL)
    {
        errorFightEngine("Unable to locate the current monster");
        return false;
    }
    unsigned int index=0;
    while(index<currentMonster->skills.size())
    {
        if(currentMonster->skills.at(index).endurance>0)
            return true;
        index++;
    }
    return false;
}

bool CommonFightEngine::currentMonsterAttackFirst(const PlayerMonster * currentMonster,const PublicPlayerMonster * otherMonster) const
{
    Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(currentMonster->monster),currentMonster->level);
    Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(otherMonster->monster),otherMonster->level);
    bool currentMonsterStatIsFirstToAttack=false;
    if(currentMonsterStat.speed>=otherMonsterStat.speed)
        currentMonsterStatIsFirstToAttack=true;
    return currentMonsterStatIsFirstToAttack;
}

void CommonFightEngine::startTheFight()
{
    doTurnIfChangeOfMonster=true;
    updateCanDoFight();
}

Skill::AttackReturn CommonFightEngine::doTheCurrentMonsterAttack(const uint16_t &skill,const uint8_t &skillLevel)
{
    return genericMonsterAttack(getCurrentMonster(),getOtherMonster(),skill,skillLevel);
}
