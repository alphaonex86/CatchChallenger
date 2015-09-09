#include "../VariableServer.h"
#include "../base/GlobalServerData.h"
#include "../base/MapServer.h"
#include "../base/Client.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/FacilityLib.h"
#include "../base/Client.h"
#include "../base/PreparedDBQuery.h"

using namespace CatchChallenger;

bool Client::tryEscape()
{
    if(CommonFightEngine::canEscape())
    {
        bool escapeSuccess=CommonFightEngine::tryEscape();
        if(escapeSuccess)//check if is in fight
            saveCurrentMonsterStat();
        else
            finishTheTurn(false);
        return escapeSuccess;
    }
    else
    {
        errorOutput("Try escape when not allowed");
        return false;
    }
}

uint32_t Client::tryCapture(const uint16_t &item)
{
    uint32_t captureSuccessId=CommonFightEngine::tryCapture(item);
    if(captureSuccessId!=0)//if success
        saveCurrentMonsterStat();
    else
        finishTheTurn(false);
    return captureSuccessId;
}

bool Client::getBattleIsValidated()
{
    return battleIsValidated;
}

void Client::saveCurrentMonsterStat()
{
    if(public_and_private_informations.playerMonster.empty())
        return;//no monsters
    PlayerMonster * monster=getCurrentMonster();
    if(monster==NULL)
        return;//problem with the selection
    saveMonsterStat(*monster);
}

void Client::saveMonsterStat(const PlayerMonster &monster)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        if(CatchChallenger::CommonDatapack::commonDatapack.monsters.find(monster.monster)==CatchChallenger::CommonDatapack::commonDatapack.monsters.cend())
        {
            errorOutput("saveMonsterStat() The monster "+std::to_string(monster.monster)+
                        " is not into the monster list ("+std::to_string(CatchChallenger::CommonDatapack::commonDatapack.monsters.size())+")");
            return;
        }
        Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(monster.monster),monster.level);
        if(monster.hp>currentMonsterStat.hp)
        {
            errorOutput("saveMonsterStat() The hp "+std::to_string(monster.hp)+
                        " of current monster "+std::to_string(monster.monster)+
                        " is greater than the max "+std::to_string(currentMonsterStat.hp)
                        );
            return;
        }
    }
    #endif
    //save into the db
    if(GlobalServerData::serverSettings.fightSync==GameServerSettings::FightSync_AtTheEndOfBattle)
    {
        if(CommonSettingsServer::commonSettingsServer.useSP)
        {
            std::string queryText=PreparedDBQueryCommon::db_query_update_monster_xp_hp_level;
            stringreplaceOne(queryText,"%1",std::to_string(monster.id));
            stringreplaceOne(queryText,"%2",std::to_string(monster.hp));
            stringreplaceOne(queryText,"%3",std::to_string(monster.remaining_xp));
            stringreplaceOne(queryText,"%4",std::to_string(monster.level));
            stringreplaceOne(queryText,"%5",std::to_string(monster.sp));
            dbQueryWriteCommon(queryText);
        }
        else
        {
            std::string queryText=PreparedDBQueryCommon::db_query_update_monster_xp_hp_level;
            stringreplaceOne(queryText,"%1",std::to_string(monster.id));
            stringreplaceOne(queryText,"%2",std::to_string(monster.hp));
            stringreplaceOne(queryText,"%3",std::to_string(monster.remaining_xp));
            stringreplaceOne(queryText,"%4",std::to_string(monster.level));
            dbQueryWriteCommon(queryText);
        }
        auto i = deferedEndurance.begin();
        while (i != deferedEndurance.cend())
        {
            auto j = i->second.begin();
            while (j != i->second.cend())
            {
                std::string queryText=PreparedDBQueryCommon::db_query_monster_skill;
                stringreplaceOne(queryText,"%1",std::to_string(j->second));
                stringreplaceOne(queryText,"%2",std::to_string(i->first));
                stringreplaceOne(queryText,"%3",std::to_string(j->first));
                dbQueryWriteCommon(queryText);
                ++j;
            }
            ++i;
        }
        deferedEndurance.clear();
    }
}

void Client::saveAllMonsterPosition()
{
    const std::vector<PlayerMonster> &playerMonsterList=getPlayerMonster();
    unsigned int index=0;
    while(index<playerMonsterList.size())
    {
        const PlayerMonster &playerMonster=playerMonsterList.at(index);
        saveMonsterPosition(playerMonster.id,index+1);
        index++;
    }
}

bool Client::checkKOCurrentMonsters()
{
    if(getCurrentMonster()->hp==0)
    {
        #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
        normalOutput("You current monster ("+std::to_string(getCurrentMonster()->monster)+") is KO");
        #endif
        saveStat();
        return true;
    }
    return false;
}

bool Client::checkLoose()
{
    if(!haveMonsters())
        return false;
    if(!currentMonsterIsKO())
        return false;
    if(!haveAnotherMonsterOnThePlayerToFight())
    {
        #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
        normalOutput("You have lost, tp to "+rescue.map->map_file+" ("+std::to_string(rescue.x)+","+std::to_string(rescue.y)+") and heal");
        #endif
        doTurnIfChangeOfMonster=true;
        //teleport
        teleportTo(rescue.map,rescue.x,rescue.y,rescue.orientation);
        //regen all the monsters
        bool tempInBattle=isInBattle();
        healAllMonsters();
        fightFinished();
        if(tempInBattle)
            updateCanDoFight();
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        normalOutput("You lost the battle");
        if(!ableToFight)
        {
            errorOutput("after lost in fight, remain unable to do a fight");
            return true;
        }
        #endif
        fightOrBattleFinish(false,0);
        return true;
    }
    return false;
}

void Client::healAllMonsters()
{
    unsigned int sub_index;
    unsigned int index=0;
    while(index<public_and_private_informations.playerMonster.size())
    {
        if(public_and_private_informations.playerMonster.at(index).egg_step==0)
        {
            const Monster::Stat &stat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(public_and_private_informations.playerMonster.at(index).monster),public_and_private_informations.playerMonster.at(index).level);
            if(public_and_private_informations.playerMonster.at(index).hp!=stat.hp)
            {
                public_and_private_informations.playerMonster[index].hp=stat.hp;
                std::string queryText=PreparedDBQueryCommon::db_query_update_monster_hp_only;
                stringreplaceOne(queryText,"%1",std::to_string(public_and_private_informations.playerMonster.at(index).hp));
                stringreplaceOne(queryText,"%2",std::to_string(public_and_private_informations.playerMonster.at(index).id));
                dbQueryWriteCommon(queryText);
            }
            if(!public_and_private_informations.playerMonster.at(index).buffs.empty())
            {
                std::string queryText=PreparedDBQueryCommon::db_query_delete_monster_buff;
                stringreplaceOne(queryText,"%1",std::to_string(public_and_private_informations.playerMonster.at(index).id));
                dbQueryWriteCommon(queryText);
                public_and_private_informations.playerMonster[index].buffs.clear();
            }
            sub_index=0;
            const unsigned int &list_size=public_and_private_informations.playerMonster.at(index).skills.size();
            while(sub_index<list_size)
            {
                unsigned int endurance=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.at(
                        public_and_private_informations.playerMonster.at(index).skills.at(sub_index).skill
                        )
                        .level.at(public_and_private_informations.playerMonster.at(index).skills.at(sub_index).level-1).endurance;
                if(public_and_private_informations.playerMonster.at(index).skills.at(sub_index).endurance!=endurance)
                {
                    std::string queryText=PreparedDBQueryCommon::db_query_monster_skill;
                    stringreplaceOne(queryText,"%1",std::to_string(endurance));
                    stringreplaceOne(queryText,"%2",std::to_string(public_and_private_informations.playerMonster.at(index).id));
                    stringreplaceOne(queryText,"%3",std::to_string(public_and_private_informations.playerMonster.at(index).skills.at(sub_index).skill));
                    dbQueryWriteCommon(queryText);
                    public_and_private_informations.playerMonster[index].skills[sub_index].endurance=endurance;
                }
                sub_index++;
            }
        }
        index++;
    }
    if(!isInBattle())
        updateCanDoFight();
}

void Client::fightFinished()
{
    battleFinished();
    CommonFightEngine::fightFinished();
}

void Client::syncForEndOfTurn()
{
    if(GlobalServerData::serverSettings.fightSync==GameServerSettings::FightSync_AtEachTurn)
        saveStat();
}

void Client::saveStat()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        PlayerMonster * currentMonster=getCurrentMonster();
        PublicPlayerMonster * otherMonster=getOtherMonster();
        Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(currentMonster->monster),currentMonster->level);
        Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(otherMonster->monster),otherMonster->level);
        if(currentMonster!=NULL)
            if(currentMonster->hp>currentMonsterStat.hp)
            {
                errorOutput("saveStat() The hp "+std::to_string(currentMonster->hp)+
                            " of current monster "+std::to_string(currentMonster->monster)+
                            " is greater than the max "+std::to_string(currentMonsterStat.hp)
                            );
                return;
            }
        if(otherMonster!=NULL)
            if(otherMonster->hp>otherMonsterStat.hp)
            {
                errorOutput("saveStat() The hp "+std::to_string(otherMonster->hp)+
                            " of other monster "+std::to_string(otherMonster->monster)+
                            " is greater than the max "+std::to_string(otherMonsterStat.hp)
                            );
                return;
            }
    }
    #endif
    std::string queryText=PreparedDBQueryCommon::db_query_update_monster_hp_only;
    stringreplaceOne(queryText,"%1",std::to_string(getCurrentMonster()->hp));
    stringreplaceOne(queryText,"%2",std::to_string(getCurrentMonster()->id));
    dbQueryWriteCommon(queryText);
}

bool Client::botFightCollision(CommonMap *map,const COORD_TYPE &x,const COORD_TYPE &y)
{
    if(isInFight())
    {
        errorOutput("error: map: "+map->map_file+" ("+std::to_string(x)+","+std::to_string(y)+"), is in fight");
        return false;
    }
    const std::vector<uint32_t> &botList=static_cast<MapServer *>(map)->botsFightTrigger.at(std::pair<uint8_t,uint8_t>(x,y));
    unsigned int index=0;
    while(index<botList.size())
    {
        const uint32_t &botFightId=botList.at(index);
        if(public_and_private_informations.bot_already_beaten.find(botFightId)==public_and_private_informations.bot_already_beaten.cend())
        {
            normalOutput("is now in fight on map "+map->map_file+" ("+std::to_string(x)+","+std::to_string(y)+") with the bot "+std::to_string(botFightId));
            botFightStart(botFightId);
            return true;
        }
        index++;
    }

    /// no fight in this zone
    return false;
}

bool Client::botFightStart(const uint32_t &botFightId)
{
    if(isInFight())
    {
        errorOutput("error: is already in fight");
        return false;
    }
    if(CommonDatapackServerSpec::commonDatapackServerSpec.botFights.find(botFightId)==CommonDatapackServerSpec::commonDatapackServerSpec.botFights.cend())
    {
        errorOutput("error: bot id "+std::to_string(botFightId)+" not found");
        return false;
    }
    const BotFight &botFight=CommonDatapackServerSpec::commonDatapackServerSpec.botFights.at(botFightId);
    if(botFight.monsters.empty())
    {
        errorOutput("error: bot id "+std::to_string(botFightId)+" have no monster to fight");
        return false;
    }
    startTheFight();
    botFightCash=botFight.cash;
    unsigned int index=0;
    while(index<botFight.monsters.size())
    {
        const BotFight::BotFightMonster &monster=botFight.monsters.at(index);
        const Monster::Stat &stat=getStat(CommonDatapack::commonDatapack.monsters.at(monster.id),monster.level);
        botFightMonsters.push_back(FacilityLib::botFightMonsterToPlayerMonster(monster,stat));
        index++;
    }
    #ifdef CATCHCHALLENGER_SERVER_EXTRA_CHECK
    if(botFightMonsters.empty())
    {
        errorOutput("error: after the bot add, remaing empty");
        return false;
    }
    #endif
    this->botFightId=botFightId;
    return true;
}

void Client::setInCityCapture(const bool &isInCityCapture)
{
    this->isInCityCapture=isInCityCapture;
}

Client *Client::getOtherPlayerBattle() const
{
    return otherPlayerBattle;
}

PublicPlayerMonster *Client::getOtherMonster()
{
    if(otherPlayerBattle!=NULL)
    {
        PlayerMonster * otherPlayerBattleCurrentMonster=otherPlayerBattle->getCurrentMonster();
        if(otherPlayerBattleCurrentMonster!=NULL)
            return otherPlayerBattleCurrentMonster;
        else
            errorOutput("Is in battle but the other monster is null");
    }
    return CommonFightEngine::getOtherMonster();
}

void Client::wildDrop(const uint32_t &monster)
{
    std::vector<MonsterDrops> drops=GlobalServerData::serverPrivateVariables.monsterDrops.at(monster);
    if(questsDrop.find(monster)!=questsDrop.cend())
        drops.insert(drops.end(),questsDrop.at(monster).begin(),questsDrop.at(monster).end());
    unsigned int index=0;
    bool success;
    uint32_t quantity;
    while(index<drops.size())
    {
        const uint32_t &tempLuck=drops.at(index).luck;
        const uint32_t &quantity_min=drops.at(index).quantity_min;
        const uint32_t &quantity_max=drops.at(index).quantity_max;
        if(tempLuck==100)
            success=true;
        else
        {
            if(rand()%100<(int32_t)tempLuck)
                success=true;
            else
                success=false;
        }
        if(success)
        {
            if(quantity_max==1)
                quantity=1;
            else if(quantity_min==quantity_max)
                quantity=quantity_min;
            else
                quantity=rand()%(quantity_max-quantity_min+1)+quantity_min;
            #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
            normalOutput("Win "+std::to_string(quantity)+" item: "+std::to_string(drops.at(index).item));
            #endif
            addObjectAndSend(drops.at(index).item,quantity);
        }
        index++;
    }
}

bool Client::isInFight() const
{
    if(CommonFightEngine::isInFight())
        return true;
    return otherPlayerBattle!=NULL || battleIsValidated;
}

bool Client::learnSkillInternal(const uint32_t &monsterId,const uint32_t &skill)
{
    unsigned int index=0;
    while(index<public_and_private_informations.playerMonster.size())
    {
        const PlayerMonster &monster=public_and_private_informations.playerMonster.at(index);
        if(monster.id==monsterId)
        {
            unsigned int sub_index2=0;
            while(sub_index2<monster.skills.size())
            {
                if(monster.skills.at(sub_index2).skill==skill)
                    break;
                sub_index2++;
            }
            int sub_index=0;
            const int &list_size=CommonDatapack::commonDatapack.monsters.at(monster.monster).learn.size();
            while(sub_index<list_size)
            {
                const Monster::AttackToLearn &learn=CommonDatapack::commonDatapack.monsters.at(monster.monster).learn.at(sub_index);
                if(learn.learnAtLevel<=monster.level && learn.learnSkill==skill)
                {
                    if((sub_index2==monster.skills.size() && learn.learnSkillLevel==1) || (monster.skills.at(sub_index2).level+1)==learn.learnSkillLevel)
                    {
                        if(CommonSettingsServer::commonSettingsServer.useSP)
                        {
                            const Skill &skillStructure=CommonDatapack::commonDatapack.monsterSkills.at(learn.learnSkill);
                            const uint32_t &sp=skillStructure.level.at(learn.learnSkillLevel-1).sp_to_learn;
                            if(sp>monster.sp)
                            {
                                errorOutput("The attack require "+std::to_string(sp)+" sp to be learned, you have only "+std::to_string(monster.sp));
                                return false;
                            }
                            public_and_private_informations.playerMonster[index].sp-=sp;
                            std::string queryText=PreparedDBQueryCommon::db_query_update_monster_sp_only;
                            stringreplaceOne(queryText,"%1",std::to_string(public_and_private_informations.playerMonster.at(index).sp));
                            stringreplaceOne(queryText,"%2",std::to_string(monsterId));
                            dbQueryWriteCommon(queryText);
                        }
                        if(learn.learnSkillLevel==1)
                        {
                            PlayerMonster::PlayerSkill temp;
                            temp.skill=skill;
                            temp.level=1;
                            temp.endurance=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.at(temp.skill).level.front().endurance;
                            public_and_private_informations.playerMonster[index].skills.push_back(temp);
                            std::string queryText=PreparedDBQueryCommon::db_query_insert_monster_skill;
                            stringreplaceOne(queryText,"%1",std::to_string(monsterId));
                            stringreplaceOne(queryText,"%2",std::to_string(temp.skill));
                            stringreplaceOne(queryText,"%3","1");
                            stringreplaceOne(queryText,"%4",std::to_string(temp.endurance));
                            dbQueryWriteCommon(queryText);
                        }
                        else
                        {
                            public_and_private_informations.playerMonster[index].skills[sub_index2].level++;
                            std::string queryText=PreparedDBQueryCommon::db_query_update_monster_skill_level;
                            stringreplaceOne(queryText,"%1",std::to_string(public_and_private_informations.playerMonster.at(index).skills.at(sub_index2).level));
                            stringreplaceOne(queryText,"%2",std::to_string(monsterId));
                            stringreplaceOne(queryText,"%3",std::to_string(skill));
                            dbQueryWriteCommon(queryText);
                        }
                        return true;
                    }
                }
                sub_index++;
            }
            errorOutput("The skill "+std::to_string(skill)+" is not into learn skill list for the monster");
            return false;
        }
        index++;
    }
    errorOutput("The monster is not found: "+std::to_string(monsterId));
    return false;
}

bool Client::isInBattle() const
{
    return (otherPlayerBattle!=NULL && battleIsValidated);
}

void Client::registerBattleRequest(Client *otherPlayerBattle)
{
    if(isInBattle())
    {
        normalOutput("Already in battle, internal error");
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput(otherPlayerBattle->public_and_private_informations.public_informations.pseudo+" have requested battle with you");
    #endif
    this->otherPlayerBattle=otherPlayerBattle;
    otherPlayerBattle->otherPlayerBattle=this;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << otherPlayerBattle->public_and_private_informations.public_informations.skinId;
    sendBattleRequest(otherPlayerBattle->rawPseudo+outputData);
}

void Client::battleCanceled()
{
    if(otherPlayerBattle!=NULL)
        otherPlayerBattle->internalBattleCanceled(true);
    internalBattleCanceled(true);
}

void Client::battleAccepted()
{
    if(otherPlayerBattle!=NULL)
        otherPlayerBattle->internalBattleAccepted(true);
    internalBattleAccepted(true);
}

void Client::battleFakeAccepted(Client *otherPlayer)
{
    battleFakeAcceptedInternal(otherPlayer);
    otherPlayer->battleFakeAcceptedInternal(this);
    otherPlayerBattle->internalBattleAccepted(true);
    internalBattleAccepted(true);
}

void Client::battleFakeAcceptedInternal(Client * otherPlayer)
{
    this->otherPlayerBattle=otherPlayer;
}

void Client::battleFinished()
{
    if(!battleIsValidated)
        return;
    if(otherPlayerBattle==NULL)
        return;
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput("Battle finished");
    #endif
    otherPlayerBattle->resetBattleAction();
    resetBattleAction();
    Client *tempOtherPlayerBattle=otherPlayerBattle;
    otherPlayerBattle->battleFinishedReset();
    battleFinishedReset();
    updateCanDoFight();
    tempOtherPlayerBattle->updateCanDoFight();
}

void Client::battleFinishedReset()
{
    otherPlayerBattle=NULL;
    battleIsValidated=false;
    mHaveCurrentSkill=false;
    mMonsterChange=false;
}

void Client::resetTheBattle()
{
    //reset out of battle
    mHaveCurrentSkill=false;
    mMonsterChange=false;
    battleIsValidated=false;
    otherPlayerBattle=NULL;
    updateCanDoFight();
}

void Client::internalBattleCanceled(const bool &send)
{
    if(otherPlayerBattle==NULL)
    {
        //normalOutput(QLatin1String("Battle already canceled"));
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput("Battle canceled");
    #endif
    bool needUpdateCanDoFight=false;
    if(battleIsValidated)
        needUpdateCanDoFight=true;
    otherPlayerBattle=NULL;
    if(send)
    {
            sendFullPacket(0xE0,0x07);
            receiveSystemText("Battle declined");
    }
    battleIsValidated=false;
    mHaveCurrentSkill=false;
    mMonsterChange=false;
    if(needUpdateCanDoFight)
        updateCanDoFight();
}

void Client::internalBattleAccepted(const bool &send)
{
    if(otherPlayerBattle==NULL)
    {
        normalOutput("Can't accept battle if not in battle");
        return;
    }
    if(battleIsValidated)
    {
        normalOutput("Battle already validated");
        return;
    }
    if(!otherPlayerBattle->getAbleToFight())
    {
        errorOutput("The other player can't fight");
        return;
    }
    if(!getAbleToFight())
    {
        errorOutput("You can't fight");
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput("Battle accepted");
    #endif
    startTheFight();
    battleIsValidated=true;
    mHaveCurrentSkill=false;
    mMonsterChange=false;
    if(send)
    {
        std::vector<PlayerMonster> playerMonstersPreview=otherPlayerBattle->public_and_private_informations.playerMonster;
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
        out << otherPlayerBattle->public_and_private_informations.public_informations.skinId;
        out << (uint8_t)playerMonstersPreview.size();
        unsigned int index=0;
        while(index<playerMonstersPreview.size() && index<255)
        {
            if(!monsterIsKO(playerMonstersPreview.at(index)))
                out << (uint8_t)0x01;
            else
                out << (uint8_t)0x02;
            index++;
        }
        out << (uint8_t)selectedMonsterNumberToMonsterPlace(getOtherSelectedMonsterNumber());
        QByteArray firstValidOtherPlayerMonster=FacilityLib::publicPlayerMonsterToBinary(FacilityLib::playerMonsterToPublicPlayerMonster(*otherPlayerBattle->getCurrentMonster()));
        const QByteArray newData(otherPlayerBattle->rawPseudo+outputData+firstValidOtherPlayerMonster);
        sendFullPacket(0xE0,0x08,newData.constData(),newData.size());
    }
}

bool Client::haveBattleAction() const
{
    return mHaveCurrentSkill || mMonsterChange;
}

void Client::resetBattleAction()
{
    mHaveCurrentSkill=false;
    mMonsterChange=false;
}

uint8_t Client::getOtherSelectedMonsterNumber() const
{
    if(!isInBattle())
        return 0;
    else
        return otherPlayerBattle->getCurrentSelectedMonsterNumber();
}

void Client::haveUsedTheBattleAction()
{
    mHaveCurrentSkill=false;
    mMonsterChange=false;
}

bool Client::currentMonsterAttackFirst(const PlayerMonster * currentMonster,const PublicPlayerMonster * otherMonster) const
{
    if(isInBattle())
    {
        const Monster::Stat &currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(currentMonster->monster),currentMonster->level);
        const Monster::Stat &otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(otherMonster->monster),otherMonster->level);
        bool currentMonsterStatIsFirstToAttack=false;
        if(currentMonsterStat.speed>otherMonsterStat.speed)
            currentMonsterStatIsFirstToAttack=true;
        if(currentMonsterStat.speed==otherMonsterStat.speed)
            currentMonsterStatIsFirstToAttack=rand()%2;
        return currentMonsterStatIsFirstToAttack;
    }
    else
        return CommonFightEngine::currentMonsterAttackFirst(currentMonster,otherMonster);
}

uint8_t Client::selectedMonsterNumberToMonsterPlace(const uint8_t &selectedMonsterNumber)
{
    return selectedMonsterNumber+1;
}

void Client::sendBattleReturn()
{
    QByteArray binarypublicPlayerMonster;
    unsigned int index,master_index;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);

    out << (uint8_t)attackReturn.size();
    master_index=0;
    while(master_index<attackReturn.size())
    {
        const Skill::AttackReturn &attackReturnTemp=attackReturn.at(master_index);
        out << (uint8_t)attackReturnTemp.doByTheCurrentMonster;
        out << (uint8_t)attackReturnTemp.attackReturnCase;
        out << (uint8_t)attackReturnTemp.success;
        out << attackReturnTemp.attack;
        //ad buff
        index=0;
        out << (uint8_t)attackReturnTemp.addBuffEffectMonster.size();
        while(index<attackReturnTemp.addBuffEffectMonster.size())
        {
            out << attackReturnTemp.addBuffEffectMonster.at(index).buff;
            out << (uint8_t)attackReturnTemp.addBuffEffectMonster.at(index).on;
            out << attackReturnTemp.addBuffEffectMonster.at(index).level;
            index++;
        }
        //remove buff
        index=0;
        out << (uint8_t)attackReturnTemp.removeBuffEffectMonster.size();
        while(index<attackReturnTemp.removeBuffEffectMonster.size())
        {
            out << attackReturnTemp.removeBuffEffectMonster.at(index).buff;
            out << (uint8_t)attackReturnTemp.removeBuffEffectMonster.at(index).on;
            out << attackReturnTemp.removeBuffEffectMonster.at(index).level;
            index++;
        }
        //life effect
        index=0;
        out << (uint8_t)attackReturnTemp.lifeEffectMonster.size();
        while(index<attackReturnTemp.lifeEffectMonster.size())
        {
            out << attackReturnTemp.lifeEffectMonster.at(index).quantity;
            out << (uint8_t)attackReturnTemp.lifeEffectMonster.at(index).on;
            index++;
        }
        //buff effect
        index=0;
        out << (uint8_t)attackReturnTemp.buffLifeEffectMonster.size();
        while(index<attackReturnTemp.buffLifeEffectMonster.size())
        {
            out << attackReturnTemp.buffLifeEffectMonster.at(index).quantity;
            out << (uint8_t)attackReturnTemp.buffLifeEffectMonster.at(index).on;
            index++;
        }
        master_index++;
    }
    if(otherPlayerBattle!=NULL && otherPlayerBattle->haveMonsterChange())
    {
        out << (uint8_t)selectedMonsterNumberToMonsterPlace(getOtherSelectedMonsterNumber());;
        binarypublicPlayerMonster=FacilityLib::publicPlayerMonsterToBinary(*getOtherMonster());
    }
    attackReturn.clear();

    const QByteArray newData(outputData+binarypublicPlayerMonster);
    sendFullPacket(0xE0,0x06,newData.constData(),newData.size());
}

void Client::sendBattleMonsterChange()
{
    QByteArray binarypublicPlayerMonster;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint8_t)0;
    out << (uint8_t)selectedMonsterNumberToMonsterPlace(getOtherSelectedMonsterNumber());;
    binarypublicPlayerMonster=FacilityLib::publicPlayerMonsterToBinary(*getOtherMonster());
    const QByteArray newData(outputData+binarypublicPlayerMonster);
    sendFullPacket(0xE0,0x06,newData.constData(),newData.size());
}

//return true if change level, multiplicator do at datapack loading
bool Client::giveXPSP(int xp,int sp)
{
    const bool &haveChangeOfLevel=CommonFightEngine::giveXPSP(xp,sp);
    if(GlobalServerData::serverSettings.fightSync==GameServerSettings::FightSync_AtEachTurn)
    {
        auto currentMonster=getCurrentMonster();
        if(CommonSettingsServer::commonSettingsServer.useSP)
        {
            if(haveChangeOfLevel)
            {
                std::string queryText=PreparedDBQueryCommon::db_query_update_monster_xp_hp_level;
                stringreplaceOne(queryText,"%1",std::to_string(currentMonster->id));
                stringreplaceOne(queryText,"%2",std::to_string(currentMonster->hp));
                stringreplaceOne(queryText,"%3",std::to_string(currentMonster->remaining_xp));
                stringreplaceOne(queryText,"%4",std::to_string(currentMonster->level));
                stringreplaceOne(queryText,"%5",std::to_string(currentMonster->sp));
                dbQueryWriteCommon(queryText);
            }
            else
            {
                std::string queryText=PreparedDBQueryCommon::db_query_update_monster_xp;
                stringreplaceOne(queryText,"%1",std::to_string(currentMonster->id));
                stringreplaceOne(queryText,"%2",std::to_string(currentMonster->remaining_xp));
                stringreplaceOne(queryText,"%3",std::to_string(currentMonster->sp));
                dbQueryWriteCommon(queryText);
            }
        }
        else
        {
            if(haveChangeOfLevel)
            {
                std::string queryText=PreparedDBQueryCommon::db_query_update_monster_xp_hp_level;
                stringreplaceOne(queryText,"%1",std::to_string(currentMonster->id));
                stringreplaceOne(queryText,"%2",std::to_string(currentMonster->hp));
                stringreplaceOne(queryText,"%3",std::to_string(currentMonster->remaining_xp));
                stringreplaceOne(queryText,"%4",std::to_string(currentMonster->level));
                dbQueryWriteCommon(queryText);
            }
            else
            {
                std::string queryText=PreparedDBQueryCommon::db_query_update_monster_xp;
                stringreplaceOne(queryText,"%1",std::to_string(currentMonster->id));
                stringreplaceOne(queryText,"%2",std::to_string(currentMonster->remaining_xp));
                dbQueryWriteCommon(queryText);
            }
        }
    }
    return haveChangeOfLevel;
}

bool Client::finishTheTurn(const bool &isBot)
{
    const bool &win=!currentMonsterIsKO() && otherMonsterIsKO();
    if(currentMonsterIsKO() || otherMonsterIsKO())
    {
        dropKOCurrentMonster();
        dropKOOtherMonster();
        checkLoose();
    }
    syncForEndOfTurn();
    if(!isInFight())
    {
        if(win)
        {
            if(isBot)
            {
                if(!isInCityCapture)
                {
                    addCash(CommonDatapackServerSpec::commonDatapackServerSpec.botFights.value(botFightId).cash);
                    public_and_private_informations.bot_already_beaten << botFightId;
                    dbQueryWriteServer(PreparedDBQueryServer::db_query_insert_bot_already_beaten
                                 .arg(character_id)
                                 .arg(botFightId)
                                 );
                }
                fightOrBattleFinish(win,botFightId);
                normalOutput(std::stringLiteral("Register the win against the bot fight: %1").arg(botFightId));
            }
            else
                normalOutput(std::stringLiteral("Have win agains a wild monster"));
        }
        fightOrBattleFinish(win,0);
    }
    return win;
}

bool Client::useSkill(const uint32_t &skill)
{
    normalOutput(std::stringLiteral("use the skill: %1").arg(skill));
    if(!isInBattle())//wild or bot
    {
        const bool &isBot=!botFightMonsters.isEmpty();
        CommonFightEngine::useSkill(skill);
        return finishTheTurn(isBot);
    }
    else
    {
        if(haveBattleAction())
        {
            errorOutput(std::stringLiteral("Have already a battle action"));
            return false;
        }
        mHaveCurrentSkill=true;
        mCurrentSkillId=skill;
        return checkIfCanDoTheTurn();
    }
}

bool Client::bothRealPlayerIsReady() const
{
    if(!haveBattleAction())
        return false;
    if(!otherPlayerBattle->haveBattleAction())
        return false;
    return true;
}

bool Client::checkIfCanDoTheTurn()
{
    if(!bothRealPlayerIsReady())
        return false;
    if(mHaveCurrentSkill)
        CommonFightEngine::useSkill(mCurrentSkillId);
    else
        doTheOtherMonsterTurn();
    sendBattleReturn();
    otherPlayerBattle->sendBattleReturn();
    if(currentMonsterIsKO() || otherMonsterIsKO())
    {
        //sendBattleMonsterChange() at changing to not block if both is KO
        if(haveAnotherMonsterOnThePlayerToFight() && otherPlayerBattle->haveAnotherMonsterOnThePlayerToFight())
        {
            dropKOCurrentMonster();
            dropKOOtherMonster();
            normalOutput(QLatin1String("Have win agains the current monster"));
        }
        else
        {
            const bool &youWin=haveAnotherMonsterOnThePlayerToFight();
            const bool &theOtherWin=otherPlayerBattle->haveAnotherMonsterOnThePlayerToFight();
            dropKOCurrentMonster();
            dropKOOtherMonster();
            Client *tempOtherPlayerBattle=otherPlayerBattle;
            checkLoose();
            tempOtherPlayerBattle->checkLoose();
            normalOutput(QLatin1String("Have win the battle"));
            if(youWin)
                emitBattleWin();
            if(theOtherWin)
                tempOtherPlayerBattle->emitBattleWin();
            return true;
        }
    }
    resetBattleAction();
    otherPlayerBattle->resetBattleAction();
    return true;
}

void Client::emitBattleWin()
{
    fightOrBattleFinish(true,0);
}

bool Client::dropKOOtherMonster()
{
    const bool &commonReturn=CommonFightEngine::dropKOOtherMonster();

    bool battleReturn=false;
    if(isInBattle())
        battleReturn=otherPlayerBattle->dropKOCurrentMonster();
    else
    {
        if(GlobalServerData::serverSettings.fightSync==GameServerSettings::FightSync_AtTheEndOfBattle)
        {
            if(!isInFight())
                saveCurrentMonsterStat();
        }
    }

    return commonReturn || battleReturn;
}

uint32_t Client::catchAWild(const bool &toStorage, const PlayerMonster &newMonster)
{
    int position=999999;
    bool ok;
    const uint32_t monster_id=getMonsterId(&ok);
    if(!ok)
    {
        errorFightEngine("No more monster id: getMonsterId(&ok) failed");
        return 0;
    }
    if(toStorage)
    {
        public_and_private_informations.warehouse_playerMonster << newMonster;
        public_and_private_informations.warehouse_playerMonster.last().id=monster_id;
        position=public_and_private_informations.warehouse_playerMonster.size();
    }
    else
    {
        public_and_private_informations.playerMonster << newMonster;
        public_and_private_informations.playerMonster.last().id=monster_id;
        position=public_and_private_informations.playerMonster.size();
    }
    if(toStorage)
        dbQueryWriteCommon(PreparedDBQueryCommon::db_query_insert_warehouse_monster_full
                     .arg(std::stringLiteral("%1,%2,%3,%4,%5,%6,%7,%8,%9")
                          .arg(monster_id)
                          .arg(newMonster.hp)
                          .arg(character_id)
                          .arg(newMonster.monster)
                          .arg(newMonster.level)
                          .arg(newMonster.remaining_xp)
                          .arg(newMonster.sp)
                          .arg(newMonster.catched_with)
                          .arg((uint8_t)newMonster.gender)
                          )
                     .arg(std::stringLiteral("%1,%2,%3")
                          .arg(newMonster.egg_step)
                          .arg(character_id)
                          .arg(position)
                          )
                     );
    else
        dbQueryWriteCommon(PreparedDBQueryCommon::db_query_insert_monster_full
                     .arg(std::stringLiteral("%1,%2,%3,%4,%5,%6,%7,%8,%9")
                          .arg(monster_id)
                          .arg(newMonster.hp)
                          .arg(character_id)
                          .arg(newMonster.monster)
                          .arg(newMonster.level)
                          .arg(newMonster.remaining_xp)
                          .arg(newMonster.sp)
                          .arg(newMonster.catched_with)
                          .arg((uint8_t)newMonster.gender)
                          )
                     .arg(std::stringLiteral("%1,%2,%3")
                          .arg(newMonster.egg_step)
                          .arg(character_id)
                          .arg(position)
                          )
                     );

    int index=0;
    while(index<newMonster.skills.size())
    {
        dbQueryWriteCommon(PreparedDBQueryCommon::db_query_insert_monster_skill
                     .arg(monster_id)
                     .arg(newMonster.skills.at(index).skill)
                     .arg(newMonster.skills.at(index).level)
                     .arg(newMonster.skills.at(index).endurance)
                     );
        index++;
    }
    index=0;
    while(index<newMonster.buffs.size())
    {
        if(CommonDatapack::commonDatapack.monsterBuffs.value(newMonster.buffs.at(index).buff).level.at(newMonster.buffs.at(index).level).duration==Buff::Duration_Always)
            dbQueryWriteCommon(PreparedDBQueryCommon::db_query_insert_monster_buff
                         .arg(monster_id)
                         .arg(newMonster.buffs.at(index).buff)
                         .arg(newMonster.buffs.at(index).level)
                         );
        index++;
    }
    wildMonsters.removeFirst();
    return monster_id;
}

bool Client::haveCurrentSkill() const
{
    return mHaveCurrentSkill;
}

uint32_t Client::getCurrentSkill() const
{
    return mCurrentSkillId;
}

bool Client::haveMonsterChange() const
{
    return mMonsterChange;
}

int Client::addCurrentBuffEffect(const Skill::BuffEffect &effect)
{
    const int &returnCode=CommonFightEngine::addCurrentBuffEffect(effect);
    if(returnCode==-2)
        return returnCode;
    if(CommonDatapack::commonDatapack.monsterBuffs.value(effect.buff).level.at(effect.level-1).duration==Buff::Duration_Always)
    {
        if(returnCode==-1)
            switch(effect.on)
            {
                case ApplyOn_AloneEnemy:
                case ApplyOn_AllEnemy:
                if(isInBattle())
                    dbQueryWriteCommon(PreparedDBQueryCommon::db_query_insert_monster_buff
                             .arg(otherPlayerBattle->getCurrentMonster()->id)
                             .arg(effect.buff)
                             .arg(effect.level)
                             );
                break;
                case ApplyOn_Themself:
                case ApplyOn_AllAlly:
                dbQueryWriteCommon(PreparedDBQueryCommon::db_query_insert_monster_buff
                         .arg(getCurrentMonster()->id)
                         .arg(effect.buff)
                         .arg(effect.level)
                         );
                break;
                default:
                    errorOutput("Not apply match, can't apply the buff");
                break;
            }
        else
            switch(effect.on)
            {
                case ApplyOn_AloneEnemy:
                case ApplyOn_AllEnemy:
                if(isInBattle())
                    dbQueryWriteCommon(PreparedDBQueryCommon::db_query_update_monster_level
                             .arg(otherPlayerBattle->getCurrentMonster()->id)
                             .arg(effect.buff)
                             .arg(effect.level)
                             );
                break;
                case ApplyOn_Themself:
                case ApplyOn_AllAlly:
                    dbQueryWriteCommon(PreparedDBQueryCommon::db_query_update_monster_level
                             .arg(getCurrentMonster()->id)
                             .arg(effect.buff)
                             .arg(effect.level)
                             );
                break;
                default:
                    errorOutput("Not apply match, can't apply the buff");
                break;
            }
    }
    return returnCode;
}

bool Client::moveUpMonster(const uint8_t &number)
{
    if(!CommonFightEngine::moveUpMonster(number))
        return false;
    if(GlobalServerData::serverSettings.fightSync!=GameServerSettings::FightSync_AtTheDisconnexion)
    {
        saveMonsterPosition(public_and_private_informations.playerMonster.value(number-1).id,number);
        saveMonsterPosition(public_and_private_informations.playerMonster.value(number).id,number+1);
    }
    return true;
}

bool Client::moveDownMonster(const uint8_t &number)
{
    if(!CommonFightEngine::moveDownMonster(number))
    {
        errorOutput("Move monster have failed");
        return false;
    }
    if(GlobalServerData::serverSettings.fightSync!=GameServerSettings::FightSync_AtTheDisconnexion)
    {
        saveMonsterPosition(public_and_private_informations.playerMonster.value(number).id,number+1);
        saveMonsterPosition(public_and_private_informations.playerMonster.value(number+1).id,number+2);
    }
    return true;

}

void Client::saveMonsterPosition(const uint32_t &monsterId,const uint8_t &monsterPosition)
{
    dbQueryWriteCommon(PreparedDBQueryCommon::db_query_update_monster_position
                 .arg(monsterPosition)
                 .arg(monsterId)
                 );
}

bool Client::changeOfMonsterInFight(const uint32_t &monsterId)
{
    const bool &doTurnIfChangeOfMonster=this->doTurnIfChangeOfMonster;

    //save for sync at end of the battle
    PlayerMonster * monster=getCurrentMonster();
    if(monster!=NULL)
        saveMonsterStat(*monster);

    if(!CommonFightEngine::changeOfMonsterInFight(monsterId))
        return false;
    if(isInBattle())
    {
        if(!doTurnIfChangeOfMonster)
            otherPlayerBattle->sendBattleMonsterChange();
        else
            mMonsterChange=true;
    }
    return true;
}

bool Client::doTheOtherMonsterTurn()
{
    if(!isInBattle())
        return CommonFightEngine::doTheOtherMonsterTurn();
    if(!bothRealPlayerIsReady())
        return false;
    if(!otherPlayerBattle->haveCurrentSkill())
        return false;
    return CommonFightEngine::doTheOtherMonsterTurn();
}

Skill::AttackReturn Client::generateOtherAttack()
{
    Skill::AttackReturn attackReturnTemp;
    attackReturnTemp.attack=0;
    attackReturnTemp.doByTheCurrentMonster=false;
    attackReturnTemp.attackReturnCase=Skill::AttackReturnCase_NormalAttack;
    attackReturnTemp.success=false;
    if(!isInBattle())
        return CommonFightEngine::generateOtherAttack();
    if(!bothRealPlayerIsReady())
    {
        errorOutput("Both player is not ready at generateOtherAttack()");
        return attackReturnTemp;
    }
    if(!otherPlayerBattle->haveCurrentSkill())
    {
        errorOutput("The other player have not skill at generateOtherAttack()");
        return attackReturnTemp;
    }
    const uint32_t &skill=otherPlayerBattle->getCurrentSkill();
    uint8_t skillLevel=getSkillLevel(skill);
    if(skillLevel==0)
    {
        if(!haveMoreEndurance() && skill==0 && CommonDatapack::commonDatapack.monsterSkills.contains(skill))
            skillLevel=1;
        else
        {
            errorOutput(std::stringLiteral("Unable to fight because the current monster have not the skill %3").arg(skill));
            return attackReturnTemp;
        }
    }
    otherPlayerBattle->doTheCurrentMonsterAttack(skill,skillLevel);
    if(currentMonsterIsKO() && haveAnotherMonsterOnThePlayerToFight())
        doTurnIfChangeOfMonster=false;
    return attackReturn.last();
}

Skill::AttackReturn Client::doTheCurrentMonsterAttack(const uint32_t &skill, const uint8_t &skillLevel)
{
    if(!isInBattle())
        return CommonFightEngine::doTheCurrentMonsterAttack(skill,skillLevel);
    attackReturn << CommonFightEngine::doTheCurrentMonsterAttack(skill,skillLevel);
    Skill::AttackReturn attackReturnTemp=attackReturn.last();
    attackReturnTemp.doByTheCurrentMonster=false;
    otherPlayerBattle->attackReturn << attackReturnTemp;
    if(currentMonsterIsKO() && haveAnotherMonsterOnThePlayerToFight())
        doTurnIfChangeOfMonster=false;
    return attackReturn.last();
}

uint8_t Client::decreaseSkillEndurance(const uint32_t &skill)
{
    PlayerMonster * currentMonster=getCurrentMonster();
    if(currentMonster==NULL)
    {
        errorOutput("Unable to locate the current monster");
        return 0;
    }
    const uint8_t &newEndurance=CommonFightEngine::decreaseSkillEndurance(skill);
    if(GlobalServerData::serverSettings.fightSync==GameServerSettings::FightSync_AtEachTurn)
    {
        dbQueryWriteCommon(PreparedDBQueryCommon::db_query_monster_skill
                     .arg(newEndurance)
                     .arg(currentMonster->id)
                     .arg(skill)
                     );
    }
    else
    {
        if(GlobalServerData::serverSettings.fightSync==GameServerSettings::FightSync_AtTheEndOfBattle)
            deferedEndurance[currentMonster->id][skill]=newEndurance;
    }
    return newEndurance;
}

void Client::confirmEvolutionTo(PlayerMonster * playerMonster,const uint32_t &monster)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    PlayerMonster * currentMonster=getCurrentMonster();
    const Monster::Stat &currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(currentMonster->monster),currentMonster->level);
    /// \note NO OTHER MONSTER because it's evolution on current monster
    if(currentMonster!=NULL)
        if(currentMonster->hp>currentMonsterStat.hp)
        {
            errorOutput(std::stringLiteral("confirmEvolutionTo() The hp %1 of current monster %2 is greater than the max %3").arg(currentMonster->hp).arg(currentMonster->monster).arg(currentMonsterStat.hp));
            return;
        }
    #endif
    CommonFightEngine::confirmEvolutionTo(playerMonster,monster);
    dbQueryWriteCommon(PreparedDBQueryCommon::db_query_update_monster_and_hp
                 .arg(playerMonster->hp)
                 .arg(playerMonster->monster)
                 .arg(playerMonster->id)
                 );
}

void Client::confirmEvolution(const uint32_t &monsterId)
{
    int index=0;
    while(index<public_and_private_informations.playerMonster.size())
    {
        if(public_and_private_informations.playerMonster.at(index).id==monsterId)
        {
            const Monster &monsterInformations=CommonDatapack::commonDatapack.monsters.value(public_and_private_informations.playerMonster.at(index).monster);
            int sub_index=0;
            while(sub_index<monsterInformations.evolutions.size())
            {
                if(
                        (monsterInformations.evolutions.at(sub_index).type==Monster::EvolutionType_Level && monsterInformations.evolutions.at(sub_index).level<=public_and_private_informations.playerMonster.at(index).level)
                        ||
                        (monsterInformations.evolutions.at(sub_index).type==Monster::EvolutionType_Trade && GlobalServerData::serverPrivateVariables.tradedMonster.contains(public_and_private_informations.playerMonster.at(index).id))
                )
                {
                    confirmEvolutionTo(&public_and_private_informations.playerMonster[index],monsterInformations.evolutions.at(sub_index).evolveTo);
                    return;
                }
                sub_index++;
            }
            errorOutput(std::stringLiteral("Evolution not found"));
            return;
        }
        index++;
    }
    errorOutput(std::stringLiteral("Monster for evolution not found"));
}

void Client::hpChange(PlayerMonster * currentMonster, const uint32_t &newHpValue)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        PlayerMonster * currentMonster=getCurrentMonster();
        PublicPlayerMonster * otherMonster=getOtherMonster();
        if(currentMonster!=NULL)
        {
            const Monster::Stat &currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(currentMonster->monster),currentMonster->level);
            if(currentMonster->hp>currentMonsterStat.hp)
            {
                errorOutput(std::stringLiteral("hpChange() The hp %1 of current monster %2 is greater than the max %3").arg(currentMonster->monster).arg(currentMonster->hp).arg(currentMonsterStat.hp));
                return;
            }
        }
        if(otherMonster!=NULL)
        {
            const Monster::Stat &otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(otherMonster->monster),otherMonster->level);
            if(otherMonster->hp>otherMonsterStat.hp)
            {
                errorOutput(std::stringLiteral("hpChange() The hp %1 of other monster %2 is greater than the max %3").arg(otherMonster->monster).arg(otherMonster->hp).arg(otherMonsterStat.hp));
                return;
            }
        }
    }
    #endif
    CommonFightEngine::hpChange(currentMonster,newHpValue);
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        PlayerMonster * currentMonster=getCurrentMonster();
        PublicPlayerMonster * otherMonster=getOtherMonster();
        if(currentMonster!=NULL)
        {
            const Monster::Stat &currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(currentMonster->monster),currentMonster->level);
            if(currentMonster->hp>currentMonsterStat.hp)
            {
                errorOutput(std::stringLiteral("hpChange() after The hp %1 of current monster %2 is greater than the max %3").arg(currentMonster->monster).arg(currentMonster->hp).arg(currentMonsterStat.hp));
                return;
            }
        }
        if(otherMonster!=NULL)
        {
            const Monster::Stat &otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(otherMonster->monster),otherMonster->level);
            if(otherMonster->hp>otherMonsterStat.hp)
            {
                errorOutput(std::stringLiteral("hpChange() after The hp %1 of other monster %2 is greater than the max %3").arg(otherMonster->monster).arg(otherMonster->hp).arg(otherMonsterStat.hp));
                return;
            }
        }
    }
    #endif
    dbQueryWriteCommon(PreparedDBQueryCommon::db_query_update_monster_hp_only
                 .arg(newHpValue)
                 .arg(currentMonster->id)
                 );
}

bool Client::removeBuffOnMonster(PlayerMonster * currentMonster, const uint32_t &buffId)
{
    const bool returnVal=CommonFightEngine::removeBuffOnMonster(currentMonster,buffId);
    if(returnVal)
        dbQueryWriteCommon(PreparedDBQueryCommon::db_query_delete_monster_specific_buff.arg(currentMonster->id).arg(buffId));
    return returnVal;
}

bool Client::removeAllBuffOnMonster(PlayerMonster * currentMonster)
{
    const bool &returnVal=CommonFightEngine::removeAllBuffOnMonster(currentMonster);
    if(returnVal)
        dbQueryWriteCommon(PreparedDBQueryCommon::db_query_delete_monster_buff.arg(currentMonster->id));
    return returnVal;
}

bool Client::addLevel(PlayerMonster * monster, const uint8_t &numberOfLevel)
{
    if(!CommonFightEngine::addLevel(monster,numberOfLevel))
        return false;
    dbQueryWriteCommon(PreparedDBQueryCommon::db_query_update_monster_level_only.arg(monster->hp).arg(monster->level).arg(monster->id));
    return true;
}

bool Client::addSkill(PlayerMonster * currentMonster,const PlayerMonster::PlayerSkill &skill)
{
    if(!CommonFightEngine::addSkill(currentMonster,skill))
        return false;
    dbQueryWriteCommon(PreparedDBQueryCommon::db_query_insert_monster_skill.arg(currentMonster->id).arg(skill.skill).arg(skill.level).arg(skill.endurance));
    return true;
}

bool Client::setSkillLevel(PlayerMonster * currentMonster,const int &index,const uint8_t &level)
{
    if(!CommonFightEngine::setSkillLevel(currentMonster,index,level))
        return false;
    dbQueryWriteCommon(PreparedDBQueryCommon::db_query_update_monster_skill_level.arg(level).arg(currentMonster->id).arg(currentMonster->skills.at(index).skill));
    return true;
}

bool Client::removeSkill(PlayerMonster * currentMonster,const int &index)
{
    if(!CommonFightEngine::removeSkill(currentMonster,index))
        return false;
    dbQueryWriteCommon(PreparedDBQueryCommon::db_query_delete_monster_specific_skill.arg(currentMonster->id).arg(currentMonster->skills.at(index).skill));
    return true;
}
