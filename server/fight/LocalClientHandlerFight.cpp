#include "../base/LocalClientHandler.h"
#include "../../general/base/ProtocolParsing.h"
#include "../base/GlobalServerData.h"

using namespace Pokecraft;

void LocalClientHandler::getRandomNumberIfNeeded()
{
    if(randomSeeds.size()<=POKECRAFT_SERVER_MIN_RANDOM_LIST_SIZE)
        emit askRandomNumber();
}

void LocalClientHandler::newRandomNumber(const QByteArray &randomData)
{
    randomSeeds+=randomData;
}

void LocalClientHandler::tryEscape()
{
    if(wildMonsters.empty())//check if is in fight
    {
        emit error(QString("error: tryEscape() when is not in fight"));
        return;
    }
    if(tryEscapeInternal())
    {
        #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
        emit message(QString("escape is successful"));
        #endif
        wildMonsters.clear();
        saveCurrentMonsterStat();
    }
    else
    {
        #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
        emit message(QString("escape is failed"));
        #endif
        generateOtherAttack();
        checkKOMonsters();
    }
}

void LocalClientHandler::saveCurrentMonsterStat()
{
    //save into the db
    if(GlobalServerData::serverSettings.database.fightSync==ServerSettings::Database::FightSync_AtTheEndOfBattle)
    {
        switch(GlobalServerData::serverSettings.database.type)
        {
            default:
            case ServerSettings::Database::DatabaseType_Mysql:
                emit dbQuery(QString("UPDATE monster SET hp=%2,xp=%3,level=%4,sp=%5 WHERE id=%1;")
                             .arg(player_informations->public_and_private_informations.playerMonster[selectedMonster].id)
                             .arg(player_informations->public_and_private_informations.playerMonster[selectedMonster].hp)
                             .arg(player_informations->public_and_private_informations.playerMonster[selectedMonster].remaining_xp)
                             .arg(player_informations->public_and_private_informations.playerMonster[selectedMonster].level)
                             .arg(player_informations->public_and_private_informations.playerMonster[selectedMonster].sp)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                emit dbQuery(QString("UPDATE monster SET hp=%2,xp=%3,level=%4,sp=%5 WHERE id=%1;")
                             .arg(player_informations->public_and_private_informations.playerMonster[selectedMonster].id)
                             .arg(player_informations->public_and_private_informations.playerMonster[selectedMonster].hp)
                             .arg(player_informations->public_and_private_informations.playerMonster[selectedMonster].remaining_xp)
                             .arg(player_informations->public_and_private_informations.playerMonster[selectedMonster].level)
                             .arg(player_informations->public_and_private_informations.playerMonster[selectedMonster].sp)
                             );
            break;
        }
    }
}

void LocalClientHandler::checkKOMonsters()
{
    int old_selectedMonster=selectedMonster;
    if(player_informations->public_and_private_informations.playerMonster[selectedMonster].hp==0)
    {
        #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
        emit message(QString("You current monster (%1) is KO").arg(player_informations->public_and_private_informations.playerMonster[selectedMonster].monster));
        #endif
        updateCanDoFight();
        if(!ableToFight)
        {
            #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
            emit message(QString("Player have lost, tp to %1 (%2,%3) and heal").arg(player_informations->rescue.map->map_file).arg(player_informations->rescue.x).arg(player_informations->rescue.y));
            #endif
            //teleport
            emit teleportTo(player_informations->rescue.map,player_informations->rescue.x,player_informations->rescue.y,player_informations->rescue.orientation);
            //regen all the monsters
            int index=0;
            int size=player_informations->public_and_private_informations.playerMonster.size();
            while(index<size)
            {
                if(player_informations->public_and_private_informations.playerMonster[index].egg_step==0)
                {
                    player_informations->public_and_private_informations.playerMonster[index].hp=
                            GlobalServerData::serverPrivateVariables.monsters[player_informations->public_and_private_informations.playerMonster[index].monster].stat.hp*
                            player_informations->public_and_private_informations.playerMonster[index].level/POKECRAFT_MONSTER_LEVEL_MAX;
                    if(GlobalServerData::serverSettings.database.fightSync==ServerSettings::Database::FightSync_AtEachTurn || GlobalServerData::serverSettings.database.fightSync==ServerSettings::Database::FightSync_AtTheEndOfBattle)
                    {
                        switch(GlobalServerData::serverSettings.database.type)
                        {
                            default:
                            case ServerSettings::Database::DatabaseType_Mysql:
                                emit dbQuery(QString("UPDATE monster SET hp=%1 WHERE id=%2;")
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
                }
                index++;
            }
            updateCanDoFight();
            #ifdef POKECRAFT_EXTRA_CHECK
            emit message("You lost the battle");
            if(!ableToFight)
            {
                emit error(QString("after lost in fight, remain unable to do a fight"));
                return;
            }
            #endif
            return;
        }
    }
    if(wildMonsters.first().hp==0)
    {
        #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
        emit message(QString("The wild monster (%1) is KO").arg(wildMonsters.first().monster));
        #endif
        wildMonsters.removeFirst();
        //drop the drop item here
        //give xp here
        //save into db here
        if(GlobalServerData::serverSettings.database.fightSync==ServerSettings::Database::FightSync_AtEachTurn ||
                (wildMonsters.empty() && GlobalServerData::serverSettings.database.fightSync==ServerSettings::Database::FightSync_AtTheEndOfBattle))
            switch(GlobalServerData::serverSettings.database.type)
            {
                default:
                case ServerSettings::Database::DatabaseType_Mysql:
                    emit dbQuery(QString("UPDATE monster SET xp=%2,level=%3,sp=%4 WHERE id=%1;")
                                 .arg(player_informations->public_and_private_informations.playerMonster[selectedMonster].id)
                                 .arg(player_informations->public_and_private_informations.playerMonster[selectedMonster].remaining_xp)
                                 .arg(player_informations->public_and_private_informations.playerMonster[selectedMonster].level)
                                 .arg(player_informations->public_and_private_informations.playerMonster[selectedMonster].sp)
                                 );
                break;
                case ServerSettings::Database::DatabaseType_SQLite:
                    emit dbQuery(QString("UPDATE monster SET xp=%2,level=%3,sp=%4 WHERE id=%1;")
                                 .arg(player_informations->public_and_private_informations.playerMonster[selectedMonster].id)
                                 .arg(player_informations->public_and_private_informations.playerMonster[selectedMonster].remaining_xp)
                                 .arg(player_informations->public_and_private_informations.playerMonster[selectedMonster].level)
                                 .arg(player_informations->public_and_private_informations.playerMonster[selectedMonster].sp)
                                 );
                break;
            }
        if(wildMonsters.empty())
        {
            #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
            emit message("You win the battle");
            #endif
        }
    }
    if(GlobalServerData::serverSettings.database.fightSync==ServerSettings::Database::FightSync_AtEachTurn ||
            (wildMonsters.empty() && GlobalServerData::serverSettings.database.fightSync==ServerSettings::Database::FightSync_AtTheEndOfBattle))
        switch(GlobalServerData::serverSettings.database.type)
        {
            default:
            case ServerSettings::Database::DatabaseType_Mysql:
                emit dbQuery(QString("UPDATE monster SET hp=%1 WHERE id=%2;")
                             .arg(player_informations->public_and_private_informations.playerMonster[old_selectedMonster].hp)
                             .arg(player_informations->public_and_private_informations.playerMonster[old_selectedMonster].id)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                emit dbQuery(QString("UPDATE monster SET hp=%1 WHERE id=%2;")
                             .arg(player_informations->public_and_private_informations.playerMonster[old_selectedMonster].hp)
                             .arg(player_informations->public_and_private_informations.playerMonster[old_selectedMonster].id)
                             );
            break;
        }
}

bool LocalClientHandler::checkFightCollision(Map *map,const COORD_TYPE &x,const COORD_TYPE &y)
{
    bool ok;
    if(!wildMonsters.empty())
    {
        emit error(QString("error: map: %1 (%2,%3), is in fight").arg(map->map_file).arg(x).arg(y));
        return false;
    }
    if(player_informations->isFake)
        return false;
    if(Pokecraft::MoveOnTheMap::isGrass(*map,x,y) && !map->grassMonster.empty())
    {
        if(!ableToFight)
        {
            emit error(QString("LocalClientHandler::singleMove(), can't walk into the grass into map: %1(%2,%3)").arg(map->map_file).arg(x).arg(y));
            return false;
        }
        if(stepFight_Grass==0)
        {
            if(randomSeeds.size()==0)
            {
                emit error(QString("error: no more random seed here, map: %1 (%2,%3), is in fight").arg(map->map_file).arg(x).arg(y));
                return false;
            }
            else
                stepFight_Grass=getOneSeed(16);
        }
        else
            stepFight_Grass--;
        if(stepFight_Grass==0)
        {
            PlayerMonster monster=getRandomMonster(map->grassMonster,&ok);
            if(ok)
            {
                #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
                emit message(QString("Start grass fight with monster id %1 level %2").arg(monster.monster).arg(monster.level));
                #endif
                wildMonsters << monster;
            }
            else
                emit error(QString("error: no more random seed here to have the get"));
            return ok;
        }
        else
            return false;
    }
    if(Pokecraft::MoveOnTheMap::isWater(*map,x,y) && !map->waterMonster.empty())
    {
        if(!ableToFight)
        {
            emit error(QString("LocalClientHandler::singleMove(), can't walk into the grass into map: %1(%2,%3)").arg(map->map_file).arg(x).arg(y));
            return false;
        }
        if(stepFight_Water==0)
        {
            if(randomSeeds.size()==0)
            {
                emit error(QString("error: no more random seed here, map: %1 (%2,%3), is in fight").arg(map->map_file).arg(x).arg(y));
                return false;
            }
            else
                stepFight_Water=getOneSeed(16);
        }
        else
            stepFight_Water--;
        if(stepFight_Water==0)
        {
            PlayerMonster monster=getRandomMonster(map->waterMonster,&ok);
            if(ok)
            {
                #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
                emit message(QString("Start water fight with monster id %1 level %2").arg(monster.monster).arg(monster.level));
                #endif
                wildMonsters << monster;
            }
            else
                emit error(QString("error: no more random seed here to have the get"));
            return ok;
        }
        else
            return false;
    }
    if(!map->caveMonster.empty())
    {
        if(!ableToFight)
        {
            emit error(QString("LocalClientHandler::singleMove(), can't walk into the grass into map: %1(%2,%3)").arg(map->map_file).arg(x).arg(y));
            return false;
        }
        if(stepFight_Cave==0)
        {
            if(randomSeeds.size()==0)
            {
                emit error(QString("error: no more random seed here, map: %1 (%2,%3), is in fight").arg(map->map_file).arg(x).arg(y));
                return false;
            }
            else
                stepFight_Cave=getOneSeed(16);
        }
        else
            stepFight_Cave--;
        if(stepFight_Cave==0)
        {
            PlayerMonster monster=getRandomMonster(map->caveMonster,&ok);
            if(ok)
            {
                #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
                emit message(QString("Start cave fight with monseter id: %1 level %2").arg(monster.monster).arg(monster.level));
                #endif
                wildMonsters << monster;
            }
            else
                emit error(QString("error: no more random seed here to have the get"));
            return ok;
        }
        else
            return false;
    }

    /// no fight in this zone
    return false;
}

quint8 LocalClientHandler::getOneSeed(const quint8 &max)
{
    quint16 number=static_cast<quint8>(randomSeeds.at(0));
    if(max!=0)
    {
        number*=max;
        number/=255;
    }
    randomSeeds.remove(0,1);
    return number;
}

PlayerMonster LocalClientHandler::getRandomMonster(const QList<MapMonster> &monsterList,bool *ok)
{
    PlayerMonster playerMonster;
    playerMonster.captured_with=0;
    playerMonster.egg_step=0;
    playerMonster.remaining_xp=0;
    playerMonster.sp=0;
    quint8 randomMonsterInt=getOneSeed(100);
    bool monsterFound=false;
    int index=0;
    while(index<monsterList.size())
    {
        int luck=monsterList.at(index).luck;
        if(randomMonsterInt<luck)
        {
            //it's this monster
            playerMonster.monster=monsterList.at(index).id;
            //select the level
            if(monsterList.at(index).maxLevel==monsterList.at(index).minLevel)
                playerMonster.level=monsterList.at(index).minLevel;
            else
                playerMonster.level=getOneSeed(monsterList.at(index).maxLevel-monsterList.at(index).minLevel+1)+monsterList.at(index).minLevel;
            monsterFound=true;
            break;
        }
        else
            randomMonsterInt-=luck;
        index++;
    }
    if(!monsterFound)
    {
        emit error(QString("error: no wild monster selected"));
        *ok=false;
        playerMonster.monster=0;
        playerMonster.level=0;
        playerMonster.gender=PlayerMonster::Unknown;
        return playerMonster;
    }
    Monster monsterDef=GlobalServerData::serverPrivateVariables.monsters[playerMonster.monster];
    if(monsterDef.ratio_gender>0 && monsterDef.ratio_gender<100)
    {
        qint8 temp_ratio=getOneSeed(101);
        if(temp_ratio<monsterDef.ratio_gender)
            playerMonster.gender=PlayerMonster::Male;
        else
            playerMonster.gender=PlayerMonster::Female;
    }
    else
    {
        switch(monsterDef.ratio_gender)
        {
            case 0:
                playerMonster.gender=PlayerMonster::Male;
            break;
            case 100:
                playerMonster.gender=PlayerMonster::Female;
            break;
            default:
                playerMonster.gender=PlayerMonster::Unknown;
            break;
        }
    }
    Monster::Stat monsterStat=getStat(monsterDef,playerMonster.level);
    playerMonster.hp=monsterStat.hp;
    index=monsterDef.attack.size()-1;
    while(index>=0 && playerMonster.skills.size()<POKECRAFT_MONSTER_WILD_SKILL_NUMBER)
    {
        if(monsterDef.attack.at(index).level<=playerMonster.level)
            playerMonster.skills << monsterDef.attack.at(index).skill;
        index--;
    }
    *ok=true;
    return playerMonster;
}

/** \warning you need check before the input data */
Monster::Stat LocalClientHandler::getStat(const Monster &monster, const quint8 &level)
{
    Monster::Stat stat=monster.stat;
    stat.attack=stat.attack*level/POKECRAFT_MONSTER_LEVEL_MAX;
    if(stat.attack==0)
        stat.attack=1;
    stat.defense=stat.defense*level/POKECRAFT_MONSTER_LEVEL_MAX;
    if(stat.defense==0)
        stat.defense=1;
    stat.hp=stat.hp*level/POKECRAFT_MONSTER_LEVEL_MAX;
    if(stat.hp==0)
        stat.hp=1;
    stat.special_attack=stat.special_attack*level/POKECRAFT_MONSTER_LEVEL_MAX;
    if(stat.special_attack==0)
        stat.special_attack=1;
    stat.special_defense=stat.special_defense*level/POKECRAFT_MONSTER_LEVEL_MAX;
    if(stat.special_defense==0)
        stat.special_defense=1;
    stat.speed=stat.speed*level/POKECRAFT_MONSTER_LEVEL_MAX;
    if(stat.speed==0)
        stat.speed=1;
    return stat;
}

bool LocalClientHandler::tryEscapeInternal()
{
    quint8 value=getOneSeed(101);
    if(wildMonsters.first().level<player_informations->public_and_private_informations.playerMonster.at(selectedMonster).level && value<75)
        return true;
    if(wildMonsters.first().level==player_informations->public_and_private_informations.playerMonster.at(selectedMonster).level && value<50)
        return true;
    if(wildMonsters.first().level>player_informations->public_and_private_informations.playerMonster.at(selectedMonster).level && value<25)
        return true;
    return false;
}

void LocalClientHandler::updateCanDoFight()
{
    ableToFight=false;
    int index=0;
    while(index<player_informations->public_and_private_informations.playerMonster.size())
    {
        const PlayerMonster &playerMonsterEntry=player_informations->public_and_private_informations.playerMonster.at(index);
        if(playerMonsterEntry.hp>0 && playerMonsterEntry.egg_step==0)
        {
            selectedMonster=index;
            ableToFight=true;
            return;
        }
        index++;
    }
}

void LocalClientHandler::generateOtherAttack()
{
    const PlayerMonster &otherMonster=wildMonsters.first();
    if(otherMonster.skills.empty())
    {
        emit message(QString("Unable to fight because the other monster (%1, level: %2) have no skill").arg(otherMonster.monster).arg(otherMonster.level));
        return;
    }
    int position;
    if(otherMonster.skills.size()==1)
        position=0;
    else
        position=getOneSeed(otherMonster.skills.size());
    const PlayerMonster::Skill &otherMonsterSkill=otherMonster.skills.at(position);
    #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
    emit message(QString("The wild monster use skill %1 at level %2").arg(otherMonsterSkill.skill).arg(otherMonsterSkill.level));
    #endif
    const Monster::Skill::SkillList &skillList=GlobalServerData::serverPrivateVariables.monsterSkills[otherMonsterSkill.skill].level.at(otherMonsterSkill.level-1);
    int index=0;
    while(index<skillList.buff.size())
    {
        const Monster::Skill::Buff &buff=skillList.buff.at(index);
        bool success;
        if(buff.success==100)
            success=true;
        else
            success=(getOneSeed(100)<buff.success);
        if(success)
            applyOtherBuffEffect(buff.effect);
        index++;
    }
    index=0;
    while(index<skillList.life.size())
    {
        const Monster::Skill::Life &life=skillList.life.at(index);
        bool success;
        if(life.success==100)
            success=true;
        else
            success=(getOneSeed(100)<life.success);
        if(success)
            applyOtherLifeEffect(life.effect);
        index++;
    }
}

void LocalClientHandler::applyOtherLifeEffect(const Monster::Skill::LifeEffect &effect)
{
    qint32 quantity;
    const Monster::Stat &stat=getStat(GlobalServerData::serverPrivateVariables.monsters[wildMonsters.first().monster],wildMonsters.first().level);
    switch(effect.on)
    {
        case Monster::ApplyOn_AloneEnemy:
        case Monster::ApplyOn_AllEnemy:
            if(effect.type==QuantityType_Quantity)
            {
                Monster::Stat otherStat=getStat(GlobalServerData::serverPrivateVariables.monsters[player_informations->public_and_private_informations.playerMonster[selectedMonster].monster],player_informations->public_and_private_informations.playerMonster[selectedMonster].level);
                if(effect.quantity<0)
                {
                    quantity=-((-effect.quantity*stat.attack*wildMonsters.first().level)/(POKECRAFT_MONSTER_LEVEL_MAX*otherStat.defense));
                    if(quantity==0)
                        quantity=-1;
                }
                else if(effect.quantity>0)//ignore the def for heal
                {
                    quantity=effect.quantity*wildMonsters.first().level/POKECRAFT_MONSTER_LEVEL_MAX;
                    if(quantity==0)
                        quantity=1;
                }
            }
            else
                quantity=(player_informations->public_and_private_informations.playerMonster[selectedMonster].hp*effect.quantity)/100;
            if(quantity<0 && (-quantity)>player_informations->public_and_private_informations.playerMonster[selectedMonster].hp)
            {
                player_informations->public_and_private_informations.playerMonster[selectedMonster].hp=0;
                player_informations->public_and_private_informations.playerMonster[selectedMonster].buffs.clear();
                updateCanDoFight();
            }
            else if(quantity>0 && quantity>(stat.hp-player_informations->public_and_private_informations.playerMonster[selectedMonster].hp))
                player_informations->public_and_private_informations.playerMonster[selectedMonster].hp=stat.hp;
            else
                player_informations->public_and_private_informations.playerMonster[selectedMonster].hp+=quantity;
            #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
            if(effect.quantity<0)
                emit message(QString("The wild monster give to you %1 of damage").arg(-quantity));
            if(effect.quantity>0)
                emit message(QString("The wild monster heal you of %1").arg(quantity));
            #endif
        break;
        case Monster::ApplyOn_Themself:
        case Monster::ApplyOn_AllAlly:
            if(effect.type==QuantityType_Quantity)
            {
                if(effect.quantity<0)
                {
                    quantity=-((-effect.quantity*stat.attack*wildMonsters.first().level)/(POKECRAFT_MONSTER_LEVEL_MAX*stat.defense));
                    if(quantity==0)
                        quantity=-1;
                }
                else if(effect.quantity>0)//ignore the def for heal
                {
                    quantity=effect.quantity*wildMonsters.first().level/POKECRAFT_MONSTER_LEVEL_MAX;
                    if(quantity==0)
                        quantity=1;
                }
            }
            else
                quantity=(wildMonsters.first().hp*effect.quantity)/100;
            if(quantity<0 && (-quantity)>wildMonsters.first().hp)
                wildMonsters.first().hp=0;
            else if(quantity>0 && quantity>(stat.hp-wildMonsters.first().hp))
                wildMonsters.first().hp=stat.hp;
            else
                wildMonsters.first().hp+=quantity;
            #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
            if(effect.quantity<0)
                emit message(QString("The wild monster take %1 of damage on them self").arg(-quantity));
            if(effect.quantity>0)
                emit message(QString("The wild monster heal them self of %1").arg(quantity));
            #endif
        break;
        default:
            emit error("Not apply match, can't apply the buff");
        break;
    }
}

void LocalClientHandler::applyOtherBuffEffect(const Monster::Skill::BuffEffect &effect)
{
    PlayerMonster::Buff tempBuff;
    tempBuff.buff=effect.buff;
    tempBuff.level=effect.level;
    switch(effect.on)
    {
        case Monster::ApplyOn_AloneEnemy:
        case Monster::ApplyOn_AllEnemy:
            player_informations->public_and_private_informations.playerMonster[selectedMonster].buffs << tempBuff;
        break;
        case Monster::ApplyOn_Themself:
        case Monster::ApplyOn_AllAlly:
            wildMonsters.first().buffs << tempBuff;
        break;
        default:
            qDebug() << "Not apply match, can't apply the buff";
        break;
    }
}

void LocalClientHandler::useSkill(const quint32 &skill)
{
    Monster::Stat currentMonsterStat=getStat(GlobalServerData::serverPrivateVariables.monsters[player_informations->public_and_private_informations.playerMonster[selectedMonster].monster],player_informations->public_and_private_informations.playerMonster[selectedMonster].level);
    Monster::Stat otherMonsterStat=getStat(GlobalServerData::serverPrivateVariables.monsters[wildMonsters.first().monster],wildMonsters.first().level);
    bool currentMonsterStatIsFirstToAttack=(currentMonsterStat.speed>=otherMonsterStat.speed);
    //do the current monster attack
    if(currentMonsterStatIsFirstToAttack)
    {
        doTheCurrentMonsterAttack(skill,currentMonsterStat,otherMonsterStat);
        checkKOMonsters();
    }
    //do the other monster attack
    generateOtherAttack();
    checkKOMonsters();
    //do the current monster attack
    if(!currentMonsterStatIsFirstToAttack)
    {
        doTheCurrentMonsterAttack(skill,currentMonsterStat,otherMonsterStat);
        checkKOMonsters();
    }
}

void LocalClientHandler::doTheCurrentMonsterAttack(const quint32 &skill,const Monster::Stat &currentMonsterStat,const Monster::Stat &otherMonsterStat)
{
    /// \todo use the variable currentMonsterStat and otherMonsterStat to have better speed
    Q_UNUSED(currentMonsterStat);
    Q_UNUSED(otherMonsterStat);
    int index=0;
    while(index<player_informations->public_and_private_informations.playerMonster[selectedMonster].skills.size())
    {
        if(player_informations->public_and_private_informations.playerMonster[selectedMonster].skills.at(index).skill==skill)
            break;
        index++;
    }
    if(index==player_informations->public_and_private_informations.playerMonster[selectedMonster].skills.size())
    {
        emit error(QString("Unable to fight because the current monster (%1, level: %2) have not the skill %3").arg(player_informations->public_and_private_informations.playerMonster[selectedMonster].monster).arg(player_informations->public_and_private_informations.playerMonster[selectedMonster].level).arg(skill));
        return;
    }

    const Monster::Skill::SkillList &skillList=GlobalServerData::serverPrivateVariables.monsterSkills[skill].level.at(player_informations->public_and_private_informations.playerMonster[selectedMonster].skills.at(index).level-1);
    #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
    emit message(QString("You use skill %1 at level %2").arg(skill).arg(player_informations->public_and_private_informations.playerMonster[selectedMonster].skills.at(index).level));
    #endif
    index=0;
    while(index<skillList.buff.size())
    {
        const Monster::Skill::Buff &buff=skillList.buff.at(index);
        bool success;
        if(buff.success==100)
            success=true;
        else
            success=(getOneSeed(100)<buff.success);
        if(success)
            applyCurrentBuffEffect(buff.effect);
        index++;
    }
    index=0;
    while(index<skillList.life.size())
    {
        const Monster::Skill::Life &life=skillList.life.at(index);
        bool success;
        if(life.success==100)
            success=true;
        else
            success=(getOneSeed(100)<life.success);
        if(success)
            applyCurrentLifeEffect(life.effect);
        index++;
    }
}

void LocalClientHandler::applyCurrentLifeEffect(const Monster::Skill::LifeEffect &effect)
{
    qint32 quantity;
    const Monster::Stat &stat=getStat(GlobalServerData::serverPrivateVariables.monsters[player_informations->public_and_private_informations.playerMonster[selectedMonster].monster],player_informations->public_and_private_informations.playerMonster[selectedMonster].level);
    switch(effect.on)
    {
        case Monster::ApplyOn_AloneEnemy:
        case Monster::ApplyOn_AllEnemy:
            if(effect.type==QuantityType_Quantity)
            {
                Monster::Stat otherStat=getStat(GlobalServerData::serverPrivateVariables.monsters[wildMonsters.first().monster],wildMonsters.first().level);
                if(effect.quantity<0)
                {
                    quantity=-((-effect.quantity*stat.attack*player_informations->public_and_private_informations.playerMonster[selectedMonster].level)/(POKECRAFT_MONSTER_LEVEL_MAX*otherStat.defense));
                    if(quantity==0)
                        quantity=-1;
                }
                else if(effect.quantity>0)//ignore the def for heal
                {
                    quantity=effect.quantity*player_informations->public_and_private_informations.playerMonster[selectedMonster].level/POKECRAFT_MONSTER_LEVEL_MAX;
                    if(quantity==0)
                        quantity=1;
                }
            }
            else
                quantity=(wildMonsters.first().hp*effect.quantity)/100;
            if(quantity<0 && (-quantity)>wildMonsters.first().hp)
                wildMonsters.first().hp=0;
            else if(quantity>0 && quantity>(stat.hp-wildMonsters.first().hp))
                wildMonsters.first().hp=stat.hp;
            else
                wildMonsters.first().hp+=quantity;
            #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
            if(effect.quantity<0)
                emit message(QString("You do %1 of damage on the wild monster").arg(-quantity));
            if(effect.quantity>0)
                emit message(QString("You heal the wild monster of %1").arg(quantity));
            #endif
        break;
        case Monster::ApplyOn_Themself:
        case Monster::ApplyOn_AllAlly:
            if(effect.type==QuantityType_Quantity)
            {
                if(effect.quantity<0)
                {
                    quantity=-((-effect.quantity*stat.attack*player_informations->public_and_private_informations.playerMonster[selectedMonster].level)/(POKECRAFT_MONSTER_LEVEL_MAX*stat.defense));
                    if(quantity==0)
                        quantity=-1;
                }
                else if(effect.quantity>0)//ignore the def for heal
                {
                    quantity=effect.quantity*player_informations->public_and_private_informations.playerMonster[selectedMonster].level/POKECRAFT_MONSTER_LEVEL_MAX;
                    if(quantity==0)
                        quantity=1;
                }
            }
            else
                quantity=(player_informations->public_and_private_informations.playerMonster[selectedMonster].hp*effect.quantity)/100;
            if(quantity<0 && (-quantity)>player_informations->public_and_private_informations.playerMonster[selectedMonster].hp)
            {
                player_informations->public_and_private_informations.playerMonster[selectedMonster].hp=0;
                player_informations->public_and_private_informations.playerMonster[selectedMonster].buffs.clear();
                updateCanDoFight();
            }
            else if(quantity>0 && quantity>(stat.hp-player_informations->public_and_private_informations.playerMonster[selectedMonster].hp))
                player_informations->public_and_private_informations.playerMonster[selectedMonster].hp=stat.hp;
            else
                player_informations->public_and_private_informations.playerMonster[selectedMonster].hp+=quantity;
            #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
            if(effect.quantity<0)
                emit message(QString("You take you self %1 of damage").arg(-quantity));
            if(effect.quantity>0)
                emit message(QString("You heal your self of %1").arg(quantity));
            #endif
        break;
        default:
            emit error("Not apply match, can't apply the life effect");
        break;
    }
}

void LocalClientHandler::applyCurrentBuffEffect(const Monster::Skill::BuffEffect &effect)
{
    PlayerMonster::Buff tempBuff;
    tempBuff.buff=effect.buff;
    tempBuff.level=effect.level;
    switch(effect.on)
    {
        case Monster::ApplyOn_AloneEnemy:
        case Monster::ApplyOn_AllEnemy:
            wildMonsters.first().buffs << tempBuff;
        break;
        case Monster::ApplyOn_Themself:
        case Monster::ApplyOn_AllAlly:
            player_informations->public_and_private_informations.playerMonster[selectedMonster].buffs << tempBuff;
        break;
        default:
            qDebug() << "Not apply match, can't apply the buff";
        break;
    }
}
