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
    unsigned int index=0;
    while(index<public_and_private_informations.playerMonster.size())
    {
        if(public_and_private_informations.playerMonster.at(index).egg_step==0)
        {
            const Monster::Stat &stat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(public_and_private_informations.playerMonster.at(index).monster),public_and_private_informations.playerMonster.at(index).level);
            public_and_private_informations.playerMonster[index].hp=stat.hp;
            public_and_private_informations.playerMonster[index].buffs.clear();
            unsigned int sub_index=0;
            while(sub_index<public_and_private_informations.playerMonster.at(index).skills.size())
            {
                const PlayerMonster::PlayerSkill &playerSkill=public_and_private_informations.playerMonster.at(index).skills.at(sub_index);
                const int &skill=playerSkill.skill;
                const int &skillIndex=playerSkill.level-1;
                if(CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.find(skill)!=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.cend())
                {
                    const Skill &fullSkill=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.at(skill);
                    public_and_private_informations.playerMonster[index].skills[sub_index].endurance=
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

bool CommonFightEngine::learnSkill(PlayerMonster *monsterPlayer, const uint16_t &skill)
{
    //search if have already the previous skill
    uint16_t sub_index2=0;
    while(sub_index2<monsterPlayer->skills.size())
    {
        if(monsterPlayer->skills.at(sub_index2).skill==skill)
            break;
        sub_index2++;
    }
    uint16_t sub_index=0;
    while(sub_index<CatchChallenger::CommonDatapack::commonDatapack.monsters.at(monsterPlayer->monster).learn.size())
    {
        const Monster::AttackToLearn &learn=CatchChallenger::CommonDatapack::commonDatapack.monsters.at(monsterPlayer->monster).learn.at(sub_index);
        if(learn.learnAtLevel<=monsterPlayer->level && learn.learnSkill==skill)
        {
            if(
                    //if skill not found
                    (sub_index2==monsterPlayer->skills.size() && learn.learnSkillLevel==1)
                    ||
                    //if skill already found and need level up
                    (sub_index2<monsterPlayer->skills.size() && (monsterPlayer->skills.at(sub_index2).level+1)==learn.learnSkillLevel)
                    )
            {
                if(CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.find(learn.learnSkill)==CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.cend())
                {
                    errorFightEngine("Skill to learn not found into learnSkill()");
                    return false;
                }
                if(learn.learnSkillLevel>CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.at(learn.learnSkill).level.size())
                {
                    errorFightEngine("Skill level to learn not found learnSkill() "+std::to_string(CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.at(learn.learnSkill).level.size())+">"+std::to_string(learn.learnSkillLevel));
                    return false;
                }
                if(CommonSettingsServer::commonSettingsServer.useSP)
                {
                    const uint32_t &sp=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.at(learn.learnSkill).level.at(learn.learnSkillLevel-1).sp_to_learn;
                    if(sp>monsterPlayer->sp)
                        return false;
                    monsterPlayer->sp-=sp;
                }
                if(learn.learnSkillLevel==1)
                {
                    PlayerMonster::PlayerSkill temp;
                    temp.skill=skill;
                    temp.level=1;
                    temp.endurance=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.at(learn.learnSkill).level.at(learn.learnSkillLevel-1).endurance;
                    addSkill(monsterPlayer,temp);
                }
                else
                    setSkillLevel(monsterPlayer,sub_index2,monsterPlayer->skills.at(sub_index2).level+1);
                return true;
            }
        }
        sub_index++;
    }
    return false;
}

bool CommonFightEngine::learnSkillByItem(PlayerMonster *playerMonster, const uint32_t &itemId)
{
    if(CatchChallenger::CommonDatapack::commonDatapack.monsters.find(playerMonster->monster)==CatchChallenger::CommonDatapack::commonDatapack.monsters.cend())
    {
        errorFightEngine("Monster id not found into learnSkillByItem()");
        return false;
    }
    if(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(playerMonster->monster).learnByItem.find(itemId)==CatchChallenger::CommonDatapack::commonDatapack.monsters.at(playerMonster->monster).learnByItem.cend())
    {
        errorFightEngine("Item id not found into learnSkillByItem()");
        return false;
    }
    const Monster::AttackToLearnByItem &attackToLearnByItem=CatchChallenger::CommonDatapack::commonDatapack.monsters.at(playerMonster->monster).learnByItem.at(itemId);
    if(CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.find(attackToLearnByItem.learnSkill)==CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.cend())
    {
        errorFightEngine("Skill to learn not found into learnSkill()");
        return false;
    }
    if(CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.at(attackToLearnByItem.learnSkill).level.size()>attackToLearnByItem.learnSkillLevel)
    {
        errorFightEngine("Skill level to learn not found learnSkill()");
        return false;
    }
    //search if have already the previous skill
    unsigned int index=0;
    while(index<playerMonster->skills.size())
    {
        if(playerMonster->skills.at(index).skill==attackToLearnByItem.learnSkill)
        {
            if(playerMonster->skills.at(index).level>=attackToLearnByItem.learnSkillLevel)
                return false;
            else
            {
                setSkillLevel(playerMonster,index,attackToLearnByItem.learnSkillLevel);
                return true;
            }
        }
        index++;
    }
    PlayerMonster::PlayerSkill temp;
    temp.skill=attackToLearnByItem.learnSkill;
    temp.level=attackToLearnByItem.learnSkillLevel;
    temp.endurance=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.at(attackToLearnByItem.learnSkill).level.at(attackToLearnByItem.learnSkillLevel-1).endurance;
    addSkill(playerMonster,temp);
    return true;
}

bool CommonFightEngine::addSkill(PlayerMonster * currentMonster,const PlayerMonster::PlayerSkill &skill)
{
    currentMonster->skills.push_back(skill);
    return true;
}

bool CommonFightEngine::setSkillLevel(PlayerMonster * currentMonster,const unsigned int &index,const uint8_t &level)
{
    if(index>=currentMonster->skills.size())
        return false;
    else
    {
        currentMonster->skills[index].level=level;
        return true;
    }
}

bool CommonFightEngine::removeSkill(PlayerMonster * currentMonster,const unsigned int &index)
{
    if(index>=currentMonster->skills.size())
        return false;
    else
    {
        currentMonster->skills.erase(currentMonster->skills.begin()+index);
        return true;
    }
}

bool CommonFightEngine::isInFight() const
{
    return !wildMonsters.empty() || !botFightMonsters.empty();
}

bool CommonFightEngine::isInFightWithWild() const
{
    return !wildMonsters.empty();
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

PlayerMonster CommonFightEngine::getRandomMonster(const std::vector<MapMonster> &monsterList,bool *ok)
{
    PlayerMonster playerMonster;
    playerMonster.catched_with=0;
    playerMonster.egg_step=0;
    playerMonster.remaining_xp=0;
    playerMonster.sp=0;
    uint8_t randomMonsterInt=getOneSeed(100);
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    uint8_t randomMonsterIntOriginal=randomMonsterInt;
    std::string monsterListString;
    #endif
    bool monsterFound=false;
    unsigned int index=0;
    while(index<monsterList.size())
    {
        const MapMonster &mapMonster=monsterList.at(index);
        const int &luck=mapMonster.luck;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        monsterListString="luck:"+std::to_string(mapMonster.luck)+";id:"+std::to_string(mapMonster.id)+";";
        #endif
        if(randomMonsterInt<=luck)// with < it crash because randomMonsterInt can be 0
        {
            //it's this monster
            playerMonster.monster=mapMonster.id;
            //select the level
            if(mapMonster.maxLevel==mapMonster.minLevel)
                playerMonster.level=mapMonster.minLevel;
            else
                playerMonster.level=getOneSeed(mapMonster.maxLevel-mapMonster.minLevel+1)+mapMonster.minLevel;
            monsterFound=true;
            break;
        }
        else
            randomMonsterInt-=luck;
        index++;
    }
    if(!monsterFound)
    {
        if(monsterList.empty())
        {
            #ifndef CATCHCHALLENGER_EXTRA_CHECK
            errorFightEngine("error: no wild monster selected, with: randomMonsterInt: "+std::to_string(randomMonsterInt));
            #else
            errorFightEngine("error: no wild monster selected, with: randomMonsterIntOriginal: "+std::to_string(randomMonsterIntOriginal)+", monsterListString: "+monsterListString);
            #endif

            *ok=false;
            playerMonster.monster=0;
            playerMonster.level=0;
            playerMonster.gender=Gender_Unknown;
            return playerMonster;
        }
        else
        {
            #ifndef CATCHCHALLENGER_EXTRA_CHECK
            messageFightEngine("warning: no wild monster selected, with: randomMonsterInt: "+std::to_string(randomMonsterInt));
            #else
            messageFightEngine("warning: no wild monster selected, with: randomMonsterIntOriginal: "+std::to_string(randomMonsterIntOriginal)+", monsterListString: "+monsterListString);
            #endif
            playerMonster.monster=monsterList.front().id;
            //select the level
            if(monsterList.front().maxLevel==monsterList.front().minLevel)
                playerMonster.level=monsterList.front().minLevel;
            else
                playerMonster.level=getOneSeed(monsterList.front().maxLevel-monsterList.front().minLevel+1)+monsterList.front().minLevel;
        }
    }
    Monster monsterDef=CommonDatapack::commonDatapack.monsters.at(playerMonster.monster);
    if(monsterDef.ratio_gender>0 && monsterDef.ratio_gender<100)
    {
        int8_t temp_ratio=getOneSeed(101);
        if(temp_ratio<monsterDef.ratio_gender)
            playerMonster.gender=Gender_Male;
        else
            playerMonster.gender=Gender_Female;
    }
    else
    {
        switch(monsterDef.ratio_gender)
        {
            case 0:
                playerMonster.gender=Gender_Male;
            break;
            case 100:
                playerMonster.gender=Gender_Female;
            break;
            default:
                playerMonster.gender=Gender_Unknown;
            break;
        }
    }
    Monster::Stat monsterStat=getStat(monsterDef,playerMonster.level);
    playerMonster.hp=monsterStat.hp;
    #ifdef CATCHCHALLENGER_DEBUG_FIGHT
    messageFightEngine("do skill while: playerMonster.skills.size()<CATCHCHALLENGER_MONSTER_WILD_SKILL_NUMBER: "+
                       std::to_string(playerMonster.skills.size())+"<"+std::to_string(CATCHCHALLENGER_MONSTER_WILD_SKILL_NUMBER));
    #endif
    playerMonster.skills=CommonFightEngine::generateWildSkill(monsterDef,playerMonster.level);
    #ifdef CATCHCHALLENGER_DEBUG_FIGHT
    {
        if(playerMonster.skills.empty())
        {
            if(monsterDef.learn.empty())
                messageFightEngine("no skill to learn for random monster");
            else
            {
                messageFightEngine("no skill for random monster, but skill to learn:");
                unsigned int index=0;
                while(index<monsterDef.learn.size())
                {
                    messageFightEngine(std::to_string(monsterDef.learn.at(index).learnSkill)+" level "+
                                       std::to_string(monsterDef.learn.at(index).learnSkillLevel)+" for monster at level "+
                                       std::to_string(monsterDef.learn.at(index).learnAtLevel));
                    index++;
                }
            }
        }
        else
        {
            unsigned int index=0;
            while(index<playerMonster.skills.size())
            {
                messageFightEngine("skill for random monster: "+std::to_string(playerMonster.skills.at(index).skill)+" level "+std::to_string(playerMonster.skills.at(index).level));
                index++;
            }
        }
        messageFightEngine("random monster id: "+std::to_string(playerMonster.monster));
    }
    #endif
    *ok=true;
    return playerMonster;
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

PlayerMonster * CommonFightEngine::getCurrentMonster()
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

Skill::LifeEffectReturn CommonFightEngine::applyLifeEffect(const uint8_t &type,const Skill::LifeEffect &effect,PublicPlayerMonster *currentMonster,PublicPlayerMonster *otherMonster)
{
    Skill::LifeEffectReturn effect_to_return;
    int32_t quantity;
    const Monster &commonMonster=CatchChallenger::CommonDatapack::commonDatapack.monsters.at(currentMonster->monster);
    const Monster &commonOtherMonster=CatchChallenger::CommonDatapack::commonDatapack.monsters.at(otherMonster->monster);
    Monster::Stat stat=getStat(commonMonster,currentMonster->level);
    Monster::Stat otherStat=getStat(commonOtherMonster,otherMonster->level);
    switch(effect.on)
    {
        case ApplyOn_AloneEnemy:
        case ApplyOn_AllEnemy:
        break;
        case ApplyOn_Themself:
        case ApplyOn_AllAlly:
            otherStat=stat;
            otherMonster=currentMonster;
        break;
        default:
            errorFightEngine("Not apply match, can't apply the buff");
        break;
    }
    if(effect.quantity==0)
        quantity=0;
    else if(effect.type==QuantityType_Quantity)
    {
        if(effect.quantity>0)
            quantity=effect.quantity;
        else
        {
            const Type &typeDefinition=CatchChallenger::CommonDatapack::commonDatapack.types.at(type);
            float OtherMulti=1.0;
            if(vectorcontainsAtLeastOne(commonMonster.type,type))
            {
                std::cout << "1.45x because the attack is same type as the current monster" << typeDefinition.name << std::endl;
                OtherMulti*=1.45;
            }
            effect_to_return.effective=1.0;
            const std::vector<uint8_t> &typeList=CatchChallenger::CommonDatapack::commonDatapack.monsters.at(otherMonster->monster).type;
            if(type!=255 && !typeList.empty())
            {
                unsigned int index=0;
                while(index<typeList.size())
                {
                    if(typeDefinition.multiplicator.find(typeList.at(index))!=typeDefinition.multiplicator.cend())
                    {
                        const int8_t &multiplicator=typeDefinition.multiplicator.at(typeList.at(index));
                        if(multiplicator>0)
                        {
                            #ifdef CATCHCHALLENGER_DEBUG_FIGHT
                            std::cout << "type: " << typeList.at(index) << "very effective againts" << typeDefinition.name << std::endl;
                            #endif
                            effect_to_return.effective*=multiplicator;
                        }
                        else
                        {
                            #ifdef CATCHCHALLENGER_DEBUG_FIGHT
                            std::cout << "type: " << typeList.at(index) << "not effective againts" << typeDefinition.name << std::endl;
                            #endif
                            effect_to_return.effective/=-multiplicator;
                        }
                    }
                    index++;
                }
            }
            float criticalHit=1.0;
            effect_to_return.critical=(getOneSeed(255)<20);
            if(effect_to_return.critical)
            {
                #ifdef CATCHCHALLENGER_DEBUG_FIGHT
                std::cout << "critical hit, then: 1.5x" << std::endl;
                #endif
                criticalHit=1.5;
            }
            uint32_t attack;
            uint32_t defense;
            if(type==0)
                attack=stat.attack;
            else
                attack=stat.special_attack;
            if(type==0)
                defense=otherStat.defense;
            else
                defense=otherStat.special_defense;
            if(defense<1)
                defense=1;
            const uint8_t &seed=getOneSeed(17);
            quantity = effect_to_return.effective*(((float)currentMonster->level*(float)1.99+10.5)/(float)255*((float)attack/(float)defense)*(float)effect.quantity)*criticalHit*OtherMulti*(100-seed)/100;
            #ifdef CATCHCHALLENGER_DEBUG_FIGHT
            {
                int32_t effect_quantity=effect.quantity;
                if(effect_quantity<0)
                    effect_quantity-=2;
            }
            std::cout << "quantity(" << quantity << ") = effect_to_return.effective(" << effect_to_return.effective << ")*(((float)currentMonster->level(" << currentMonster->level << ")*(float)1.99+10.5)/(float)255*((float)attack(" << attack << ")/(float)defense(" << defense << "))*(float)effect.quantity(" << effect.quantity << "))*criticalHit(" << criticalHit << ")*OtherMulti(" << OtherMulti << ")*(100-getOneSeed(17)(" << seed << "))/100; effect_quantity: " << effect_quantity << std::endl;
            #endif
            /*if(effect.quantity<0)
                quantity=-((-effect.quantity*stat.attack)/(otherStat.defense*4));
            else if(effect.quantity>0)//ignore the def for heal
                quantity=effect.quantity*otherMonster->level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
            const std::vector<uint8_t> &typeList=CatchChallenger::CommonDatapack::commonDatapack.monsters.at(otherMonster->monster).type;
            if(type!=255 && !typeList.empty())
            {
                unsigned int index=0;
                while(index<typeList.size())
                {
                    const Type &typeDefinition=CatchChallenger::CommonDatapack::commonDatapack.types.at(typeList.at(index));
                    if(typeDefinition.multiplicator.contains(type))
                    {
                        const int8_t &multiplicator=typeDefinition.multiplicator.at(type);
                        if(multiplicator>=0)
                            quantity*=multiplicator;
                        else
                            quantity/=-multiplicator;
                    }
                    index++;
                }
            }*/
        }
    }
    else
        quantity=((int64_t)currentMonster->hp*(int64_t)effect.quantity)/(int64_t)100;
    if(effect.quantity<0)
    {
        if(quantity==0)
            quantity=-1;
    }
    else if(effect.quantity>0)
    {
        if(quantity==0)
            quantity=1;
    }
    //check
    if(effect.quantity<0 && quantity>0)
    {
        quantity=-1;
        errorFightEngine("Wrong calculated value");
    }
    if(effect.quantity>0 && quantity<0)
    {
        quantity=1;
        errorFightEngine("Wrong calculated value");
    }
    #ifdef CATCHCHALLENGER_DEBUG_FIGHT
    std::cout << "Life: Apply on" << otherMonster->monster << "quantity" << quantity << "hp was:" << otherMonster->hp << std::endl;
    #endif
    //kill
    if(quantity<0 && (-quantity)>=(int32_t)otherMonster->hp)
    {
        quantity=-(int32_t)otherMonster->hp;
        otherMonster->hp=0;
    }
    //full heal
    else if(quantity>0 && quantity>=(int32_t)(stat.hp-otherMonster->hp))
    {
        quantity=(int32_t)(stat.hp-otherMonster->hp);
        otherMonster->hp=stat.hp;
    }
    //other life change
    else
        otherMonster->hp+=quantity;
    #ifdef CATCHCHALLENGER_DEBUG_FIGHT
    std::cout << "Life: Apply on" << otherMonster->monster << "quantity" << quantity << "hp is now:" << otherMonster->hp << std::endl;
    #endif
    effect_to_return.critical=false;
    effect_to_return.on=effect.on;
    effect_to_return.quantity=quantity;
    return effect_to_return;
}

int CommonFightEngine::addOtherBuffEffect(const Skill::BuffEffect &effect)
{
    return addBuffEffectFull(effect,getOtherMonster(),getCurrentMonster());
}

int CommonFightEngine::addCurrentBuffEffect(const Skill::BuffEffect &effect)
{
    return addBuffEffectFull(effect,getCurrentMonster(),getOtherMonster());
}

/* return code:
 * >=0 if the index of the buff updated
 * -1 buff added
 * -2 same buff already found
 * -3 failed by hp
 * -4 failed by other
 * */
int CommonFightEngine::addBuffEffectFull(const Skill::BuffEffect &effect, PublicPlayerMonster *currentMonster, PublicPlayerMonster *otherMonster)
{
    if(CommonDatapack::commonDatapack.monsterBuffs.find(effect.buff)==CommonDatapack::commonDatapack.monsterBuffs.cend())
    {
        errorFightEngine("apply a unknown buff");
        return -4;
    }
    if(effect.level>CommonDatapack::commonDatapack.monsterBuffs.at(effect.buff).level.size())
    {
        errorFightEngine("apply buff level out of range");
        return -4;
    }
    PlayerBuff tempBuff;
    tempBuff.buff=effect.buff;
    tempBuff.level=effect.level;
    tempBuff.remainingNumberOfTurn=CommonDatapack::commonDatapack.monsterBuffs.at(effect.buff).level.at(effect.level-1).durationNumberOfTurn;
    unsigned int index=0;
    switch(effect.on)
    {
        case ApplyOn_AloneEnemy:
        case ApplyOn_AllEnemy:
            if(otherMonster->hp==0)
                return -3;
            while(index<otherMonster->buffs.size())
            {
                if(otherMonster->buffs.at(index).buff==effect.buff)
                {
                    if(otherMonster->buffs.at(index).level==effect.level)
                        return -2;
                    otherMonster->buffs[index].level=effect.level;
                        return index;
                }
                index++;
            }
            otherMonster->buffs.push_back(tempBuff);
            return -1;
        break;
        case ApplyOn_Themself:
        case ApplyOn_AllAlly:
            if(currentMonster->hp==0)
                return -3;
            while(index<currentMonster->buffs.size())
            {
                if(currentMonster->buffs.at(index).buff==effect.buff)
                {
                    if(currentMonster->buffs.at(index).level==effect.level)
                        return -2;
                    currentMonster->buffs[index].level=effect.level;
                        return index;
                }
                index++;
            }
            currentMonster->buffs.push_back(tempBuff);
            return -1;
        break;
        default:
            errorFightEngine("Not apply match, can't apply the buff");
            return -4;
        break;
    }
    return -4;
}

void CommonFightEngine::removeBuffEffectFull(const Skill::BuffEffect &effect)
{
    if(CommonDatapack::commonDatapack.monsterBuffs.find(effect.buff)==CommonDatapack::commonDatapack.monsterBuffs.cend())
    {
        errorFightEngine("apply a unknown buff");
        return;
    }
    if(effect.level>CommonDatapack::commonDatapack.monsterBuffs.at(effect.buff).level.size())
    {
        errorFightEngine("apply buff level out of range");
        return;
    }
    unsigned int index=0;
    switch(effect.on)
    {
        case ApplyOn_AloneEnemy:
        case ApplyOn_AllEnemy:
        {
            PublicPlayerMonster * otherMonster=getOtherMonster();
            if(otherMonster==NULL)
            {
                errorFightEngine("Other monster not found for buff remove");
                return;
            }
            while(index<otherMonster->buffs.size())
            {
                if(otherMonster->buffs.at(index).buff==effect.buff)
                {
                    if(otherMonster->buffs.at(index).level!=effect.level)
                        messageFightEngine("the buff removed "+std::to_string(effect.buff)+" have not the same level "+std::to_string(otherMonster->buffs.at(index).level)+"!="+std::to_string(effect.level));
                    otherMonster->buffs.erase(otherMonster->buffs.cbegin()+index);
                    return;
                }
                index++;
            }
        }
        break;
        case ApplyOn_Themself:
        case ApplyOn_AllAlly:
        {
            PublicPlayerMonster * currentMonster=getCurrentMonster();
            if(currentMonster==NULL)
            {
                errorFightEngine("Other monster not found for buff remove");
                return;
            }
            while(index<currentMonster->buffs.size())
            {
                if(currentMonster->buffs.at(index).buff==effect.buff)
                {
                    if(currentMonster->buffs.at(index).level!=effect.level)
                        messageFightEngine("the buff removed "+std::to_string(effect.buff)+" have not the same level "+std::to_string(currentMonster->buffs.at(index).level)+"!="+std::to_string(effect.level));
                    currentMonster->buffs.erase(currentMonster->buffs.cbegin()+index);
                    return;
                }
                index++;
            }
        }
        break;
        default:
            errorFightEngine("Not apply match, can't apply the buff");
            return;
        break;
    }
    errorFightEngine("Buff not found to remove");
}

void CommonFightEngine::confirmEvolutionTo(PlayerMonster * playerMonster,const uint32_t &monster)
{
    const Monster &monsterInformations=CommonDatapack::commonDatapack.monsters.at(playerMonster->monster);
    const Monster::Stat oldStat=getStat(monsterInformations,playerMonster->level);
    playerMonster->monster=monster;
    const Monster::Stat stat=getStat(CommonDatapack::commonDatapack.monsters.at(playerMonster->monster),playerMonster->level);
    if(oldStat.hp>stat.hp)
    {
        if(playerMonster->hp>stat.hp)
            playerMonster->hp=stat.hp;
    }
    else if(oldStat.hp<stat.hp)
    {
        if(playerMonster->hp>0)
            playerMonster->hp+=(stat.hp-oldStat.hp);
    }
}

/// \note Object quantity verified in LocalClientHandler::useObjectOnMonster()
bool CommonFightEngine::useObjectOnMonster(const uint16_t &object, const uint32_t &monster)
{
    if(!haveThisMonster(monster))
    {
        errorFightEngine("have not this monster: "+std::to_string(object));
        return false;
    }
    PlayerMonster * playerMonster=monsterById(monster);
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

bool CommonFightEngine::removeBuffOnMonster(PlayerMonster * currentMonster, const uint32_t &buffId)
{
    unsigned int index=0;
    while(index<currentMonster->buffs.size())
    {
        const PlayerBuff &playerBuff=currentMonster->buffs.at(index);
        if(playerBuff.buff==buffId)
        {
            currentMonster->buffs.erase(currentMonster->buffs.begin()+index);
            return true;
        }
    }
    return false;
}

bool CommonFightEngine::removeAllBuffOnMonster(PlayerMonster * currentMonster)
{
    if(!currentMonster->buffs.empty())
    {
        currentMonster->buffs.clear();
        return true;
    }
    return false;
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

bool CommonFightEngine::internalTryCapture(const Trap &trap)
{
    if(wildMonsters.empty())
    {
        errorFightEngine("Not againts wild monster");
        return false;
    }
    const Monster::Stat &wildMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(wildMonsters.front().monster),wildMonsters.front().level);

    float bonusStat=1.0;
    if(wildMonsters.front().buffs.size())
    {
        bonusStat=0;
        unsigned int index=0;
        while(index<wildMonsters.front().buffs.size())
        {
            const PlayerBuff &playerBuff=wildMonsters.front().buffs.at(index);
            if(CatchChallenger::CommonDatapack::commonDatapack.monsterBuffs.find(playerBuff.buff)!=CatchChallenger::CommonDatapack::commonDatapack.monsterBuffs.cend())
            {
                const Buff &buff=CatchChallenger::CommonDatapack::commonDatapack.monsterBuffs.at(playerBuff.buff);
                if(playerBuff.level>0 && playerBuff.level<=buff.level.size())
                    bonusStat+=buff.level.at(playerBuff.level-1).capture_bonus;
                else
                {
                    errorFightEngine("Buff level for wild monter not found: "+std::to_string(playerBuff.buff)+" at level "+std::to_string(playerBuff.level));
                    bonusStat+=1.0;
                }
            }
            else
            {
                errorFightEngine("Buff for wild monter not found: "+std::to_string(playerBuff.buff));
                bonusStat+=1.0;
            }
            index++;
        }
        bonusStat/=wildMonsters.front().buffs.size();
    }
    const uint32_t maxTempRate=12;
    const uint32_t minTempRate=5;
    const uint32_t tryCapture=4;
    const uint32_t catch_rate=(uint32_t)CatchChallenger::CommonDatapack::commonDatapack.monsters.at(wildMonsters.front().monster).catch_rate;
    uint32_t tempRate=(catch_rate*(wildMonsterStat.hp*maxTempRate-wildMonsters.front().hp*minTempRate)*bonusStat*trap.bonus_rate)/(wildMonsterStat.hp*maxTempRate);
    if(tempRate>255)
        return true;
    uint32_t realRate=65535*std::pow((float)tempRate/(float)255,0.25);

    messageFightEngine("Formule: ("+std::to_string(catch_rate)+
                       "*("+std::to_string(wildMonsterStat.hp*maxTempRate)+
                       "-"+std::to_string(minTempRate*wildMonsters.front().hp)+
                       ")*"+std::to_string(bonusStat)+
                       "*"+std::to_string(trap.bonus_rate)+
                       ")/("+std::to_string(wildMonsterStat.hp*maxTempRate)+
                       "), realRate: "+std::to_string(realRate)
                       );

    uint32_t index=0;
    while(index<tryCapture)
    {
        uint16_t number=rand()%65536;
        if(number>=realRate)
            return false;
        index++;
    }
    return true;
}

uint32_t CommonFightEngine::tryCapture(const uint16_t &item)
{
    doTurnIfChangeOfMonster=true;
    if(internalTryCapture(CommonDatapack::commonDatapack.items.trap.at(item)))
    {
        #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
        messageFightEngine("capture is successful");
        #endif
        PlayerMonster newMonster;
        newMonster.buffs=wildMonsters.front().buffs;
        newMonster.catched_with=item;
        newMonster.egg_step=0;
        newMonster.gender=wildMonsters.front().gender;
        newMonster.hp=wildMonsters.front().hp;
        newMonster.level=wildMonsters.front().level;
        newMonster.monster=wildMonsters.front().monster;
        newMonster.remaining_xp=0;
        newMonster.skills=wildMonsters.front().skills;
        newMonster.sp=0;
        newMonster.id=catchAWild(public_and_private_informations.playerMonster.size()>=CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters,newMonster);
        return newMonster.id;
    }
    else
    {
        #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
        messageFightEngine("capture is failed");
        #endif
        generateOtherAttack();//Skill::AttackReturn attackReturn=
        return 0;
    }
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

void CommonFightEngine::addPlayerMonster(const std::vector<PlayerMonster> &playerMonster)
{
    public_and_private_informations.playerMonster.insert(public_and_private_informations.playerMonster.cend(),playerMonster.cbegin(),playerMonster.cend());
    updateCanDoFight();
}

void CommonFightEngine::addPlayerMonster(const PlayerMonster &playerMonster)
{
    public_and_private_informations.playerMonster.push_back(playerMonster);
    updateCanDoFight();
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

bool CommonFightEngine::removeMonster(const uint32_t &monsterId)
{
    unsigned int index=0;
    while(index<public_and_private_informations.playerMonster.size())
    {
        if(public_and_private_informations.playerMonster.at(index).id==monsterId)
        {
            public_and_private_informations.playerMonster.erase(public_and_private_informations.playerMonster.cbegin()+index);
            updateCanDoFight();
            return true;
        }
        index++;
    }
    return false;
}

bool CommonFightEngine::haveThisMonster(const uint32_t &monsterId) const
{
    unsigned int index=0;
    while(index<public_and_private_informations.playerMonster.size())
    {
        if(public_and_private_informations.playerMonster.at(index).id==monsterId)
            return true;
        index++;
    }
    return false;
}

PlayerMonster * CommonFightEngine::monsterById(const uint32_t &monsterId)
{
    unsigned int index=0;
    while(index<public_and_private_informations.playerMonster.size())
    {
        if(public_and_private_informations.playerMonster.at(index).id==monsterId)
            return &public_and_private_informations.playerMonster[index];
        index++;
    }
    return NULL;
}

PlayerMonster * CommonFightEngine::monsterByPosition(const uint8_t &monsterPosition)
{
    if(monsterPosition>=public_and_private_informations.playerMonster.size())
        return NULL;
    else
        return &public_and_private_informations.playerMonster[monsterPosition];
}

void CommonFightEngine::wildDrop(const uint32_t &)
{
}

bool CommonFightEngine::checkKOOtherMonstersForGain()
{
    messageFightEngine("checkKOOtherMonstersForGain()");
    bool winTheTurn=false;
    if(!wildMonsters.empty())
    {
        if(wildMonsters.front().hp==0)
        {
            winTheTurn=true;
            #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
            messageFightEngine("The wild monster ("+std::to_string(wildMonsters.front().monster)+") is KO");
            #endif
            //drop the drop item here
            wildDrop(wildMonsters.front().monster);
            //give xp/sp here
            const Monster &wildmonster=CommonDatapack::commonDatapack.monsters.at(wildMonsters.front().monster);
            //multiplicator do at datapack loading
            int sp=wildmonster.give_sp*wildMonsters.front().level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
            int xp=wildmonster.give_xp*wildMonsters.front().level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
            giveXPSP(xp,sp);
            #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
            messageFightEngine("You win "+std::to_string(wildmonster.give_xp*wildMonsters.front().level/CATCHCHALLENGER_MONSTER_LEVEL_MAX)+
                               " xp and "+std::to_string(wildmonster.give_sp*wildMonsters.front().level/CATCHCHALLENGER_MONSTER_LEVEL_MAX)+" sp");
            #endif
        }
    }
    else if(!botFightMonsters.empty())
    {
        if(botFightMonsters.front().hp==0)
        {
            winTheTurn=true;
            #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
            messageFightEngine("The wild monster ("+std::to_string(botFightMonsters.front().monster)+") is KO");
            #endif
            //don't drop item because it's not a wild fight
            //give xp/sp here
            const Monster &botmonster=CommonDatapack::commonDatapack.monsters.at(botFightMonsters.front().monster);
            //multiplicator do at datapack loading
            int sp=botmonster.give_sp*botFightMonsters.front().level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
            int xp=botmonster.give_xp*botFightMonsters.front().level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
            giveXPSP(xp,sp);
            #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
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
        const uint32_t &old_max_hp=getStat(monsterInformations,level).hp;
        const uint32_t &new_max_hp=getStat(monsterInformations,level+1).hp;
        xp-=monsterInformations.level_to_xp.at(level-1)-remaining_xp;
        remaining_xp=0;
        level++;
        levelUp(level,getCurrentSelectedMonsterNumber());
        haveChangeOfLevel=true;
        if(new_max_hp>old_max_hp)
            monster->hp+=new_max_hp-old_max_hp;
        #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
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
    #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
    messageFightEngine("You pass to the level "+std::to_string(level)+" on your monster "+std::to_string(monster->id));
    #endif

    if((public_and_private_informations.playerMonster.at(monsterIndex).level+numberOfLevel)>=CATCHCHALLENGER_MONSTER_LEVEL_MAX)
        public_and_private_informations.playerMonster[monsterIndex].level=CATCHCHALLENGER_MONSTER_LEVEL_MAX;
    else
        public_and_private_informations.playerMonster[monsterIndex].level+=numberOfLevel;
    public_and_private_informations.playerMonster[monsterIndex].remaining_xp=0;
    levelUp(level,monsterIndex);
    return true;
}

std::vector<Monster::AttackToLearn> CommonFightEngine::autoLearnSkill(const uint8_t &level,const uint8_t &monsterIndex)
{
    std::vector<Monster::AttackToLearn> returnVar;
    const PlayerMonster &monster=public_and_private_informations.playerMonster.at(monsterIndex);
    unsigned int sub_index=0;
    unsigned int sub_index2=0;
    const unsigned int &learn_size=CatchChallenger::CommonDatapack::commonDatapack.monsters.at(monster.monster).learn.size();
    while(sub_index<learn_size)
    {
        const Monster::AttackToLearn &learn=CatchChallenger::CommonDatapack::commonDatapack.monsters.at(monster.monster).learn.at(sub_index);
        if(learn.learnAtLevel==level)
        {
            //search if have already the previous skill
            sub_index2=0;
            const unsigned int &monster_skill_size=monster.skills.size();
            while(sub_index2<monster_skill_size)
            {
                if(monster.skills.at(sub_index2).skill==learn.learnSkill)
                {
                    if(monster.skills.at(sub_index2).level<learn.learnSkillLevel)
                    {
                        setSkillLevel(&public_and_private_informations.playerMonster[monsterIndex],sub_index2,learn.learnSkillLevel);
                        returnVar.push_back(learn);
                    }
                    break;
                }
                sub_index2++;
            }
            if(sub_index2==monster_skill_size)
            {
                if(learn.learnSkillLevel==1)
                {
                    PlayerMonster::PlayerSkill temp;
                    temp.skill=learn.learnSkill;
                    temp.level=1;
                    temp.endurance=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.at(learn.learnSkill).level.at(learn.learnSkillLevel-1).endurance;
                    addSkill(&public_and_private_informations.playerMonster[monsterIndex],temp);
                    returnVar.push_back(learn);
                }
            }
        }
        sub_index++;
    }
    return returnVar;
}

void CommonFightEngine::levelUp(const uint8_t &level,const uint8_t &monsterIndex)//call after done the level
{
    autoLearnSkill(level,monsterIndex);
}

bool CommonFightEngine::tryEscape()
{
    if(!canEscape())//check if is in fight
        return false;
    if(internalTryEscape())
    {
        #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
        messageFightEngine("escape is successful");
        #endif
        wildMonsters.clear();
        return true;
    }
    else
    {
        #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
        messageFightEngine("escape is failed");
        #endif
        generateOtherAttack();//Skill::AttackReturn attackReturn=
        return false;
    }
}

bool CommonFightEngine::canDoFightAction()
{
    if(randomSeedsSize()>5)
        return true;
    else
        return false;
}

bool CommonFightEngine::doTheOtherMonsterTurn()
{
    generateOtherAttack();
    if(currentMonsterIsKO())
        return true;
    else
    {
        if(otherMonsterIsKO())
        {
            checkKOOtherMonstersForGain();
            return true;
        }
    }
    return false;
}

void CommonFightEngine::doTheTurn(const uint32_t &skill,const uint8_t &skillLevel,const bool currentMonsterStatIsFirstToAttack)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        PlayerMonster * currentMonster=getCurrentMonster();
        if(currentMonster==NULL)
        {
            errorFightEngine("Unable to locate the current monster");
            return;
        }
        if(currentMonsterIsKO())
        {
            errorFightEngine("Can't attack with KO monster");
            return;
        }
        const PublicPlayerMonster * otherMonster=getOtherMonster();
        if(otherMonster==NULL)
        {
            errorFightEngine("Unable to locate the other monster");
            return;
        }
        Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(currentMonster->monster),currentMonster->level);
        Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(otherMonster->monster),otherMonster->level);
        if(currentMonster->hp>currentMonsterStat.hp)
        {
            errorFightEngine("The hp "+std::to_string(currentMonster->hp)+
                             " of current monster "+std::to_string(currentMonster->monster)+
                             " at level "+std::to_string(currentMonster->level)+
                             " is greater than the max "+std::to_string(currentMonsterStat.hp)+" the skill use doTheTurn() at 0)");
            return;
        }
        if(otherMonster->hp>otherMonsterStat.hp)
        {
            errorFightEngine("The hp "+std::to_string(otherMonster->hp)+
                             " of other monster "+std::to_string(otherMonster->monster)+
                             " at level "+std::to_string(otherMonster->level)+
                             " is greater than the max "+std::to_string(otherMonsterStat.hp)+
                             " the skill use doTheTurn() at 0)");
            return;
        }
    }
    #endif
    bool turnIsEnd=false;
    if(currentMonsterStatIsFirstToAttack)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        {
            PlayerMonster * currentMonster=getCurrentMonster();
            if(currentMonster==NULL)
            {
                errorFightEngine("Unable to locate the current monster");
                return;
            }
            if(currentMonsterIsKO())
            {
                errorFightEngine("Can't attack with KO monster");
                return;
            }
            const PublicPlayerMonster * otherMonster=getOtherMonster();
            if(otherMonster==NULL)
            {
                errorFightEngine("Unable to locate the other monster");
                return;
            }
            Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(currentMonster->monster),currentMonster->level);
            Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(otherMonster->monster),otherMonster->level);
            if(currentMonster->hp>currentMonsterStat.hp)
            {
                errorFightEngine("The hp "+std::to_string(currentMonster->hp)+
                                 " of current monster "+std::to_string(currentMonster->monster)+
                                 " at level "+std::to_string(currentMonster->level)+
                                 " is greater than the max "+std::to_string(currentMonsterStat.hp)+
                                 " before the skill use doTheTurn() at 1)");
                return;
            }
            if(otherMonster->hp>otherMonsterStat.hp)
            {
                errorFightEngine("The hp "+std::to_string(otherMonster->hp)+
                                 " of other monster "+std::to_string(otherMonster->monster)+
                                 " at level "+std::to_string(otherMonster->level)+
                                 " is greater than the max "+std::to_string(otherMonsterStat.hp)+
                                 " before the skill use doTheTurn() at 1)");
                return;
            }
        }
        #endif
        doTheCurrentMonsterAttack(skill,skillLevel);
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        {
            PlayerMonster * currentMonster=getCurrentMonster();
            if(currentMonster==NULL)
            {
                errorFightEngine("Unable to locate the current monster");
                return;
            }
            if(currentMonsterIsKO())
            {
                errorFightEngine("Can't attack with KO monster");
                return;
            }
            const PublicPlayerMonster * otherMonster=getOtherMonster();
            if(otherMonster==NULL)
            {
                errorFightEngine("Unable to locate the other monster");
                return;
            }
            Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(currentMonster->monster),currentMonster->level);
            Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(otherMonster->monster),otherMonster->level);
            if(currentMonster->hp>currentMonsterStat.hp)
            {
                errorFightEngine("The hp "+std::to_string(currentMonster->hp)+
                                 " of current monster "+std::to_string(currentMonster->monster)+
                                 " at level "+std::to_string(currentMonster->level)+
                                 " is greater than the max "+std::to_string(currentMonsterStat.hp)+
                                 " after the skill use doTheTurn() at 1)");
                return;
            }
            if(otherMonster->hp>otherMonsterStat.hp)
            {
                errorFightEngine("The hp "+std::to_string(otherMonster->hp)+
                                 " of other monster "+std::to_string(otherMonster->monster)+
                                 " at level "+std::to_string(otherMonster->level)+
                                 " is greater than the max "+std::to_string(otherMonsterStat.hp)+
                                 " after the skill use doTheTurn() at 1)");
                return;
            }
        }
        #endif
        if(currentMonsterIsKO())
            turnIsEnd=true;
        else
        {
            if(otherMonsterIsKO())
            {
                checkKOOtherMonstersForGain();
                turnIsEnd=true;
            }
        }
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        PlayerMonster * currentMonster=getCurrentMonster();
        if(currentMonster==NULL)
        {
            errorFightEngine("Unable to locate the current monster");
            return;
        }
        const PublicPlayerMonster * otherMonster=getOtherMonster();
        if(otherMonster==NULL)
        {
            errorFightEngine("Unable to locate the other monster");
            return;
        }
        Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(currentMonster->monster),currentMonster->level);
        Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(otherMonster->monster),otherMonster->level);
        if(currentMonster->hp>currentMonsterStat.hp)
        {
            errorFightEngine("The hp "+std::to_string(currentMonster->hp)+
                             " of current monster "+std::to_string(currentMonster->monster)+
                             " at level "+std::to_string(currentMonster->level)+
                             " is greater than the max "+std::to_string(currentMonsterStat.hp)+
                             " after the skill use doTheTurn() at 1)");
            return;
        }
        if(otherMonster->hp>otherMonsterStat.hp)
        {
            errorFightEngine("The hp "+std::to_string(otherMonster->hp)+
                             " of other monster "+std::to_string(otherMonster->monster)+
                             " at level "+std::to_string(otherMonster->level)+
                             " is greater than the max "+std::to_string(otherMonsterStat.hp)+
                             " after the skill use doTheTurn() at 1)");
            return;
        }
    }
    #endif
    //do the other monster attack
    if(!turnIsEnd)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        {
            PlayerMonster * currentMonster=getCurrentMonster();
            if(currentMonster==NULL)
            {
                errorFightEngine("Unable to locate the current monster");
                return;
            }
            if(currentMonsterIsKO())
            {
                errorFightEngine("Can't attack with KO monster");
                return;
            }
            const PublicPlayerMonster * otherMonster=getOtherMonster();
            if(otherMonster==NULL)
            {
                errorFightEngine("Unable to locate the other monster");
                return;
            }
            Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(currentMonster->monster),currentMonster->level);
            Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(otherMonster->monster),otherMonster->level);
            if(currentMonster->hp>currentMonsterStat.hp)
            {
                errorFightEngine("The hp "+std::to_string(currentMonster->hp)+
                                 " of current monster "+std::to_string(currentMonster->monster)+
                                 " at level "+std::to_string(currentMonster->level)+
                                 " is greater than the max "+std::to_string(currentMonsterStat.hp)+
                                 " after the skill use doTheTurn() at 1)");
                return;
            }
            if(otherMonster->hp>otherMonsterStat.hp)
            {
                errorFightEngine("The hp "+std::to_string(otherMonster->hp)+
                                 " of other monster "+std::to_string(otherMonster->monster)+
                                 " at level "+std::to_string(otherMonster->level)+
                                 " is greater than the max "+std::to_string(otherMonsterStat.hp)+
                                 " after the skill use doTheTurn() at 1)");
                return;
            }
        }
        #endif
        if(doTheOtherMonsterTurn())
            turnIsEnd=true;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        {
            PlayerMonster * currentMonster=getCurrentMonster();
            if(currentMonster==NULL)
            {
                errorFightEngine("Unable to locate the current monster");
                return;
            }
            const PublicPlayerMonster * otherMonster=getOtherMonster();
            if(otherMonster==NULL)
            {
                errorFightEngine("Unable to locate the other monster");
                return;
            }
            Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(currentMonster->monster),currentMonster->level);
            Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(otherMonster->monster),otherMonster->level);
            if(currentMonster->hp>currentMonsterStat.hp)
            {
                errorFightEngine("The hp "+std::to_string(currentMonster->hp)+
                                 " of current monster "+std::to_string(currentMonster->monster)+
                                 " at level "+std::to_string(currentMonster->level)+
                                 " is greater than the max "+std::to_string(currentMonsterStat.hp)+
                                 " after the skill use doTheTurn() at 1)");
                return;
            }
            if(otherMonster->hp>otherMonsterStat.hp)
            {
                errorFightEngine("The hp "+std::to_string(otherMonster->hp)+
                                 " of other monster "+std::to_string(otherMonster->monster)+
                                 " at level "+std::to_string(otherMonster->level)+
                                 " is greater than the max "+std::to_string(otherMonsterStat.hp)+
                                 " after the skill use doTheTurn() at 1)");
                return;
            }
        }
        #endif
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        PlayerMonster * currentMonster=getCurrentMonster();
        if(currentMonster==NULL)
        {
            errorFightEngine("Unable to locate the current monster");
            return;
        }
        const PublicPlayerMonster * otherMonster=getOtherMonster();
        if(otherMonster==NULL)
        {
            errorFightEngine("Unable to locate the other monster");
            return;
        }
        Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(currentMonster->monster),currentMonster->level);
        Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(otherMonster->monster),otherMonster->level);
        if(currentMonster->hp>currentMonsterStat.hp)
        {
            errorFightEngine("The hp "+std::to_string(currentMonster->hp)+
                             " of current monster "+std::to_string(currentMonster->monster)+
                             " at level "+std::to_string(currentMonster->level)+
                             " is greater than the max "+std::to_string(currentMonsterStat.hp)+
                             " after the skill use doTheTurn() at 1)");
            return;
        }
        if(otherMonster->hp>otherMonsterStat.hp)
        {
            errorFightEngine("The hp "+std::to_string(otherMonster->hp)+
                             " of other monster "+std::to_string(otherMonster->monster)+
                             " at level "+std::to_string(otherMonster->level)+
                             " is greater than the max "+std::to_string(otherMonsterStat.hp)+
                             " after the skill use doTheTurn() at 1)");
            return;
        }
    }
    #endif
    //do the current monster attack
    if(!turnIsEnd && !currentMonsterStatIsFirstToAttack)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        {
            PlayerMonster * currentMonster=getCurrentMonster();
            if(currentMonster==NULL)
            {
                errorFightEngine("Unable to locate the current monster");
                return;
            }
            if(currentMonsterIsKO())
            {
                errorFightEngine("Can't attack with KO monster");
                return;
            }
            const PublicPlayerMonster * otherMonster=getOtherMonster();
            if(otherMonster==NULL)
            {
                errorFightEngine("Unable to locate the other monster");
                return;
            }
            Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(currentMonster->monster),currentMonster->level);
            Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(otherMonster->monster),otherMonster->level);
            if(currentMonster->hp>currentMonsterStat.hp)
            {
                errorFightEngine("The hp "+std::to_string(currentMonster->hp)+
                                 " of current monster "+std::to_string(currentMonster->monster)+
                                 " at level "+std::to_string(currentMonster->level)+
                                 " is greater than the max "+std::to_string(currentMonsterStat.hp)+
                                 " after the skill use doTheTurn() at 1)");
                return;
            }
            if(otherMonster->hp>otherMonsterStat.hp)
            {
                errorFightEngine("The hp "+std::to_string(otherMonster->hp)+
                                 " of other monster "+std::to_string(otherMonster->monster)+
                                 " at level "+std::to_string(otherMonster->level)+
                                 " is greater than the max "+std::to_string(otherMonsterStat.hp)+
                                 " after the skill use doTheTurn() at 1)");
                return;
            }
        }
        #endif
        doTheCurrentMonsterAttack(skill,skillLevel);
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        {
            PlayerMonster * currentMonster=getCurrentMonster();
            if(currentMonster==NULL)
            {
                errorFightEngine("Unable to locate the current monster");
                return;
            }
            const PublicPlayerMonster * otherMonster=getOtherMonster();
            if(otherMonster==NULL)
            {
                errorFightEngine("Unable to locate the other monster");
                return;
            }
            Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(currentMonster->monster),currentMonster->level);
            Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(otherMonster->monster),otherMonster->level);
            if(currentMonster->hp>currentMonsterStat.hp)
            {
                errorFightEngine("The hp "+std::to_string(currentMonster->hp)+
                                 " of current monster "+std::to_string(currentMonster->monster)+
                                 " at level "+std::to_string(currentMonster->level)+
                                 " is greater than the max "+std::to_string(currentMonsterStat.hp)+
                                 " after the skill use doTheTurn() at 1)");
                return;
            }
            if(otherMonster->hp>otherMonsterStat.hp)
            {
                errorFightEngine("The hp "+std::to_string(otherMonster->hp)+
                                 " of other monster "+std::to_string(otherMonster->monster)+
                                 " at level "+std::to_string(otherMonster->level)+
                                 " is greater than the max "+std::to_string(otherMonsterStat.hp)+
                                 " after the skill use doTheTurn() at 1)");
                return;
            }
        }
        #endif
        if(currentMonsterIsKO())
            turnIsEnd=true;
        else
        {
            if(otherMonsterIsKO())
            {
                checkKOOtherMonstersForGain();
                turnIsEnd=true;
            }
        }
    }
    (void)turnIsEnd;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        PlayerMonster * currentMonster=getCurrentMonster();
        if(currentMonster==NULL)
        {
            errorFightEngine("Unable to locate the current monster");
            return;
        }
        const PublicPlayerMonster * otherMonster=getOtherMonster();
        if(otherMonster==NULL)
        {
            errorFightEngine("Unable to locate the other monster");
            return;
        }
        Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(currentMonster->monster),currentMonster->level);
        Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(otherMonster->monster),otherMonster->level);
        if(currentMonster->hp>currentMonsterStat.hp)
        {
            errorFightEngine("The hp "+std::to_string(currentMonster->hp)+
                             " of current monster "+std::to_string(currentMonster->monster)+
                             " at level "+std::to_string(currentMonster->level)+
                             " is greater than the max "+std::to_string(currentMonsterStat.hp)+
                             " after the skill use doTheTurn() at 1)");
            return;
        }
        if(otherMonster->hp>otherMonsterStat.hp)
        {
            errorFightEngine("The hp "+std::to_string(otherMonster->hp)+
                             " of other monster "+std::to_string(otherMonster->monster)+
                             " at level "+std::to_string(otherMonster->level)+
                             " is greater than the max "+std::to_string(otherMonsterStat.hp)+
                             " after the skill use doTheTurn() at 1)");
            return;
        }
    }
    #endif
}

std::vector<Skill::BuffEffect> CommonFightEngine::removeOldBuff(PublicPlayerMonster * playerMonster)
{
    std::vector<Skill::BuffEffect> returnValue;
    unsigned int index=0;
    while(index<playerMonster->buffs.size())
    {
        const PlayerBuff &playerBuff=playerMonster->buffs.at(index);
        if(CommonDatapack::commonDatapack.monsterBuffs.find(playerBuff.buff)==CommonDatapack::commonDatapack.monsterBuffs.cend())
            playerMonster->buffs.erase(playerMonster->buffs.begin()+index);
        else
        {
            const Buff &buff=CommonDatapack::commonDatapack.monsterBuffs.at(playerBuff.buff);
            if(buff.level.at(playerBuff.level-1).duration==Buff::Duration_NumberOfTurn)
            {
                if(playerMonster->buffs.at(index).remainingNumberOfTurn>0)
                    playerMonster->buffs[index].remainingNumberOfTurn--;
                if(playerMonster->buffs.at(index).remainingNumberOfTurn<=0)
                {
                    Skill::BuffEffect buffEffect;
                    buffEffect.buff=playerBuff.buff;
                    buffEffect.on=ApplyOn_Themself;
                    buffEffect.level=playerBuff.level;
                    playerMonster->buffs.erase(playerMonster->buffs.begin()+index);
                    returnValue.push_back(buffEffect);
                    continue;
                }
            }
            index++;
        }
    }
    return returnValue;
}

std::vector<Skill::LifeEffectReturn> CommonFightEngine::applyBuffLifeEffect(PublicPlayerMonster *playerMonster)
{
    std::vector<Skill::LifeEffectReturn> returnValue;
    unsigned int index=0;
    while(index<playerMonster->buffs.size())
    {
        const PlayerBuff &playerBuff=playerMonster->buffs.at(index);
        if(CommonDatapack::commonDatapack.monsterBuffs.find(playerBuff.buff)==CommonDatapack::commonDatapack.monsterBuffs.cend())
            playerMonster->buffs.erase(playerMonster->buffs.begin()+index);
        else
        {
            const Buff &buff=CommonDatapack::commonDatapack.monsterBuffs.at(playerBuff.buff);
            const std::vector<Buff::Effect> &effects=buff.level.at(playerBuff.level-1).fight;
            unsigned int sub_index=0;
            while(sub_index<effects.size())
            {
                const Buff::Effect &effect=effects.at(sub_index);
                #ifdef CATCHCHALLENGER_DEBUG_FIGHT
                std::cout << "Buff: Apply on" << playerMonster->monster << "the buff" << playerBuff.buff << "and effect on" << effect.on << "quantity" << effect.quantity << "type" << effect.type << "was hp:" << playerMonster->hp << std::endl;
                #endif
                if(effect.on==Buff::Effect::EffectOn_HP)
                {
                    int32_t quantity=0;
                    Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(playerMonster->monster),playerMonster->level);
                    if(effect.type==QuantityType_Quantity)
                    {
                        quantity=effect.quantity;
                        if(quantity<0 && (int32_t)playerMonster->hp<=(-quantity))
                        {
                            quantity=-playerMonster->hp;
                            playerMonster->hp=0;
                            playerMonster->buffs.clear();
                        }
                        if(quantity>0 && quantity>=(int32_t)(currentMonsterStat.hp-playerMonster->hp))
                        {
                            quantity=currentMonsterStat.hp-playerMonster->hp;
                            playerMonster->hp=currentMonsterStat.hp;
                        }
                    }
                    if(effect.type==QuantityType_Percent)
                    {
                        quantity=((int64_t)currentMonsterStat.hp*(int64_t)effect.quantity)/(int64_t)100;
                        if(effect.quantity<0)
                        {
                            if(quantity==0)
                                quantity=-1;
                        }
                        else if(effect.quantity>0)
                        {
                            if(quantity==0)
                                quantity=1;
                        }
                    }
                    if(quantity<0 || quantity>0)
                    {
                        Skill::LifeEffectReturn lifeEffectReturn;
                        lifeEffectReturn.on=ApplyOn_Themself;
                        lifeEffectReturn.quantity=quantity;
                        returnValue.push_back(lifeEffectReturn);
                        playerMonster->hp+=quantity;
                    }
                }
                #ifdef CATCHCHALLENGER_DEBUG_FIGHT
                std::cout << "Buff: Apply on" << playerMonster->monster << "the buff" << playerBuff.buff << "and effect on" << effect.on << "quantity" << effect.quantity << "type" << effect.type << "new hp:" << playerMonster->hp << std::endl;
                #endif
                sub_index++;
            }
            index++;
        }
    }
    return returnValue;
}

bool CommonFightEngine::useSkill(const uint32_t &skill)
{
    doTurnIfChangeOfMonster=true;
    if(!isInFight())
    {
        errorFightEngine("Try use skill when not in fight");
        return false;
    }
    PlayerMonster * currentMonster=getCurrentMonster();
    if(currentMonster==NULL)
    {
        errorFightEngine("Unable to locate the current monster");
        return false;
    }
    if(currentMonsterIsKO())
    {
        errorFightEngine("Can't attack with KO monster");
        return false;
    }
    const PublicPlayerMonster * otherMonster=getOtherMonster();
    if(otherMonster==NULL)
    {
        errorFightEngine("Unable to locate the other monster");
        return false;
    }
    uint8_t skillLevel=getSkillLevel(skill);
    if(skillLevel==0)
    {
        if(!haveMoreEndurance() && skill==0 && CommonDatapack::commonDatapack.monsterSkills.find(skill)!=CommonDatapack::commonDatapack.monsterSkills.cend())
            skillLevel=1;
        else
        {
            errorFightEngine("Unable to fight because the current monster ("+std::to_string(currentMonster->monster)+
                             ", level: "+std::to_string(currentMonster->level)+
                             ") have not the skill "+std::to_string(skill));
            return false;
        }
    }
    else
        decreaseSkillEndurance(skill);
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(currentMonster->monster),currentMonster->level);
        Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(otherMonster->monster),otherMonster->level);
        if(currentMonster->hp>currentMonsterStat.hp)
        {
            errorFightEngine("The hp "+std::to_string(currentMonster->hp)+
                             " of current monster "+std::to_string(currentMonster->monster)+
                             " at level "+std::to_string(currentMonster->level)+
                             " is greater than the max "+std::to_string(currentMonsterStat.hp)+
                             " after the skill use doTheTurn() at 1)");
            return false;
        }
        if(otherMonster->hp>otherMonsterStat.hp)
        {
            errorFightEngine("The hp "+std::to_string(otherMonster->hp)+
                             " of other monster "+std::to_string(otherMonster->monster)+
                             " at level "+std::to_string(otherMonster->level)+
                             " is greater than the max "+std::to_string(otherMonsterStat.hp)+
                             " after the skill use doTheTurn() at 1)");
            return false;
        }
    }
    #endif
    doTheTurn(skill,skillLevel,currentMonsterAttackFirst(currentMonster,otherMonster));
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(currentMonster->monster),currentMonster->level);
        Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(otherMonster->monster),otherMonster->level);
        if(currentMonster->hp>currentMonsterStat.hp)
        {
            errorFightEngine("The hp "+std::to_string(currentMonster->hp)+
                             " of current monster "+std::to_string(currentMonster->monster)+
                             " at level "+std::to_string(currentMonster->level)+
                             " is greater than the max "+std::to_string(currentMonsterStat.hp)+
                             " after the skill use doTheTurn() at 1)");
            return false;
        }
        if(otherMonster->hp>otherMonsterStat.hp)
        {
            errorFightEngine("The hp "+std::to_string(otherMonster->hp)+
                             " of other monster "+std::to_string(otherMonster->monster)+
                             " at level "+std::to_string(otherMonster->level)+
                             " is greater than the max "+std::to_string(otherMonsterStat.hp)+
                             " after the skill use doTheTurn() at 1)");
            return false;
        }
    }
    #endif
    return true;
}

uint8_t CommonFightEngine::getSkillLevel(const uint32_t &skill)
{
    PlayerMonster * currentMonster=getCurrentMonster();
    if(currentMonster==NULL)
    {
        errorFightEngine("Unable to locate the current monster");
        return 0;
    }
    unsigned int index=0;
    while(index<currentMonster->skills.size())
    {
        if(currentMonster->skills.at(index).skill==skill && currentMonster->skills.at(index).endurance>0)
            return currentMonster->skills.at(index).level;
        index++;
    }
    return 0;
}

uint8_t CommonFightEngine::decreaseSkillEndurance(const uint32_t &skill)
{
    PlayerMonster * currentMonster=getCurrentMonster();
    if(currentMonster==NULL)
    {
        errorFightEngine("Unable to locate the current monster");
        return 0;
    }
    unsigned int index=0;
    while(index<currentMonster->skills.size())
    {
        if(currentMonster->skills.at(index).skill==skill && currentMonster->skills.at(index).endurance>0)
        {
            currentMonster->skills[index].endurance--;
            return currentMonster->skills.at(index).endurance;
        }
        index++;
    }
    return 0;
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

//return true if now have wild monter to fight
bool CommonFightEngine::generateWildFightIfCollision(CommonMap *map, const COORD_TYPE &x, const COORD_TYPE &y,
                                                     #ifdef MAXIMIZEPERFORMANCEOVERDATABASESIZE
                                                     const std::unordered_map<uint16_t, uint32_t> &items
                                                     #else
                                                     const std::map<uint16_t, uint32_t> &items
                                                     #endif
                                                     , const std::vector<uint8_t> &events)
{
    bool ok;
    if(isInFight())
    {
        errorFightEngine("error: map: "+map->map_file+" ("+std::to_string(x)+","+std::to_string(y)+"), is in fight");
        return false;
    }

    if(map->parsed_layer.monstersCollisionMap==NULL)
    {
        /// no fight in this zone
        return false;
    }
    uint8_t zoneCode=map->parsed_layer.monstersCollisionMap[x+y*map->width];
    if(zoneCode>=map->parsed_layer.monstersCollisionList.size())
    {
        errorFightEngine("error: map: "+map->map_file+" ("+std::to_string(x)+","+std::to_string(y)+"), zone code out of range");
        /// no fight in this zone
        return false;
    }
    const MonstersCollisionValue &monstersCollisionValue=map->parsed_layer.monstersCollisionList.at(zoneCode);
    unsigned int index=0;
    while(index<monstersCollisionValue.walkOn.size())
    {
        const CatchChallenger::MonstersCollision &monstersCollision=CommonDatapack::commonDatapack.monstersCollision.at(monstersCollisionValue.walkOn.at(index));
        if(monstersCollision.item==0 || items.find(monstersCollision.item)!=items.cend())
        {
            if(monstersCollisionValue.walkOnMonsters.at(index).defaultMonsters.empty())
            {
                /// no fight in this zone
                return false;
            }
            else
            {
                if(!ableToFight)
                {
                    errorFightEngine("LocalClientHandlerFight::singleMove(), can't walk into the grass into map: "+map->map_file+" ("+std::to_string(x)+","+std::to_string(y)+")");
                    return false;
                }
                if(stepFight==0)
                {
                    if(randomSeedsSize()==0)
                    {
                        errorFightEngine("error: no more random seed here, map: "+map->map_file+" ("+std::to_string(x)+","+std::to_string(y)+"), is in fight");
                        return false;
                    }
                    else
                        stepFight=getOneSeed(16);
                }
                else
                    stepFight--;
                if(stepFight==0)
                {
                    const MonstersCollisionValue::MonstersCollisionContent &monstersCollisionContent=monstersCollisionValue.walkOnMonsters.at(index);
                    std::vector<MapMonster> monsterList;
                    unsigned int index_condition=0;
                    while(index_condition<monstersCollisionContent.conditions.size())
                    {
                        const MonstersCollisionValue::MonstersCollisionValueOnCondition &monstersCollisionValueOnCondition=monstersCollisionContent.conditions.at(index_condition);
                        if(events.at(monstersCollisionValueOnCondition.event)==monstersCollisionValueOnCondition.event_value)
                        {
                            monsterList=monstersCollisionValueOnCondition.monsters;
                            break;
                        }
                        index_condition++;
                    }
                    if(index_condition==monstersCollisionContent.conditions.size())
                        monsterList=monstersCollisionContent.defaultMonsters;
                    if(monsterList.empty())
                        return false;
                    else
                    {
                        const PlayerMonster &monster=getRandomMonster(monsterList,&ok);
                        if(ok)
                        {
                            #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
                            messageFightEngine("Start grass fight with monster id "+std::to_string(monster.monster)+" level "+std::to_string(monster.level));
                            #endif
                            startTheFight();
                            wildMonsters.push_back(monster);
                            return true;
                        }
                        else
                        {
                            errorFightEngine("error: no more random seed here to have the get on map: "+map->map_file+" ("+std::to_string(x)+","+std::to_string(y)+")");
                            return false;
                        }
                    }
                }
                else
                    return false;
            }
        }
        index++;
    }
    /// no fight in this zone
    return false;
}

Skill::AttackReturn CommonFightEngine::generateOtherAttack()
{
    Skill::AttackReturn attackReturn;
    attackReturn.attack=0;
    attackReturn.doByTheCurrentMonster=false;
    attackReturn.attackReturnCase=Skill::AttackReturnCase_NormalAttack;
    attackReturn.success=false;
    PlayerMonster *otherMonster;
    if(!wildMonsters.empty())
        otherMonster=&wildMonsters.front();
    else if(!botFightMonsters.empty())
        otherMonster=&botFightMonsters.front();
    else
    {
        errorFightEngine("no other monster found");
        return attackReturn;
    }
    if(otherMonster->skills.empty())
    {
        if(CommonDatapack::commonDatapack.monsterSkills.find(0)!=CommonDatapack::commonDatapack.monsterSkills.cend())
        {
            messageFightEngine("Generated bot/wild default attack");
            attackReturn=genericMonsterAttack(otherMonster,getCurrentMonster(),0,1);
            attackReturn.doByTheCurrentMonster=false;
            return attackReturn;
        }
        else
        {
            messageFightEngine("No other monster attack todo");
            return attackReturn;
        }
    }
    unsigned int position;
    if(otherMonster->skills.size()==1)
        position=0;
    else
        position=getOneSeed(otherMonster->skills.size()-1);
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(position>=otherMonster->skills.size())
    {
        messageFightEngine("Position out of range: "+std::to_string(position)+" on "+std::to_string(otherMonster->skills.size())+" total skill(s)");
        position=position%otherMonster->skills.size();
    }
    #endif
    const PlayerMonster::PlayerSkill &otherMonsterSkill=otherMonster->skills.at(position);
    messageFightEngine("Generated bot/wild attack: "+std::to_string(otherMonsterSkill.skill)+
                       " (position: "+std::to_string(position)+
                       ") at level "+std::to_string(otherMonsterSkill.level)+
                       " on "+std::to_string(otherMonster->skills.size())+
                       " total skill(s)");
    attackReturn=genericMonsterAttack(otherMonster,getCurrentMonster(),otherMonsterSkill.skill,otherMonsterSkill.level);
    attackReturn.doByTheCurrentMonster=false;
    return attackReturn;
}

Skill::AttackReturn CommonFightEngine::genericMonsterAttack(PublicPlayerMonster *currentMonster,PublicPlayerMonster *otherMonster,const uint32_t &skill, const uint8_t &skillLevel)
{
    Skill::AttackReturn attackReturn;
    attackReturn.doByTheCurrentMonster=true;
    attackReturn.attackReturnCase=Skill::AttackReturnCase_NormalAttack;
    attackReturn.attack=skill;
    attackReturn.success=false;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        if(currentMonster==NULL)
        {
            errorFightEngine("Unable to locate the current monster");
            return attackReturn;
        }
        if(currentMonster->hp==0)
        {
            errorFightEngine("Can't attack with KO monster");
            return attackReturn;
        }
        if(otherMonster==NULL)
        {
            errorFightEngine("Unable to locate the other monster");
            return attackReturn;
        }
        if(otherMonster->hp==0)
        {
            errorFightEngine("Can't attack with KO monster");
            return attackReturn;
        }
        Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(currentMonster->monster),currentMonster->level);
        Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(otherMonster->monster),otherMonster->level);
        if(currentMonster->hp>currentMonsterStat.hp)
        {
            errorFightEngine("The hp "+std::to_string(currentMonster->hp)+
                             " of current monster "+std::to_string(currentMonster->monster)+
                             " at level "+std::to_string(currentMonster->level)+
                             " is greater than the max "+std::to_string(currentMonsterStat.hp)+
                             " after the skill use doTheTurn() at 1)");
            return attackReturn;
        }
        if(otherMonster->hp>otherMonsterStat.hp)
        {
            errorFightEngine("The hp "+std::to_string(otherMonster->hp)+
                             " of other monster "+std::to_string(otherMonster->monster)+
                             " at level "+std::to_string(otherMonster->level)+
                             " is greater than the max "+std::to_string(otherMonsterStat.hp)+
                             " after the skill use doTheTurn() at 1)");
            return attackReturn;
        }
    }
    #endif
    const Skill &skillDef=CommonDatapack::commonDatapack.monsterSkills.at(skill);
    const Skill::SkillList &skillList=skillDef.level.at(skillLevel-1);
    #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
    messageFightEngine("You use skill "+std::to_string(skill)+" at level "+std::to_string(skillLevel));
    #endif
    unsigned int index;
    //do the skill
    if(!genericMonsterIsKO(currentMonster))
    {
        index=0;
        while(index<skillList.life.size())
        {
            const Skill::Life &life=skillList.life.at(index);
            if(life.effect.quantity!=0)
            {
                bool success;
                if(life.success==100)
                    success=true;
                else
                    success=(getOneSeed(100)<life.success);
                if(success)
                {
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    //don't use pointer  here because the value of currentMonster->hp will change
                    const uint32_t currentMonsterHp=currentMonster->hp;
                    const uint32_t otherMonsterHp=otherMonster->hp;
                    #endif
                    attackReturn.success=true;//the attack have work because at less have a buff
                    Skill::LifeEffectReturn lifeEffectReturn;
                    lifeEffectReturn=applyLifeEffect(skillDef.type,life.effect,currentMonster,otherMonster);
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    if(lifeEffectReturn.on==ApplyOn_AllAlly || lifeEffectReturn.on==ApplyOn_Themself)
                    {
                        if(currentMonster->hp!=(currentMonsterHp+lifeEffectReturn.quantity))
                        {
                            errorFightEngine("life effect: Returned damage don't match with the real effect on current monster: "+
                                             std::to_string(currentMonster->hp)+"!=("+std::to_string(currentMonsterHp)+
                                             "+"+std::to_string(lifeEffectReturn.quantity)+")");
                            return attackReturn;
                        }
                    }
                    if(lifeEffectReturn.on==ApplyOn_AllEnemy || lifeEffectReturn.on==ApplyOn_AloneEnemy)
                    {
                        if(otherMonster->hp!=(otherMonsterHp+lifeEffectReturn.quantity))
                        {
                            errorFightEngine("life effect: Returned damage don't match with the real effect on other monster: "+
                                             std::to_string(otherMonster->hp)+"!=("+std::to_string(otherMonsterHp)+
                                             "+"+std::to_string(lifeEffectReturn.quantity)+")");
                            return attackReturn;
                        }
                    }
                    #endif
                    attackReturn.lifeEffectMonster.push_back(lifeEffectReturn);
                }
            }
            index++;
        }
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        //both can be KO here
        Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(currentMonster->monster),currentMonster->level);
        Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(otherMonster->monster),otherMonster->level);
        if(currentMonster->hp>currentMonsterStat.hp)
        {
            errorFightEngine("The hp "+std::to_string(currentMonster->hp)+
                             " of current monster "+std::to_string(currentMonster->monster)+
                             " at level "+std::to_string(currentMonster->level)+
                             " is greater than the max "+std::to_string(currentMonsterStat.hp)+
                             " after the skill use doTheTurn() at 1)");
            return attackReturn;
        }
        if(otherMonster->hp>otherMonsterStat.hp)
        {
            errorFightEngine("The hp "+std::to_string(otherMonster->hp)+
                             " of other monster "+std::to_string(otherMonster->monster)+
                             " at level "+std::to_string(otherMonster->level)+
                             " is greater than the max "+std::to_string(otherMonsterStat.hp)+
                             " after the skill use doTheTurn() at 1)");
            return attackReturn;
        }
    }
    #endif
    //do the buff
    if(!genericMonsterIsKO(currentMonster))
    {
        index=0;
        while(index<skillList.buff.size())
        {
            const Skill::Buff &buff=skillList.buff.at(index);
            #ifdef CATCHCHALLENGER_SERVER_EXTRA_CHECK
            if(!buffIsValid(buff.effect))
            {
                errorFightEngine("Buff is not valid");
                return attackReturn;
            }
            #endif
            bool success;
            if(buff.success==100)
                success=true;
            else
            {
                success=(getOneSeed(100)<buff.success);
                #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
                if(success)
                    messageFightEngine("Add successfull buff: "+std::to_string(buff.effect.buff)+" at level: "+std::to_string(buff.effect.level)+" on "+std::to_string(buff.effect.on));
                #endif
            }
            if(success)
            {
                #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
                if(success)
                    messageFightEngine("Add buff: "+std::to_string(buff.effect.buff)+" at level: "+std::to_string(buff.effect.level)+" on "+std::to_string(buff.effect.on));
                #endif
                if(addBuffEffectFull(buff.effect,currentMonster,otherMonster)>=-2)//0 to X, update buff, -1 added, -2 updated same buff at same level
                {
                    attackReturn.success=true;//the attack have work because at less have a buff
                    attackReturn.addBuffEffectMonster.push_back(buff.effect);
                }
            }
            index++;
        }
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        //both can be KO here
        Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(currentMonster->monster),currentMonster->level);
        Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(otherMonster->monster),otherMonster->level);
        if(currentMonster->hp>currentMonsterStat.hp)
        {
            errorFightEngine("The hp "+std::to_string(currentMonster->hp)+
                             " of current monster "+std::to_string(currentMonster->monster)+
                             " at level "+std::to_string(currentMonster->level)+
                             " is greater than the max "+std::to_string(currentMonsterStat.hp)+
                             " after the skill use doTheTurn() at 1)");
            return attackReturn;
        }
        if(otherMonster->hp>otherMonsterStat.hp)
        {
            errorFightEngine("The hp "+std::to_string(otherMonster->hp)+
                             " of other monster "+std::to_string(otherMonster->monster)+
                             " at level "+std::to_string(otherMonster->level)+
                             " is greater than the max "+std::to_string(otherMonsterStat.hp)+
                             " after the skill use doTheTurn() at 1)");
            return attackReturn;
        }
    }
    #endif
    //apply the effect of current buff
    if(!genericMonsterIsKO(currentMonster))
    {
        auto removeOldBuffVar=removeOldBuff(currentMonster);
        attackReturn.removeBuffEffectMonster.insert(attackReturn.removeBuffEffectMonster.cend(),removeOldBuffVar.cbegin(),removeOldBuffVar.cend());
        const std::vector<Skill::LifeEffectReturn> &lifeEffectMonster=applyBuffLifeEffect(currentMonster);
        attackReturn.lifeEffectMonster.insert(attackReturn.lifeEffectMonster.cend(),lifeEffectMonster.cbegin(),lifeEffectMonster.cend());
    }
    if(genericMonsterIsKO(currentMonster) && !genericMonsterIsKO(otherMonster))
        doTurnIfChangeOfMonster=false;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        //both can be KO here
        Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(currentMonster->monster),currentMonster->level);
        Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(otherMonster->monster),otherMonster->level);
        if(currentMonster->hp>currentMonsterStat.hp)
        {
            errorFightEngine("The hp "+std::to_string(currentMonster->hp)+
                             " of current monster "+std::to_string(currentMonster->monster)+
                             " at level "+std::to_string(currentMonster->level)+
                             " is greater than the max "+std::to_string(currentMonsterStat.hp)+
                             " after the skill use doTheTurn() at 1)");
            return attackReturn;
        }
        if(otherMonster->hp>otherMonsterStat.hp)
        {
            errorFightEngine("The hp "+std::to_string(otherMonster->hp)+
                             " of other monster "+std::to_string(otherMonster->monster)+
                             " at level "+std::to_string(otherMonster->level)+
                             " is greater than the max "+std::to_string(otherMonsterStat.hp)+
                             " after the skill use doTheTurn() at 1)");
            return attackReturn;
        }
    }
    #endif
    return attackReturn;
}

Skill::AttackReturn CommonFightEngine::doTheCurrentMonsterAttack(const uint32_t &skill,const uint8_t &skillLevel)
{
    return genericMonsterAttack(getCurrentMonster(),getOtherMonster(),skill,skillLevel);
}
