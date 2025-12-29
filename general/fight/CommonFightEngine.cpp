#include "CommonFightEngine.hpp"
#include "../base/CommonDatapack.hpp"
#include "../base/GeneralVariable.hpp"

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
    return !get_public_and_private_informations_ro().monsters.empty();
}

//return is have random seed to do random step
bool CommonFightEngine::canDoRandomFight(const CommonMap &map,const uint8_t &x,const uint8_t &y) const
{
    if(isInFight())
    {
        messageFightEngine("map canDoRandomFight is in fight");
        return false;
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if((x+y*map.width)>=map.flat_simplified_map.size())
    {
        std::cerr << "CommonFightEngine::canDoRandomFight() map.flat_simplified_map_first_index>=CommonMap::flat_simplified_map_list_size" << std::endl;
        abort();
    }
    #endif

    const uint8_t &zoneCode=map.flat_simplified_map.at(x+y*map.width);
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(zoneCode>=map.zones.size())
    {
        std::cerr << "CommonFightEngine::canDoRandomFight() zoneCode>=map.monstersCollisionList.size()" << std::endl;
        abort();
    }
    #endif
    const MonstersCollisionValue &monstersCollisionValue=map.zones.at(zoneCode);
    if(monstersCollisionValue.walkOn.empty())
        return true;
    else
        return randomSeedsSize()>=CATCHCHALLENGER_MIN_RANDOM_TO_FIGHT;

    /// no fight in this zone
    messageFightEngine("map no fight in this zone");
    return true;
}

void CommonFightEngine::updateCanDoFight()
{
    if(isInBattle())
    {
        errorFightEngine("Can't auto select the next monster when is in battle");
        return;
    }
    ableToFight=false;
    uint8_t index=0;
    const std::vector<PlayerMonster> &playerMonster=get_public_and_private_informations().monsters;
    while(index<playerMonster.size())
    {
        const PlayerMonster &playerMonsterEntry=playerMonster.at(index);
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
    while(index<get_public_and_private_informations_ro().monsters.size())
    {
        const PlayerMonster &playerMonsterEntry=get_public_and_private_informations_ro().monsters.at(index);
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
    const size_t &playerMonsterSize=get_public_and_private_informations().monsters.size();
    if(selectedMonster<playerMonsterSize)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(CommonDatapack::commonDatapack.get_monsters().find(get_public_and_private_informations().monsters.at(selectedMonster).monster)==CommonDatapack::commonDatapack.get_monsters().cend())
        {
            errorFightEngine("Current monster don't exists: "+std::to_string(get_public_and_private_informations().monsters.at(selectedMonster).monster));
            return NULL;
        }
        #endif
        return &get_public_and_private_informations().monsters[selectedMonster];
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
    while(index<get_public_and_private_informations_ro().monsters.size())
    {
        const PlayerMonster &playerMonsterEntry=get_public_and_private_informations_ro().monsters.at(index);
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
    if(CatchChallenger::CommonDatapack::commonDatapack.get_items().evolutionItem.find(object)!=CatchChallenger::CommonDatapack::commonDatapack.get_items().evolutionItem.cend())
    {
        if(isInFight())
        {
            errorFightEngine("this item "+std::to_string(object)+" can't be used in fight");
            return false;
        }
        if(CatchChallenger::CommonDatapack::commonDatapack.get_items().evolutionItem.at(object).find(playerMonster->monster)==CatchChallenger::CommonDatapack::commonDatapack.get_items().evolutionItem.at(object).cend())
        {
            errorFightEngine("this item "+std::to_string(object)+" can't be applied on this monster "+std::to_string(playerMonster->monster));
            return false;
        }
        confirmEvolutionTo(playerMonster,CatchChallenger::CommonDatapack::commonDatapack.get_items().evolutionItem.at(object).at(playerMonster->monster));
        return true;
    }
    else if(CommonDatapack::commonDatapack.get_items().monsterItemEffect.find(object)!=CommonDatapack::commonDatapack.get_items().monsterItemEffect.cend()
            || CommonDatapack::commonDatapack.get_items().monsterItemEffectOutOfFight.find(object)!=CommonDatapack::commonDatapack.get_items().monsterItemEffectOutOfFight.cend())
    {
        if(CommonDatapack::commonDatapack.get_items().monsterItemEffect.find(object)!=CommonDatapack::commonDatapack.get_items().monsterItemEffect.cend())
        {
            const Monster::Stat &playerMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.get_monsters().at(playerMonster->monster),playerMonster->level);
            const std::vector<MonsterItemEffect> monsterItemEffect = CommonDatapack::commonDatapack.get_items().monsterItemEffect.at(object);
            unsigned int index=0;
            while(index<monsterItemEffect.size())
            {
                const MonsterItemEffect &effect=monsterItemEffect.at(index);
                switch(effect.type)
                {
                    case MonsterItemEffectType_AddHp:
                        if(effect.data.hp>0 && (playerMonsterStat.hp-playerMonster->hp)>(uint32_t)effect.data.hp)
                            hpChange(playerMonster,playerMonster->hp+effect.data.hp);
                        else if(playerMonsterStat.hp!=playerMonster->hp)
                            hpChange(playerMonster,playerMonsterStat.hp);
                        else if(monsterItemEffect.size()==1)
                            return false;
                    break;
                    case MonsterItemEffectType_RemoveBuff:
                        if(effect.data.buff>0)
                            removeBuffOnMonster(playerMonster,effect.data.buff);
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
        else if(CommonDatapack::commonDatapack.get_items().monsterItemEffectOutOfFight.find(object)!=CommonDatapack::commonDatapack.get_items().monsterItemEffectOutOfFight.cend())
        {
            if(isInFight())
            {
                errorFightEngine("this item "+std::to_string(object)+" can't be used in fight");
                return false;
            }
            const std::vector<MonsterItemEffectOutOfFight> monsterItemEffectOutOfFight = CommonDatapack::commonDatapack.get_items().monsterItemEffectOutOfFight.at(object);
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
    else if(CommonDatapack::commonDatapack.get_items().itemToLearn.find(object)!=CommonDatapack::commonDatapack.get_items().itemToLearn.cend())
    {
        if(isInFight())
        {
            errorFightEngine("this item "+std::to_string(object)+" can't be used in fight");
            return false;
        }
        if(CatchChallenger::CommonDatapack::commonDatapack.get_items().itemToLearn.at(object).find(playerMonster->monster)==
                CatchChallenger::CommonDatapack::commonDatapack.get_items().itemToLearn.at(object).cend())
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
    if(monsterPosition>=get_public_and_private_informations().monsters.size())
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
    const PlayerMonster &playerMonsterEntry=get_public_and_private_informations().monsters.at(monsterPosition);
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

std::vector<uint8_t> CommonFightEngine::addPlayerMonster(const std::vector<PlayerMonster> &playerMonster)
{
    std::vector<uint8_t> positionList;
    {
        uint8_t index=0;
        while(index<playerMonster.size())
        {
            positionList.push_back(static_cast<uint8_t>(get_public_and_private_informations().monsters.size())+index);
            index++;
        }
    }
    get_public_and_private_informations().monsters.insert(get_public_and_private_informations().monsters.cend(),playerMonster.cbegin(),playerMonster.cend());
    updateCanDoFight();
    return positionList;
}

std::vector<uint8_t> CommonFightEngine::addPlayerMonster(const PlayerMonster &playerMonster)
{
    std::vector<uint8_t> positionList;
    positionList.push_back(static_cast<uint8_t>(get_public_and_private_informations().monsters.size()));
    get_public_and_private_informations().monsters.push_back(playerMonster);
    updateCanDoFight();
    return positionList;
}

void CommonFightEngine::insertPlayerMonster(const uint8_t &place,const PlayerMonster &playerMonster)
{
    get_public_and_private_informations().monsters.insert(get_public_and_private_informations().monsters.cbegin()+place,playerMonster);
    updateCanDoFight();
}

std::vector<PlayerMonster> CommonFightEngine::getPlayerMonster() const
{
    return get_public_and_private_informations_ro().monsters;
}

bool CommonFightEngine::moveUpMonster(const uint8_t &number)
{
    if(isInFight())
        return false;
    if(number==0)
        return false;
    if(number>=(get_public_and_private_informations().monsters.size()))
        return false;
    std::swap(get_public_and_private_informations().monsters[number],get_public_and_private_informations().monsters[number-1]);
    updateCanDoFight();
    return true;
}

bool CommonFightEngine::moveDownMonster(const uint8_t &number)
{
    if(isInFight())
        return false;
    if(number>=(get_public_and_private_informations().monsters.size()-1))
        return false;
    std::swap(get_public_and_private_informations().monsters[number],get_public_and_private_informations().monsters[number+1]);
    updateCanDoFight();
    return true;
}

bool CommonFightEngine::removeMonsterByPosition(const uint8_t &monsterPosition)
{
    if(monsterPosition>=get_public_and_private_informations().monsters.size())
        return false;
    else
    {
        get_public_and_private_informations().monsters.erase(get_public_and_private_informations().monsters.cbegin()+monsterPosition);
        updateCanDoFight();
        return true;
    }
}

bool CommonFightEngine::haveThisMonsterByPosition(const uint8_t &monsterPosition) const
{
    if(monsterPosition>=get_public_and_private_informations_ro().monsters.size())
        return false;
    else
        return true;
}

PlayerMonster * CommonFightEngine::monsterByPosition(const uint8_t &monsterPosition)
{
    if(monsterPosition>=get_public_and_private_informations().monsters.size())
        return NULL;
    else
        return &get_public_and_private_informations().monsters[monsterPosition];
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
    const Monster::Stat &currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.get_monsters().at(currentMonster->monster),currentMonster->level);
    const Monster::Stat &otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.get_monsters().at(otherMonster->monster),otherMonster->level);
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
