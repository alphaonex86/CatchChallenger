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
    botFightCash=0;
}

LocalClientHandlerFight::~LocalClientHandlerFight()
{
}

void LocalClientHandlerFight::getRandomNumberIfNeeded()
{
    if(randomSeeds.size()<=CATCHCHALLENGER_SERVER_MIN_RANDOM_LIST_SIZE)
        emit askRandomNumber();
}

bool LocalClientHandlerFight::tryEscape()
{
    if(!CommonFightEngine::tryEscape())//check if is in fight
        return false;
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
        Skill::AttackReturn attackReturn=CommonFightEngine::generateOtherAttack();
        if(checkKOCurrentMonsters())
            checkLoose();
    }
    return true;
}

bool LocalClientHandlerFight::getBattleIsValidated()
{
    return battleIsValidated;
}

void LocalClientHandlerFight::saveCurrentMonsterStat()
{
    PlayerMonster * monster=getCurrentMonster();
    if(monster==NULL)
    {
        //no monsters
        return;
    }
    //save into the db
    if(GlobalServerData::serverSettings.database.fightSync==ServerSettings::Database::FightSync_AtTheEndOfBattle)
    {
        switch(GlobalServerData::serverSettings.database.type)
        {
            default:
            case ServerSettings::Database::DatabaseType_Mysql:
                emit dbQuery(QString("UPDATE monster SET hp=%2,xp=%3,level=%4,sp=%5 WHERE id=%1;")
                             .arg(getCurrentMonster()->id)
                             .arg(getCurrentMonster()->hp)
                             .arg(getCurrentMonster()->remaining_xp)
                             .arg(getCurrentMonster()->level)
                             .arg(getCurrentMonster()->sp)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                emit dbQuery(QString("UPDATE monster SET hp=%2,xp=%3,level=%4,sp=%5 WHERE id=%1;")
                             .arg(getCurrentMonster()->id)
                             .arg(getCurrentMonster()->hp)
                             .arg(getCurrentMonster()->remaining_xp)
                             .arg(getCurrentMonster()->level)
                             .arg(getCurrentMonster()->sp)
                             );
            break;
        }
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

void LocalClientHandlerFight::setVariable(Player_internal_informations *player_informations)
{
    this->player_informations=player_informations;
    CommonFightEngine::setVariable(&player_informations->public_and_private_informations);
}

bool LocalClientHandlerFight::checkLoose()
{
    updateCanDoFight();
    if(!ableToFight)
    {
        #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
        emit message(QString("You have lost, tp to %1 (%2,%3) and heal").arg(player_informations->rescue.map->map_file).arg(player_informations->rescue.x).arg(player_informations->rescue.y));
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
                        CommonDatapack::commonDatapack.monsters[player_informations->public_and_private_informations.playerMonster[index].monster].stat.hp*
                        player_informations->public_and_private_informations.playerMonster[index].level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
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
        battleFinished();
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        emit message("You lost the battle");
        if(!ableToFight)
        {
            emit error(QString("after lost in fight, remain unable to do a fight"));
            return true;
        }
        #endif
        return true;
    }
    return false;
}

bool LocalClientHandlerFight::checkKOOtherMonstersForGain()
{
    bool winTheTurn=false;
    quint32 give_xp=0;
    if(!wildMonsters.isEmpty())
    {
        if(wildMonsters.first().hp==0)
        {
            winTheTurn=true;
            #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
            emit message(QString("The wild monster (%1) is KO").arg(wildMonsters.first().monster));
            #endif
            //drop the drop item here
            QList<MonsterDrops> drops=GlobalServerData::serverPrivateVariables.monsterDrops.values(wildMonsters.first().monster);
            if(player_informations->questsDrop.contains(wildMonsters.first().monster))
                drops+=player_informations->questsDrop.values(wildMonsters.first().monster);
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
            //give xp/sp here
            const Monster &wildmonster=CommonDatapack::commonDatapack.monsters[wildMonsters.first().monster];
            getCurrentMonster()->sp+=wildmonster.give_sp*wildMonsters.first().level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
            give_xp=wildmonster.give_xp*wildMonsters.first().level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
            #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
            emit message(QString("You win %1 xp and %2 sp").arg(give_xp).arg(wildmonster.give_sp*wildMonsters.first().level/CATCHCHALLENGER_MONSTER_LEVEL_MAX));
            #endif
            wildMonsters.removeFirst();
        }
    }
    if(!botFightMonsters.isEmpty())
    {
        if(botFightMonsters.first().hp==0)
        {
            winTheTurn=true;
            #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
            emit message(QString("The wild monster (%1) is KO").arg(botFightMonsters.first().monster));
            #endif
            //don't drop item because it's not a wild fight
            //give xp/sp here
            const Monster &wildmonster=CommonDatapack::commonDatapack.monsters[botFightMonsters.first().monster];
            getCurrentMonster()->sp+=wildmonster.give_sp*botFightMonsters.first().level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
            give_xp=wildmonster.give_xp*botFightMonsters.first().level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
            #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
            emit message(QString("You win %1 xp and %2 sp").arg(give_xp).arg(wildmonster.give_sp*botFightMonsters.first().level/CATCHCHALLENGER_MONSTER_LEVEL_MAX));
            #endif
            botFightMonsters.removeFirst();
        }
    }
    else if(battleIsValidated)
    {
        emit message("check the other monster is KO");
        PublicPlayerMonster firstValidOtherPlayerMonster=FacilityLib::playerMonsterToPublicPlayerMonster(*otherPlayerBattle->getCurrentMonster());
        if(firstValidOtherPlayerMonster.hp==0)
        {
            emit message("the other monster is KO");
            winTheTurn=true;
            #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
            emit message(QString("The other player monster in battle is KO"));
            #endif
            //no item
            //no xp to prevent cheatings (hight level do only bad attack)
            otherPlayerBattle->saveStat();
            otherPlayerBattle->updateCanDoFight();
            if(!otherPlayerBattle->getAbleToFight())
            {
                otherPlayerBattle->checkLoose();
                battleFinished();
            }
        }
    }
    else
    {
        emit message("unknown other monster type");
        return false;
    }
    if(winTheTurn && give_xp>0)
    {
        bool haveChangeOfLevel=false;
        const Monster &currentmonster=CommonDatapack::commonDatapack.monsters[getCurrentMonster()->monster];
        quint32 xp=getCurrentMonster()->remaining_xp;
        quint32 level=getCurrentMonster()->level;
        while(currentmonster.level_to_xp.at(level-1)<(xp+give_xp))
        {
            quint32 old_max_hp=currentmonster.stat.hp*level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
            quint32 new_max_hp=currentmonster.stat.hp*(level+1)/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
            give_xp-=currentmonster.level_to_xp.at(level-1)-xp;
            xp=0;
            level++;
            haveChangeOfLevel=true;
            getCurrentMonster()->hp+=new_max_hp-old_max_hp;
            #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
            emit message(QString("You pass to the level %1").arg(level));
            #endif
        }
        xp+=give_xp;
        getCurrentMonster()->remaining_xp=xp;
        if(haveChangeOfLevel)
            getCurrentMonster()->level=level;

        //save into db here
        if(!isInFight() && GlobalServerData::serverSettings.database.fightSync==ServerSettings::Database::FightSync_AtTheEndOfBattle)
            saveCurrentMonsterStat();
        if(GlobalServerData::serverSettings.database.fightSync==ServerSettings::Database::FightSync_AtEachTurn)
            switch(GlobalServerData::serverSettings.database.type)
            {
                default:
                case ServerSettings::Database::DatabaseType_Mysql:
                    emit dbQuery(QString("UPDATE monster SET xp=%2,sp=%3 WHERE id=%1;")
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
        if(haveChangeOfLevel)
        {
            if(GlobalServerData::serverSettings.database.fightSync==ServerSettings::Database::FightSync_AtEachTurn)
                switch(GlobalServerData::serverSettings.database.type)
                {
                    default:
                    case ServerSettings::Database::DatabaseType_Mysql:
                        emit dbQuery(QString("UPDATE monster SET level=%2,hp=%3 WHERE id=%1;")
                                     .arg(getCurrentMonster()->id)
                                     .arg(getCurrentMonster()->level)
                                     .arg(getCurrentMonster()->hp)
                                     );
                    break;
                    case ServerSettings::Database::DatabaseType_SQLite:
                        emit dbQuery(QString("UPDATE monster SET level=%2,hp=%3 WHERE id=%1;")
                                     .arg(getCurrentMonster()->id)
                                     .arg(getCurrentMonster()->level)
                                     .arg(getCurrentMonster()->hp)
                                     );
                    break;
                }
        }
        if(!isInFight())
        {
            #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
            emit message("You win the battle");
            #endif
        }
    }
    return winTheTurn;
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
            emit dbQuery(QString("UPDATE monster SET hp=%1 WHERE id=%2;")
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

bool LocalClientHandlerFight::checkFightCollision(Map *map,const COORD_TYPE &x,const COORD_TYPE &y)
{
    bool ok;
    if(isInFight())
    {
        emit error(QString("error: map: %1 (%2,%3), is in fight").arg(map->map_file).arg(x).arg(y));
        return false;
    }
    if(player_informations->isFake)
        return false;
    if(CatchChallenger::MoveOnTheMap::isGrass(*map,x,y) && !map->grassMonster.empty())
    {
        if(!ableToFight)
        {
            emit error(QString("LocalClientHandlerFight::singleMove(), can't walk into the grass into map: %1(%2,%3)").arg(map->map_file).arg(x).arg(y));
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
    if(CatchChallenger::MoveOnTheMap::isWater(*map,x,y) && !map->waterMonster.empty())
    {
        if(!ableToFight)
        {
            emit error(QString("LocalClientHandlerFight::singleMove(), can't walk into the grass into map: %1(%2,%3)").arg(map->map_file).arg(x).arg(y));
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
            emit error(QString("LocalClientHandlerFight::singleMove(), can't walk into the grass into map: %1(%2,%3)").arg(map->map_file).arg(x).arg(y));
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

bool LocalClientHandlerFight::tryEscapeInternal()
{
    #ifdef CATCHCHALLENGER_SERVER_EXTRA_CHECK
    if(wildMonsters.isEmpty())
    {
        emit error("Can't try escape if your are not against wild monster");
        return false;
    }
    #endif
    quint8 value=getOneSeed(101);
    if(wildMonsters.first().level<player_informations->public_and_private_informations.playerMonster.at(selectedMonster).level && value<75)
        return true;
    if(wildMonsters.first().level==player_informations->public_and_private_informations.playerMonster.at(selectedMonster).level && value<50)
        return true;
    if(wildMonsters.first().level>player_informations->public_and_private_informations.playerMonster.at(selectedMonster).level && value<25)
        return true;
    return false;
}

void LocalClientHandlerFight::useSkill(const quint32 &skill)
{
    if(!isInFight())
    {
        emit error("Try use skill when not in fight");
        return;
    }
    int index=0;
    while(index<getCurrentMonster()->skills.size())
    {
        if(getCurrentMonster()->skills.at(index).skill==skill)
            break;
        index++;
    }
    if(index==getCurrentMonster()->skills.size())
    {
        emit error(QString("Unable to fight because the current monster (%1, level: %2) have not the skill %3").arg(getCurrentMonster()->monster).arg(getCurrentMonster()->level).arg(skill));
        return;
    }
    quint8 skillLevel=getCurrentMonster()->skills.at(index).level;
    if(battleIsValidated)
    {
        useBattleSkill(skill,skillLevel);
        return;
    }
    if(!wildMonsters.isEmpty())
    {
        useSkillAgainstWildMonster(skill,skillLevel);
        return;
    }
    if(!botFightMonsters.isEmpty())
    {
        useSkillAgainstBotMonster(skill,skillLevel);
        return;
    }
    emit error("Unable to locate the battle monster or is not in battle to use a skill");
}

void LocalClientHandlerFight::useSkillAgainstWildMonster(const quint32 &skill,const quint8 &skillLevel)
{
    Monster::Stat currentMonsterStat=getStat(CommonDatapack::commonDatapack.monsters[getCurrentMonster()->monster],getCurrentMonster()->level);
    Monster::Stat otherMonsterStat=getStat(CommonDatapack::commonDatapack.monsters[wildMonsters.first().monster],wildMonsters.first().level);
    bool currentMonsterStatIsFirstToAttack=(currentMonsterStat.speed>=otherMonsterStat.speed);
    QList<Skill::AttackReturn> monsterReturnList;
    bool isKO=false;
    bool currentMonsterisKO=false,otherMonsterisKO=false;
    bool currentPlayerLoose=false,otherPlayerLoose=false;
    if(currentMonsterStatIsFirstToAttack)
    {
        monsterReturnList << doTheCurrentMonsterAttack(skill,skillLevel,currentMonsterStat,otherMonsterStat);
        monsterReturnList.last().doByTheCurrentMonster=true;
        currentMonsterisKO=checkKOCurrentMonsters();
        if(currentMonsterisKO)
        {
            emit message("current player is KO");
            currentPlayerLoose=checkLoose();
            isKO=true;
        }
        else
        {
            emit message("check other player monster");
            otherMonsterisKO=checkKOOtherMonstersForGain();
            if(otherMonsterisKO)
                isKO=true;
        }
    }
    //do the other monster attack
    if(!isKO)
    {
        Skill::AttackReturn attackReturn=CommonFightEngine::generateOtherAttack();
        otherMonsterisKO=checkKOOtherMonstersForGain();
        if(otherMonsterisKO)
        {
            emit message("middle other player is KO");
            otherPlayerLoose=checkLoose();
            isKO=true;
        }
        else
        {
            emit message("middle current player is KO");
            currentMonsterisKO=!ableToFight;
            if(currentMonsterisKO)
                isKO=true;
        }
    }
    //do the current monster attack
    if(!isKO)
        if(!currentMonsterStatIsFirstToAttack)
        {
            monsterReturnList << doTheCurrentMonsterAttack(skill,skillLevel,currentMonsterStat,otherMonsterStat);
            monsterReturnList.last().doByTheCurrentMonster=true;
            currentMonsterisKO=checkKOCurrentMonsters();
            if(currentMonsterisKO)
            {
                emit message("current player is KO");
                currentPlayerLoose=checkLoose();
                isKO=true;
            }
            else
            {
                emit message("check other player monster");
                otherMonsterisKO=checkKOOtherMonstersForGain();
                if(otherMonsterisKO)
                    isKO=true;
            }
        }
    syncForEndOfTurn();
    if(currentPlayerLoose)
        emit message("The wild monster put all your monster KO");
    if(otherPlayerLoose)
        emit message("You have put KO the wild monster");
}

PublicPlayerMonster * LocalClientHandlerFight::getOtherMonster()
{
    if(!botFightMonsters.isEmpty())
        return &botFightMonsters.first();
    return CommonFightEngine::getOtherMonster();
}

void LocalClientHandlerFight::useSkillAgainstBotMonster(const quint32 &skill,const quint8 &skillLevel)
{
    Monster::Stat currentMonsterStat=getStat(CommonDatapack::commonDatapack.monsters[getCurrentMonster()->monster],getCurrentMonster()->level);
    Monster::Stat otherMonsterStat=getStat(CommonDatapack::commonDatapack.monsters[botFightMonsters.first().monster],botFightMonsters.first().level);
    bool currentMonsterStatIsFirstToAttack=(currentMonsterStat.speed>=otherMonsterStat.speed);
    QList<Skill::AttackReturn> monsterReturnList;
    bool isKO=false;
    bool currentMonsterisKO=false,otherMonsterisKO=false;
    bool currentPlayerLoose=false,otherPlayerLoose=false;
    if(currentMonsterStatIsFirstToAttack)
    {
        monsterReturnList << doTheCurrentMonsterAttack(skill,skillLevel,currentMonsterStat,otherMonsterStat);
        monsterReturnList.last().doByTheCurrentMonster=true;
        currentMonsterisKO=checkKOCurrentMonsters();
        if(currentMonsterisKO)
        {
            emit message("current player is KO");
            currentPlayerLoose=checkLoose();
            isKO=true;
        }
        else
        {
            emit message("check other player monster");
            otherMonsterisKO=checkKOOtherMonstersForGain();
            if(otherMonsterisKO)
                isKO=true;
        }
    }
    //do the other monster attack
    if(!isKO)
    {
        Skill::AttackReturn attackReturn=CommonFightEngine::generateOtherAttack();
        otherMonsterisKO=checkKOOtherMonstersForGain();
        if(otherMonsterisKO)
        {
            emit message("middle other player is KO");
            otherPlayerLoose=checkLoose();
            isKO=true;
        }
        else
        {
            emit message("middle current player is KO");
            currentMonsterisKO=!ableToFight;
            if(currentMonsterisKO)
                isKO=true;
        }
    }
    //do the current monster attack
    if(!isKO)
        if(!currentMonsterStatIsFirstToAttack)
        {
            monsterReturnList << doTheCurrentMonsterAttack(skill,skillLevel,currentMonsterStat,otherMonsterStat);
            monsterReturnList.last().doByTheCurrentMonster=true;
            currentMonsterisKO=checkKOCurrentMonsters();
            if(currentMonsterisKO)
            {
                emit message("current player is KO");
                currentPlayerLoose=checkLoose();
                isKO=true;
            }
            else
            {
                emit message("check other player monster");
                otherMonsterisKO=checkKOOtherMonstersForGain();
                if(otherMonsterisKO)
                    isKO=true;
            }
        }
    syncForEndOfTurn();
    otherPlayerLoose=otherMonsterisKO;//workaround, bug on multiple monster
    if(currentPlayerLoose)
        emit message("The bot fight put all your monster KO");
    if(otherPlayerLoose)
        emit message("You have put KO the bot fight");
    if(!isInFight())
    {
        if(otherPlayerLoose)
        {
            emit message(QString("Register the win against the bot fight: %1").arg(botFightId));
            addCash(CommonDatapack::commonDatapack.botFights[botFightId].cash);
            player_informations->public_and_private_informations.bot_already_beaten << botFightId;
            switch(GlobalServerData::serverSettings.database.type)
            {
                default:
                case ServerSettings::Database::DatabaseType_Mysql:
                    emit dbQuery(QString("INSERT INTO bot_already_beaten(player_id,botfight_id) VALUES(%1,%2);")
                                 .arg(player_informations->id)
                                 .arg(botFightId)
                                 );
                break;
                case ServerSettings::Database::DatabaseType_SQLite:
                    emit dbQuery(QString("INSERT INTO bot_already_beaten(player_id,botfight_id) VALUES(%1,%2);")
                                 .arg(player_informations->id)
                                 .arg(botFightId)
                                 );
                break;
            }
        }
    }
}

bool LocalClientHandlerFight::buffIsValid(const Skill::BuffEffect &buffEffect)
{
    if(!CommonDatapack::commonDatapack.monsterBuffs.contains(buffEffect.buff))
        return false;
    if(buffEffect.level<=0)
        return false;
    if(buffEffect.level<=0)
        return false;
    if(buffEffect.level>CommonDatapack::commonDatapack.monsterBuffs[buffEffect.buff].level.size())
        return false;
    switch(buffEffect.on)
    {
        case ApplyOn_AloneEnemy:
        case ApplyOn_AllEnemy:
        case ApplyOn_Themself:
        case ApplyOn_AllAlly:
        case ApplyOn_Nobody:
        break;
        default:
        return false;
    }
    return true;
}

Skill::AttackReturn LocalClientHandlerFight::doTheCurrentMonsterAttack(const quint32 &skill,const quint8 &skillLevel,const Monster::Stat &currentMonsterStat,const Monster::Stat &otherMonsterStat)
{
    /// \todo use the variable currentMonsterStat and otherMonsterStat to have better speed
    Q_UNUSED(currentMonsterStat);
    Q_UNUSED(otherMonsterStat);
    Skill::AttackReturn tempReturnBuff;
    tempReturnBuff.doByTheCurrentMonster=true;
    tempReturnBuff.attack=skill;
    const Skill::SkillList &skillList=CommonDatapack::commonDatapack.monsterSkills[skill].level.at(skillLevel-1);
    #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
    emit message(QString("You use skill %1 at level %2").arg(skill).arg(skillLevel));
    #endif
    int index=0;
    while(index<skillList.buff.size())
    {
        const Skill::Buff &buff=skillList.buff.at(index);
        #ifdef CATCHCHALLENGER_SERVER_EXTRA_CHECK
        if(!buffIsValid(buff.effect))
        {
            emit error("Buff is not valid");
            return tempReturnBuff;
        }
        #endif
        bool success;
        if(buff.success==100)
            success=true;
        else
        {
            success=(getOneSeed(100)<buff.success);
            #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
            if(success)
                emit message(QString("Add successfull buff: %1 at level: %2 on %3").arg(buff.effect.buff).arg(buff.effect.level).arg(buff.effect.on));
            #endif
        }
        if(success)
        {
            #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
            if(success)
                emit message(QString("Add buff: %1 at level: %2 on %3").arg(buff.effect.buff).arg(buff.effect.level).arg(buff.effect.on));
            #endif
            tempReturnBuff.success=true;
            tempReturnBuff.buffEffectMonster << buff.effect;
            applyCurrentBuffEffect(buff.effect);
        }
        index++;
    }
    index=0;
    while(index<skillList.life.size())
    {
        tempReturnBuff.success=true;
        const Skill::Life &life=skillList.life.at(index);
        bool success;
        if(life.success==100)
            success=true;
        else
            success=(getOneSeed(100)<life.success);
        if(success)
        {
            Skill::LifeEffectReturn lifeEffectReturn;
            lifeEffectReturn.on=life.effect.on;
            lifeEffectReturn.quantity=applyCurrentLifeEffect(life.effect).quantity;
            tempReturnBuff.lifeEffectMonster << lifeEffectReturn;
        }
        index++;
    }
    return tempReturnBuff;
}

bool LocalClientHandlerFight::isInFight()
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
                        quint32 sp=CommonDatapack::commonDatapack.monsterSkills[learn.learnSkill].level.at(learn.learnSkillLevel).sp;
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
                                emit dbQuery(QString("UPDATE monster SET sp=%1 WHERE id=%2;")
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
                            player_informations->public_and_private_informations.playerMonster[index].skills << temp;
                            switch(GlobalServerData::serverSettings.database.type)
                            {
                                default:
                                case ServerSettings::Database::DatabaseType_Mysql:
                                    emit dbQuery(QString("INSERT INTO monster_skill(monster,skill,level) VALUES(%1,%2,1);")
                                                 .arg(monsterId)
                                                 .arg(skill)
                                                 );
                                break;
                                case ServerSettings::Database::DatabaseType_SQLite:
                                    emit dbQuery(QString("INSERT INTO monster_skill(monster,skill,level) VALUES(%1,%2,1);")
                                                 .arg(monsterId)
                                                 .arg(skill)
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
                                    emit dbQuery(QString("UPDATE monster_skill SET level=%1 WHERE monster=%2 AND skill=%3;")
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

bool LocalClientHandlerFight::getInBattle()
{
    return (otherPlayerBattle!=NULL);
}

void LocalClientHandlerFight::registerBattleRequest(LocalClientHandlerFight * otherPlayerBattle)
{
    if(getInBattle())
    {
        emit message("Already in battle, internal error");
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    emit message(QString("%1 have requested battle with you").arg(otherPlayerBattle->player_informations->public_and_private_informations.public_informations.pseudo));
    #endif
    this->otherPlayerBattle=otherPlayerBattle;
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

void LocalClientHandlerFight::battleFinished()
{
    if(!battleIsValidated)
        return;
    if(otherPlayerBattle==NULL)
        return;
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    emit message("Battle finished");
    #endif
    #ifdef CATCHCHALLENGER_SERVER_EXTRA_CHECK
    otherPlayerBattle->updateCanDoFight();
    if(!otherPlayerBattle->getAbleToFight())
        emit error(QString("Error: The other player monster in battle is KO into battleFinished()"));
    updateCanDoFight();
    if(!getAbleToFight())
        emit error(QString("Error: Your monster is KO into battleFinished()"));
    #endif
    otherPlayerBattle->battleFinishedReset();
    battleFinishedReset();
}

void LocalClientHandlerFight::battleFinishedReset()
{
    otherPlayerBattle=NULL;
    battleIsValidated=false;
    haveCurrentSkill=false;
}

void LocalClientHandlerFight::resetTheBattle()
{
    //reset out of battle
    haveCurrentSkill=false;
    battleIsValidated=false;
    otherPlayerBattle=NULL;
    updateCanDoFight();
}

/*void LocalClientHandlerFight::battleAddBattleMonster(const quint32 &monsterId)
{
    if(!battleIsValidated)
    {
        emit error("Battle not valid");
        return;
    }
    if(battleIsFreezed)
    {
        emit error("Battle is freezed, unable to change something");
        return;
    }
    if(player_informations->public_and_private_informations.playerMonster.size()<=1)
    {
        emit error("Unable to battle your last monster");
        return;
    }
    if(isInFight())
    {
        emit error("You can't battle monster because you are in fight");
        return;
    }

    emit error("Battle monster not found");
}*/

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
    if(battleIsValidated)
        updateCanDoFight();
    otherPlayerBattle=NULL;
    if(send)
    {
            emit sendPacket(0xE0,0x0007);
            emit receiveSystemText(QString("Battle declined"));
    }
    battleIsValidated=false;
    haveCurrentSkill=false;
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
    battleIsValidated=true;
    haveCurrentSkill=false;
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
        emit sendPacket(0xE0,0x0008,otherPlayerBattle->player_informations->rawPseudo+outputData+firstValidOtherPlayerMonster);
    }
}

bool LocalClientHandlerFight::haveBattleSkill()
{
    return haveCurrentSkill;
}

void LocalClientHandlerFight::haveUsedTheBattleSkill()
{
    haveCurrentSkill=false;
}

void LocalClientHandlerFight::useBattleSkill(const quint32 &skill,const quint8 &skillLevel)
{
    if(haveCurrentSkill)
    {
        emit error("Have already the current skill");
        return;
    }
    if(battleIsValidated)
    {
        currentSkill=skill;
        haveCurrentSkill=true;
    }
    #ifdef CATCHCHALLENGER_SERVER_EXTRA_CHECK
    if(otherPlayerBattle==NULL)
    {
        emit error("Internal error: other player is not connected");
        return;
    }
    LocalClientHandlerFight * tempOtherPlayerBattle=otherPlayerBattle;
    #endif
    if(!tempOtherPlayerBattle->haveBattleSkill())
    {
        //in waiting of the other player skill
        return;
    }
    //calculate the result
    QList<Skill::AttackReturn> monsterReturnList;
    Monster::Stat currentMonsterStat=getStat(CommonDatapack::commonDatapack.monsters[getCurrentMonster()->monster],getCurrentMonster()->level);
    Monster::Stat otherMonsterStat=getStat(CommonDatapack::commonDatapack.monsters[getOtherMonster()->monster],getOtherMonster()->level);
    bool currentMonsterStatIsFirstToAttack;
    if(currentMonsterStat.speed==otherMonsterStat.speed)
        currentMonsterStatIsFirstToAttack=rand()%2;
    else
        currentMonsterStatIsFirstToAttack=(currentMonsterStat.speed>=otherMonsterStat.speed);
    //do the current monster attack
    bool isKO=false;
    bool currentMonsterisKO=false,otherMonsterisKO=false;
    bool currentPlayerLoose=false,otherPlayerLoose=false;
    if(currentMonsterStatIsFirstToAttack)
    {
        monsterReturnList << doTheCurrentMonsterAttack(skill,skillLevel,currentMonsterStat,otherMonsterStat);
        monsterReturnList.last().doByTheCurrentMonster=true;
        currentMonsterisKO=checkKOCurrentMonsters();
        if(currentMonsterisKO)
        {
            emit message("current player is KO");
            currentPlayerLoose=checkLoose();
            isKO=true;
        }
        else
        {
            emit message("check other player monster");
            otherMonsterisKO=checkKOOtherMonstersForGain();
            if(otherMonsterisKO)
                isKO=true;
        }
    }
    //do the other monster attack
    if(!isKO)
    {
        monsterReturnList << tempOtherPlayerBattle->doTheCurrentMonsterAttack(skill,skillLevel,currentMonsterStat,otherMonsterStat);
        monsterReturnList.last().doByTheCurrentMonster=false;
        otherMonsterisKO=tempOtherPlayerBattle->checkKOCurrentMonsters();
        if(otherMonsterisKO)
        {
            emit message("middle other player is KO");
            otherPlayerLoose=tempOtherPlayerBattle->checkLoose();
            isKO=true;
        }
        else
        {
            emit message("middle current player is KO");
            currentMonsterisKO=tempOtherPlayerBattle->checkKOOtherMonstersForGain();
            if(currentMonsterisKO)
                isKO=true;
        }
    }
    //do the current monster attack
    if(!isKO)
        if(!currentMonsterStatIsFirstToAttack)
        {
            monsterReturnList << doTheCurrentMonsterAttack(skill,skillLevel,currentMonsterStat,otherMonsterStat);
            monsterReturnList.last().doByTheCurrentMonster=true;
            currentMonsterisKO=checkKOCurrentMonsters();
            if(currentMonsterisKO)
            {
                emit message("current player is KO");
                currentPlayerLoose=checkLoose();
                isKO=true;
            }
            else
            {
                emit message("check other player monster");
                otherMonsterisKO=checkKOOtherMonstersForGain();
                if(otherMonsterisKO)
                    isKO=true;
            }
        }
    syncForEndOfTurn();
    //send to the return
    if(otherMonsterisKO && !otherPlayerLoose)
        sendBattleReturn(monsterReturnList,getOtherSelectedMonsterNumber(),*getOtherMonster());
    else
        sendBattleReturn(monsterReturnList);
    int index=0;
    while(index<monsterReturnList.size())
    {
        monsterReturnList[index].doByTheCurrentMonster=!monsterReturnList[index].doByTheCurrentMonster;
        index++;
    }
    if(currentMonsterisKO && !currentPlayerLoose)
        tempOtherPlayerBattle->sendBattleReturn(monsterReturnList,selectedMonsterNumberToMonsterPlace(getCurrentSelectedMonsterNumber()),FacilityLib::playerMonsterToPublicPlayerMonster(*getCurrentMonster()));
    else
        tempOtherPlayerBattle->sendBattleReturn(monsterReturnList);
    //reset all
    haveUsedTheBattleSkill();
    tempOtherPlayerBattle->haveUsedTheBattleSkill();
    //if the battle is finish
    if(!currentPlayerLoose || !otherPlayerLoose)
        battleFinished();
}

quint8 LocalClientHandlerFight::selectedMonsterNumberToMonsterPlace(const quint8 &selectedMonsterNumber)
{
    return selectedMonsterNumber+1;
}

void LocalClientHandlerFight::sendBattleReturn(const QList<Skill::AttackReturn> &attackReturn, const quint8 &monsterPlace, const PublicPlayerMonster &publicPlayerMonster)
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
        index=0;
        out << (quint8)attackReturnTemp.buffEffectMonster.size();
        while(index<attackReturnTemp.buffEffectMonster.size())
        {
            out << (quint32)attackReturnTemp.buffEffectMonster.at(index).buff;
            out << (quint8)attackReturnTemp.buffEffectMonster.at(index).on;
            out << (quint8)attackReturnTemp.buffEffectMonster.at(index).level;
            index++;
        }
        index=0;
        out << (quint8)attackReturnTemp.lifeEffectMonster.size();
        while(index<attackReturnTemp.lifeEffectMonster.size())
        {
            out << (qint32)attackReturnTemp.lifeEffectMonster.at(index).quantity;
            out << (quint8)attackReturnTemp.lifeEffectMonster.at(index).on;
            index++;
        }
        master_index++;
    }
    out << (quint8)monsterPlace;
    if(monsterPlace!=0x00)
        binarypublicPlayerMonster=FacilityLib::publicPlayerMonsterToBinary(publicPlayerMonster);

    emit sendPacket(0xE0,0x0006,outputData+binarypublicPlayerMonster);
}

