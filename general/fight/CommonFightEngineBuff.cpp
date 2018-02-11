#include "CommonFightEngine.h"
#include "../base/CommonDatapack.h"
#include "../base/CommonDatapackServerSpec.h"
#include "../base/CommonSettingsServer.h"
#include "../base/CommonSettingsCommon.h"
#include "../base/GeneralVariable.h"

#include <iostream>

using namespace CatchChallenger;

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
                        quantity=static_cast<uint32_t>(((int64_t)currentMonsterStat.hp*(int64_t)effect.quantity)/(int64_t)100);
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

