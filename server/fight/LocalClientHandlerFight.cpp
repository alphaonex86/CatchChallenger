#include "../base/GlobalServerData.hpp"
#include "../base/MapServer.hpp"
#include "../base/Client.hpp"
#include "../base/Client.hpp"
#include "../base/PreparedDBQuery.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include "../../general/base/CommonDatapackServerSpec.hpp"
#include "../../general/base/CommonSettingsServer.hpp"
#include "../../general/base/FacilityLib.hpp"
#include "../base/MapManagement/ClientWithMap.hpp"

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
    {
        saveCurrentMonsterStat();
        if(!public_and_private_informations.playerMonster.empty())
            if(addToEncyclopedia(public_and_private_informations.playerMonster.at(public_and_private_informations.playerMonster.size()-1).monster))
                updateMonsterInDatabaseEncyclopedia();
    }
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
        if(CatchChallenger::CommonDatapack::commonDatapack.get_monsters().find(monster.monster)==CatchChallenger::CommonDatapack::commonDatapack.get_monsters().cend())
        {
            errorOutput("saveMonsterStat() The monster "+std::to_string(monster.monster)+
                        " is not into the monster list ("+std::to_string(CatchChallenger::CommonDatapack::commonDatapack.get_monsters().size())+")");
            abort();
            return;
        }
        Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.get_monsters().at(monster.monster),monster.level);
        if(monster.hp>currentMonsterStat.hp)
        {
            errorOutput("saveMonsterStat() The hp "+std::to_string(monster.hp)+
                        " of current monster "+std::to_string(monster.monster)+
                        " is greater than the max "+std::to_string(currentMonsterStat.hp)
                        );
            abort();
            return;
        }
    }
    #endif
    //save into the db
    if(GlobalServerData::serverSettings.fightSync==GameServerSettings::FightSync_AtTheEndOfBattle)
    {
        #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
        if(CommonSettingsServer::commonSettingsServer.useSP)
            GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_monster_xp_hp_level.asyncWrite({
                        std::to_string(monster.hp),
                        std::to_string(monster.remaining_xp),
                        std::to_string(monster.level),
                        std::to_string(monster.sp),
                        std::to_string(monster.id)
                        });
        else
            GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_monster_xp_hp_level.asyncWrite({
                        std::to_string(monster.hp),
                        std::to_string(monster.remaining_xp),
                        std::to_string(monster.level),
                        std::to_string(monster.id)
                        });
        #elif CATCHCHALLENGER_DB_BLACKHOLE
        #elif CATCHCHALLENGER_DB_FILE
        #else
        #error Define what do here
        #endif
        /*auto i = deferedEndurance.begin();
        while (i != deferedEndurance.cend())
        {
            auto j = i->second.begin();
            while (j != i->second.cend())
            {
                std::string queryText=GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_monster_skill;
                stringreplaceOne(queryText,"%1",std::to_string(j->second));
                stringreplaceOne(queryText,"%2",std::to_string(i->first));
                stringreplaceOne(queryText,"%3",std::to_string(j->first));
                dbQueryWriteCommon(queryText);
                ++j;
            }
            ++i;
        }
        deferedEndurance.clear();*/

        if(!monster.skills.empty())
            syncMonsterSkillAndEndurance(monster);
    }
}

bool Client::checkKOCurrentMonsters()
{
    if(getCurrentMonster()->hp==0)
    {
        #ifdef CATCHCHALLENGER_DEBUG_FIGHT
        normalOutput("You current monster ("+std::to_string(getCurrentMonster()->monster)+") is KO");
        #endif
        saveStat();
        return true;
    }
    return false;
}

bool Client::checkLoose(bool withTeleport)
{
    if(!haveMonsters())
        return false;
    if(!currentMonsterIsKO())
        return false;
    if(!haveAnotherMonsterOnThePlayerToFight())
    {
        #ifdef CATCHCHALLENGER_DEBUG_FIGHT
        normalOutput("You have lost, tp to "+rescue.map->map_file+" ("+std::to_string(rescue.x)+","+std::to_string(rescue.y)+") and heal");
        #endif
        doTurnIfChangeOfMonster=true;
        //teleport
        if(withTeleport)
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
            const Monster::Stat &stat=getStat(CatchChallenger::CommonDatapack::commonDatapack.get_monsters().at(public_and_private_informations.playerMonster.at(index).monster),public_and_private_informations.playerMonster.at(index).level);
            if(public_and_private_informations.playerMonster.at(index).hp!=stat.hp)
            {
                public_and_private_informations.playerMonster[index].hp=stat.hp;
                #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
                GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_monster_hp_only.asyncWrite({
                            std::to_string(public_and_private_informations.playerMonster.at(index).hp),
                            std::to_string(public_and_private_informations.playerMonster.at(index).id)
                            });
                #elif CATCHCHALLENGER_DB_BLACKHOLE
                #elif CATCHCHALLENGER_DB_FILE
                #else
                #error Define what do here
                #endif
            }
            if(!public_and_private_informations.playerMonster.at(index).buffs.empty())
            {
                #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
                GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_delete_monster_buff.asyncWrite({std::to_string(public_and_private_informations.playerMonster.at(index).id)});
                #elif CATCHCHALLENGER_DB_BLACKHOLE
                #elif CATCHCHALLENGER_DB_FILE
                #else
                #error Define what do here
                #endif
                public_and_private_informations.playerMonster[index].buffs.clear();
            }
            bool endurance_have_change=false;
            sub_index=0;
            while(sub_index<public_and_private_informations.playerMonster.at(index).skills.size())
            {
                uint8_t endurance=CatchChallenger::CommonDatapack::commonDatapack.get_monsterSkills().at(
                        public_and_private_informations.playerMonster.at(index).skills.at(sub_index).skill
                        )
                        .level.at(public_and_private_informations.playerMonster.at(index).skills.at(sub_index).level-1).endurance;
                if(public_and_private_informations.playerMonster.at(index).skills.at(sub_index).endurance!=endurance)
                {
                    /*const std::string &queryText=GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_monster_skill.compose(

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

bool Client::botFightCollision(const Map_server_MapVisibility_Simple_StoreOnSender &map,const COORD_TYPE &x,const COORD_TYPE &y)
{
    if(isInFight())
    {
        errorOutput("error: map: "+map.map_file+" ("+std::to_string(x)+","+std::to_string(y)+"), is in fight");
        return false;
    }
    std::pair<uint8_t,uint8_t> pos(x,y);
    if(map.botsFightTrigger.find(pos)!=map.botsFightTrigger.cend())
    {
        const std::vector<uint8_t> &botList=map.botsFightTrigger.at(pos);
        unsigned int index=0;
        while(index<botList.size())
        {
            const uint16_t &botFightId=botList.at(index);
            if(public_and_private_informations.bot_already_beaten!=NULL)
            {
                if(!haveBeatBot(botFightId))
                {
                    normalOutput("is now in fight on map "+map.map_file+" ("+std::to_string(x)+","+std::to_string(y)+") with the bot "+std::to_string(botFightId));
                    botFightStart(botFightId);
                    return true;
                }
            }
            index++;
        }
    }

    /// no fight in this zone
    return false;
}

bool Client::botFightStart(const std::pair<CATCHCHALLENGER_TYPE_MAPID/*mapId*/,uint8_t/*botId*/> &botFight)
{
    if(isInFight())
    {
        errorOutput("error: is already in fight");
        return false;
    }
    if(CommonDatapackServerSpec::commonDatapackServerSpec.get_botFights().find(botFight)==CommonDatapackServerSpec::commonDatapackServerSpec.get_botFights().cend())
    {
        errorOutput("error: bot id "+std::to_string(botFight)+" not found");
        return false;
    }
    const BotFight &botFight=CommonDatapackServerSpec::commonDatapackServerSpec.get_botFights().at(botFight);
    if(botFight.monsters.empty())
    {
        errorOutput("error: bot id "+std::to_string(botFight)+" have no monster to fight");
        return false;
    }
    startTheFight();
    botFightCash=botFight.cash;
    unsigned int index=0;
    while(index<botFight.monsters.size())
    {
        const BotFight::BotFightMonster &monster=botFight.monsters.at(index);
        const Monster::Stat &stat=getStat(CommonDatapack::commonDatapack.get_monsters().at(monster.id),monster.level);
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
    this->botFight=botFight;
    return true;
}

void Client::setInCityCapture(const bool &isInCityCapture)
{
    this->isInCityCapture=isInCityCapture;
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



bool Client::isInFight() const
{
    if(CommonFightEngine::isInFight())
        return true;
    return otherPlayerBattle!=NULL || battleIsValidated;
}

uint8_t Client::getOtherSelectedMonsterNumber() const
{
    if(!isInBattle())
        return 0;
    else
        return otherPlayerBattle->getCurrentSelectedMonsterNumber();
}

bool Client::currentMonsterAttackFirst(const PlayerMonster * currentMonster,const PublicPlayerMonster * otherMonster) const
{
    if(isInBattle())
    {
        const Monster::Stat &currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.get_monsters().at(currentMonster->monster),currentMonster->level);
        const Monster::Stat &otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.get_monsters().at(otherMonster->monster),otherMonster->level);
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

//return true if change level, multiplicator do at datapack loading
bool Client::giveXP(int xp)
{
    const bool &haveChangeOfLevel=CommonFightEngine::giveXP(xp);
    if(GlobalServerData::serverSettings.fightSync==GameServerSettings::FightSync_AtEachTurn)
    {
        #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
        auto currentMonster=getCurrentMonster();
        if(haveChangeOfLevel)
            GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_monster_xp_hp_level.asyncWrite({
                        std::to_string(currentMonster->hp),
                        std::to_string(currentMonster->remaining_xp),
                        std::to_string(currentMonster->level),
                        std::to_string(currentMonster->id)
                        });
        else
            GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_monster_xp.asyncWrite({
                        std::to_string(currentMonster->remaining_xp),
                        std::to_string(currentMonster->id)
                        });
        #elif CATCHCHALLENGER_DB_BLACKHOLE
        (void)currentMonster;
        #elif CATCHCHALLENGER_DB_FILE
        #else
        #error Define what do here
        #endif
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
                #ifndef EPOLLCATCHCHALLENGERSERVER
                if(!isInCityCapture)
                #endif
                {
                    addCash(CommonDatapackServerSpec::commonDatapackServerSpec.get_botFights().at(botFightId).cash);
                    if(public_and_private_informations.bot_already_beaten!=NULL)
                    {
                        public_and_private_informations.bot_already_beaten[botFightId/8]|=(1<<(7-botFightId%8));
                        /*const std::string &queryText=GlobalServerData::serverPrivateVariables.preparedDBQueryServer.db_query_insert_bot_already_beaten;
                        stringreplaceOne(queryText,"%1",std::to_string(character_id));
                        stringreplaceOne(queryText,"%2",std::to_string(botFightId));
                        dbQueryWriteServer(queryText);*/
                        syncBotAlreadyBeaten();
                    }
                }
                fightOrBattleFinish(win,botFightId);
                normalOutput("Register the win against the bot fight: "+std::to_string(botFightId));
            }
            else
                normalOutput("Have win agains a wild monster");
        }
        fightOrBattleFinish(win,0);
    }
    #ifdef CATCHCHALLENGER_DEBUG_FIGHT
    displayCurrentStatus();
    #endif
    return win;
}

#ifdef CATCHCHALLENGER_DEBUG_FIGHT
void Client::displayCurrentStatus()
{
    PlayerMonster * currentMonster=getCurrentMonster();
    if(currentMonster==NULL)
        return;
    std::cout << "current monster: " << currentMonster->hp;
    PublicPlayerMonster * otherMonster=getOtherMonster();
    if(otherMonster!=NULL)
        std::cout << ", other monster: " << otherMonster->hp;
    std::cout << std::endl;
}
#endif

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
    if(otherPlayerBattle==NULL)
        return false;
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

bool Client::haveMonsterChange() const
{
    return mMonsterChange;
}

bool Client::moveUpMonster(const uint8_t &number)
{
    if(!CommonFightEngine::moveUpMonster(number))
    {
        errorOutput("Move monster have failed");
        return false;
    }
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
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_monster_position.asyncWrite({
                std::to_string(monsterPosition),
                std::to_string(monsterId)
                });
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    (void)monsterId;
    (void)monsterPosition;
    #else
    #error Define what do here
    #endif
}

bool Client::changeOfMonsterInFight(const uint8_t &monsterPosition)
{
    const bool /*&:reference drop: prevent change during battle*/doTurnIfChangeOfMonster=this->doTurnIfChangeOfMonster;

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
    attackReturnTemp.item=0;
    attackReturnTemp.monsterPlace=0;
    attackReturnTemp.on_current_monster=true;
    attackReturnTemp.publicPlayerMonster.catched_with=0;
    attackReturnTemp.publicPlayerMonster.gender=Gender_Unknown;
    attackReturnTemp.publicPlayerMonster.hp=0;
    attackReturnTemp.publicPlayerMonster.level=0;
    attackReturnTemp.publicPlayerMonster.monster=0;
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
    const uint16_t &skill=otherPlayerBattle->getCurrentSkill();
    uint8_t skillLevel=otherPlayerBattle->getSkillLevel(skill);
    if(skillLevel==0)
    {
        if(!haveMoreEndurance() && skill==0 && CommonDatapack::commonDatapack.get_monsterSkills().find(skill)!=CommonDatapack::commonDatapack.get_monsterSkills().cend())
            skillLevel=1;
        else
        {
            errorOutput("Unable to fight because the current monster have not the skill "+std::to_string(skill)+" and is level 0");
            return attackReturnTemp;
        }
    }
    otherPlayerBattle->doTheCurrentMonsterAttack(skill,skillLevel);
    if(currentMonsterIsKO() && haveAnotherMonsterOnThePlayerToFight())
        doTurnIfChangeOfMonster=false;
    return attackReturn.back();
}

Skill::AttackReturn Client::doTheCurrentMonsterAttack(const uint16_t &skill, const uint8_t &skillLevel)
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

std::vector<uint8_t> Client::addPlayerMonster(const std::vector<PlayerMonster> &playerMonster)
{
    const auto &returnVar=CommonFightEngine::addPlayerMonster(playerMonster);
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
    return returnVar;
}

std::vector<uint8_t> Client::addPlayerMonster(const PlayerMonster &playerMonster)
{
    const auto &returnVar=CommonFightEngine::addPlayerMonster(playerMonster);
    if(addPlayerMonsterWithChange(playerMonster))
        updateMonsterInDatabaseEncyclopedia();
            /*if(haveChange)updateMonsterInDatabase();
        else
            updateMonsterInDatabaseAndEncyclopedia();*/
    return returnVar;
}

bool Client::addPlayerMonsterWithChange(const PlayerMonster &playerMonster)
{
    if(public_and_private_informations.encyclopedia_monster==NULL)
    {
        errorOutput("public_and_private_informations.encyclopedia_monster==NULL");
        return false;
    }
    const uint16_t bittoUp=playerMonster.monster;
    if(public_and_private_informations.encyclopedia_monster[bittoUp/8] & (1<<(7-bittoUp%8)))
        return false;
    else
    {
        public_and_private_informations.encyclopedia_monster[bittoUp/8]|=(1<<(7-bittoUp%8));
        return true;
    }
}

void Client::confirmEvolutionTo(PlayerMonster * playerMonster, const uint16_t &monster)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    PlayerMonster * currentMonster=getCurrentMonster();
    const Monster::Stat &currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.get_monsters().at(currentMonster->monster),currentMonster->level);
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
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_monster_and_hp.asyncWrite({
                std::to_string(playerMonster->hp),
                std::to_string(playerMonster->monster),
                std::to_string(playerMonster->id)
                });
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif
    if(addToEncyclopedia(playerMonster->monster))
        updateMonsterInDatabaseEncyclopedia();
}

bool Client::addToEncyclopedia(const uint16_t &monster)
{
    if(public_and_private_informations.encyclopedia_monster==NULL)
    {
        errorOutput("public_and_private_informations.encyclopedia_monster==NULL");
        return false;
    }
    const uint16_t bittoUp=monster;
    if(public_and_private_informations.encyclopedia_monster[bittoUp/8] & (1<<(7-bittoUp%8)))
        return false;
    else
    {
        public_and_private_informations.encyclopedia_monster[bittoUp/8]|=(1<<(7-bittoUp%8));
        return true;
    }
}

void Client::confirmEvolution(const uint8_t &monsterPosition)
{
    if(monsterPosition>=public_and_private_informations.playerMonster.size())
    {
        errorOutput("Monster for evolution not found");
        return;
    }
    unsigned int index=monsterPosition;
    const Monster &monsterInformations=CommonDatapack::commonDatapack.get_monsters().at(public_and_private_informations.playerMonster.at(index).monster);
    unsigned int sub_index=0;
    while(sub_index<monsterInformations.evolutions.size())
    {
        if(
                (monsterInformations.evolutions.at(sub_index).type==Monster::EvolutionType_Level &&
                 monsterInformations.evolutions.at(sub_index).data.level<=public_and_private_informations.playerMonster.at(index).level)
                ||
                (monsterInformations.evolutions.at(sub_index).type==Monster::EvolutionType_Trade &&
                 GlobalServerData::serverPrivateVariables.tradedMonster.find(public_and_private_informations.playerMonster.at(index).id)!=
                 GlobalServerData::serverPrivateVariables.tradedMonster.cend())
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
            const Monster::Stat &currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.get_monsters().at(currentMonster->monster),currentMonster->level);
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
            const Monster::Stat &otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.get_monsters().at(otherMonster->monster),otherMonster->level);
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
            const Monster::Stat &currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.get_monsters().at(currentMonster->monster),currentMonster->level);
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
            const Monster::Stat &otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.get_monsters().at(otherMonster->monster),otherMonster->level);
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
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_monster_hp_only.asyncWrite({
                std::to_string(newHpValue),
                std::to_string(currentMonster->id)
                });
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif
}

bool Client::addLevel(PlayerMonster * monster, const uint8_t &numberOfLevel)
{
    if(!CommonFightEngine::addLevel(monster,numberOfLevel))
        return false;
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_monster_hp_and_level.asyncWrite({
                std::to_string(monster->hp),
                std::to_string(monster->level),
                std::to_string(monster->id)
                });
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif
    return true;
}

uint8_t Client::selectedMonsterNumberToMonsterPlace(const uint8_t &selectedMonsterNumber)
{
    return selectedMonsterNumber+1;
}
