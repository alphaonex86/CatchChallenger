#include "LocalClientHandlerFight.h"
#include "../VariableServer.h"
#include "../base/GlobalServerData.h"
#include "../base/MapServer.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/FacilityLib.h"

using namespace CatchChallenger;

LocalClientHandlerFight::LocalClientHandlerFight()
{
    otherPlayerBattle=NULL;
    battleIsValidated=false;
    player_informations=NULL;
    botFightCash=0;
    mHaveCurrentSkill=false;
    mMonsterChange=false;
    isInCityCapture=false;
}

LocalClientHandlerFight::~LocalClientHandlerFight()
{
}

void LocalClientHandlerFight::getRandomNumberIfNeeded() const
{
    if(randomSeeds.size()<=CATCHCHALLENGER_SERVER_MIN_RANDOM_LIST_SIZE)
        emit askRandomNumber();
}

bool LocalClientHandlerFight::tryEscape()
{
    bool escapeSuccess=CommonFightEngine::tryEscape();
    if(escapeSuccess)//check if is in fight
        saveCurrentMonsterStat();
    else
        finishTheTurn(false);
    return escapeSuccess;
}

quint32 LocalClientHandlerFight::tryCapture(const quint32 &item)
{
    bool captureSuccess=CommonFightEngine::tryCapture(item);
    if(captureSuccess)//check if is in fight
        saveCurrentMonsterStat();
    else
        finishTheTurn(false);
    return captureSuccess;
}

bool LocalClientHandlerFight::getBattleIsValidated()
{
    return battleIsValidated;
}

void LocalClientHandlerFight::saveCurrentMonsterStat()
{
    if(player_informations->public_and_private_informations.playerMonster.isEmpty())
        return;//no monsters
    PlayerMonster * monster=getCurrentMonster();
    if(monster==NULL)
        return;//problem with the selection
    saveMonsterStat(*monster);
}

void LocalClientHandlerFight::saveMonsterStat(const PlayerMonster &monster)
{
    //save into the db
    if(GlobalServerData::serverSettings.database.fightSync==ServerSettings::Database::FightSync_AtTheEndOfBattle)
    {
        switch(GlobalServerData::serverSettings.database.type)
        {
            default:
            case ServerSettings::Database::DatabaseType_Mysql:
                emit dbQuery(QString("UPDATE `monster` SET `hp`=%2,`xp`=%3,`level`=%4,`sp`=%5 WHERE `id`=%1;")
                             .arg(monster.id)
                             .arg(monster.hp)
                             .arg(monster.remaining_xp)
                             .arg(monster.level)
                             .arg(monster.sp)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                emit dbQuery(QString("UPDATE monster SET hp=%2,xp=%3,level=%4,sp=%5 WHERE id=%1;")
                             .arg(monster.id)
                             .arg(monster.hp)
                             .arg(monster.remaining_xp)
                             .arg(monster.level)
                             .arg(monster.sp)
                             );
            break;
        }
    }
}

void LocalClientHandlerFight::savePosition()
{
    const QList<PlayerMonster> &playerMonsterList=getPlayerMonster();
    int index=0;
    while(index<playerMonsterList.size())
    {
        const PlayerMonster &playerMonster=playerMonsterList.at(index);
        saveMonsterPosition(index+1,playerMonster.id);
        index++;
    }
}

bool LocalClientHandlerFight::checkKOCurrentMonsters()
{
    if(getCurrentMonster()->hp==0)
    {
        #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
        emit message(QString("You current monster (%1) is KO").arg(getCurrentMonster()->monster));
        #endif
        saveStat();
        return true;
    }
    return false;
}

void LocalClientHandlerFight::setVariableInternal(Player_internal_informations *player_informations)
{
    this->player_informations=player_informations;
    CommonFightEngine::setVariable(&player_informations->public_and_private_informations);
    updateCanDoFight();
}

bool LocalClientHandlerFight::checkLoose()
{
    if(!haveMonsters())
        return false;
    if(!currentMonsterIsKO())
        return false;
    if(!haveAnotherMonsterOnThePlayerToFight())
    {
        #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
        emit message(QString("You have lost, tp to %1 (%2,%3) and heal").arg(player_informations->rescue.map->map_file).arg(player_informations->rescue.x).arg(player_informations->rescue.y));
        #endif
        doTurnIfChangeOfMonster=true;
        //teleport
        emit teleportTo(player_informations->rescue.map,player_informations->rescue.x,player_informations->rescue.y,player_informations->rescue.orientation);
        //regen all the monsters
        bool tempInBattle=isInBattle();
        healAllMonsters();
        fightFinished();
        if(tempInBattle)
            updateCanDoFight();
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        emit message("You lost the battle");
        if(!ableToFight)
        {
            emit error(QString("after lost in fight, remain unable to do a fight"));
            return true;
        }
        #endif
        emit fightOrBattleFinish(false,0);
        return true;
    }
    return false;
}

void LocalClientHandlerFight::healAllMonsters()
{
    int sub_index;
    int index=0;
    while(index<player_informations->public_and_private_informations.playerMonster.size())
    {
        if(player_informations->public_and_private_informations.playerMonster.at(index).egg_step==0)
        {
            const Monster::Stat &stat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters[player_informations->public_and_private_informations.playerMonster.at(index).monster],player_informations->public_and_private_informations.playerMonster.at(index).level);
            if(player_informations->public_and_private_informations.playerMonster[index].hp!=stat.hp)
            {
                player_informations->public_and_private_informations.playerMonster[index].hp=stat.hp;
                switch(GlobalServerData::serverSettings.database.type)
                {
                    default:
                    case ServerSettings::Database::DatabaseType_Mysql:
                        emit dbQuery(QString("UPDATE `monster` SET `hp`=%1 WHERE `id`=%2;")
                                     .arg(player_informations->public_and_private_informations.playerMonster[index].hp)
                                     .arg(player_informations->public_and_private_informations.playerMonster[index].id)
                                     );
                    break;
                    case ServerSettings::Database::DatabaseType_SQLite:
                        emit dbQuery(QString("UPDATE monster SET hp=%1 WHERE id=%2;")
                                     .arg(player_informations->public_and_private_informations.playerMonster[index].hp)
                                     .arg(player_informations->public_and_private_informations.playerMonster[index].id)
                                     );
                    break;
                }
            }
            if(!player_informations->public_and_private_informations.playerMonster[index].buffs.isEmpty())
            {
                switch(GlobalServerData::serverSettings.database.type)
                {
                    default:
                    case ServerSettings::Database::DatabaseType_Mysql:
                        emit dbQuery(QString("DELETE FROM `monster_buff` WHERE `monster`=%1")
                                 .arg(player_informations->public_and_private_informations.playerMonster[index].id)
                                 );
                    break;
                    case ServerSettings::Database::DatabaseType_SQLite:
                        emit dbQuery(QString("DELETE FROM monster_buff WHERE monster=%1")
                                 .arg(player_informations->public_and_private_informations.playerMonster[index].id)
                                 );
                    break;
                }
                player_informations->public_and_private_informations.playerMonster[index].buffs.clear();
            }
            sub_index=0;
            while(sub_index<player_informations->public_and_private_informations.playerMonster[index].skills.size())
            {
                int endurance=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills[
                        player_informations->public_and_private_informations.playerMonster[index].skills.at(sub_index).skill
                        ]
                        .level.at(player_informations->public_and_private_informations.playerMonster[index].skills.at(sub_index).level-1).endurance;
                if(player_informations->public_and_private_informations.playerMonster[index].skills[sub_index].endurance!=endurance)
                {
                    switch(GlobalServerData::serverSettings.database.type)
                    {
                        default:
                        case ServerSettings::Database::DatabaseType_Mysql:
                            emit dbQuery(QString("UPDATE `monster_skill` SET `endurance`=%1 WHERE `monster`=%2 AND `skill`=%3;")
                                         .arg(player_informations->public_and_private_informations.playerMonster[index].skills[sub_index].endurance)
                                         .arg(player_informations->public_and_private_informations.playerMonster[index].id)
                                         .arg(player_informations->public_and_private_informations.playerMonster[index].skills[sub_index].skill)
                                         );
                        break;
                        case ServerSettings::Database::DatabaseType_SQLite:
                            emit dbQuery(QString("UPDATE monster_skill SET endurance=%1 WHERE monster=%2 AND skill=%3;")
                                         .arg(player_informations->public_and_private_informations.playerMonster[index].skills[sub_index].endurance)
                                         .arg(player_informations->public_and_private_informations.playerMonster[index].id)
                                         .arg(player_informations->public_and_private_informations.playerMonster[index].skills[sub_index].skill)
                                         );
                        break;
                    }
                    player_informations->public_and_private_informations.playerMonster[index].skills[sub_index].endurance=endurance;
                }
                sub_index++;
            }
        }
        index++;
    }
    if(!isInBattle())
        updateCanDoFight();
}

void LocalClientHandlerFight::fightFinished()
{
    battleFinished();
    CommonFightEngine::fightFinished();
}

void LocalClientHandlerFight::syncForEndOfTurn()
{
    if(GlobalServerData::serverSettings.database.fightSync==ServerSettings::Database::FightSync_AtEachTurn)
        saveStat();
}

void LocalClientHandlerFight::saveStat()
{
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            emit dbQuery(QString("UPDATE `monster` SET `hp`=%1 WHERE `id`=%2;")
                         .arg(getCurrentMonster()->hp)
                         .arg(getCurrentMonster()->id)
                         );
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            emit dbQuery(QString("UPDATE monster SET hp=%1 WHERE id=%2;")
                         .arg(getCurrentMonster()->hp)
                         .arg(getCurrentMonster()->id)
                         );
        break;
    }
}

bool LocalClientHandlerFight::botFightCollision(Map *map,const COORD_TYPE &x,const COORD_TYPE &y)
{
    if(isInFight())
    {
        emit error(QString("error: map: %1 (%2,%3), is in fight").arg(map->map_file).arg(x).arg(y));
        return false;
    }
    if(player_informations->isFake)
        return false;
    QList<quint32> botList=static_cast<MapServer *>(map)->botsFightTrigger.values(QPair<quint8,quint8>(x,y));
    int index=0;
    while(index<botList.size())
    {
        const quint32 &botFightId=botList.at(index);
        if(!player_informations->public_and_private_informations.bot_already_beaten.contains(botFightId))
        {
            emit message(QString("is now in fight on map %1 (%2,%3) with the bot %4").arg(map->map_file).arg(x).arg(y).arg(botFightId));
            botFightStart(botFightId);
            return true;
        }
        index++;
    }

    /// no fight in this zone
    return false;
}

bool LocalClientHandlerFight::botFightStart(const quint32 &botFightId)
{
    if(isInFight())
    {
        emit error(QString("error: is already in fight"));
        return false;
    }
    if(!CommonDatapack::commonDatapack.botFights.contains(botFightId))
    {
        emit error(QString("error: bot id %1 not found").arg(botFightId));
        return false;
    }
    const BotFight &botFight=CommonDatapack::commonDatapack.botFights[botFightId];
    if(botFight.monsters.isEmpty())
    {
        emit error(QString("error: bot id %1 have no monster to fight").arg(botFightId));
        return false;
    }
    startTheFight();
    botFightCash=botFight.cash;
    int index=0;
    while(index<botFight.monsters.size())
    {
        const BotFight::BotFightMonster &monster=botFight.monsters.at(index);
        const Monster::Stat &stat=getStat(CommonDatapack::commonDatapack.monsters[monster.id],monster.level);
        botFightMonsters << FacilityLib::botFightMonsterToPlayerMonster(monster,stat);
        index++;
    }
    #ifdef CATCHCHALLENGER_SERVER_EXTRA_CHECK
    if(botFightMonsters.isEmpty())
    {
        emit error(QString("error: after the bot add, remaing empty").arg(botFightId));
        return false;
    }
    #endif
    this->botFightId=botFightId;
    return true;
}

void LocalClientHandlerFight::setInCityCapture(const bool &isInCityCapture)
{
    this->isInCityCapture=isInCityCapture;
}

LocalClientHandlerFight * LocalClientHandlerFight::getOtherPlayerBattle() const
{
    return otherPlayerBattle;
}

PublicPlayerMonster *LocalClientHandlerFight::getOtherMonster() const
{
    if(otherPlayerBattle!=NULL)
    {
        PlayerMonster * otherPlayerBattleCurrentMonster=otherPlayerBattle->getCurrentMonster();
        if(otherPlayerBattleCurrentMonster!=NULL)
            return otherPlayerBattleCurrentMonster;
        else
            emit error("Is in battle but the other monster is null");
    }
    return CommonFightEngine::getOtherMonster();
}

quint8 LocalClientHandlerFight::getOneSeed(const quint8 &max)
{
    quint8 seed=CommonFightEngine::getOneSeed(max);
    getRandomNumberIfNeeded();
    return seed;
}

void LocalClientHandlerFight::wildDrop(const quint32 &monster)
{
    QList<MonsterDrops> drops=GlobalServerData::serverPrivateVariables.monsterDrops.values(monster);
    if(player_informations->questsDrop.contains(monster))
        drops+=player_informations->questsDrop.values(monster);
    int index=0;
    bool success;
    quint32 quantity;
    while(index<drops.size())
    {
        if(drops.at(index).luck==100)
            success=true;
        else
        {
            if(rand()%100<(qint8)drops.at(index).luck)
                success=true;
            else
                success=false;
        }
        if(success)
        {
            if(drops.at(index).quantity_max==1)
                quantity=1;
            else
                quantity=rand()%(drops.at(index).quantity_max-drops.at(index).quantity_min+1)+drops.at(index).quantity_min;
            #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
            emit message(QString("Win %1 item: %2").arg(quantity).arg(drops.at(index).item));
            #endif
            emit addObjectAndSend(drops.at(index).item,quantity);
        }
        index++;
    }
}

bool LocalClientHandlerFight::isInFight() const
{
    if(CommonFightEngine::isInFight())
        return true;
    return otherPlayerBattle!=NULL || battleIsValidated;
}

bool LocalClientHandlerFight::learnSkillInternal(const quint32 &monsterId,const quint32 &skill)
{
    int index=0;
    while(index<player_informations->public_and_private_informations.playerMonster.size())
    {
        const PlayerMonster &monster=player_informations->public_and_private_informations.playerMonster.at(index);
        if(monster.id==monsterId)
        {
            int sub_index2=0;
            while(sub_index2<monster.skills.size())
            {
                if(monster.skills.at(sub_index2).skill==skill)
                    break;
                sub_index2++;
            }
            int sub_index=0;
            while(sub_index<CommonDatapack::commonDatapack.monsters[monster.monster].learn.size())
            {
                const Monster::AttackToLearn &learn=CommonDatapack::commonDatapack.monsters[monster.monster].learn.at(sub_index);
                if(learn.learnAtLevel<=monster.level && learn.learnSkill==skill)
                {
                    if((sub_index2==monster.skills.size() && learn.learnSkillLevel==1) || (monster.skills[sub_index2].level+1)==learn.learnSkillLevel)
                    {
                        quint32 sp=CommonDatapack::commonDatapack.monsterSkills[learn.learnSkill].level.at(learn.learnSkillLevel).sp_to_learn;
                        if(sp>monster.sp)
                        {
                            emit error(QString("The attack require %1 sp to be learned, you have only %2").arg(sp).arg(monster.sp));
                            return false;
                        }
                        player_informations->public_and_private_informations.playerMonster[index].sp-=sp;
                        switch(GlobalServerData::serverSettings.database.type)
                        {
                            default:
                            case ServerSettings::Database::DatabaseType_Mysql:
                                emit dbQuery(QString("UPDATE `monster` SET `sp`=%1 WHERE `id`=%2;")
                                             .arg(player_informations->public_and_private_informations.playerMonster[index].sp)
                                             .arg(monsterId)
                                             );
                            break;
                            case ServerSettings::Database::DatabaseType_SQLite:
                                emit dbQuery(QString("UPDATE monster SET sp=%1 WHERE id=%2;")
                                             .arg(player_informations->public_and_private_informations.playerMonster[index].sp)
                                             .arg(monsterId)
                                             );
                            break;
                        }
                        if(learn.learnSkillLevel==1)
                        {
                            PlayerMonster::PlayerSkill temp;
                            temp.skill=skill;
                            temp.level=1;
                            temp.endurance=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills[temp.skill].level.first().endurance;
                            player_informations->public_and_private_informations.playerMonster[index].skills << temp;
                            switch(GlobalServerData::serverSettings.database.type)
                            {
                                default:
                                case ServerSettings::Database::DatabaseType_Mysql:
                                    emit dbQuery(QString("INSERT INTO `monster_skill`(`monster`,`skill`,`level`,`endurance`) VALUES(%1,%2,1,%3);")
                                                 .arg(monsterId)
                                                 .arg(temp.skill)
                                                 .arg(temp.endurance)
                                                 );
                                break;
                                case ServerSettings::Database::DatabaseType_SQLite:
                                    emit dbQuery(QString("INSERT INTO monster_skill(monster,skill,level,endurance) VALUES(%1,%2,1,%3);")
                                                 .arg(monsterId)
                                                 .arg(temp.skill)
                                                 .arg(temp.endurance)
                                                 );
                                break;
                            }
                        }
                        else
                        {
                            player_informations->public_and_private_informations.playerMonster[index].skills[sub_index2].level++;
                            switch(GlobalServerData::serverSettings.database.type)
                            {
                                default:
                                case ServerSettings::Database::DatabaseType_Mysql:
                                    emit dbQuery(QString("UPDATE `monster_skill` SET `level`=%1 WHERE `monster`=%2 AND `skill`=%3;")
                                                 .arg(player_informations->public_and_private_informations.playerMonster[index].skills[sub_index2].level)
                                                 .arg(monsterId)
                                                 .arg(skill)
                                                 );
                                break;
                                case ServerSettings::Database::DatabaseType_SQLite:
                                    emit dbQuery(QString("UPDATE monster_skill SET level=%1 WHERE monster=%2 AND skill=%3;")
                                                 .arg(player_informations->public_and_private_informations.playerMonster[index].skills[sub_index2].level)
                                                 .arg(monsterId)
                                                 .arg(skill)
                                                 );
                                break;
                            }
                        }
                        return true;
                    }
                }
                sub_index++;
            }
            emit error(QString("The skill %1 is not into learn skill list for the monster").arg(skill));
            return false;
        }
        index++;
    }
    emit error(QString("The monster is not found: %1").arg(monsterId));
    return false;
}

bool LocalClientHandlerFight::isInBattle() const
{
    return (otherPlayerBattle!=NULL && battleIsValidated);
}

void LocalClientHandlerFight::registerBattleRequest(LocalClientHandlerFight * otherPlayerBattle)
{
    if(isInBattle())
    {
        emit message("Already in battle, internal error");
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    emit message(QString("%1 have requested battle with you").arg(otherPlayerBattle->player_informations->public_and_private_informations.public_informations.pseudo));
    #endif
    this->otherPlayerBattle=otherPlayerBattle;
    otherPlayerBattle->otherPlayerBattle=this;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << otherPlayerBattle->player_informations->public_and_private_informations.public_informations.skinId;
    emit sendBattleRequest(otherPlayerBattle->player_informations->rawPseudo+outputData);
}

void LocalClientHandlerFight::battleCanceled()
{
    if(otherPlayerBattle!=NULL)
        otherPlayerBattle->internalBattleCanceled(true);
    internalBattleCanceled(true);
}

void LocalClientHandlerFight::battleAccepted()
{
    if(otherPlayerBattle!=NULL)
        otherPlayerBattle->internalBattleAccepted(true);
    internalBattleAccepted(true);
}

void LocalClientHandlerFight::battleFakeAccepted(LocalClientHandlerFight * otherPlayer)
{
    battleFakeAcceptedInternal(otherPlayer);
    otherPlayer->battleFakeAcceptedInternal(this);
    otherPlayerBattle->internalBattleAccepted(true);
    internalBattleAccepted(true);
}

void LocalClientHandlerFight::battleFakeAcceptedInternal(LocalClientHandlerFight * otherPlayer)
{
    this->otherPlayerBattle=otherPlayer;
}

void LocalClientHandlerFight::battleFinished()
{
    if(!battleIsValidated)
        return;
    if(otherPlayerBattle==NULL)
        return;
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    emit message("Battle finished");
    #endif
    otherPlayerBattle->resetBattleAction();
    resetBattleAction();
    LocalClientHandlerFight *tempOtherPlayerBattle=otherPlayerBattle;
    otherPlayerBattle->battleFinishedReset();
    battleFinishedReset();
    updateCanDoFight();
    tempOtherPlayerBattle->updateCanDoFight();
}

void LocalClientHandlerFight::battleFinishedReset()
{
    otherPlayerBattle=NULL;
    battleIsValidated=false;
    mHaveCurrentSkill=false;
    mMonsterChange=false;
}

void LocalClientHandlerFight::resetTheBattle()
{
    //reset out of battle
    mHaveCurrentSkill=false;
    mMonsterChange=false;
    battleIsValidated=false;
    otherPlayerBattle=NULL;
    updateCanDoFight();
}

void LocalClientHandlerFight::internalBattleCanceled(const bool &send)
{
    if(otherPlayerBattle==NULL)
    {
        //emit message("Battle already canceled");
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    emit message("Battle canceled");
    #endif
    bool needUpdateCanDoFight=false;
    if(battleIsValidated)
        needUpdateCanDoFight=true;
    otherPlayerBattle=NULL;
    if(send)
    {
            emit sendFullPacket(0xE0,0x0007);
            emit receiveSystemText(QString("Battle declined"));
    }
    battleIsValidated=false;
    mHaveCurrentSkill=false;
    mMonsterChange=false;
    if(needUpdateCanDoFight)
        updateCanDoFight();
}

void LocalClientHandlerFight::internalBattleAccepted(const bool &send)
{
    if(otherPlayerBattle==NULL)
    {
        emit message("Can't accept battle if not in battle");
        return;
    }
    if(battleIsValidated)
    {
        emit message("Battle already validated");
        return;
    }
    if(!otherPlayerBattle->getAbleToFight())
    {
        emit error("The other player can't fight");
        return;
    }
    if(!getAbleToFight())
    {
        emit error("You can't fight");
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    emit message("Battle accepted");
    #endif
    startTheFight();
    battleIsValidated=true;
    mHaveCurrentSkill=false;
    mMonsterChange=false;
    if(send)
    {
        QList<PlayerMonster> playerMonstersPreview=otherPlayerBattle->player_informations->public_and_private_informations.playerMonster;
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);
        out << otherPlayerBattle->player_informations->public_and_private_informations.public_informations.skinId;
        out << (quint8)playerMonstersPreview.size();
        int index=0;
        while(index<playerMonstersPreview.size() && index<255)
        {
            if(!monsterIsKO(playerMonstersPreview.at(index)))
                out << (quint8)0x01;
            else
                out << (quint8)0x02;
            index++;
        }
        out << (quint8)selectedMonsterNumberToMonsterPlace(getOtherSelectedMonsterNumber());
        QByteArray firstValidOtherPlayerMonster=FacilityLib::publicPlayerMonsterToBinary(FacilityLib::playerMonsterToPublicPlayerMonster(*otherPlayerBattle->getCurrentMonster()));
        emit sendFullPacket(0xE0,0x0008,otherPlayerBattle->player_informations->rawPseudo+outputData+firstValidOtherPlayerMonster);
    }
}

bool LocalClientHandlerFight::haveBattleAction() const
{
    return mHaveCurrentSkill || mMonsterChange;
}

void LocalClientHandlerFight::resetBattleAction()
{
    mHaveCurrentSkill=false;
    mMonsterChange=false;
}

quint8 LocalClientHandlerFight::getOtherSelectedMonsterNumber() const
{
    if(!isInBattle())
        return 0;
    else
        return otherPlayerBattle->getCurrentSelectedMonsterNumber();
}

void LocalClientHandlerFight::haveUsedTheBattleAction()
{
    mHaveCurrentSkill=false;
    mMonsterChange=false;
}

bool LocalClientHandlerFight::currentMonsterAttackFirst(const PlayerMonster * currentMonster,const PublicPlayerMonster * otherMonster) const
{
    if(isInBattle())
    {
        Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters[currentMonster->monster],currentMonster->level);
        Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters[otherMonster->monster],otherMonster->level);
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

quint8 LocalClientHandlerFight::selectedMonsterNumberToMonsterPlace(const quint8 &selectedMonsterNumber)
{
    return selectedMonsterNumber+1;
}

void LocalClientHandlerFight::sendBattleReturn()
{
    QByteArray binarypublicPlayerMonster;
    int index,master_index;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);

    out << (quint8)attackReturn.size();
    master_index=0;
    while(master_index<attackReturn.size())
    {
        const Skill::AttackReturn &attackReturnTemp=attackReturn.at(master_index);
        out << (quint8)attackReturnTemp.doByTheCurrentMonster;
        out << (quint8)attackReturnTemp.success;
        out << (quint32)attackReturnTemp.attack;
        //ad buff
        index=0;
        out << (quint8)attackReturnTemp.addBuffEffectMonster.size();
        while(index<attackReturnTemp.addBuffEffectMonster.size())
        {
            out << (quint32)attackReturnTemp.addBuffEffectMonster.at(index).buff;
            out << (quint8)attackReturnTemp.addBuffEffectMonster.at(index).on;
            out << (quint8)attackReturnTemp.addBuffEffectMonster.at(index).level;
            index++;
        }
        //remove buff
        index=0;
        out << (quint8)attackReturnTemp.removeBuffEffectMonster.size();
        while(index<attackReturnTemp.removeBuffEffectMonster.size())
        {
            out << (quint32)attackReturnTemp.removeBuffEffectMonster.at(index).buff;
            out << (quint8)attackReturnTemp.removeBuffEffectMonster.at(index).on;
            out << (quint8)attackReturnTemp.removeBuffEffectMonster.at(index).level;
            index++;
        }
        //life effect
        index=0;
        out << (quint8)attackReturnTemp.lifeEffectMonster.size();
        while(index<attackReturnTemp.lifeEffectMonster.size())
        {
            out << (qint32)attackReturnTemp.lifeEffectMonster.at(index).quantity;
            out << (quint8)attackReturnTemp.lifeEffectMonster.at(index).on;
            index++;
        }
        //buff effect
        index=0;
        out << (quint8)attackReturnTemp.buffLifeEffectMonster.size();
        while(index<attackReturnTemp.buffLifeEffectMonster.size())
        {
            out << (qint32)attackReturnTemp.buffLifeEffectMonster.at(index).quantity;
            out << (quint8)attackReturnTemp.buffLifeEffectMonster.at(index).on;
            index++;
        }
        master_index++;
    }
    if(otherPlayerBattle==NULL)
        out << (quint8)0x00;
    else if(otherPlayerBattle->haveMonsterChange())
    {
        out << (quint8)selectedMonsterNumberToMonsterPlace(getOtherSelectedMonsterNumber());;
        binarypublicPlayerMonster=FacilityLib::publicPlayerMonsterToBinary(*getOtherMonster());
    }
    else
        out << (quint8)0x00;
    attackReturn.clear();

    emit sendFullPacket(0xE0,0x0006,outputData+binarypublicPlayerMonster);
}

void LocalClientHandlerFight::sendBattleMonsterChange()
{
    QByteArray binarypublicPlayerMonster;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0;
    out << (quint8)selectedMonsterNumberToMonsterPlace(getOtherSelectedMonsterNumber());;
    binarypublicPlayerMonster=FacilityLib::publicPlayerMonsterToBinary(*getOtherMonster());
    emit sendFullPacket(0xE0,0x0006,outputData+binarypublicPlayerMonster);
}

//return true if change level
bool LocalClientHandlerFight::giveXPSP(int xp,int sp)
{
    bool haveChangeOfLevel=CommonFightEngine::giveXPSP(xp,sp);
    if(GlobalServerData::serverSettings.database.fightSync==ServerSettings::Database::FightSync_AtEachTurn)
    {
        if(haveChangeOfLevel)
            switch(GlobalServerData::serverSettings.database.type)
            {
                default:
                case ServerSettings::Database::DatabaseType_Mysql:
                    emit dbQuery(QString("UPDATE `monster` SET `level`=%2,`hp`=%3,`xp`=%2,`sp`=%3 WHERE `id`=%1;")
                                 .arg(getCurrentMonster()->id)
                                 .arg(getCurrentMonster()->level)
                                 .arg(getCurrentMonster()->hp)
                                 .arg(getCurrentMonster()->remaining_xp)
                                 .arg(getCurrentMonster()->sp)
                                 );
                break;
                case ServerSettings::Database::DatabaseType_SQLite:
                    emit dbQuery(QString("UPDATE monster SET level=%2,hp=%3,xp=%2,sp=%3 WHERE id=%1;")
                                 .arg(getCurrentMonster()->id)
                                 .arg(getCurrentMonster()->level)
                                 .arg(getCurrentMonster()->hp)
                                 .arg(getCurrentMonster()->remaining_xp)
                                 .arg(getCurrentMonster()->sp)
                                 );
                break;
            }
        else
            switch(GlobalServerData::serverSettings.database.type)
            {
                default:
                case ServerSettings::Database::DatabaseType_Mysql:
                    emit dbQuery(QString("UPDATE `monster` SET `xp`=%2,`sp`=%3 WHERE `id`=%1;")
                                 .arg(getCurrentMonster()->id)
                                 .arg(getCurrentMonster()->remaining_xp)
                                 .arg(getCurrentMonster()->sp)
                                 );
                break;
                case ServerSettings::Database::DatabaseType_SQLite:
                    emit dbQuery(QString("UPDATE monster SET xp=%2,sp=%3 WHERE id=%1;")
                                 .arg(getCurrentMonster()->id)
                                 .arg(getCurrentMonster()->remaining_xp)
                                 .arg(getCurrentMonster()->sp)
                                 );
                break;
            }
    }
    return haveChangeOfLevel;
}

bool LocalClientHandlerFight::finishTheTurn(const bool &isBot)
{
    bool win=!currentMonsterIsKO() && otherMonsterIsKO();
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
                    addCash(CommonDatapack::commonDatapack.botFights[botFightId].cash);
                    player_informations->public_and_private_informations.bot_already_beaten << botFightId;
                    switch(GlobalServerData::serverSettings.database.type)
                    {
                        default:
                        case ServerSettings::Database::DatabaseType_Mysql:
                            emit dbQuery(QString("INSERT INTO `bot_already_beaten`(`character`,`botfight_id`) VALUES(%1,%2);")
                                         .arg(player_informations->character_id)
                                         .arg(botFightId)
                                         );
                        break;
                        case ServerSettings::Database::DatabaseType_SQLite:
                            emit dbQuery(QString("INSERT INTO bot_already_beaten(character,botfight_id) VALUES(%1,%2);")
                                         .arg(player_informations->character_id)
                                         .arg(botFightId)
                                         );
                        break;
                    }
                }
                emit fightOrBattleFinish(win,botFightId);
                emit message(QString("Register the win against the bot fight: %1").arg(botFightId));
            }
            else
                emit message(QString("Have win agains a wild monster"));
        }
        emit fightOrBattleFinish(win,0);
    }
    return win;
}

bool LocalClientHandlerFight::useSkill(const quint32 &skill)
{
    emit message(QString("use the skill: %1").arg(skill));
    if(!isInBattle())//wild or bot
    {
        bool isBot=!botFightMonsters.isEmpty();
        CommonFightEngine::useSkill(skill);
        return finishTheTurn(isBot);
    }
    else
    {
        if(haveBattleAction())
        {
            emit error("Have already a battle action");
            return false;
        }
        mHaveCurrentSkill=true;
        mCurrentSkillId=skill;
        return checkIfCanDoTheTurn();
    }
}

bool LocalClientHandlerFight::bothRealPlayerIsReady() const
{
    if(!haveBattleAction())
        return false;
    if(!otherPlayerBattle->haveBattleAction())
        return false;
    return true;
}

bool LocalClientHandlerFight::checkIfCanDoTheTurn()
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
            emit message(QString("Have win agains the current monster"));
        }
        else
        {
            bool youWin=haveAnotherMonsterOnThePlayerToFight();
            bool theOtherWin=otherPlayerBattle->haveAnotherMonsterOnThePlayerToFight();
            dropKOCurrentMonster();
            dropKOOtherMonster();
            LocalClientHandlerFight *tempOtherPlayerBattle=otherPlayerBattle;
            checkLoose();
            tempOtherPlayerBattle->checkLoose();
            emit message(QString("Have win the battle"));
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

void LocalClientHandlerFight::emitBattleWin()
{
    emit fightOrBattleFinish(true,0);
}

bool LocalClientHandlerFight::dropKOOtherMonster()
{
    bool commonReturn=CommonFightEngine::dropKOOtherMonster();

    bool battleReturn=false;
    if(isInBattle())
        battleReturn=otherPlayerBattle->dropKOCurrentMonster();
    else
    {
        if(GlobalServerData::serverSettings.database.fightSync==ServerSettings::Database::FightSync_AtTheEndOfBattle)
        {
            if(!isInFight())
                saveCurrentMonsterStat();
        }
    }

    return commonReturn || battleReturn;
}

quint32 LocalClientHandlerFight::captureAWild(const bool &toStorage, const PlayerMonster &newMonster)
{
    int position=999999;
    quint32 monster_id;
    {
        QMutexLocker(&GlobalServerData::serverPrivateVariables.monsterIdMutex);
        GlobalServerData::serverPrivateVariables.maxMonsterId++;
        monster_id=GlobalServerData::serverPrivateVariables.maxMonsterId;
    }
    if(toStorage)
    {
        player_informations->public_and_private_informations.warehouse_playerMonster << newMonster;
        player_informations->public_and_private_informations.warehouse_playerMonster.last().id=monster_id;
        position=player_informations->public_and_private_informations.warehouse_playerMonster.size();
    }
    else
    {
        player_informations->public_and_private_informations.playerMonster << newMonster;
        player_informations->public_and_private_informations.playerMonster.last().id=monster_id;
        position=player_informations->public_and_private_informations.playerMonster.size();
    }
    QString place;
    if(toStorage)
        place="'warehouse'";
    else
        place="'wear'";
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            emit dbQuery(QString("INSERT INTO `monster`(`id`,`hp`,`character`,`monster`,`level`,`xp`,`sp`,`captured_with`,`gender`,`egg_step`,`character_origin`,`place`,`position`) VALUES(%1,%2);")
                         .arg(QString("%1,%2,%3,%4,%5,%6,%7,%8,\"%9\"")
                              .arg(monster_id)
                              .arg(newMonster.hp)
                              .arg(player_informations->character_id)
                              .arg(newMonster.monster)
                              .arg(newMonster.level)
                              .arg(newMonster.remaining_xp)
                              .arg(newMonster.sp)
                              .arg(newMonster.captured_with)
                              .arg(FacilityLib::genderToString(newMonster.gender))
                              )
                         .arg(QString("%1,%2,%3,%4")
                              .arg(newMonster.egg_step)
                              .arg(player_informations->character_id)
                              .arg(place)
                              .arg(position)
                              )
                         );
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            emit dbQuery(QString("INSERT INTO monster(id,hp,character,monster,level,xp,sp,captured_with,gender,egg_step,character_origin,place,position) VALUES(%1,%2);")
                         .arg(QString("%1,%2,%3,%4,%5,%6,%7,%8,\"%9\"")
                              .arg(monster_id)
                              .arg(newMonster.hp)
                              .arg(player_informations->character_id)
                              .arg(newMonster.monster)
                              .arg(newMonster.level)
                              .arg(newMonster.remaining_xp)
                              .arg(newMonster.sp)
                              .arg(newMonster.captured_with)
                              .arg(FacilityLib::genderToString(newMonster.gender))
                              )
                         .arg(QString("%1,%2,%3,%4")
                              .arg(newMonster.egg_step)
                              .arg(player_informations->character_id)
                              .arg(place)
                              .arg(position)
                              )
                         );
        break;
    }
    int index=0;
    while(index<newMonster.skills.size())
    {
        switch(GlobalServerData::serverSettings.database.type)
        {
            default:
            case ServerSettings::Database::DatabaseType_Mysql:
                emit dbQuery(QString("INSERT INTO `monster_skill`(`monster`,`skill`,`level`,`endurance`) VALUES(%1,%2,%3,%4);")
                             .arg(monster_id)
                             .arg(newMonster.skills.at(index).skill)
                             .arg(newMonster.skills.at(index).level)
                             .arg(newMonster.skills.at(index).endurance)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                emit dbQuery(QString("INSERT INTO monster_skill(monster,skill,level,endurance) VALUES(%1,%2,%3,%4);")
                             .arg(monster_id)
                             .arg(newMonster.skills.at(index).skill)
                             .arg(newMonster.skills.at(index).level)
                             .arg(newMonster.skills.at(index).endurance)
                             );
            break;
        }
        index++;
    }
    index=0;
    while(index<newMonster.buffs.size())
    {
        switch(GlobalServerData::serverSettings.database.type)
        {
            default:
            case ServerSettings::Database::DatabaseType_Mysql:
                emit dbQuery(QString("INSERT INTO `monster_buff`(`monster`,`buff`,`level`) VALUES(%1,%2,%3);")
                             .arg(monster_id)
                             .arg(newMonster.buffs.at(index).buff)
                             .arg(newMonster.buffs.at(index).level)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                emit dbQuery(QString("INSERT INTO monster_buff(monster,buff,level) VALUES(%1,%2,%3);")
                             .arg(monster_id)
                             .arg(newMonster.buffs.at(index).buff)
                             .arg(newMonster.buffs.at(index).level)
                             );
            break;
        }
        index++;
    }
    wildMonsters.removeFirst();
    return monster_id;
}

bool LocalClientHandlerFight::haveCurrentSkill() const
{
    return mHaveCurrentSkill;
}

quint32 LocalClientHandlerFight::getCurrentSkill() const
{
    return mCurrentSkillId;
}

bool LocalClientHandlerFight::haveMonsterChange() const
{
    return mMonsterChange;
}

void LocalClientHandlerFight::requestFight(const quint32 &fightId)
{
    botFightStart(fightId);
}

int LocalClientHandlerFight::addCurrentBuffEffect(const Skill::BuffEffect &effect)
{
    const int &returnCode=CommonFightEngine::addCurrentBuffEffect(effect);
    if(returnCode==-2)
        return returnCode;
    if(CommonDatapack::commonDatapack.monsterBuffs[effect.buff].level.at(effect.level-1).duration==Buff::Duration_Always)
    {
        if(returnCode==-1)
            switch(effect.on)
            {
                case ApplyOn_AloneEnemy:
                case ApplyOn_AllEnemy:
                if(isInBattle())
                    switch(GlobalServerData::serverSettings.database.type)
                    {
                        default:
                        case ServerSettings::Database::DatabaseType_Mysql:
                            emit dbQuery(QString("INSERT INTO `monster_buff`(`monster`,`buff`,`level`) VALUES(%1,%2,%3);")
                                     .arg(otherPlayerBattle->getCurrentMonster()->id)
                                     .arg(effect.buff)
                                     .arg(effect.level)
                                     );
                        break;
                        case ServerSettings::Database::DatabaseType_SQLite:
                            emit dbQuery(QString("INSERT INTO monster_buff(monster,buff,level) VALUES(%1,%2,%3);")
                                     .arg(otherPlayerBattle->getCurrentMonster()->id)
                                     .arg(effect.buff)
                                     .arg(effect.level)
                                     );
                        break;
                    }
                break;
                case ApplyOn_Themself:
                case ApplyOn_AllAlly:
                switch(GlobalServerData::serverSettings.database.type)
                {
                    default:
                    case ServerSettings::Database::DatabaseType_Mysql:
                        emit dbQuery(QString("INSERT INTO `monster_buff`(`monster`,`buff`,`level`) VALUES(%1,%2,%3);")
                                 .arg(getCurrentMonster()->id)
                                 .arg(effect.buff)
                                 .arg(effect.level)
                                 );
                    break;
                    case ServerSettings::Database::DatabaseType_SQLite:
                        emit dbQuery(QString("INSERT INTO monster_buff(monster,buff,level) VALUES(%1,%2,%3);")
                                 .arg(getCurrentMonster()->id)
                                 .arg(effect.buff)
                                 .arg(effect.level)
                                 );
                    break;
                }
                break;
                default:
                    emit error("Not apply match, can't apply the buff");
                break;
            }
        else
            switch(effect.on)
            {
                case ApplyOn_AloneEnemy:
                case ApplyOn_AllEnemy:
                if(isInBattle())
                    switch(GlobalServerData::serverSettings.database.type)
                    {
                        default:
                        case ServerSettings::Database::DatabaseType_Mysql:
                            emit dbQuery(QString("UPDATE `monster` SET `level`=%3 WHERE `monster`=%1 AND buff=%2;")
                                         .arg(otherPlayerBattle->getCurrentMonster()->id)
                                         .arg(effect.buff)
                                         .arg(effect.level)
                                         );
                        break;
                        case ServerSettings::Database::DatabaseType_SQLite:
                            emit dbQuery(QString("UPDATE monster SET level=%3 WHERE monster=%1 AND buff=%2;")
                                         .arg(otherPlayerBattle->getCurrentMonster()->id)
                                         .arg(effect.buff)
                                         .arg(effect.level)
                                         );
                        break;
                    }
                break;
                case ApplyOn_Themself:
                case ApplyOn_AllAlly:
                switch(GlobalServerData::serverSettings.database.type)
                {
                    default:
                    case ServerSettings::Database::DatabaseType_Mysql:
                        emit dbQuery(QString("UPDATE `monster` SET `level`=%3 WHERE `monster`=%1 AND `buff`=%2;")
                                     .arg(getCurrentMonster()->id)
                                     .arg(effect.buff)
                                     .arg(effect.level)
                                     );
                    break;
                    case ServerSettings::Database::DatabaseType_SQLite:
                        emit dbQuery(QString("UPDATE monster SET level=%3 WHERE monster=%1 AND buff=%2;")
                                     .arg(getCurrentMonster()->id)
                                     .arg(effect.buff)
                                     .arg(effect.level)
                                     );
                    break;
                }
                break;
                default:
                    emit error("Not apply match, can't apply the buff");
                break;
            }
    }
    return returnCode;
}

bool LocalClientHandlerFight::moveUpMonster(const quint8 &number)
{
    if(!CommonFightEngine::moveUpMonster(number))
        return false;
    if(GlobalServerData::serverSettings.database.fightSync!=ServerSettings::Database::FightSync_AtTheDisconnexion)
    {
        saveMonsterPosition(player_informations->public_and_private_informations.playerMonster[number-1].id,number);
        saveMonsterPosition(player_informations->public_and_private_informations.playerMonster[number].id,number+1);
    }
    return true;
}

bool LocalClientHandlerFight::moveDownMonster(const quint8 &number)
{
    if(!CommonFightEngine::moveDownMonster(number))
    {
        emit error("Move monster have failed");
        return false;
    }
    if(GlobalServerData::serverSettings.database.fightSync!=ServerSettings::Database::FightSync_AtTheDisconnexion)
    {
        saveMonsterPosition(player_informations->public_and_private_informations.playerMonster[number].id,number+1);
        saveMonsterPosition(player_informations->public_and_private_informations.playerMonster[number+1].id,number+2);
    }
    return true;

}

void LocalClientHandlerFight::saveMonsterPosition(const quint32 &monsterId,const quint8 &monsterPosition)
{
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            emit dbQuery(QString("UPDATE `monster` SET `position`=%1 WHERE `id`=%2;")
                         .arg(monsterPosition)
                         .arg(monsterId)
                         );
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            emit dbQuery(QString("UPDATE monster SET position=%1 WHERE id=%2;")
                         .arg(monsterPosition)
                         .arg(monsterId)
                         );
        break;
    }
}

bool LocalClientHandlerFight::changeOfMonsterInFight(const quint32 &monsterId)
{
    bool doTurnIfChangeOfMonster=this->doTurnIfChangeOfMonster;

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

bool LocalClientHandlerFight::doTheOtherMonsterTurn()
{
    if(!isInBattle())
        return CommonFightEngine::doTheOtherMonsterTurn();
    if(!bothRealPlayerIsReady())
        return false;
    if(!otherPlayerBattle->haveCurrentSkill())
        return false;
    return CommonFightEngine::doTheOtherMonsterTurn();
}

Skill::AttackReturn LocalClientHandlerFight::generateOtherAttack()
{
    Skill::AttackReturn attackReturnTemp;
    attackReturnTemp.attack=0;
    attackReturnTemp.doByTheCurrentMonster=false;
    attackReturnTemp.success=false;
    if(!isInBattle())
        return CommonFightEngine::generateOtherAttack();
    if(!bothRealPlayerIsReady())
    {
        emit error("Both player is not ready at generateOtherAttack()");
        return attackReturnTemp;
    }
    if(!otherPlayerBattle->haveCurrentSkill())
    {
        emit error("The other player have not skill at generateOtherAttack()");
        return attackReturnTemp;
    }
    quint32 skill=otherPlayerBattle->getCurrentSkill();
    quint8 skillLevel=getSkillLevel(skill);
    if(skillLevel==0)
    {
        if(!haveMoreEndurance() && skill==0 && CommonDatapack::commonDatapack.monsterSkills.contains(skill))
            skillLevel=1;
        else
        {
            emit error(QString("Unable to fight because the current monster have not the skill %3").arg(skill));
            return attackReturnTemp;
        }
    }
    otherPlayerBattle->doTheCurrentMonsterAttack(skill,skillLevel);
    if(currentMonsterIsKO() && haveAnotherMonsterOnThePlayerToFight())
        doTurnIfChangeOfMonster=false;
    return attackReturn.last();
}

Skill::AttackReturn LocalClientHandlerFight::doTheCurrentMonsterAttack(const quint32 &skill, const quint8 &skillLevel)
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

quint8 LocalClientHandlerFight::decreaseSkillEndurance(const quint32 &skill)
{
    PlayerMonster * currentMonster=getCurrentMonster();
    if(currentMonster==NULL)
    {
        emit error("Unable to locate the current monster");
        return 0;
    }
    quint8 newEndurance=CommonFightEngine::decreaseSkillEndurance(skill);
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            emit dbQuery(QString("UPDATE `monster_skill` SET `endurance`=%1 WHERE `monster`=%2 AND `skill`=%3;")
                         .arg(newEndurance)
                         .arg(currentMonster->id)
                         .arg(skill)
                         );
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            emit dbQuery(QString("UPDATE monster_skill SET endurance=%1 WHERE monster=%2 AND skill=%3;")
                         .arg(newEndurance)
                         .arg(currentMonster->id)
                         .arg(skill)
                         );
        break;
    }
    return newEndurance;
}

void LocalClientHandlerFight::confirmEvolutionTo(PlayerMonster * playerMonster,const quint32 &monster)
{
    CommonFightEngine::confirmEvolutionTo(playerMonster,monster);
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            emit dbQuery(QString("UPDATE `monster` SET `hp`=%1,`monster`=%2 WHERE `id`=%3")
                         .arg(playerMonster->hp)
                         .arg(playerMonster->monster)
                         .arg(playerMonster->id)
                         );
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            emit dbQuery(QString("UPDATE monster SET hp=%1,monster=%2 WHERE id=%3;")
                         .arg(playerMonster->hp)
                         .arg(playerMonster->monster)
                         .arg(playerMonster->id)
                         );
        break;
    }
}

void LocalClientHandlerFight::confirmEvolution(const quint32 &monsterId)
{
    int index=0;
    while(index<player_informations->public_and_private_informations.playerMonster.size())
    {
        if(player_informations->public_and_private_informations.playerMonster.at(index).id==monsterId)
        {
            const Monster &monsterInformations=CommonDatapack::commonDatapack.monsters[player_informations->public_and_private_informations.playerMonster.at(index).monster];
            int sub_index=0;
            while(sub_index<monsterInformations.evolutions.size())
            {
                if(
                        (monsterInformations.evolutions.at(sub_index).type==Monster::EvolutionType_Level && monsterInformations.evolutions.at(sub_index).level<=player_informations->public_and_private_informations.playerMonster.at(index).level)
                        ||
                        (monsterInformations.evolutions.at(sub_index).type==Monster::EvolutionType_Trade && GlobalServerData::serverPrivateVariables.tradedMonster.contains(player_informations->public_and_private_informations.playerMonster.at(index).id))
                )
                {
                    confirmEvolutionTo(&player_informations->public_and_private_informations.playerMonster[index],monsterInformations.evolutions.at(sub_index).evolveTo);
                    return;
                }
                sub_index++;
            }
            emit error("Evolution not found");
            return;
        }
        index++;
    }
    emit error("Monster for evolution not found");
}

void LocalClientHandlerFight::hpChange(PlayerMonster * currentMonster, const quint32 &newHpValue)
{
    CommonFightEngine::hpChange(currentMonster,newHpValue);
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            emit dbQuery(QString("UPDATE `monster` SET `hp`=%1 WHERE `id`=%2")
                         .arg(newHpValue)
                         .arg(currentMonster->id)
                         );
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            emit dbQuery(QString("UPDATE monster SET hp=%1 WHERE id=%2")
                         .arg(newHpValue)
                         .arg(currentMonster->id)
                         );
        break;
    }
}

bool LocalClientHandlerFight::removeBuffOnMonster(PlayerMonster * currentMonster, const quint32 &buffId)
{
    const bool returnVal=CommonFightEngine::removeBuffOnMonster(currentMonster,buffId);
    if(returnVal)
    {
        switch(GlobalServerData::serverSettings.database.type)
        {
            default:
            case ServerSettings::Database::DatabaseType_Mysql:
                emit dbQuery(QString("DELETE FROM `monster_buff` WHERE `monster`=%1 AND `buff`=%2")
                             .arg(currentMonster->id)
                             .arg(buffId)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                emit dbQuery(QString("DELETE FROM monster_buff WHERE monster=%1 AND buff=%2")
                             .arg(currentMonster->id)
                             .arg(buffId)
                             );
            break;
        }
    }
    return returnVal;
}

bool LocalClientHandlerFight::removeAllBuffOnMonster(PlayerMonster * currentMonster)
{
    const bool returnVal=CommonFightEngine::removeAllBuffOnMonster(currentMonster);
    if(returnVal)
    {
        switch(GlobalServerData::serverSettings.database.type)
        {
            default:
            case ServerSettings::Database::DatabaseType_Mysql:
                emit dbQuery(QString("DELETE FROM `monster_buff` WHERE `monster`=%1")
                             .arg(currentMonster->id)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                emit dbQuery(QString("DELETE FROM monster_buff WHERE monster=%1")
                             .arg(currentMonster->id)
                             );
            break;
        }
    }
    return returnVal;
}
