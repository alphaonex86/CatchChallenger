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
            const std::string &queryText=PreparedDBQueryCommon::db_query_update_monster_xp_hp_level.compose(
                        std::to_string(monster.hp),
                        std::to_string(monster.remaining_xp),
                        std::to_string(monster.level),
                        std::to_string(monster.sp),
                        std::to_string(monster.id)
                        );
            dbQueryWriteCommon(queryText);
        }
        else
        {
            const std::string &queryText=PreparedDBQueryCommon::db_query_update_monster_xp_hp_level.compose(
                        std::to_string(monster.hp),
                        std::to_string(monster.remaining_xp),
                        std::to_string(monster.level),
                        std::to_string(monster.id)
                        );
            dbQueryWriteCommon(queryText);
        }
        /*auto i = deferedEndurance.begin();
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
        deferedEndurance.clear();*/

        syncMonsterSkillAndEndurance(monster);
    }
}

void Client::syncMonsterBuff(const PlayerMonster &monster)
{
    char raw_buff[(1+1+1)*monster.buffs.size()];
    uint8_t lastBuffId=0;
    uint32_t sub_index=0;
    while(sub_index<monster.buffs.size())
    {
        const PlayerBuff &buff=monster.buffs.at(sub_index);

        #ifdef MAXIMIZEPERFORMANCEOVERDATABASESIZE
        //not ordened
        if(lastBuffId<=buff.buff)
        {
            const uint8_t &buffInt=buff.buff-lastBuffId;
            lastBuffId=buff.buff;
        }
        else
        {
            const uint8_t &buffInt=256-lastBuffId+buff.buff;
            lastBuffId=buff.buff;
        }
        #else
        //ordened
        const uint8_t &buffInt=buff.buff-lastBuffId;
        lastBuffId=buff.buff;
        #endif

        raw_buff[sub_index*3+0]=buffInt;
        raw_buff[sub_index*3+1]=buff.level;
        raw_buff[sub_index*3+2]=buff.remainingNumberOfTurn;
        sub_index++;
    }
    const std::string &queryText=PreparedDBQueryCommon::db_query_update_monster_buff.compose(
                binarytoHexa(raw_buff,sizeof(raw_buff)),
                std::to_string(monster.id)
                );
    dbQueryWriteCommon(queryText);
}

void Client::syncMonsterSkillAndEndurance(const PlayerMonster &monster)
{
    char skills_endurance[monster.skills.size()*(1)];
    char skills[monster.skills.size()*(2+1)];
    unsigned int sub_index=0;
    uint16_t lastSkillId=0;
    const unsigned int &sub_size=monster.skills.size();
    while(sub_index<sub_size)
    {
        const PlayerMonster::PlayerSkill &playerSkill=monster.skills.at(sub_index);
        skills_endurance[sub_index]=playerSkill.endurance;

        #ifdef MAXIMIZEPERFORMANCEOVERDATABASESIZE
        //not ordened
        if(lastSkillId<=playerSkill.skill)
        {
            const uint16_t &skillInt=playerSkill.skill-lastSkillId;
            lastSkillId=playerSkill.skill;
        }
        else
        {
            const uint16_t &skillInt=65536-lastSkillId+playerSkill.skill;
            lastSkillId=playerSkill.skill;
        }
        #else
        //ordened
        const uint16_t &skillInt=playerSkill.skill-lastSkillId;
        lastSkillId=playerSkill.skill;
        #endif

        *reinterpret_cast<uint16_t *>(skills+sub_index*(2+1))=htole16(skillInt);
        skills[2+sub_index*(2+1)]=playerSkill.level;

        sub_index++;
    }
    const std::string &queryText=PreparedDBQueryCommon::db_query_monster_update_skill_and_endurance.compose(
                binarytoHexa(skills,sizeof(skills)),
                binarytoHexa(skills_endurance,sizeof(skills_endurance)),
                std::to_string(monster.id)
                );
    dbQueryWriteCommon(queryText);
}

void Client::syncMonsterEndurance(const PlayerMonster &monster)
{
    char skills_endurance[monster.skills.size()*(1)];
    unsigned int sub_index=0;
    const unsigned int &sub_size=monster.skills.size();
    while(sub_index<sub_size)
    {
        const PlayerMonster::PlayerSkill &playerSkill=monster.skills.at(sub_index);
        skills_endurance[sub_index]=playerSkill.endurance;

        sub_index++;
    }
    const std::string &queryText=PreparedDBQueryCommon::db_query_monster_update_endurance.compose(
                binarytoHexa(skills_endurance,sizeof(skills_endurance)),
                std::to_string(monster.id)
                );
    dbQueryWriteCommon(queryText);
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
                const std::string &queryText=PreparedDBQueryCommon::db_query_update_monster_hp_only.compose(
                            std::to_string(public_and_private_informations.playerMonster.at(index).hp),
                            std::to_string(public_and_private_informations.playerMonster.at(index).id)
                            );
                dbQueryWriteCommon(queryText);
            }
            if(!public_and_private_informations.playerMonster.at(index).buffs.empty())
            {
                const std::string &queryText=PreparedDBQueryCommon::db_query_delete_monster_buff.compose(
                                std::to_string(public_and_private_informations.playerMonster.at(index).id)
                            );
                dbQueryWriteCommon(queryText);
                public_and_private_informations.playerMonster[index].buffs.clear();
            }
            bool endurance_have_change=false;
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
                    /*const std::string &queryText=PreparedDBQueryCommon::db_query_monster_skill.compose(

                                );
                    stringreplaceOne(queryText,"%1",std::to_string(endurance));
                    stringreplaceOne(queryText,"%2",std::to_string(public_and_private_informations.playerMonster.at(index).id));
                    stringreplaceOne(queryText,"%3",std::to_string(public_and_private_informations.playerMonster.at(index).skills.at(sub_index).skill));
                    dbQueryWriteCommon(queryText);*/
                    public_and_private_informations.playerMonster[index].skills[sub_index].endurance=endurance;
                    endurance_have_change=true;
                }
                sub_index++;
            }
            if(endurance_have_change)
                syncMonsterEndurance(public_and_private_informations.playerMonster[index]);
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
    const PlayerMonster * const monster=getCurrentMonster();
    const std::string &queryText=PreparedDBQueryCommon::db_query_update_monster_hp_only.compose(
                std::to_string(monster->hp),
                std::to_string(monster->id)
                );
    dbQueryWriteCommon(queryText);
}

bool Client::botFightCollision(CommonMap *map,const COORD_TYPE &x,const COORD_TYPE &y)
{
    if(isInFight())
    {
        errorOutput("error: map: "+map->map_file+" ("+std::to_string(x)+","+std::to_string(y)+"), is in fight");
        return false;
    }
    std::pair<uint8_t,uint8_t> pos(x,y);
    if(static_cast<MapServer *>(map)->botsFightTrigger.find(pos)!=static_cast<MapServer *>(map)->botsFightTrigger.cend())
    {
        const std::vector<uint32_t> &botList=static_cast<MapServer *>(map)->botsFightTrigger.at(pos);
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

bool Client::learnSkillInternal(const uint8_t &monsterPosition,const uint32_t &skill)
{
    if(monsterPosition>=public_and_private_informations.playerMonster.size())
    {
        errorOutput("The monster is not found: "+std::to_string(monsterPosition));
        return false;
    }
    unsigned int index=monsterPosition;
    const PlayerMonster &monster=public_and_private_informations.playerMonster.at(index);
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
                    const std::string &queryText=PreparedDBQueryCommon::db_query_update_monster_sp_only.compose(
                                std::to_string(public_and_private_informations.playerMonster.at(index).sp),
                                std::to_string(public_and_private_informations.playerMonster.at(index).id)
                                );
                    dbQueryWriteCommon(queryText);
                }
                syncMonsterSkillAndEndurance(public_and_private_informations.playerMonster[index]);
/*                        if(learn.learnSkillLevel==1)
                {
                    PlayerMonster::PlayerSkill temp;
                    temp.skill=skill;
                    temp.level=1;
                    temp.endurance=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.at(temp.skill).level.front().endurance;
                    public_and_private_informations.playerMonster[index].skills.push_back(temp);
                    const std::string &queryText=PreparedDBQueryCommon::db_query_insert_monster_skill;
                    stringreplaceOne(queryText,"%1",std::to_string(monsterId));
                    stringreplaceOne(queryText,"%2",std::to_string(temp.skill));
                    stringreplaceOne(queryText,"%3","1");
                    stringreplaceOne(queryText,"%4",std::to_string(temp.endurance));
                    dbQueryWriteCommon(queryText);
                }
                else
                {
                    public_and_private_informations.playerMonster[index].skills[sub_index2].level++;
                    const std::string &queryText=PreparedDBQueryCommon::db_query_update_monster_skill_level;
                    stringreplaceOne(queryText,"%1",std::to_string(public_and_private_informations.playerMonster.at(index).skills.at(sub_index2).level));
                    stringreplaceOne(queryText,"%2",std::to_string(monsterId));
                    stringreplaceOne(queryText,"%3",std::to_string(skill));
                    dbQueryWriteCommon(queryText);
                }*/
                return true;
            }
        }
        sub_index++;
    }
    errorOutput("The skill "+std::to_string(skill)+" is not into learn skill list for the monster");
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

    //send the network reply
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0xDF;
    posOutput+=1+1+4;

    {
        const std::string &text=otherPlayerBattle->public_and_private_informations.public_informations.pseudo;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=text.size();
        posOutput+=1;
        memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,text.data(),text.size());
        posOutput+=text.size();
    }
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=otherPlayerBattle->public_and_private_informations.public_informations.skinId;
    posOutput+=1;

    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(posOutput-1-1-4);//set the dynamic size
    sendBattleRequest(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
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
        //send the network message
        ProtocolParsingBase::tempBigBufferForOutput[0x00]=0x51;
        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,1);
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

        //send the network message
        uint32_t posOutput=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x52;
        posOutput+=1+4;

        {
            const std::string &text=otherPlayerBattle->public_and_private_informations.public_informations.pseudo;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=text.size();
            posOutput+=1;
            memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,text.data(),text.size());
            posOutput+=text.size();
        }
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=otherPlayerBattle->public_and_private_informations.public_informations.skinId;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=playerMonstersPreview.size();
        posOutput+=1;
        unsigned int index=0;
        while(index<playerMonstersPreview.size() && index<255)
        {
            if(!monsterIsKO(playerMonstersPreview.at(index)))
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x01;
            else
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x02;
            posOutput+=1;
            index++;
        }
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=selectedMonsterNumberToMonsterPlace(getOtherSelectedMonsterNumber());
        posOutput+=1;
        posOutput+=FacilityLib::publicPlayerMonsterToBinary(ProtocolParsingBase::tempBigBufferForOutput+posOutput,FacilityLib::playerMonsterToPublicPlayerMonster(*otherPlayerBattle->getCurrentMonster()));

        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(posOutput-1-4);//set the dynamic size
        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
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
    unsigned int index,master_index;

    //send the network message
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x50;
    posOutput+=1+4;

    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=attackReturn.size();
    posOutput+=1;
    master_index=0;
    while(master_index<attackReturn.size())
    {
        const Skill::AttackReturn &attackReturnTemp=attackReturn.at(master_index);
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=attackReturnTemp.doByTheCurrentMonster;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=attackReturnTemp.attackReturnCase;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=attackReturnTemp.success;
        posOutput+=1;
        *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(attackReturnTemp.attack);
        posOutput+=2;
        //ad buff
        index=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=attackReturnTemp.addBuffEffectMonster.size();
        posOutput+=1;
        while(index<attackReturnTemp.addBuffEffectMonster.size())
        {
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=attackReturnTemp.addBuffEffectMonster.at(index).buff;
            posOutput+=1;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=attackReturnTemp.addBuffEffectMonster.at(index).on;
            posOutput+=1;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=attackReturnTemp.addBuffEffectMonster.at(index).level;
            posOutput+=1;
            index++;
        }
        //remove buff
        index=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=attackReturnTemp.removeBuffEffectMonster.size();
        posOutput+=1;
        while(index<attackReturnTemp.removeBuffEffectMonster.size())
        {
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=attackReturnTemp.removeBuffEffectMonster.at(index).buff;
            posOutput+=1;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=attackReturnTemp.removeBuffEffectMonster.at(index).on;
            posOutput+=1;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=attackReturnTemp.removeBuffEffectMonster.at(index).level;
            posOutput+=1;
            index++;
        }
        //life effect
        index=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=attackReturnTemp.lifeEffectMonster.size();
        posOutput+=1;
        while(index<attackReturnTemp.lifeEffectMonster.size())
        {
            *reinterpret_cast<int32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(attackReturnTemp.lifeEffectMonster.at(index).quantity);
            posOutput+=4;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=attackReturnTemp.lifeEffectMonster.at(index).on;
            posOutput+=1;
            index++;
        }
        //buff effect
        index=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=attackReturnTemp.buffLifeEffectMonster.size();
        posOutput+=1;
        while(index<attackReturnTemp.buffLifeEffectMonster.size())
        {
            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(attackReturnTemp.buffLifeEffectMonster.at(index).quantity);
            posOutput+=4;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=attackReturnTemp.buffLifeEffectMonster.at(index).on;
            posOutput+=1;
            index++;
        }
        master_index++;
    }
    if(otherPlayerBattle!=NULL && otherPlayerBattle->haveMonsterChange())
    {
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=selectedMonsterNumberToMonsterPlace(getOtherSelectedMonsterNumber());
        posOutput+=1;
        posOutput+=FacilityLib::publicPlayerMonsterToBinary(ProtocolParsingBase::tempBigBufferForOutput+posOutput,*getOtherMonster());
    }
    attackReturn.clear();

    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(posOutput-1-4);//set the dynamic size
    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}

void Client::sendBattleMonsterChange()
{
    //send the network message
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x50;
    posOutput+=1+4;

    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0;
    posOutput+=1;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=selectedMonsterNumberToMonsterPlace(getOtherSelectedMonsterNumber());
    posOutput+=1;
    posOutput+=FacilityLib::publicPlayerMonsterToBinary(ProtocolParsingBase::tempBigBufferForOutput+posOutput,*getOtherMonster());

    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(posOutput-1-4);//set the dynamic size
    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
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
                const std::string &queryText=PreparedDBQueryCommon::db_query_update_monster_xp_hp_level.compose(
                            std::to_string(currentMonster->hp),
                            std::to_string(currentMonster->remaining_xp),
                            std::to_string(currentMonster->level),
                            std::to_string(currentMonster->sp),
                            std::to_string(currentMonster->id)
                            );
                dbQueryWriteCommon(queryText);
            }
            else
            {
                const std::string &queryText=PreparedDBQueryCommon::db_query_update_monster_xp.compose(
                            std::to_string(currentMonster->remaining_xp),
                            std::to_string(currentMonster->sp),
                            std::to_string(currentMonster->id)
                            );
                dbQueryWriteCommon(queryText);
            }
        }
        else
        {
            if(haveChangeOfLevel)
            {
                const std::string &queryText=PreparedDBQueryCommon::db_query_update_monster_xp_hp_level.compose(
                            std::to_string(currentMonster->hp),
                            std::to_string(currentMonster->remaining_xp),
                            std::to_string(currentMonster->level),
                            std::to_string(currentMonster->id)
                            );
                dbQueryWriteCommon(queryText);
            }
            else
            {
                const std::string &queryText=PreparedDBQueryCommon::db_query_update_monster_xp.compose(
                            std::to_string(currentMonster->remaining_xp),
                            std::to_string(currentMonster->id)
                            );
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
                    addCash(CommonDatapackServerSpec::commonDatapackServerSpec.botFights.at(botFightId).cash);
                    public_and_private_informations.bot_already_beaten.insert(botFightId);
                    /*const std::string &queryText=PreparedDBQueryServer::db_query_insert_bot_already_beaten;
                    stringreplaceOne(queryText,"%1",std::to_string(character_id));
                    stringreplaceOne(queryText,"%2",std::to_string(botFightId));
                    dbQueryWriteServer(queryText);*/
                    syncBotAlreadyBeaten();
                }
                fightOrBattleFinish(win,botFightId);
                normalOutput("Register the win against the bot fight: "+std::to_string(botFightId));
            }
            else
                normalOutput("Have win agains a wild monster");
        }
        fightOrBattleFinish(win,0);
    }
    return win;
}

bool Client::useSkill(const uint32_t &skill)
{
    normalOutput("use the skill: "+std::to_string(skill));
    if(!isInBattle())//wild or bot
    {
        CommonFightEngine::useSkill(skill);
        return finishTheTurn(!botFightMonsters.empty());
    }
    else
    {
        if(haveBattleAction())
        {
            errorOutput("Have already a battle action");
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
            normalOutput("Have win agains the current monster");
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
            normalOutput("Have win the battle");
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

    char raw_skill[(2+1)*newMonster.skills.size()],raw_skill_endurance[1*newMonster.skills.size()];
    //the skills
    unsigned int sub_index=0;
    while(sub_index<newMonster.skills.size())
    {
        const PlayerMonster::PlayerSkill &skill=newMonster.skills.at(sub_index);
        *reinterpret_cast<uint16_t *>(raw_skill+sub_index*(2+1))=htole16(skill.skill);
        raw_skill[sub_index*(2+1)+2]=skill.level;
        raw_skill_endurance[sub_index]=skill.endurance;
        sub_index++;
    }

    char raw_buff[(1+1+1)*newMonster.buffs.size()];
    //the skills
    sub_index=0;
    while(sub_index<newMonster.buffs.size())
    {
        const PlayerBuff &buff=newMonster.buffs.at(sub_index);
        raw_buff[sub_index*3+0]=buff.buff;
        raw_buff[sub_index*3+1]=buff.level;
        raw_buff[sub_index*3+2]=buff.remainingNumberOfTurn;
        sub_index++;
    }

    if(toStorage)
    {
        public_and_private_informations.warehouse_playerMonster.push_back(newMonster);
        public_and_private_informations.warehouse_playerMonster.back().id=monster_id;
        position=public_and_private_informations.warehouse_playerMonster.size();
        const std::string &queryText=PreparedDBQueryCommon::db_query_insert_warehouse_monster.compose(
                    std::to_string(monster_id),
                    std::to_string(newMonster.hp),
                    std::to_string(newMonster.monster),
                    std::to_string(newMonster.level),
                    std::to_string(newMonster.remaining_xp),
                    std::to_string(newMonster.sp),
                    std::to_string(newMonster.catched_with),
                    std::to_string((uint8_t)newMonster.gender),
                    std::to_string(newMonster.egg_step),
                    std::to_string(character_id),
                    std::to_string(position),
                    binarytoHexa(raw_skill,sizeof(raw_skill)),
                    binarytoHexa(raw_skill_endurance,sizeof(raw_skill_endurance)),
                    binarytoHexa(raw_buff,sizeof(raw_buff))
                    );
        dbQueryWriteCommon(queryText);
    }
    else
    {
        public_and_private_informations.playerMonster.push_back(newMonster);
        public_and_private_informations.playerMonster.back().id=monster_id;
        position=public_and_private_informations.playerMonster.size();
        const std::string &queryText=PreparedDBQueryCommon::db_query_insert_monster.compose(
                    std::to_string(monster_id),
                    std::to_string(newMonster.hp),
                    std::to_string(newMonster.monster),
                    std::to_string(newMonster.level),
                    std::to_string(newMonster.remaining_xp),
                    std::to_string(newMonster.sp),
                    std::to_string(newMonster.catched_with),
                    std::to_string((uint8_t)newMonster.gender),
                    std::to_string(newMonster.egg_step),
                    std::to_string(character_id),
                    std::to_string(position),
                    binarytoHexa(raw_skill,sizeof(raw_skill)),
                    binarytoHexa(raw_skill_endurance,sizeof(raw_skill_endurance)),
                    binarytoHexa(raw_buff,sizeof(raw_buff))
                    );
        dbQueryWriteCommon(queryText);
    }

    /*unsigned int index=0;
    while(index<newMonster.skills.size())
    {
        std::string queryText=PreparedDBQueryCommon::db_query_insert_monster_skill;
        stringreplaceOne(queryText,"%1",std::to_string(monster_id));
        stringreplaceOne(queryText,"%2",std::to_string(newMonster.skills.at(index).skill));
        stringreplaceOne(queryText,"%3",std::to_string(newMonster.skills.at(index).level));
        stringreplaceOne(queryText,"%4",std::to_string(newMonster.skills.at(index).endurance));
        dbQueryWriteCommon(queryText);
        index++;
    }
    index=0;
    while(index<newMonster.buffs.size())
    {
        if(CommonDatapack::commonDatapack.monsterBuffs.at(newMonster.buffs.at(index).buff).level.at(newMonster.buffs.at(index).level).duration==Buff::Duration_Always)
        {
            std::string queryText=PreparedDBQueryCommon::db_query_insert_monster_buff;
            stringreplaceOne(queryText,"%1",std::to_string(monster_id));
            stringreplaceOne(queryText,"%2",std::to_string(newMonster.buffs.at(index).buff));
            stringreplaceOne(queryText,"%3",std::to_string(newMonster.buffs.at(index).level));
            dbQueryWriteCommon(queryText);
        }
        index++;
    }*/
    wildMonsters.erase(wildMonsters.begin());
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
    if(CommonDatapack::commonDatapack.monsterBuffs.at(effect.buff).level.at(effect.level-1).duration==Buff::Duration_Always)
    {
        //if(returnCode==-1)
            switch(effect.on)
            {
                case ApplyOn_AloneEnemy:
                case ApplyOn_AllEnemy:
                if(isInBattle())
                {
                    syncMonsterBuff(*otherPlayerBattle->getCurrentMonster());
                    /*std::string queryText=PreparedDBQueryCommon::db_query_insert_monster_buff;
                    stringreplaceOne(queryText,"%1",std::to_string(->id));
                    stringreplaceOne(queryText,"%2",std::to_string(effect.buff));
                    stringreplaceOne(queryText,"%3",std::to_string(effect.level));
                    dbQueryWriteCommon(queryText);*/
                }
                break;
                case ApplyOn_Themself:
                case ApplyOn_AllAlly:
                {
                    syncMonsterBuff(*getCurrentMonster());
                    /*std::string queryText=PreparedDBQueryCommon::db_query_insert_monster_buff;
                    stringreplaceOne(queryText,"%1",std::to_string(getCurrentMonster()->id));
                    stringreplaceOne(queryText,"%2",std::to_string(effect.buff));
                    stringreplaceOne(queryText,"%3",std::to_string(effect.level));
                    dbQueryWriteCommon(queryText);*/
                }
                break;
                default:
                    errorOutput("Not apply match, can't apply the buff");
                break;
            }
        /*else
            switch(effect.on)
            {
                case ApplyOn_AloneEnemy:
                case ApplyOn_AllEnemy:
                if(isInBattle())
                {
                    std::string queryText=PreparedDBQueryCommon::db_query_update_monster_level;
                    stringreplaceOne(queryText,"%1",std::to_string(otherPlayerBattle->getCurrentMonster()->id));
                    stringreplaceOne(queryText,"%2",std::to_string(effect.buff));
                    stringreplaceOne(queryText,"%3",std::to_string(effect.level));
                    dbQueryWriteCommon(queryText);
                }
                break;
                case ApplyOn_Themself:
                case ApplyOn_AllAlly:
                {
                    std::string queryText=PreparedDBQueryCommon::db_query_update_monster_level;
                    stringreplaceOne(queryText,"%1",std::to_string(getCurrentMonster()->id));
                    stringreplaceOne(queryText,"%2",std::to_string(effect.buff));
                    stringreplaceOne(queryText,"%3",std::to_string(effect.level));
                    dbQueryWriteCommon(queryText);
                }
                break;
                default:
                    errorOutput("Not apply match, can't apply the buff");
                break;
            }*/
    }
    return returnCode;
}

bool Client::moveUpMonster(const uint8_t &number)
{
    if(!CommonFightEngine::moveUpMonster(number))
        return false;
    if(GlobalServerData::serverSettings.fightSync!=GameServerSettings::FightSync_AtTheDisconnexion)
    {
        saveMonsterPosition(public_and_private_informations.playerMonster.at(number-1).id,number);
        saveMonsterPosition(public_and_private_informations.playerMonster.at(number).id,number+1);
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
        saveMonsterPosition(public_and_private_informations.playerMonster.at(number).id,number+1);
        saveMonsterPosition(public_and_private_informations.playerMonster.at(number+1).id,number+2);
    }
    return true;

}

void Client::saveMonsterPosition(const uint32_t &monsterId,const uint8_t &monsterPosition)
{
    const std::string &queryText=PreparedDBQueryCommon::db_query_update_monster_position.compose(
                std::to_string(monsterPosition),
                std::to_string(monsterId)
                );
    dbQueryWriteCommon(queryText);
}

bool Client::changeOfMonsterInFight(const uint8_t &monsterPosition)
{
    const bool &doTurnIfChangeOfMonster=this->doTurnIfChangeOfMonster;

    //save for sync at end of the battle
    PlayerMonster * monster=getCurrentMonster();
    if(monster!=NULL)
        saveMonsterStat(*monster);

    if(!CommonFightEngine::changeOfMonsterInFight(monsterPosition))
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
        if(!haveMoreEndurance() && skill==0 && CommonDatapack::commonDatapack.monsterSkills.find(skill)!=CommonDatapack::commonDatapack.monsterSkills.cend())
            skillLevel=1;
        else
        {
            errorOutput("Unable to fight because the current monster have not the skill "+std::to_string(skill));
            return attackReturnTemp;
        }
    }
    otherPlayerBattle->doTheCurrentMonsterAttack(skill,skillLevel);
    if(currentMonsterIsKO() && haveAnotherMonsterOnThePlayerToFight())
        doTurnIfChangeOfMonster=false;
    return attackReturn.back();
}

Skill::AttackReturn Client::doTheCurrentMonsterAttack(const uint32_t &skill, const uint8_t &skillLevel)
{
    if(!isInBattle())
        return CommonFightEngine::doTheCurrentMonsterAttack(skill,skillLevel);
    attackReturn.push_back(CommonFightEngine::doTheCurrentMonsterAttack(skill,skillLevel));
    Skill::AttackReturn attackReturnTemp=attackReturn.back();
    attackReturnTemp.doByTheCurrentMonster=false;
    otherPlayerBattle->attackReturn.push_back(attackReturnTemp);
    if(currentMonsterIsKO() && haveAnotherMonsterOnThePlayerToFight())
        doTurnIfChangeOfMonster=false;
    return attackReturn.back();
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
        /*std::string queryText=PreparedDBQueryCommon::db_query_monster_skill;
        stringreplaceOne(queryText,"%1",std::to_string(newEndurance));
        stringreplaceOne(queryText,"%2",std::to_string(currentMonster->id));
        stringreplaceOne(queryText,"%3",std::to_string(skill));
        dbQueryWriteCommon(queryText);*/
        syncMonsterEndurance(*currentMonster);
    }
    else
    {
        if(GlobalServerData::serverSettings.fightSync==GameServerSettings::FightSync_AtTheEndOfBattle)
        {
            //deferedEnduranceSync[currentMonster][skill]=newEndurance;
            deferedEnduranceSync.insert(currentMonster);
        }
    }
    return newEndurance;
}

void Client::addPlayerMonster(const std::vector<PlayerMonster> &playerMonster)
{
    CommonFightEngine::addPlayerMonster(playerMonster);
    bool haveChange=false;
    uint32_t index=0;
    while(index<playerMonster.size())
    {
        if(addPlayerMonsterWithChange(playerMonster.at(index)))
            haveChange=true;
        index++;
    }
    if(haveChange)
        updateMonsterInDatabaseEncyclopedia();
        /*if(haveChange)updateMonsterInDatabase();
    else
        updateMonsterInDatabaseAndEncyclopedia();*/
}

void Client::addPlayerMonster(const PlayerMonster &playerMonster)
{
    CommonFightEngine::addPlayerMonster(playerMonster);
    if(addPlayerMonsterWithChange(playerMonster))
        updateMonsterInDatabaseEncyclopedia();
            /*if(haveChange)updateMonsterInDatabase();
        else
            updateMonsterInDatabaseAndEncyclopedia();*/
}

bool Client::addPlayerMonsterWithChange(const PlayerMonster &playerMonster)
{
    if(public_and_private_informations.encyclopedia_monster==NULL)
    {
        errorOutput("public_and_private_informations.encyclopedia_monster==NULL");
        return false;
    }
    const uint16_t bittoUp=playerMonster.monster;
    public_and_private_informations.encyclopedia_monster[bittoUp/8]|=(1<<(7-bittoUp%8));
    if(public_and_private_informations.encyclopedia_monster[bittoUp/8] & (1<<(7-bittoUp%8)))
        return false;
    else
    {
        public_and_private_informations.encyclopedia_monster[bittoUp/8]|=(1<<(7-bittoUp%8));
        return true;
    }
}

void Client::confirmEvolutionTo(PlayerMonster * playerMonster,const uint32_t &monster)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    PlayerMonster * currentMonster=getCurrentMonster();
    const Monster::Stat &currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(currentMonster->monster),currentMonster->level);
    /// \note NO OTHER MONSTER because it's evolution on current monster
    if(currentMonster!=NULL)
        if(currentMonster->hp>currentMonsterStat.hp)
        {
            errorOutput("confirmEvolutionTo() The hp "+std::to_string(currentMonster->hp)+
                        " of current monster "+std::to_string(currentMonster->monster)+
                        " is greater than the max "+std::to_string(currentMonsterStat.hp)
                        );
            return;
        }
    #endif
    CommonFightEngine::confirmEvolutionTo(playerMonster,monster);
    const std::string &queryText=PreparedDBQueryCommon::db_query_update_monster_and_hp.compose(
                std::to_string(playerMonster->hp),
                std::to_string(playerMonster->monster),
                std::to_string(playerMonster->id)
                );
    dbQueryWriteCommon(queryText);
}

void Client::confirmEvolution(const uint8_t &monsterPosition)
{
    if(monsterPosition>=public_and_private_informations.playerMonster.size())
    {
        errorOutput("Monster for evolution not found");
        return;
    }
    unsigned int index=monsterPosition;
    const Monster &monsterInformations=CommonDatapack::commonDatapack.monsters.at(public_and_private_informations.playerMonster.at(index).monster);
    unsigned int sub_index=0;
    while(sub_index<monsterInformations.evolutions.size())
    {
        if(
                (monsterInformations.evolutions.at(sub_index).type==Monster::EvolutionType_Level && monsterInformations.evolutions.at(sub_index).level<=public_and_private_informations.playerMonster.at(index).level)
                ||
                (monsterInformations.evolutions.at(sub_index).type==Monster::EvolutionType_Trade && GlobalServerData::serverPrivateVariables.tradedMonster.find(public_and_private_informations.playerMonster.at(index).id)!=GlobalServerData::serverPrivateVariables.tradedMonster.cend())
        )
        {
            confirmEvolutionTo(&public_and_private_informations.playerMonster[index],monsterInformations.evolutions.at(sub_index).evolveTo);
            return;
        }
        sub_index++;
    }
    errorOutput("Evolution not found");
}

void Client::hpChange(PlayerMonster * currentMonster, const uint32_t &newHpValue)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        PlayerMonster * currentMonster=getCurrentMonster();
        PublicPlayerMonster * otherMonster=getOtherMonster();
        if(currentMonster!=NULL)
        {
            const Monster::Stat &currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(currentMonster->monster),currentMonster->level);
            if(currentMonster->hp>currentMonsterStat.hp)
            {
                errorOutput("hpChange() The hp "+std::to_string(currentMonster->monster)+
                            " of current monster "+std::to_string(currentMonster->hp)+
                            " is greater than the max "+std::to_string(currentMonsterStat.hp)
                            );
                return;
            }
        }
        if(otherMonster!=NULL)
        {
            const Monster::Stat &otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(otherMonster->monster),otherMonster->level);
            if(otherMonster->hp>otherMonsterStat.hp)
            {
                errorOutput("hpChange() The hp "+std::to_string(otherMonster->monster)+
                            " of other monster "+std::to_string(otherMonster->hp)+
                            " is greater than the max "+std::to_string(otherMonsterStat.hp)
                            );
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
            const Monster::Stat &currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(currentMonster->monster),currentMonster->level);
            if(currentMonster->hp>currentMonsterStat.hp)
            {
                errorOutput("hpChange() after The hp "+std::to_string(currentMonster->monster)+
                            " of current monster "+std::to_string(currentMonster->hp)+
                            " is greater than the max "+std::to_string(currentMonsterStat.hp)
                            );
                return;
            }
        }
        if(otherMonster!=NULL)
        {
            const Monster::Stat &otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(otherMonster->monster),otherMonster->level);
            if(otherMonster->hp>otherMonsterStat.hp)
            {
                errorOutput("hpChange() after The hp "+std::to_string(otherMonster->monster)+
                            " of other monster "+std::to_string(otherMonster->hp)+
                            " is greater than the max "+std::to_string(otherMonsterStat.hp)
                            );
                return;
            }
        }
    }
    #endif
    const std::string &queryText=PreparedDBQueryCommon::db_query_update_monster_hp_only.compose(
                std::to_string(newHpValue),
                std::to_string(currentMonster->id)
                );
    dbQueryWriteCommon(queryText);
}

bool Client::removeBuffOnMonster(PlayerMonster * currentMonster, const uint32_t &buffId)
{
    if(currentMonster==NULL)
    {
        errorOutput("removeBuffOnMonster(): PlayerMonster * currentMonster==NULL");
        return false;
    }
    const bool returnVal=CommonFightEngine::removeBuffOnMonster(currentMonster,buffId);
    if(returnVal)
    {
        /*cosnt std::string &queryText=PreparedDBQueryCommon::db_query_delete_monster_specific_buff;
        stringreplaceOne(queryText,"%1",std::to_string(currentMonster->id));
        stringreplaceOne(queryText,"%2",std::to_string(buffId));
        dbQueryWriteCommon(queryText);*/
        syncMonsterBuff(*currentMonster);
    }
    return returnVal;
}

bool Client::removeAllBuffOnMonster(PlayerMonster * currentMonster)
{
    const bool &returnVal=CommonFightEngine::removeAllBuffOnMonster(currentMonster);
    if(returnVal)
    {
        /*std::string queryText=PreparedDBQueryCommon::db_query_delete_monster_buff;
        stringreplaceOne(queryText,"%1",std::to_string(currentMonster->id));
        dbQueryWriteCommon(queryText);*/
        syncMonsterBuff(*currentMonster);
    }
    return returnVal;
}

bool Client::addLevel(PlayerMonster * monster, const uint8_t &numberOfLevel)
{
    if(!CommonFightEngine::addLevel(monster,numberOfLevel))
        return false;
    const std::string &queryText=PreparedDBQueryCommon::db_query_update_monster_hp_and_level.compose(
                std::to_string(monster->hp),
                std::to_string(monster->level),
                std::to_string(monster->id)
                );
/*    stringreplaceOne(queryText,"%1",std::to_string(monster->hp));
    stringreplaceOne(queryText,"%2",std::to_string(monster->level));
    stringreplaceOne(queryText,"%3",std::to_string(monster->id));*/
    dbQueryWriteCommon(queryText);
    return true;
}

bool Client::addSkill(PlayerMonster * currentMonster,const PlayerMonster::PlayerSkill &skill)
{
    if(!CommonFightEngine::addSkill(currentMonster,skill))
        return false;
    /*std::string queryText=PreparedDBQueryCommon::db_query_insert_monster_skill;
    stringreplaceOne(queryText,"%1",std::to_string(currentMonster->id));
    stringreplaceOne(queryText,"%2",std::to_string(skill.skill));
    stringreplaceOne(queryText,"%3",std::to_string(skill.level));
    stringreplaceOne(queryText,"%4",std::to_string(skill.endurance));
    dbQueryWriteCommon(queryText);*/
    syncMonsterSkillAndEndurance(*currentMonster);
    return true;
}

bool Client::setSkillLevel(PlayerMonster * currentMonster,const unsigned int &index,const uint8_t &level)
{
    if(!CommonFightEngine::setSkillLevel(currentMonster,index,level))
        return false;
    /*std::string queryText=PreparedDBQueryCommon::db_query_update_monster_skill_level;
    stringreplaceOne(queryText,"%1",std::to_string(level));
    stringreplaceOne(queryText,"%2",std::to_string(currentMonster->id));
    stringreplaceOne(queryText,"%3",std::to_string(currentMonster->skills.at(index).skill));
    dbQueryWriteCommon(queryText);*/
    syncMonsterSkillAndEndurance(*currentMonster);
    return true;
}

bool Client::removeSkill(PlayerMonster * currentMonster,const unsigned int &index)
{
    if(!CommonFightEngine::removeSkill(currentMonster,index))
        return false;
    /*const std::string &queryText=PreparedDBQueryCommon::db_query_delete_monster_specific_skill;
    stringreplaceOne(queryText,"%1",std::to_string(currentMonster->id));
    stringreplaceOne(queryText,"%2",std::to_string(currentMonster->skills.at(index).skill));
    dbQueryWriteCommon(queryText);*/
    syncMonsterSkillAndEndurance(*currentMonster);
    return true;
}
