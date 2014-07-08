#include "../VariableServer.h"
#include "../base/GlobalServerData.h"
#include "../base/MapServer.h"
#include "../base/Client.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/FacilityLib.h"
#include "../base/Client.h"

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
        errorOutput(QStringLiteral("Try escape when not allowed"));
        return false;
    }
}

quint32 Client::tryCapture(const quint32 &item)
{
    quint32 captureSuccessId=CommonFightEngine::tryCapture(item);
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
    if(public_and_private_informations.playerMonster.isEmpty())
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
        if(!CatchChallenger::CommonDatapack::commonDatapack.monsters.contains(monster.monster))
        {
            errorOutput(QStringLiteral("saveMonsterStat() The monster %1 is not into the monster list (%2)").arg(monster.monster).arg(CatchChallenger::CommonDatapack::commonDatapack.monsters.size()));
            return;
        }
        Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(monster.monster),monster.level);
        if(monster.hp>currentMonsterStat.hp)
        {
            errorOutput(QStringLiteral("saveMonsterStat() The hp %1 of current monster %2 is greater than the max %3").arg(monster.hp).arg(monster.monster).arg(currentMonsterStat.hp));
            return;
        }
    }
    #endif
    //save into the db
    if(GlobalServerData::serverSettings.database.fightSync==ServerSettings::Database::FightSync_AtTheEndOfBattle)
    {
        if(CommonSettings::commonSettings.useSP)
            switch(GlobalServerData::serverSettings.database.type)
            {
                default:
                case ServerSettings::Database::DatabaseType_Mysql:
                    dbQueryWrite(QStringLiteral("UPDATE `monster` SET `hp`=%2,`xp`=%3,`level`=%4,`sp`=%5 WHERE `id`=%1;")
                                 .arg(monster.id)
                                 .arg(monster.hp)
                                 .arg(monster.remaining_xp)
                                 .arg(monster.level)
                                 .arg(monster.sp)
                                 );
                break;
                case ServerSettings::Database::DatabaseType_SQLite:
                    dbQueryWrite(QStringLiteral("UPDATE monster SET hp=%2,xp=%3,level=%4,sp=%5 WHERE id=%1;")
                                 .arg(monster.id)
                                 .arg(monster.hp)
                                 .arg(monster.remaining_xp)
                                 .arg(monster.level)
                                 .arg(monster.sp)
                                 );
                break;
                case ServerSettings::Database::DatabaseType_PostgreSQL:
                    dbQueryWrite(QStringLiteral("UPDATE monster SET hp=%2,xp=%3,level=%4,sp=%5 WHERE id=%1;")
                                 .arg(monster.id)
                                 .arg(monster.hp)
                                 .arg(monster.remaining_xp)
                                 .arg(monster.level)
                                 .arg(monster.sp)
                                 );
                break;
            }
        else
            switch(GlobalServerData::serverSettings.database.type)
            {
                default:
                case ServerSettings::Database::DatabaseType_Mysql:
                    dbQueryWrite(QStringLiteral("UPDATE `monster` SET `hp`=%2,`xp`=%3,`level`=%4 WHERE `id`=%1;")
                                 .arg(monster.id)
                                 .arg(monster.hp)
                                 .arg(monster.remaining_xp)
                                 .arg(monster.level)
                                 );
                break;
                case ServerSettings::Database::DatabaseType_SQLite:
                    dbQueryWrite(QStringLiteral("UPDATE monster SET hp=%2,xp=%3,level=%4 WHERE id=%1;")
                                 .arg(monster.id)
                                 .arg(monster.hp)
                                 .arg(monster.remaining_xp)
                                 .arg(monster.level)
                                 );
                break;
                case ServerSettings::Database::DatabaseType_PostgreSQL:
                    dbQueryWrite(QStringLiteral("UPDATE monster SET hp=%2,xp=%3,level=%4 WHERE id=%1;")
                                 .arg(monster.id)
                                 .arg(monster.hp)
                                 .arg(monster.remaining_xp)
                                 .arg(monster.level)
                                 );
                break;
            }
        QHash<quint32, QHash<quint32,quint32> >::const_iterator i = deferedEndurance.constBegin();
        while (i != deferedEndurance.constEnd()) {
            QHash<quint32,quint32>::const_iterator j = i.value().constBegin();
            while (j != i.value().constEnd()) {
                switch(GlobalServerData::serverSettings.database.type)
                {
                    default:
                    case ServerSettings::Database::DatabaseType_Mysql:
                        dbQueryWrite(QStringLiteral("UPDATE `monster_skill` SET `endurance`=%1 WHERE `monster`=%2 AND `skill`=%3;")
                                     .arg(j.value())
                                     .arg(i.key())
                                     .arg(j.key())
                                     );
                    break;
                    case ServerSettings::Database::DatabaseType_SQLite:
                        dbQueryWrite(QStringLiteral("UPDATE monster_skill SET endurance=%1 WHERE monster=%2 AND skill=%3;")
                                     .arg(j.value())
                                     .arg(i.key())
                                     .arg(j.key())
                                     );
                    break;
                    case ServerSettings::Database::DatabaseType_PostgreSQL:
                        dbQueryWrite(QStringLiteral("UPDATE monster_skill SET endurance=%1 WHERE monster=%2 AND skill=%3;")
                                     .arg(j.value())
                                     .arg(i.key())
                                     .arg(j.key())
                                     );
                    break;
                }
                ++j;
            }
            ++i;
        }
        deferedEndurance.clear();
    }
}

void Client::saveAllMonsterPosition()
{
    const QList<PlayerMonster> &playerMonsterList=getPlayerMonster();
    int index=0;
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
        normalOutput(QStringLiteral("You current monster (%1) is KO").arg(getCurrentMonster()->monster));
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
        normalOutput(QStringLiteral("You have lost, tp to %1 (%2,%3) and heal").arg(rescue.map->map_file).arg(rescue.x).arg(rescue.y));
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
            errorOutput(QStringLiteral("after lost in fight, remain unable to do a fight"));
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
    int sub_index;
    int index=0;
    while(index<public_and_private_informations.playerMonster.size())
    {
        if(public_and_private_informations.playerMonster.at(index).egg_step==0)
        {
            const Monster::Stat &stat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(public_and_private_informations.playerMonster.at(index).monster),public_and_private_informations.playerMonster.at(index).level);
            if(public_and_private_informations.playerMonster.value(index).hp!=stat.hp)
            {
                public_and_private_informations.playerMonster[index].hp=stat.hp;
                switch(GlobalServerData::serverSettings.database.type)
                {
                    default:
                    case ServerSettings::Database::DatabaseType_Mysql:
                        dbQueryWrite(QStringLiteral("UPDATE `monster` SET `hp`=%1 WHERE `id`=%2;")
                                     .arg(public_and_private_informations.playerMonster.value(index).hp)
                                     .arg(public_and_private_informations.playerMonster.value(index).id)
                                     );
                    break;
                    case ServerSettings::Database::DatabaseType_SQLite:
                        dbQueryWrite(QStringLiteral("UPDATE monster SET hp=%1 WHERE id=%2;")
                                     .arg(public_and_private_informations.playerMonster.value(index).hp)
                                     .arg(public_and_private_informations.playerMonster.value(index).id)
                                     );
                    break;
                    case ServerSettings::Database::DatabaseType_PostgreSQL:
                        dbQueryWrite(QStringLiteral("UPDATE monster SET hp=%1 WHERE id=%2;")
                                     .arg(public_and_private_informations.playerMonster.value(index).hp)
                                     .arg(public_and_private_informations.playerMonster.value(index).id)
                                     );
                    break;
                }
            }
            if(!public_and_private_informations.playerMonster.value(index).buffs.isEmpty())
            {
                switch(GlobalServerData::serverSettings.database.type)
                {
                    default:
                    case ServerSettings::Database::DatabaseType_Mysql:
                        dbQueryWrite(QStringLiteral("DELETE FROM `monster_buff` WHERE `monster`=%1")
                                 .arg(public_and_private_informations.playerMonster.value(index).id)
                                 );
                    break;
                    case ServerSettings::Database::DatabaseType_SQLite:
                        dbQueryWrite(QStringLiteral("DELETE FROM monster_buff WHERE monster=%1")
                                 .arg(public_and_private_informations.playerMonster.value(index).id)
                                 );
                    break;
                    case ServerSettings::Database::DatabaseType_PostgreSQL:
                        dbQueryWrite(QStringLiteral("DELETE FROM monster_buff WHERE monster=%1")
                                 .arg(public_and_private_informations.playerMonster.value(index).id)
                                 );
                    break;
                }
                public_and_private_informations.playerMonster[index].buffs.clear();
            }
            sub_index=0;
            const int &list_size=public_and_private_informations.playerMonster.value(index).skills.size();
            while(sub_index<list_size)
            {
                int endurance=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.value(
                        public_and_private_informations.playerMonster.value(index).skills.at(sub_index).skill
                        )
                        .level.at(public_and_private_informations.playerMonster.value(index).skills.at(sub_index).level-1).endurance;
                if(public_and_private_informations.playerMonster.value(index).skills.value(sub_index).endurance!=endurance)
                {
                    switch(GlobalServerData::serverSettings.database.type)
                    {
                        default:
                        case ServerSettings::Database::DatabaseType_Mysql:
                            dbQueryWrite(QStringLiteral("UPDATE `monster_skill` SET `endurance`=%1 WHERE `monster`=%2 AND `skill`=%3;")
                                         .arg(endurance)
                                         .arg(public_and_private_informations.playerMonster.value(index).id)
                                         .arg(public_and_private_informations.playerMonster.value(index).skills.value(sub_index).skill)
                                         );
                        break;
                        case ServerSettings::Database::DatabaseType_SQLite:
                            dbQueryWrite(QStringLiteral("UPDATE monster_skill SET endurance=%1 WHERE monster=%2 AND skill=%3;")
                                         .arg(endurance)
                                         .arg(public_and_private_informations.playerMonster.value(index).id)
                                         .arg(public_and_private_informations.playerMonster.value(index).skills.value(sub_index).skill)
                                         );
                        break;
                        case ServerSettings::Database::DatabaseType_PostgreSQL:
                            dbQueryWrite(QStringLiteral("UPDATE monster_skill SET endurance=%1 WHERE monster=%2 AND skill=%3;")
                                         .arg(endurance)
                                         .arg(public_and_private_informations.playerMonster.value(index).id)
                                         .arg(public_and_private_informations.playerMonster.value(index).skills.value(sub_index).skill)
                                         );
                        break;
                    }
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
    if(GlobalServerData::serverSettings.database.fightSync==ServerSettings::Database::FightSync_AtEachTurn)
        saveStat();
}

void Client::saveStat()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        PlayerMonster * currentMonster=getCurrentMonster();
        PublicPlayerMonster * otherMonster=getOtherMonster();
        Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(currentMonster->monster),currentMonster->level);
        Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(otherMonster->monster),otherMonster->level);
        if(currentMonster!=NULL)
            if(currentMonster->hp>currentMonsterStat.hp)
            {
                errorOutput(QStringLiteral("saveStat() The hp %1 of current monster %2 is greater than the max %3").arg(currentMonster->hp).arg(currentMonster->monster).arg(currentMonsterStat.hp));
                return;
            }
        if(otherMonster!=NULL)
            if(otherMonster->hp>otherMonsterStat.hp)
            {
                errorOutput(QStringLiteral("saveStat() The hp %1 of other monster %2 is greater than the max %3").arg(otherMonster->hp).arg(otherMonster->monster).arg(otherMonsterStat.hp));
                return;
            }
    }
    #endif
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            dbQueryWrite(QStringLiteral("UPDATE `monster` SET `hp`=%1 WHERE `id`=%2;")
                         .arg(getCurrentMonster()->hp)
                         .arg(getCurrentMonster()->id)
                         );
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            dbQueryWrite(QStringLiteral("UPDATE monster SET hp=%1 WHERE id=%2;")
                         .arg(getCurrentMonster()->hp)
                         .arg(getCurrentMonster()->id)
                         );
        break;
        case ServerSettings::Database::DatabaseType_PostgreSQL:
            dbQueryWrite(QStringLiteral("UPDATE monster SET hp=%1 WHERE id=%2;")
                         .arg(getCurrentMonster()->hp)
                         .arg(getCurrentMonster()->id)
                         );
        break;
    }
}

bool Client::botFightCollision(CommonMap *map,const COORD_TYPE &x,const COORD_TYPE &y)
{
    if(isInFight())
    {
        errorOutput(QStringLiteral("error: map: %1 (%2,%3), is in fight").arg(map->map_file).arg(x).arg(y));
        return false;
    }
    const QList<quint32> &botList=static_cast<MapServer *>(map)->botsFightTrigger.values(QPair<quint8,quint8>(x,y));
    int index=0;
    while(index<botList.size())
    {
        const quint32 &botFightId=botList.at(index);
        if(!public_and_private_informations.bot_already_beaten.contains(botFightId))
        {
            normalOutput(QStringLiteral("is now in fight on map %1 (%2,%3) with the bot %4").arg(map->map_file).arg(x).arg(y).arg(botFightId));
            botFightStart(botFightId);
            return true;
        }
        index++;
    }

    /// no fight in this zone
    return false;
}

bool Client::botFightStart(const quint32 &botFightId)
{
    if(isInFight())
    {
        errorOutput(QStringLiteral("error: is already in fight"));
        return false;
    }
    if(!CommonDatapack::commonDatapack.botFights.contains(botFightId))
    {
        errorOutput(QStringLiteral("error: bot id %1 not found").arg(botFightId));
        return false;
    }
    const BotFight &botFight=CommonDatapack::commonDatapack.botFights.value(botFightId);
    if(botFight.monsters.isEmpty())
    {
        errorOutput(QStringLiteral("error: bot id %1 have no monster to fight").arg(botFightId));
        return false;
    }
    startTheFight();
    botFightCash=botFight.cash;
    int index=0;
    while(index<botFight.monsters.size())
    {
        const BotFight::BotFightMonster &monster=botFight.monsters.at(index);
        const Monster::Stat &stat=getStat(CommonDatapack::commonDatapack.monsters.value(monster.id),monster.level);
        botFightMonsters << FacilityLib::botFightMonsterToPlayerMonster(monster,stat);
        index++;
    }
    #ifdef CATCHCHALLENGER_SERVER_EXTRA_CHECK
    if(botFightMonsters.isEmpty())
    {
        errorOutput(QStringLiteral("error: after the bot add, remaing empty").arg(botFightId));
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
            errorOutput(QStringLiteral("Is in battle but the other monster is null"));
    }
    return CommonFightEngine::getOtherMonster();
}

void Client::wildDrop(const quint32 &monster)
{
    QList<MonsterDrops> drops=GlobalServerData::serverPrivateVariables.monsterDrops.values(monster);
    if(questsDrop.contains(monster))
        drops+=questsDrop.values(monster);
    int index=0;
    bool success;
    quint32 quantity;
    while(index<drops.size())
    {
        const quint32 &tempLuck=drops.at(index).luck;
        const quint32 &quantity_min=drops.at(index).quantity_min;
        const quint32 &quantity_max=drops.at(index).quantity_max;
        if(tempLuck==100)
            success=true;
        else
        {
            if(rand()%100<(qint32)tempLuck)
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
            normalOutput(QStringLiteral("Win %1 item: %2").arg(quantity).arg(drops.at(index).item));
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

bool Client::learnSkillInternal(const quint32 &monsterId,const quint32 &skill)
{
    int index=0;
    while(index<public_and_private_informations.playerMonster.size())
    {
        const PlayerMonster &monster=public_and_private_informations.playerMonster.at(index);
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
            const int &list_size=CommonDatapack::commonDatapack.monsters.value(monster.monster).learn.size();
            while(sub_index<list_size)
            {
                const Monster::AttackToLearn &learn=CommonDatapack::commonDatapack.monsters.value(monster.monster).learn.at(sub_index);
                if(learn.learnAtLevel<=monster.level && learn.learnSkill==skill)
                {
                    if((sub_index2==monster.skills.size() && learn.learnSkillLevel==1) || (monster.skills.value(sub_index2).level+1)==learn.learnSkillLevel)
                    {
                        if(CommonSettings::commonSettings.useSP)
                        {
                            const quint32 &sp=CommonDatapack::commonDatapack.monsterSkills.value(learn.learnSkill).level.at(learn.learnSkillLevel).sp_to_learn;
                            if(sp>monster.sp)
                            {
                                errorOutput(QStringLiteral("The attack require %1 sp to be learned, you have only %2").arg(sp).arg(monster.sp));
                                return false;
                            }
                            public_and_private_informations.playerMonster[index].sp-=sp;
                            switch(GlobalServerData::serverSettings.database.type)
                            {
                                default:
                                case ServerSettings::Database::DatabaseType_Mysql:
                                    dbQueryWrite(QStringLiteral("UPDATE `monster` SET `sp`=%1 WHERE `id`=%2;")
                                                 .arg(public_and_private_informations.playerMonster.value(index).sp)
                                                 .arg(monsterId)
                                                 );
                                break;
                                case ServerSettings::Database::DatabaseType_SQLite:
                                    dbQueryWrite(QStringLiteral("UPDATE monster SET sp=%1 WHERE id=%2;")
                                                 .arg(public_and_private_informations.playerMonster.value(index).sp)
                                                 .arg(monsterId)
                                                 );
                                break;
                                case ServerSettings::Database::DatabaseType_PostgreSQL:
                                    dbQueryWrite(QStringLiteral("UPDATE monster SET sp=%1 WHERE id=%2;")
                                                 .arg(public_and_private_informations.playerMonster.value(index).sp)
                                                 .arg(monsterId)
                                                 );
                                break;
                            }
                        }
                        if(learn.learnSkillLevel==1)
                        {
                            PlayerMonster::PlayerSkill temp;
                            temp.skill=skill;
                            temp.level=1;
                            temp.endurance=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.value(temp.skill).level.first().endurance;
                            public_and_private_informations.playerMonster[index].skills << temp;
                            switch(GlobalServerData::serverSettings.database.type)
                            {
                                default:
                                case ServerSettings::Database::DatabaseType_Mysql:
                                    dbQueryWrite(QStringLiteral("INSERT INTO `monster_skill`(`monster`,`skill`,`level`,`endurance`) VALUES(%1,%2,1,%3);")
                                                 .arg(monsterId)
                                                 .arg(temp.skill)
                                                 .arg(temp.endurance)
                                                 );
                                break;
                                case ServerSettings::Database::DatabaseType_SQLite:
                                    dbQueryWrite(QStringLiteral("INSERT INTO monster_skill(monster,skill,level,endurance) VALUES(%1,%2,1,%3);")
                                                 .arg(monsterId)
                                                 .arg(temp.skill)
                                                 .arg(temp.endurance)
                                                 );
                                break;
                                case ServerSettings::Database::DatabaseType_PostgreSQL:
                                    dbQueryWrite(QStringLiteral("INSERT INTO monster_skill(monster,skill,level,endurance) VALUES(%1,%2,1,%3);")
                                                 .arg(monsterId)
                                                 .arg(temp.skill)
                                                 .arg(temp.endurance)
                                                 );
                                break;
                            }
                        }
                        else
                        {
                            public_and_private_informations.playerMonster[index].skills[sub_index2].level++;
                            switch(GlobalServerData::serverSettings.database.type)
                            {
                                default:
                                case ServerSettings::Database::DatabaseType_Mysql:
                                    dbQueryWrite(QStringLiteral("UPDATE `monster_skill` SET `level`=%1 WHERE `monster`=%2 AND `skill`=%3;")
                                                 .arg(public_and_private_informations.playerMonster.value(index).skills.value(sub_index2).level)
                                                 .arg(monsterId)
                                                 .arg(skill)
                                                 );
                                break;
                                case ServerSettings::Database::DatabaseType_SQLite:
                                    dbQueryWrite(QStringLiteral("UPDATE monster_skill SET level=%1 WHERE monster=%2 AND skill=%3;")
                                                 .arg(public_and_private_informations.playerMonster.value(index).skills.value(sub_index2).level)
                                                 .arg(monsterId)
                                                 .arg(skill)
                                                 );
                                break;
                                case ServerSettings::Database::DatabaseType_PostgreSQL:
                                    dbQueryWrite(QStringLiteral("UPDATE monster_skill SET level=%1 WHERE monster=%2 AND skill=%3;")
                                                 .arg(public_and_private_informations.playerMonster.value(index).skills.value(sub_index2).level)
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
            errorOutput(QStringLiteral("The skill %1 is not into learn skill list for the monster").arg(skill));
            return false;
        }
        index++;
    }
    errorOutput(QStringLiteral("The monster is not found: %1").arg(monsterId));
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
        normalOutput(QLatin1String("Already in battle, internal error"));
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput(QStringLiteral("%1 have requested battle with you").arg(otherPlayerBattle->public_and_private_informations.public_informations.pseudo));
    #endif
    this->otherPlayerBattle=otherPlayerBattle;
    otherPlayerBattle->otherPlayerBattle=this;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
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
    normalOutput(QLatin1String("Battle finished"));
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
    normalOutput(QLatin1String("Battle canceled"));
    #endif
    bool needUpdateCanDoFight=false;
    if(battleIsValidated)
        needUpdateCanDoFight=true;
    otherPlayerBattle=NULL;
    if(send)
    {
            sendFullPacket(0xE0,0x0007);
            receiveSystemText(QLatin1String("Battle declined"));
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
        normalOutput(QLatin1String("Can't accept battle if not in battle"));
        return;
    }
    if(battleIsValidated)
    {
        normalOutput(QLatin1String("Battle already validated"));
        return;
    }
    if(!otherPlayerBattle->getAbleToFight())
    {
        errorOutput(QStringLiteral("The other player can't fight"));
        return;
    }
    if(!getAbleToFight())
    {
        errorOutput(QStringLiteral("You can't fight"));
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput(QLatin1String("Battle accepted"));
    #endif
    startTheFight();
    battleIsValidated=true;
    mHaveCurrentSkill=false;
    mMonsterChange=false;
    if(send)
    {
        QList<PlayerMonster> playerMonstersPreview=otherPlayerBattle->public_and_private_informations.playerMonster;
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);
        out << otherPlayerBattle->public_and_private_informations.public_informations.skinId;
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
        sendFullPacket(0xE0,0x0008,otherPlayerBattle->rawPseudo+outputData+firstValidOtherPlayerMonster);
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

quint8 Client::getOtherSelectedMonsterNumber() const
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
        const Monster::Stat &currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(currentMonster->monster),currentMonster->level);
        const Monster::Stat &otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(otherMonster->monster),otherMonster->level);
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

quint8 Client::selectedMonsterNumberToMonsterPlace(const quint8 &selectedMonsterNumber)
{
    return selectedMonsterNumber+1;
}

void Client::sendBattleReturn()
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

    sendFullPacket(0xE0,0x0006,outputData+binarypublicPlayerMonster);
}

void Client::sendBattleMonsterChange()
{
    QByteArray binarypublicPlayerMonster;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0;
    out << (quint8)selectedMonsterNumberToMonsterPlace(getOtherSelectedMonsterNumber());;
    binarypublicPlayerMonster=FacilityLib::publicPlayerMonsterToBinary(*getOtherMonster());
    sendFullPacket(0xE0,0x0006,outputData+binarypublicPlayerMonster);
}

//return true if change level, multiplicator do at datapack loading
bool Client::giveXPSP(int xp,int sp)
{
    const bool &haveChangeOfLevel=CommonFightEngine::giveXPSP(xp,sp);
    if(GlobalServerData::serverSettings.database.fightSync==ServerSettings::Database::FightSync_AtEachTurn)
    {
        if(CommonSettings::commonSettings.useSP)
        {
            if(haveChangeOfLevel)
                switch(GlobalServerData::serverSettings.database.type)
                {
                    default:
                    case ServerSettings::Database::DatabaseType_Mysql:
                        dbQueryWrite(QStringLiteral("UPDATE `monster` SET `level`=%2,`hp`=%3,`xp`=%2,`sp`=%3 WHERE `id`=%1;")
                                     .arg(getCurrentMonster()->id)
                                     .arg(getCurrentMonster()->level)
                                     .arg(getCurrentMonster()->hp)
                                     .arg(getCurrentMonster()->remaining_xp)
                                     .arg(getCurrentMonster()->sp)
                                     );
                    break;
                    case ServerSettings::Database::DatabaseType_SQLite:
                        dbQueryWrite(QStringLiteral("UPDATE monster SET level=%2,hp=%3,xp=%2,sp=%3 WHERE id=%1;")
                                     .arg(getCurrentMonster()->id)
                                     .arg(getCurrentMonster()->level)
                                     .arg(getCurrentMonster()->hp)
                                     .arg(getCurrentMonster()->remaining_xp)
                                     .arg(getCurrentMonster()->sp)
                                     );
                    break;
                    case ServerSettings::Database::DatabaseType_PostgreSQL:
                        dbQueryWrite(QStringLiteral("UPDATE monster SET level=%2,hp=%3,xp=%2,sp=%3 WHERE id=%1;")
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
                        dbQueryWrite(QStringLiteral("UPDATE `monster` SET `xp`=%2,`sp`=%3 WHERE `id`=%1;")
                                     .arg(getCurrentMonster()->id)
                                     .arg(getCurrentMonster()->remaining_xp)
                                     .arg(getCurrentMonster()->sp)
                                     );
                    break;
                    case ServerSettings::Database::DatabaseType_SQLite:
                        dbQueryWrite(QStringLiteral("UPDATE monster SET xp=%2,sp=%3 WHERE id=%1;")
                                     .arg(getCurrentMonster()->id)
                                     .arg(getCurrentMonster()->remaining_xp)
                                     .arg(getCurrentMonster()->sp)
                                     );
                    break;
                    case ServerSettings::Database::DatabaseType_PostgreSQL:
                        dbQueryWrite(QStringLiteral("UPDATE monster SET xp=%2,sp=%3 WHERE id=%1;")
                                     .arg(getCurrentMonster()->id)
                                     .arg(getCurrentMonster()->remaining_xp)
                                     .arg(getCurrentMonster()->sp)
                                     );
                    break;
                }
        }
        else
        {
            if(haveChangeOfLevel)
                switch(GlobalServerData::serverSettings.database.type)
                {
                    default:
                    case ServerSettings::Database::DatabaseType_Mysql:
                        dbQueryWrite(QStringLiteral("UPDATE `monster` SET `level`=%2,`hp`=%3,`xp`=%2 WHERE `id`=%1;")
                                     .arg(getCurrentMonster()->id)
                                     .arg(getCurrentMonster()->level)
                                     .arg(getCurrentMonster()->hp)
                                     .arg(getCurrentMonster()->remaining_xp)
                                     );
                    break;
                    case ServerSettings::Database::DatabaseType_SQLite:
                        dbQueryWrite(QStringLiteral("UPDATE monster SET level=%2,hp=%3,xp=%2 WHERE id=%1;")
                                     .arg(getCurrentMonster()->id)
                                     .arg(getCurrentMonster()->level)
                                     .arg(getCurrentMonster()->hp)
                                     .arg(getCurrentMonster()->remaining_xp)
                                     );
                    break;
                    case ServerSettings::Database::DatabaseType_PostgreSQL:
                        dbQueryWrite(QStringLiteral("UPDATE monster SET level=%2,hp=%3,xp=%2 WHERE id=%1;")
                                     .arg(getCurrentMonster()->id)
                                     .arg(getCurrentMonster()->level)
                                     .arg(getCurrentMonster()->hp)
                                     .arg(getCurrentMonster()->remaining_xp)
                                     );
                    break;
                }
            else
                switch(GlobalServerData::serverSettings.database.type)
                {
                    default:
                    case ServerSettings::Database::DatabaseType_Mysql:
                        dbQueryWrite(QStringLiteral("UPDATE `monster` SET `xp`=%2 WHERE `id`=%1;")
                                     .arg(getCurrentMonster()->id)
                                     .arg(getCurrentMonster()->remaining_xp)
                                     );
                    break;
                    case ServerSettings::Database::DatabaseType_SQLite:
                        dbQueryWrite(QStringLiteral("UPDATE monster SET xp=%2 WHERE id=%1;")
                                     .arg(getCurrentMonster()->id)
                                     .arg(getCurrentMonster()->remaining_xp)
                                     );
                    break;
                    case ServerSettings::Database::DatabaseType_PostgreSQL:
                        dbQueryWrite(QStringLiteral("UPDATE monster SET xp=%2 WHERE id=%1;")
                                     .arg(getCurrentMonster()->id)
                                     .arg(getCurrentMonster()->remaining_xp)
                                     );
                    break;
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
                    addCash(CommonDatapack::commonDatapack.botFights.value(botFightId).cash);
                    public_and_private_informations.bot_already_beaten << botFightId;
                    switch(GlobalServerData::serverSettings.database.type)
                    {
                        default:
                        case ServerSettings::Database::DatabaseType_Mysql:
                            dbQueryWrite(QStringLiteral("INSERT INTO `bot_already_beaten`(`character`,`botfight_id`) VALUES(%1,%2);")
                                         .arg(character_id)
                                         .arg(botFightId)
                                         );
                        break;
                        case ServerSettings::Database::DatabaseType_SQLite:
                            dbQueryWrite(QStringLiteral("INSERT INTO bot_already_beaten(character,botfight_id) VALUES(%1,%2);")
                                         .arg(character_id)
                                         .arg(botFightId)
                                         );
                        break;
                        case ServerSettings::Database::DatabaseType_PostgreSQL:
                            dbQueryWrite(QStringLiteral("INSERT INTO bot_already_beaten(character,botfight_id) VALUES(%1,%2);")
                                         .arg(character_id)
                                         .arg(botFightId)
                                         );
                        break;
                    }
                }
                fightOrBattleFinish(win,botFightId);
                normalOutput(QStringLiteral("Register the win against the bot fight: %1").arg(botFightId));
            }
            else
                normalOutput(QStringLiteral("Have win agains a wild monster"));
        }
        fightOrBattleFinish(win,0);
    }
    return win;
}

bool Client::useSkill(const quint32 &skill)
{
    normalOutput(QStringLiteral("use the skill: %1").arg(skill));
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
            errorOutput(QStringLiteral("Have already a battle action"));
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
        if(GlobalServerData::serverSettings.database.fightSync==ServerSettings::Database::FightSync_AtTheEndOfBattle)
        {
            if(!isInFight())
                saveCurrentMonsterStat();
        }
    }

    return commonReturn || battleReturn;
}

quint32 Client::catchAWild(const bool &toStorage, const PlayerMonster &newMonster)
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
    QString place;
    if(toStorage)
        place="'warehouse'";
    else
        place="'wear'";
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            dbQueryWrite(QStringLiteral("INSERT INTO `monster`(`id`,`hp`,`character`,`monster`,`level`,`xp`,`sp`,`captured_with`,`gender`,`egg_step`,`character_origin`,`place`,`position`) VALUES(%1,%2);")
                         .arg(QStringLiteral("%1,%2,%3,%4,%5,%6,%7,%8,'%9'")
                              .arg(monster_id)
                              .arg(newMonster.hp)
                              .arg(character_id)
                              .arg(newMonster.monster)
                              .arg(newMonster.level)
                              .arg(newMonster.remaining_xp)
                              .arg(newMonster.sp)
                              .arg(newMonster.catched_with)
                              .arg(FacilityLib::genderToString(newMonster.gender))
                              )
                         .arg(QStringLiteral("%1,%2,%3,%4")
                              .arg(newMonster.egg_step)
                              .arg(character_id)
                              .arg(place)
                              .arg(position)
                              )
                         );
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            dbQueryWrite(QStringLiteral("INSERT INTO monster(id,hp,character,monster,level,xp,sp,captured_with,gender,egg_step,character_origin,place,position) VALUES(%1,%2);")
                         .arg(QStringLiteral("%1,%2,%3,%4,%5,%6,%7,%8,'%9'")
                              .arg(monster_id)
                              .arg(newMonster.hp)
                              .arg(character_id)
                              .arg(newMonster.monster)
                              .arg(newMonster.level)
                              .arg(newMonster.remaining_xp)
                              .arg(newMonster.sp)
                              .arg(newMonster.catched_with)
                              .arg(FacilityLib::genderToString(newMonster.gender))
                              )
                         .arg(QStringLiteral("%1,%2,%3,%4")
                              .arg(newMonster.egg_step)
                              .arg(character_id)
                              .arg(place)
                              .arg(position)
                              )
                         );
        break;
        case ServerSettings::Database::DatabaseType_PostgreSQL:
            dbQueryWrite(QStringLiteral("INSERT INTO monster(id,hp,character,monster,level,xp,sp,captured_with,gender,egg_step,character_origin,place,position) VALUES(%1,%2);")
                         .arg(QStringLiteral("%1,%2,%3,%4,%5,%6,%7,%8,'%9'")
                              .arg(monster_id)
                              .arg(newMonster.hp)
                              .arg(character_id)
                              .arg(newMonster.monster)
                              .arg(newMonster.level)
                              .arg(newMonster.remaining_xp)
                              .arg(newMonster.sp)
                              .arg(newMonster.catched_with)
                              .arg(FacilityLib::genderToString(newMonster.gender))
                              )
                         .arg(QStringLiteral("%1,%2,%3,%4")
                              .arg(newMonster.egg_step)
                              .arg(character_id)
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
                dbQueryWrite(QStringLiteral("INSERT INTO `monster_skill`(`monster`,`skill`,`level`,`endurance`) VALUES(%1,%2,%3,%4);")
                             .arg(monster_id)
                             .arg(newMonster.skills.at(index).skill)
                             .arg(newMonster.skills.at(index).level)
                             .arg(newMonster.skills.at(index).endurance)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                dbQueryWrite(QStringLiteral("INSERT INTO monster_skill(monster,skill,level,endurance) VALUES(%1,%2,%3,%4);")
                             .arg(monster_id)
                             .arg(newMonster.skills.at(index).skill)
                             .arg(newMonster.skills.at(index).level)
                             .arg(newMonster.skills.at(index).endurance)
                             );
            break;
            case ServerSettings::Database::DatabaseType_PostgreSQL:
                dbQueryWrite(QStringLiteral("INSERT INTO monster_skill(monster,skill,level,endurance) VALUES(%1,%2,%3,%4);")
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
                dbQueryWrite(QStringLiteral("INSERT INTO `monster_buff`(`monster`,`buff`,`level`) VALUES(%1,%2,%3);")
                             .arg(monster_id)
                             .arg(newMonster.buffs.at(index).buff)
                             .arg(newMonster.buffs.at(index).level)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                dbQueryWrite(QStringLiteral("INSERT INTO monster_buff(monster,buff,level) VALUES(%1,%2,%3);")
                             .arg(monster_id)
                             .arg(newMonster.buffs.at(index).buff)
                             .arg(newMonster.buffs.at(index).level)
                             );
            break;
            case ServerSettings::Database::DatabaseType_PostgreSQL:
                dbQueryWrite(QStringLiteral("INSERT INTO monster_buff(monster,buff,level) VALUES(%1,%2,%3);")
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

bool Client::haveCurrentSkill() const
{
    return mHaveCurrentSkill;
}

quint32 Client::getCurrentSkill() const
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
                    switch(GlobalServerData::serverSettings.database.type)
                    {
                        default:
                        case ServerSettings::Database::DatabaseType_Mysql:
                            dbQueryWrite(QStringLiteral("INSERT INTO `monster_buff`(`monster`,`buff`,`level`) VALUES(%1,%2,%3);")
                                     .arg(otherPlayerBattle->getCurrentMonster()->id)
                                     .arg(effect.buff)
                                     .arg(effect.level)
                                     );
                        break;
                        case ServerSettings::Database::DatabaseType_SQLite:
                            dbQueryWrite(QStringLiteral("INSERT INTO monster_buff(monster,buff,level) VALUES(%1,%2,%3);")
                                     .arg(otherPlayerBattle->getCurrentMonster()->id)
                                     .arg(effect.buff)
                                     .arg(effect.level)
                                     );
                        break;
                        case ServerSettings::Database::DatabaseType_PostgreSQL:
                            dbQueryWrite(QStringLiteral("INSERT INTO monster_buff(monster,buff,level) VALUES(%1,%2,%3);")
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
                        dbQueryWrite(QStringLiteral("INSERT INTO `monster_buff`(`monster`,`buff`,`level`) VALUES(%1,%2,%3);")
                                 .arg(getCurrentMonster()->id)
                                 .arg(effect.buff)
                                 .arg(effect.level)
                                 );
                    break;
                    case ServerSettings::Database::DatabaseType_SQLite:
                        dbQueryWrite(QStringLiteral("INSERT INTO monster_buff(monster,buff,level) VALUES(%1,%2,%3);")
                                 .arg(getCurrentMonster()->id)
                                 .arg(effect.buff)
                                 .arg(effect.level)
                                 );
                    break;
                    case ServerSettings::Database::DatabaseType_PostgreSQL:
                        dbQueryWrite(QStringLiteral("INSERT INTO monster_buff(monster,buff,level) VALUES(%1,%2,%3);")
                                 .arg(getCurrentMonster()->id)
                                 .arg(effect.buff)
                                 .arg(effect.level)
                                 );
                    break;
                }
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
                    switch(GlobalServerData::serverSettings.database.type)
                    {
                        default:
                        case ServerSettings::Database::DatabaseType_Mysql:
                            dbQueryWrite(QStringLiteral("UPDATE `monster` SET `level`=%3 WHERE `monster`=%1 AND buff=%2;")
                                         .arg(otherPlayerBattle->getCurrentMonster()->id)
                                         .arg(effect.buff)
                                         .arg(effect.level)
                                         );
                        break;
                        case ServerSettings::Database::DatabaseType_SQLite:
                            dbQueryWrite(QStringLiteral("UPDATE monster SET level=%3 WHERE monster=%1 AND buff=%2;")
                                         .arg(otherPlayerBattle->getCurrentMonster()->id)
                                         .arg(effect.buff)
                                         .arg(effect.level)
                                         );
                        break;
                        case ServerSettings::Database::DatabaseType_PostgreSQL:
                            dbQueryWrite(QStringLiteral("UPDATE monster SET level=%3 WHERE monster=%1 AND buff=%2;")
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
                        dbQueryWrite(QStringLiteral("UPDATE `monster` SET `level`=%3 WHERE `monster`=%1 AND `buff`=%2;")
                                     .arg(getCurrentMonster()->id)
                                     .arg(effect.buff)
                                     .arg(effect.level)
                                     );
                    break;
                    case ServerSettings::Database::DatabaseType_SQLite:
                        dbQueryWrite(QStringLiteral("UPDATE monster SET level=%3 WHERE monster=%1 AND buff=%2;")
                                     .arg(getCurrentMonster()->id)
                                     .arg(effect.buff)
                                     .arg(effect.level)
                                     );
                    break;
                    case ServerSettings::Database::DatabaseType_PostgreSQL:
                        dbQueryWrite(QStringLiteral("UPDATE monster SET level=%3 WHERE monster=%1 AND buff=%2;")
                                     .arg(getCurrentMonster()->id)
                                     .arg(effect.buff)
                                     .arg(effect.level)
                                     );
                    break;
                }
                break;
                default:
                    errorOutput("Not apply match, can't apply the buff");
                break;
            }
    }
    return returnCode;
}

bool Client::moveUpMonster(const quint8 &number)
{
    if(!CommonFightEngine::moveUpMonster(number))
        return false;
    if(GlobalServerData::serverSettings.database.fightSync!=ServerSettings::Database::FightSync_AtTheDisconnexion)
    {
        saveMonsterPosition(public_and_private_informations.playerMonster.value(number-1).id,number);
        saveMonsterPosition(public_and_private_informations.playerMonster.value(number).id,number+1);
    }
    return true;
}

bool Client::moveDownMonster(const quint8 &number)
{
    if(!CommonFightEngine::moveDownMonster(number))
    {
        errorOutput("Move monster have failed");
        return false;
    }
    if(GlobalServerData::serverSettings.database.fightSync!=ServerSettings::Database::FightSync_AtTheDisconnexion)
    {
        saveMonsterPosition(public_and_private_informations.playerMonster.value(number).id,number+1);
        saveMonsterPosition(public_and_private_informations.playerMonster.value(number+1).id,number+2);
    }
    return true;

}

void Client::saveMonsterPosition(const quint32 &monsterId,const quint8 &monsterPosition)
{
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            dbQueryWrite(QStringLiteral("UPDATE `monster` SET `position`=%1 WHERE `id`=%2;")
                         .arg(monsterPosition)
                         .arg(monsterId)
                         );
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            dbQueryWrite(QStringLiteral("UPDATE monster SET position=%1 WHERE id=%2;")
                         .arg(monsterPosition)
                         .arg(monsterId)
                         );
        break;
        case ServerSettings::Database::DatabaseType_PostgreSQL:
            dbQueryWrite(QStringLiteral("UPDATE monster SET position=%1 WHERE id=%2;")
                         .arg(monsterPosition)
                         .arg(monsterId)
                         );
        break;
    }
}

bool Client::changeOfMonsterInFight(const quint32 &monsterId)
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
    const quint32 &skill=otherPlayerBattle->getCurrentSkill();
    quint8 skillLevel=getSkillLevel(skill);
    if(skillLevel==0)
    {
        if(!haveMoreEndurance() && skill==0 && CommonDatapack::commonDatapack.monsterSkills.contains(skill))
            skillLevel=1;
        else
        {
            errorOutput(QStringLiteral("Unable to fight because the current monster have not the skill %3").arg(skill));
            return attackReturnTemp;
        }
    }
    otherPlayerBattle->doTheCurrentMonsterAttack(skill,skillLevel);
    if(currentMonsterIsKO() && haveAnotherMonsterOnThePlayerToFight())
        doTurnIfChangeOfMonster=false;
    return attackReturn.last();
}

Skill::AttackReturn Client::doTheCurrentMonsterAttack(const quint32 &skill, const quint8 &skillLevel)
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

quint8 Client::decreaseSkillEndurance(const quint32 &skill)
{
    PlayerMonster * currentMonster=getCurrentMonster();
    if(currentMonster==NULL)
    {
        errorOutput("Unable to locate the current monster");
        return 0;
    }
    const quint8 &newEndurance=CommonFightEngine::decreaseSkillEndurance(skill);
    if(GlobalServerData::serverSettings.database.fightSync==ServerSettings::Database::FightSync_AtEachTurn)
    {
        switch(GlobalServerData::serverSettings.database.type)
        {
            default:
            case ServerSettings::Database::DatabaseType_Mysql:
                dbQueryWrite(QStringLiteral("UPDATE `monster_skill` SET `endurance`=%1 WHERE `monster`=%2 AND `skill`=%3;")
                             .arg(newEndurance)
                             .arg(currentMonster->id)
                             .arg(skill)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                dbQueryWrite(QStringLiteral("UPDATE monster_skill SET endurance=%1 WHERE monster=%2 AND skill=%3;")
                             .arg(newEndurance)
                             .arg(currentMonster->id)
                             .arg(skill)
                             );
            break;
            case ServerSettings::Database::DatabaseType_PostgreSQL:
                dbQueryWrite(QStringLiteral("UPDATE monster_skill SET endurance=%1 WHERE monster=%2 AND skill=%3;")
                             .arg(newEndurance)
                             .arg(currentMonster->id)
                             .arg(skill)
                             );
            break;
        }
    }
    else
    {
        if(GlobalServerData::serverSettings.database.fightSync==ServerSettings::Database::FightSync_AtTheEndOfBattle)
            deferedEndurance[currentMonster->id][skill]=newEndurance;
    }
    return newEndurance;
}

void Client::confirmEvolutionTo(PlayerMonster * playerMonster,const quint32 &monster)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    PlayerMonster * currentMonster=getCurrentMonster();
    PublicPlayerMonster * otherMonster=getOtherMonster();
    const Monster::Stat &currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(currentMonster->monster),currentMonster->level);
    const Monster::Stat &otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(otherMonster->monster),otherMonster->level);
    if(currentMonster!=NULL)
        if(currentMonster->hp>currentMonsterStat.hp)
        {
            errorOutput(QStringLiteral("confirmEvolutionTo() The hp %1 of current monster %2 is greater than the max %3").arg(currentMonster->hp).arg(currentMonster->monster).arg(currentMonsterStat.hp));
            return;
        }
    if(otherMonster!=NULL)
        if(otherMonster->hp>otherMonsterStat.hp)
        {
            errorOutput(QStringLiteral("confirmEvolutionTo() The hp %1 of other monster %2 is greater than the max %3").arg(otherMonster->hp).arg(otherMonster->monster).arg(otherMonsterStat.hp));
            return;
        }
    #endif
    CommonFightEngine::confirmEvolutionTo(playerMonster,monster);
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            dbQueryWrite(QStringLiteral("UPDATE `monster` SET `hp`=%1,`monster`=%2 WHERE `id`=%3")
                         .arg(playerMonster->hp)
                         .arg(playerMonster->monster)
                         .arg(playerMonster->id)
                         );
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            dbQueryWrite(QStringLiteral("UPDATE monster SET hp=%1,monster=%2 WHERE id=%3;")
                         .arg(playerMonster->hp)
                         .arg(playerMonster->monster)
                         .arg(playerMonster->id)
                         );
        break;
        case ServerSettings::Database::DatabaseType_PostgreSQL:
            dbQueryWrite(QStringLiteral("UPDATE monster SET hp=%1,monster=%2 WHERE id=%3;")
                         .arg(playerMonster->hp)
                         .arg(playerMonster->monster)
                         .arg(playerMonster->id)
                         );
        break;
    }
}

void Client::confirmEvolution(const quint32 &monsterId)
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
            errorOutput(QStringLiteral("Evolution not found"));
            return;
        }
        index++;
    }
    errorOutput(QStringLiteral("Monster for evolution not found"));
}

void Client::hpChange(PlayerMonster * currentMonster, const quint32 &newHpValue)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        PlayerMonster * currentMonster=getCurrentMonster();
        PublicPlayerMonster * otherMonster=getOtherMonster();
        const Monster::Stat &currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(currentMonster->monster),currentMonster->level);
        const Monster::Stat &otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(otherMonster->monster),otherMonster->level);
        if(currentMonster!=NULL)
            if(currentMonster->hp>currentMonsterStat.hp)
            {
                errorOutput(QStringLiteral("hpChange() The hp %1 of current monster %2 is greater than the max %3").arg(currentMonster->monster).arg(currentMonster->hp).arg(currentMonsterStat.hp));
                return;
            }
        if(otherMonster!=NULL)
            if(otherMonster->hp>otherMonsterStat.hp)
            {
                errorOutput(QStringLiteral("hpChange() The hp %1 of other monster %2 is greater than the max %3").arg(otherMonster->monster).arg(otherMonster->hp).arg(otherMonsterStat.hp));
                return;
            }
    }
    #endif
    CommonFightEngine::hpChange(currentMonster,newHpValue);
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        PlayerMonster * currentMonster=getCurrentMonster();
        PublicPlayerMonster * otherMonster=getOtherMonster();
        const Monster::Stat &currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(currentMonster->monster),currentMonster->level);
        const Monster::Stat &otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(otherMonster->monster),otherMonster->level);
        if(currentMonster!=NULL)
            if(currentMonster->hp>currentMonsterStat.hp)
            {
                errorOutput(QStringLiteral("hpChange() after The hp %1 of current monster %2 is greater than the max %3").arg(currentMonster->monster).arg(currentMonster->hp).arg(currentMonsterStat.hp));
                return;
            }
        if(otherMonster!=NULL)
            if(otherMonster->hp>otherMonsterStat.hp)
            {
                errorOutput(QStringLiteral("hpChange() after The hp %1 of other monster %2 is greater than the max %3").arg(otherMonster->monster).arg(otherMonster->hp).arg(otherMonsterStat.hp));
                return;
            }
    }
    #endif
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            dbQueryWrite(QStringLiteral("UPDATE `monster` SET `hp`=%1 WHERE `id`=%2")
                         .arg(newHpValue)
                         .arg(currentMonster->id)
                         );
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            dbQueryWrite(QStringLiteral("UPDATE monster SET hp=%1 WHERE id=%2")
                         .arg(newHpValue)
                         .arg(currentMonster->id)
                         );
        break;
        case ServerSettings::Database::DatabaseType_PostgreSQL:
            dbQueryWrite(QStringLiteral("UPDATE monster SET hp=%1 WHERE id=%2")
                         .arg(newHpValue)
                         .arg(currentMonster->id)
                         );
        break;
    }
}

bool Client::removeBuffOnMonster(PlayerMonster * currentMonster, const quint32 &buffId)
{
    const bool returnVal=CommonFightEngine::removeBuffOnMonster(currentMonster,buffId);
    if(returnVal)
    {
        switch(GlobalServerData::serverSettings.database.type)
        {
            default:
            case ServerSettings::Database::DatabaseType_Mysql:
                dbQueryWrite(QStringLiteral("DELETE FROM `monster_buff` WHERE `monster`=%1 AND `buff`=%2")
                             .arg(currentMonster->id)
                             .arg(buffId)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                dbQueryWrite(QStringLiteral("DELETE FROM monster_buff WHERE monster=%1 AND buff=%2")
                             .arg(currentMonster->id)
                             .arg(buffId)
                             );
            break;
            case ServerSettings::Database::DatabaseType_PostgreSQL:
                dbQueryWrite(QStringLiteral("DELETE FROM monster_buff WHERE monster=%1 AND buff=%2")
                             .arg(currentMonster->id)
                             .arg(buffId)
                             );
            break;
        }
    }
    return returnVal;
}

bool Client::removeAllBuffOnMonster(PlayerMonster * currentMonster)
{
    const bool &returnVal=CommonFightEngine::removeAllBuffOnMonster(currentMonster);
    if(returnVal)
    {
        switch(GlobalServerData::serverSettings.database.type)
        {
            default:
            case ServerSettings::Database::DatabaseType_Mysql:
                dbQueryWrite(QStringLiteral("DELETE FROM `monster_buff` WHERE `monster`=%1")
                             .arg(currentMonster->id)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                dbQueryWrite(QStringLiteral("DELETE FROM monster_buff WHERE monster=%1")
                             .arg(currentMonster->id)
                             );
            break;
            case ServerSettings::Database::DatabaseType_PostgreSQL:
                dbQueryWrite(QStringLiteral("DELETE FROM monster_buff WHERE monster=%1")
                             .arg(currentMonster->id)
                             );
            break;
        }
    }
    return returnVal;
}

bool Client::addLevel(PlayerMonster * monster, const quint8 &numberOfLevel)
{
    if(!CommonFightEngine::addLevel(monster,numberOfLevel))
        return false;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            dbQueryWrite(QStringLiteral("UPDATE `monster` SET `hp`=%1,`xp`=0,`level`=%2 WHERE `id`=%3;")
                         .arg(monster->hp)
                         .arg(monster->level)
                         .arg(monster->id)
                         );
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            dbQueryWrite(QStringLiteral("UPDATE monster SET hp=%1,xp=0,level=%2 WHERE id=%3;")
                         .arg(monster->hp)
                         .arg(monster->level)
                         .arg(monster->id)
                         );
        break;
        case ServerSettings::Database::DatabaseType_PostgreSQL:
            dbQueryWrite(QStringLiteral("UPDATE monster SET hp=%1,xp=0,level=%2 WHERE id=%3;")
                         .arg(monster->hp)
                         .arg(monster->level)
                         .arg(monster->id)
                         );
        break;
    }
    return true;
}

bool Client::addSkill(PlayerMonster * currentMonster,const PlayerMonster::PlayerSkill &skill)
{
    if(!CommonFightEngine::addSkill(currentMonster,skill))
        return false;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            dbQueryWrite(QStringLiteral("INSERT INTO `monster_skill`(`monster`,`skill`,`level`,`endurance`) VALUES(%1,%2,%3,%4);")
                         .arg(currentMonster->id)
                         .arg(skill.skill)
                         .arg(skill.level)
                         .arg(skill.endurance)
                         );
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            dbQueryWrite(QStringLiteral("INSERT INTO monster_skill(monster,skill,level,endurance) VALUES(%1,%2,%3,%4);")
                         .arg(currentMonster->id)
                         .arg(skill.skill)
                         .arg(skill.level)
                         .arg(skill.endurance)
                         );
        break;
        case ServerSettings::Database::DatabaseType_PostgreSQL:
            dbQueryWrite(QStringLiteral("INSERT INTO monster_skill(monster,skill,level,endurance) VALUES(%1,%2,%3,%4);")
                         .arg(currentMonster->id)
                         .arg(skill.skill)
                         .arg(skill.level)
                         .arg(skill.endurance)
                         );
        break;
    }
    return true;
}

bool Client::setSkillLevel(PlayerMonster * currentMonster,const int &index,const quint8 &level)
{
    if(!CommonFightEngine::setSkillLevel(currentMonster,index,level))
        return false;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            dbQueryWrite(QStringLiteral("UPDATE `monster_skill` SET `level`=%1 WHERE `monster`=%2 AND `skill`=%3;")
                         .arg(level)
                         .arg(currentMonster->id)
                         .arg(currentMonster->skills.at(index).skill)
                         );
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            dbQueryWrite(QStringLiteral("UPDATE monster_skill SET level=%1 WHERE monster=%2 AND skill=%3;")
                         .arg(level)
                         .arg(currentMonster->id)
                         .arg(currentMonster->skills.at(index).skill)
                         );
        break;
        case ServerSettings::Database::DatabaseType_PostgreSQL:
            dbQueryWrite(QStringLiteral("UPDATE monster_skill SET level=%1 WHERE monster=%2 AND skill=%3;")
                         .arg(level)
                         .arg(currentMonster->id)
                         .arg(currentMonster->skills.at(index).skill)
                         );
        break;
    }
    return true;
}

bool Client::removeSkill(PlayerMonster * currentMonster,const int &index)
{
    if(!CommonFightEngine::removeSkill(currentMonster,index))
        return false;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            dbQueryWrite(QStringLiteral("DELETE FROM `monster_skill` WHERE `monster`=%1 AND `skill`=%2;")
                         .arg(currentMonster->id)
                         .arg(currentMonster->skills.at(index).skill)
                     );
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            dbQueryWrite(QStringLiteral("DELETE FROM monster_skill WHERE monster=%1 AND skill=%2;")
                         .arg(currentMonster->id)
                         .arg(currentMonster->skills.at(index).skill)
                     );
        break;
        case ServerSettings::Database::DatabaseType_PostgreSQL:
            dbQueryWrite(QStringLiteral("DELETE FROM monster_skill WHERE monster=%1 AND skill=%2;")
                         .arg(currentMonster->id)
                         .arg(currentMonster->skills.at(index).skill)
                     );
        break;
    }
    return true;
}
