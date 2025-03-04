#include "CommonFightEngine.hpp"
#include "../base/CommonDatapack.hpp"
#include "../base/CommonSettingsCommon.hpp"
#include "../base/GeneralVariable.hpp"

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
    Monster monsterDef=CommonDatapack::commonDatapack.get_monsters().at(playerMonster.monster);
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
bool CommonFightEngine::generateWildFightIfCollision(const CommonMap &map, const COORD_TYPE &x, const COORD_TYPE &y,
                                                     #ifdef MAXIMIZEPERFORMANCEOVERDATABASESIZE
                                                     const std::unordered_map<CATCHCHALLENGER_TYPE_ITEM, CATCHCHALLENGER_TYPE_ITEM_QUANTITY> &items
                                                     #else
                                                     const std::map<CATCHCHALLENGER_TYPE_ITEM, CATCHCHALLENGER_TYPE_ITEM_QUANTITY> &items
                                                     #endif
                                                     , const std::vector<uint8_t> &events)
{
    bool ok;
    if(isInFight())
    {
        errorFightEngine("error: map: "+std::to_string(map.id)+" ("+std::to_string(x)+","+std::to_string(y)+"), is in fight");
        return false;
    }

    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(map.parsed_layer.simplifiedMapIndex>=CommonMap::flat_simplified_map_list_size)
    {
        std::cerr << "CommonFightEngine::generateWildFightIfCollision() map.parsed_layer.simplifiedMapIndex>=CommonMap::flat_simplified_map_list_size" << std::endl;
        abort();
    }
    #endif
    const uint8_t &zoneCode=*(CommonMap::flat_simplified_map+map.parsed_layer.simplifiedMapIndex+x+y*map.width);
    /* No wild monster into:
     * 253 ParsedLayerLedges_LedgesBottom
     * 252 ParsedLayerLedges_LedgesTop
     * 251 ParsedLayerLedges_LedgesRight
     * 250 ParsedLayerLedges_LedgesLeft */
    if(zoneCode==250 || zoneCode==251 || zoneCode==252 || zoneCode==253)
        return false;
    if(zoneCode>=map.parsed_layer.monstersCollisionList.size())
    {
        //errorFightEngine("error: map: "+std::to_string(map.id)+" ("+std::to_string(x)+","+std::to_string(y)+"), zone code out of range"); -> bug is TP in colision
        /// no fight in this zone
        return false;
    }
    const MonstersCollisionValue &monstersCollisionValue=map.parsed_layer.monstersCollisionList.at(zoneCode);
    unsigned int index=0;
    while(index<monstersCollisionValue.walkOn.size())
    {
        const CatchChallenger::MonstersCollision &monstersCollision=CommonDatapack::commonDatapack.get_monstersCollision().at(monstersCollisionValue.walkOn.at(index));
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
                    errorFightEngine("LocalClientHandlerFight::singleMove(), can't walk into the grass into map: "+std::to_string(map.id)+" ("+std::to_string(x)+","+std::to_string(y)+")");
                    return false;
                }
                if(stepFight==0)
                {
                    if(randomSeedsSize()==0)
                    {
                        errorFightEngine("error: no more random seed here, map: "+std::to_string(map.id)+" ("+std::to_string(x)+","+std::to_string(y)+"), is in fight");
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
                            errorFightEngine("error: no more random seed here to have the get on map: "+std::to_string(map.id)+" ("+std::to_string(x)+","+std::to_string(y)+")");
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
    if(internalTryCapture(CommonDatapack::commonDatapack.get_items().trap.at(item)))
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
        #ifndef CATCHCHALLENGER_VERSION_SINGLESERVER
        newMonster.id=catchAWild(get_public_and_private_informations().playerMonster.size()>=CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters,newMonster);
        return newMonster.id;
        #else
        catchAWild(get_public_and_private_informations().playerMonster.size()>=CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters,newMonster);
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
    const Monster::Stat &wildMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.get_monsters().at(wildMonsters.front().monster),wildMonsters.front().level);

    float bonusStat=1.0;
    if(wildMonsters.front().buffs.size())
    {
        bonusStat=0;
        unsigned int index=0;
        while(index<wildMonsters.front().buffs.size())
        {
            const PlayerBuff &playerBuff=wildMonsters.front().buffs.at(index);
            if(CatchChallenger::CommonDatapack::commonDatapack.get_monsterBuffs().find(playerBuff.buff)!=CatchChallenger::CommonDatapack::commonDatapack.get_monsterBuffs().cend())
            {
                const Buff &buff=CatchChallenger::CommonDatapack::commonDatapack.get_monsterBuffs().at(playerBuff.buff);
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
    const uint32_t catch_rate=(uint32_t)CatchChallenger::CommonDatapack::commonDatapack.get_monsters().at(wildMonsters.front().monster).catch_rate;
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
