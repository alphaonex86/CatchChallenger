#include "CommonFightEngine.h"
#include "../base/CommonDatapack.h"
#include "../base/CommonDatapackServerSpec.h"
#include "../base/CommonSettingsServer.h"
#include "../base/CommonSettingsCommon.h"
#include "../base/GeneralVariable.h"

#include <iostream>
#include <cmath>

using namespace CatchChallenger;

bool CommonFightEngine::isInFightWithWild() const
{
    return !wildMonsters.empty();
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

//return true if now have wild monter to fight
bool CommonFightEngine::generateWildFightIfCollision(const CommonMap *map, const COORD_TYPE &x, const COORD_TYPE &y,
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

    if(map->parsed_layer.simplifiedMap==NULL)
    {
        /// no fight in this zone
        return false;
    }
    uint8_t zoneCode=map->parsed_layer.simplifiedMap[x+y*map->width];
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
                    if(!events.empty())
                    {
                        unsigned int index_condition=0;
                        while(index_condition<monstersCollisionContent.conditions.size())
                        {
                            const MonstersCollisionValue::MonstersCollisionValueOnCondition &monstersCollisionValueOnCondition=monstersCollisionContent.conditions.at(index_condition);
                            if(monstersCollisionValueOnCondition.event<events.size())
                            {
                                if(events.at(monstersCollisionValueOnCondition.event)==monstersCollisionValueOnCondition.event_value)
                                {
                                    monsterList=monstersCollisionValueOnCondition.monsters;
                                    break;
                                }
                            }
                            index_condition++;
                        }
                        if(index_condition==monstersCollisionContent.conditions.size())
                            monsterList=monstersCollisionContent.defaultMonsters;
                    }
                    else
                        monsterList=monstersCollisionContent.defaultMonsters;
                    if(monsterList.empty())
                        return false;
                    else
                    {
                        const PlayerMonster &monster=getRandomMonster(monsterList,&ok);
                        if(ok)
                        {
                            #ifdef CATCHCHALLENGER_DEBUG_FIGHT
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

uint32_t CommonFightEngine::tryCapture(const uint16_t &item)
{
    doTurnIfChangeOfMonster=true;
    if(internalTryCapture(CommonDatapack::commonDatapack.items.trap.at(item)))
    {
        #ifdef CATCHCHALLENGER_DEBUG_FIGHT
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
        #ifndef CATCHCHALLENGER_VERSION_SINGLESERVER
        newMonster.id=catchAWild(public_and_private_informations.playerMonster.size()>=CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters,newMonster);
        return newMonster.id;
        #else
        catchAWild(public_and_private_informations.playerMonster.size()>=CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters,newMonster);
        return 999;
        #endif
    }
    else
    {
        #ifdef CATCHCHALLENGER_DEBUG_FIGHT
        messageFightEngine("capture is failed");
        #endif
        generateOtherAttack();//Skill::AttackReturn attackReturn=
        return 0;
    }
}

void CommonFightEngine::wildDrop(const uint16_t &)
{
}

bool CommonFightEngine::tryEscape()
{
    if(!canEscape())//check if is in fight
        return false;
    if(internalTryEscape())
    {
        #ifdef CATCHCHALLENGER_DEBUG_FIGHT
        messageFightEngine("escape is successful");
        #endif
        wildMonsters.clear();
        return true;
    }
    else
    {
        #ifdef CATCHCHALLENGER_DEBUG_FIGHT
        messageFightEngine("escape is failed");
        #endif
        generateOtherAttack();//Skill::AttackReturn attackReturn=
        return false;
    }
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
        uint16_t number=static_cast<uint16_t>(rand()%65536);
        if(number>=realRate)
            return false;
        index++;
    }
    return true;
}
