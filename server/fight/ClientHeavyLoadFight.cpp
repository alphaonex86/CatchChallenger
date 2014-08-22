#include "../base/Client.h"
#include "../base/GlobalServerData.h"

#include "../../general/base/GeneralVariable.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/fight/CommonFightEngine.h"
#include "../../general/base/FacilityLib.h"

using namespace CatchChallenger;

void Client::loadMonsters()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(GlobalServerData::serverPrivateVariables.db_query_select_monsters_by_player_id.isEmpty())
    {
        errorOutput(QStringLiteral("loadMonsters() Query is empty, bug"));
        return;
    }
    #endif
    const QString &queryText=GlobalServerData::serverPrivateVariables.db_query_select_monsters_by_player_id.arg(character_id);
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db.asyncRead(queryText.toLatin1(),this,&Client::loadMonsters_static);
    if(callback==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db.errorMessage());
        loadReputation();
        return;
    }
    else
        callbackRegistred << callback;
}

void Client::loadMonsters_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->loadMonsters_return();
}

void Client::loadMonsters_return()
{
    callbackRegistred.removeFirst();
    bool ok;
    Monster monster;
    while(GlobalServerData::serverPrivateVariables.db.next())
    {
        monster.give_sp=88889;
        PlayerMonster playerMonster;
        playerMonster.id=QString(GlobalServerData::serverPrivateVariables.db.value(0)).toUInt(&ok);
        if(!ok)
            normalOutput(QStringLiteral("monsterId: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db.value(0)));
        if(ok)
        {
            playerMonster.monster=QString(GlobalServerData::serverPrivateVariables.db.value(2)).toUInt(&ok);
            if(ok)
            {
                if(!CommonDatapack::commonDatapack.monsters.contains(playerMonster.monster))
                {
                    ok=false;
                    normalOutput(QStringLiteral("monster: %1 is not into monster list").arg(playerMonster.monster));
                }
                else
                    monster=CommonDatapack::commonDatapack.monsters.value(playerMonster.monster);
            }
            else
                normalOutput(QStringLiteral("monster: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db.value(2)));
        }
        if(ok)
        {
            playerMonster.level=QString(GlobalServerData::serverPrivateVariables.db.value(3)).toUInt(&ok);
            if(ok)
            {
                if(playerMonster.level>CATCHCHALLENGER_MONSTER_LEVEL_MAX)
                {
                    normalOutput(QStringLiteral("level: %1 greater than %2, truncated").arg(playerMonster.level).arg(CATCHCHALLENGER_MONSTER_LEVEL_MAX));
                    playerMonster.level=CATCHCHALLENGER_MONSTER_LEVEL_MAX;
                }
            }
            else
                normalOutput(QStringLiteral("level: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db.value(3)));
        }
        if(ok)
        {
            playerMonster.remaining_xp=QString(GlobalServerData::serverPrivateVariables.db.value(4)).toUInt(&ok);
            if(ok)
            {
                if(playerMonster.level>monster.level_to_xp.size())
                {
                    normalOutput(QStringLiteral("monster level: %1 greater than loaded level %2").arg(playerMonster.level).arg(monster.level_to_xp.size()));
                    ok=false;
                }
                else if(playerMonster.remaining_xp>monster.level_to_xp.at(playerMonster.level-1))
                {
                    normalOutput(QStringLiteral("monster xp: %1 greater than %2, truncated").arg(playerMonster.remaining_xp).arg(monster.level_to_xp.at(playerMonster.level-1)));
                    playerMonster.remaining_xp=0;
                }
            }
            else
                normalOutput(QStringLiteral("monster xp: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db.value(4)));
        }
        if(ok)
        {
            if(CommonSettings::commonSettings.useSP)
            {
                playerMonster.sp=QString(GlobalServerData::serverPrivateVariables.db.value(5)).toUInt(&ok);
                if(!ok)
                    normalOutput(QStringLiteral("monster sp: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db.value(5)));
            }
            else
                playerMonster.sp=0;
        }
        int sp_offset;
        if(CommonSettings::commonSettings.useSP)
            sp_offset=0;
        else
            sp_offset=-1;
        if(ok)
        {
            playerMonster.catched_with=QString(GlobalServerData::serverPrivateVariables.db.value(6+sp_offset)).toUInt(&ok);
            if(ok)
            {
                if(!CommonDatapack::commonDatapack.items.item.contains(playerMonster.catched_with))
                    normalOutput(QStringLiteral("captured_with: %1 is not is not into items list").arg(playerMonster.catched_with));
            }
            else
                normalOutput(QStringLiteral("captured_with: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db.value(6+sp_offset)));
        }
        if(ok)
        {
            const quint32 &genderInt=QString(GlobalServerData::serverPrivateVariables.db.value(7+sp_offset)).toUInt(&ok);
            if(ok)
            {
                if(genderInt>=1 && genderInt<=3)
                    playerMonster.gender=static_cast<Gender>(genderInt);
                else
                {
                    playerMonster.gender=Gender_Unknown;
                    normalOutput(QStringLiteral("unknown monster gender, out of range: %1").arg(GlobalServerData::serverPrivateVariables.db.value(7+sp_offset)));
                    ok=false;
                }
            }
            else
            {
                playerMonster.gender=Gender_Unknown;
                normalOutput(QStringLiteral("unknown monster gender: %1").arg(GlobalServerData::serverPrivateVariables.db.value(7+sp_offset)));
                ok=false;
            }
        }
        if(ok)
        {
            playerMonster.egg_step=QString(GlobalServerData::serverPrivateVariables.db.value(8+sp_offset)).toUInt(&ok);
            if(!ok)
                normalOutput(QStringLiteral("monster egg_step: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db.value(8+sp_offset)));
        }
        if(ok)
        {
            playerMonster.character_origin=QString(GlobalServerData::serverPrivateVariables.db.value(9+sp_offset)).toUInt(&ok);
            if(!ok)
                normalOutput(QStringLiteral("monster character_origin: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db.value(9+sp_offset)));
        }
        //stats
        if(ok)
        {
            playerMonster.hp=QString(GlobalServerData::serverPrivateVariables.db.value(1)).toUInt(&ok);
            if(ok)
            {
                const Monster::Stat &stat=CommonFightEngine::getStat(monster,playerMonster.level);
                if(playerMonster.hp>stat.hp)
                {
                    normalOutput(QStringLiteral("monster hp: %1 greater than max hp %2 for the level %3 of the monster %4, truncated")
                                 .arg(playerMonster.hp)
                                 .arg(stat.hp)
                                 .arg(playerMonster.level)
                                 .arg(playerMonster.monster)
                                 );
                    playerMonster.hp=stat.hp;
                }
            }
            else
                normalOutput(QStringLiteral("monster hp: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db.value(1)));
        }
        //finish it
        if(ok)
            public_and_private_informations.playerMonster << playerMonster;
    }
    loadMonstersWarehouse();
}

void Client::loadMonstersWarehouse()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(GlobalServerData::serverPrivateVariables.db_query_select_monsters_warehouse_by_player_id.isEmpty())
    {
        errorOutput(QStringLiteral("loadMonsters() Query is empty, bug"));
        return;
    }
    #endif
    const QString &queryText=GlobalServerData::serverPrivateVariables.db_query_select_monsters_warehouse_by_player_id.arg(character_id);
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db.asyncRead(queryText.toLatin1(),this,&Client::loadMonstersWarehouse_static);
    if(callback==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db.errorMessage());
        loadReputation();
        return;
    }
    else
        callbackRegistred << callback;
}

void Client::loadMonstersWarehouse_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->loadMonstersWarehouse_return();
}

void Client::loadMonstersWarehouse_return()
{
    callbackRegistred.removeFirst();
    bool ok;
    Monster monster;
    while(GlobalServerData::serverPrivateVariables.db.next())
    {
        monster.give_sp=88889;
        PlayerMonster playerMonster;
        playerMonster.id=QString(GlobalServerData::serverPrivateVariables.db.value(0)).toUInt(&ok);
        if(!ok)
            normalOutput(QStringLiteral("monsterId: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db.value(0)));
        if(ok)
        {
            playerMonster.monster=QString(GlobalServerData::serverPrivateVariables.db.value(2)).toUInt(&ok);
            if(ok)
            {
                if(!CommonDatapack::commonDatapack.monsters.contains(playerMonster.monster))
                {
                    ok=false;
                    normalOutput(QStringLiteral("monster: %1 is not into monster list").arg(playerMonster.monster));
                }
                else
                    monster=CommonDatapack::commonDatapack.monsters.value(playerMonster.monster);
            }
            else
                normalOutput(QStringLiteral("monster: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db.value(2)));
        }
        if(ok)
        {
            playerMonster.level=QString(GlobalServerData::serverPrivateVariables.db.value(3)).toUInt(&ok);
            if(ok)
            {
                if(playerMonster.level>CATCHCHALLENGER_MONSTER_LEVEL_MAX)
                {
                    normalOutput(QStringLiteral("level: %1 greater than %2, truncated").arg(playerMonster.level).arg(CATCHCHALLENGER_MONSTER_LEVEL_MAX));
                    playerMonster.level=CATCHCHALLENGER_MONSTER_LEVEL_MAX;
                }
            }
            else
                normalOutput(QStringLiteral("level: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db.value(3)));
        }
        if(ok)
        {
            playerMonster.remaining_xp=QString(GlobalServerData::serverPrivateVariables.db.value(4)).toUInt(&ok);
            if(ok)
            {
                if(playerMonster.level>monster.level_to_xp.size())
                {
                    normalOutput(QStringLiteral("monster level: %1 greater than loaded level %2").arg(playerMonster.level).arg(monster.level_to_xp.size()));
                    ok=false;
                }
                else if(playerMonster.remaining_xp>monster.level_to_xp.at(playerMonster.level-1))
                {
                    normalOutput(QStringLiteral("monster xp: %1 greater than %2, truncated").arg(playerMonster.remaining_xp).arg(monster.level_to_xp.at(playerMonster.level-1)));
                    playerMonster.remaining_xp=0;
                }
            }
            else
                normalOutput(QStringLiteral("monster xp: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db.value(4)));
        }
        if(ok)
        {
            if(CommonSettings::commonSettings.useSP)
            {
                playerMonster.sp=QString(GlobalServerData::serverPrivateVariables.db.value(5)).toUInt(&ok);
                if(!ok)
                    normalOutput(QStringLiteral("monster sp: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db.value(5)));
            }
            else
                playerMonster.sp=0;
        }
        int sp_offset;
        if(CommonSettings::commonSettings.useSP)
            sp_offset=0;
        else
            sp_offset=-1;
        if(ok)
        {
            playerMonster.catched_with=QString(GlobalServerData::serverPrivateVariables.db.value(6+sp_offset)).toUInt(&ok);
            if(ok)
            {
                if(!CommonDatapack::commonDatapack.items.item.contains(playerMonster.catched_with))
                    normalOutput(QStringLiteral("captured_with: %1 is not is not into items list").arg(playerMonster.catched_with));
            }
            else
                normalOutput(QStringLiteral("captured_with: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db.value(6+sp_offset)));
        }
        if(ok)
        {
            const quint32 &genderInt=QString(GlobalServerData::serverPrivateVariables.db.value(7+sp_offset)).toUInt(&ok);
            if(ok)
            {
                if(genderInt>=1 && genderInt<=3)
                    playerMonster.gender=static_cast<Gender>(genderInt);
                else
                {
                    playerMonster.gender=Gender_Unknown;
                    normalOutput(QStringLiteral("unknown monster gender, out of range: %1").arg(GlobalServerData::serverPrivateVariables.db.value(7+sp_offset)));
                    ok=false;
                }
            }
            else
            {
                playerMonster.gender=Gender_Unknown;
                normalOutput(QStringLiteral("unknown monster gender: %1").arg(GlobalServerData::serverPrivateVariables.db.value(7+sp_offset)));
                ok=false;
            }
        }
        if(ok)
        {
            playerMonster.egg_step=QString(GlobalServerData::serverPrivateVariables.db.value(8+sp_offset)).toUInt(&ok);
            if(!ok)
                normalOutput(QStringLiteral("monster egg_step: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db.value(8+sp_offset)));
        }
        if(ok)
        {
            playerMonster.character_origin=QString(GlobalServerData::serverPrivateVariables.db.value(9+sp_offset)).toUInt(&ok);
            if(!ok)
                normalOutput(QStringLiteral("monster character_origin: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db.value(9+sp_offset)));
        }
        //stats
        if(ok)
        {
            playerMonster.hp=QString(GlobalServerData::serverPrivateVariables.db.value(1)).toUInt(&ok);
            if(ok)
            {
                const Monster::Stat &stat=CommonFightEngine::getStat(monster,playerMonster.level);
                if(playerMonster.hp>stat.hp)
                {
                    normalOutput(QStringLiteral("monster hp: %1 greater than max hp %2 for the level %3 of the monster %4, truncated")
                                 .arg(playerMonster.hp)
                                 .arg(stat.hp)
                                 .arg(playerMonster.level)
                                 .arg(playerMonster.monster)
                                 );
                    playerMonster.hp=stat.hp;
                }
            }
            else
                normalOutput(QStringLiteral("monster hp: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db.value(1)));
        }
        //finish it
        if(ok)
            public_and_private_informations.warehouse_playerMonster << playerMonster;
    }
    if(!public_and_private_informations.playerMonster.isEmpty())
        loadPlayerMonsterBuffs(0);
    else if(!public_and_private_informations.warehouse_playerMonster.isEmpty())
        loadPlayerMonsterBuffs(0);
    else
        loadReputation();
}

void Client::loadPlayerMonsterBuffs(const quint32 &index)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(GlobalServerData::serverPrivateVariables.db_query_select_monstersBuff_by_id.isEmpty())
    {
        errorOutput(QStringLiteral("loadPlayerMonsterBuffs() Query is empty, bug"));
        loadPlayerMonsterSkills(0);
        return;
    }
    #endif
    QString queryText;
    if(index<(quint32)public_and_private_informations.playerMonster.size())
        queryText=GlobalServerData::serverPrivateVariables.db_query_select_monstersBuff_by_id.arg(public_and_private_informations.playerMonster.at(index).id);
    else if(index<(quint32)(public_and_private_informations.playerMonster.size()+public_and_private_informations.warehouse_playerMonster.size()))
        queryText=GlobalServerData::serverPrivateVariables.db_query_select_monstersBuff_by_id.arg(public_and_private_informations.playerMonster.at(index-public_and_private_informations.playerMonster.size()).id);
    else
    {
        loadPlayerMonsterSkills(0);
        return;
    }
    if(!queryText.isEmpty())
    {
        SelectIndexParam *selectIndexParam=new SelectIndexParam;
        selectIndexParam->index=index;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(index>(quint32)(public_and_private_informations.playerMonster.size()+public_and_private_informations.warehouse_playerMonster.size()))
            abort();
        #endif

        CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db.asyncRead(queryText.toLatin1(),this,&Client::loadPlayerMonsterBuffs_static);
        if(callback==NULL)
        {
            qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db.errorMessage());
            delete selectIndexParam;
            loadPlayerMonsterSkills(0);
            return;
        }
        else
        {
            callbackRegistred << callback;
            paramToPassToCallBack << selectIndexParam;
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            paramToPassToCallBackType << QStringLiteral("SelectIndexParam");
            #endif
        }
    }
}

void Client::loadPlayerMonsterBuffs_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->loadPlayerMonsterBuffs_object();
}

void Client::loadPlayerMonsterBuffs_object()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBack.isEmpty())
    {
        qDebug() << "paramToPassToCallBack.isEmpty()" << __FILE__ << __LINE__;
        abort();
    }
    #endif
    SelectIndexParam *selectIndexParam=static_cast<SelectIndexParam *>(paramToPassToCallBack.takeFirst());
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(selectIndexParam==NULL)
        abort();
    #endif
    loadPlayerMonsterBuffs_return(selectIndexParam->index);
    delete selectIndexParam;
}

void Client::loadPlayerMonsterBuffs_return(const quint32 &index)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBackType.takeFirst()!=QStringLiteral("SelectIndexParam"))
    {
        qDebug() << "is not SelectIndexParam" << paramToPassToCallBackType.join(";") << __FILE__ << __LINE__;
        abort();
    }
    #endif
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(index>(quint32)(public_and_private_informations.playerMonster.size()+public_and_private_informations.warehouse_playerMonster.size()))
        abort();
    #endif
    callbackRegistred.removeFirst();
    PlayerMonster *playerMonster;
    if(index<(quint32)public_and_private_informations.playerMonster.size())
        playerMonster=&public_and_private_informations.playerMonster[index];
    else
        playerMonster=&public_and_private_informations.warehouse_playerMonster[index-public_and_private_informations.playerMonster.size()];
    const quint32 &monsterId=playerMonster->id;
    bool ok;
    while(GlobalServerData::serverPrivateVariables.db.next())
    {
        PlayerBuff buff;
        buff.buff=QString(GlobalServerData::serverPrivateVariables.db.value(0)).toUInt(&ok);
        if(ok)
        {
            if(!CommonDatapack::commonDatapack.monsterBuffs.contains(buff.buff))
            {
                ok=false;
                normalOutput(QStringLiteral("buff %1 for monsterId: %2 is not found into buff list").arg(buff.buff).arg(monsterId));
            }
        }
        else
            normalOutput(QStringLiteral("buff id: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db.value(0)));
        if(ok)
        {
            buff.level=QString(GlobalServerData::serverPrivateVariables.db.value(1)).toUInt(&ok);
            if(ok)
            {
                if(buff.level>CommonDatapack::commonDatapack.monsterBuffs.value(buff.buff).level.size())
                {
                    ok=false;
                    normalOutput(QStringLiteral("buff %1 for monsterId: %2 have not the level: %3").arg(buff.buff).arg(monsterId).arg(buff.level));
                }
            }
            else
                normalOutput(QStringLiteral("buff level: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db.value(2)));
        }
        if(ok)
        {
            if(CommonDatapack::commonDatapack.monsterBuffs.value(buff.buff).level.at(buff.level-1).duration!=Buff::Duration_Always)
            {
                ok=false;
                DebugClass::debugConsole(QStringLiteral("buff %1 for monsterId: %2 can't be loaded from the db if is not permanent").arg(buff.buff).arg(monsterId));
            }
        }
        if(ok)
            playerMonster->buffs << buff;
    }
    loadPlayerMonsterBuffs(index+1);
}

void Client::loadPlayerMonsterSkills(const quint32 &index)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(GlobalServerData::serverPrivateVariables.db_query_select_monstersSkill_by_id.isEmpty())
    {
        errorOutput(QStringLiteral("loadMonsterSkills() Query is empty, bug"));
        loadReputation();
        return;
    }
    #endif
    QString queryText;
    if(index<(quint32)public_and_private_informations.playerMonster.size())
        queryText=GlobalServerData::serverPrivateVariables.db_query_select_monstersSkill_by_id.arg(public_and_private_informations.playerMonster.at(index).id);
    else if(index<(quint32)(public_and_private_informations.playerMonster.size()+public_and_private_informations.warehouse_playerMonster.size()))
        queryText=GlobalServerData::serverPrivateVariables.db_query_select_monstersSkill_by_id.arg(public_and_private_informations.playerMonster.at(index-public_and_private_informations.playerMonster.size()).id);
    else
    {
        loadReputation();
        return;
    }
    if(!queryText.isEmpty())
    {
        SelectIndexParam *selectIndexParam=new SelectIndexParam;
        selectIndexParam->index=index;
        CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db.asyncRead(queryText.toLatin1(),this,&Client::loadPlayerMonsterSkills_static);
        if(callback==NULL)
        {
            qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db.errorMessage());
            delete selectIndexParam;
            loadReputation();
            return;
        }
        else
        {
            callbackRegistred << callback;
            paramToPassToCallBack << selectIndexParam;
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            paramToPassToCallBackType << QStringLiteral("SelectIndexParam");
            #endif
        }
    }
}

void Client::loadPlayerMonsterSkills_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->loadPlayerMonsterSkills_object();
}

void Client::loadPlayerMonsterSkills_object()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBack.isEmpty())
    {
        qDebug() << "paramToPassToCallBack.isEmpty()" << __FILE__ << __LINE__;
        abort();
    }
    #endif
    SelectIndexParam *selectIndexParam=static_cast<SelectIndexParam *>(paramToPassToCallBack.takeFirst());
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(selectIndexParam==NULL)
        abort();
    #endif
    loadPlayerMonsterSkills_return(selectIndexParam->index);
    delete selectIndexParam;
}

void Client::loadPlayerMonsterSkills_return(const quint32 &index)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBackType.takeFirst()!=QStringLiteral("SelectIndexParam"))
    {
        qDebug() << "is not SelectIndexParam" << paramToPassToCallBackType.join(";") << __FILE__ << __LINE__;
        abort();
    }
    #endif
    callbackRegistred.removeFirst();
    PlayerMonster *playerMonster;
    if(index<(quint32)public_and_private_informations.playerMonster.size())
        playerMonster=&public_and_private_informations.playerMonster[index];
    else
        playerMonster=&public_and_private_informations.warehouse_playerMonster[index-public_and_private_informations.playerMonster.size()];
    bool ok;
    while(GlobalServerData::serverPrivateVariables.db.next())
    {
        PlayerMonster::PlayerSkill skill;
        skill.skill=QString(GlobalServerData::serverPrivateVariables.db.value(0)).toUInt(&ok);
        if(ok)
        {
            if(!CommonDatapack::commonDatapack.monsterSkills.contains(skill.skill))
            {
                ok=false;
                normalOutput(QStringLiteral("skill %1 for monsterId: %2 is not found into skill list").arg(skill.skill).arg(playerMonster->id));
            }
        }
        else
            normalOutput(QStringLiteral("skill id: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db.value(0)));
        if(ok)
        {
            skill.level=QString(GlobalServerData::serverPrivateVariables.db.value(1)).toUInt(&ok);
            if(ok)
            {
                if(skill.level>CommonDatapack::commonDatapack.monsterSkills.value(skill.skill).level.size())
                {
                    ok=false;
                    normalOutput(QStringLiteral("skill %1 for monsterId: %2 have not the level: %3").arg(skill.skill).arg(playerMonster->id).arg(skill.level));
                }
            }
            else
                normalOutput(QStringLiteral("skill level: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db.value(1)));
        }
        if(ok)
        {
            skill.endurance=QString(GlobalServerData::serverPrivateVariables.db.value(2)).toUInt(&ok);
            if(ok)
            {
                if(skill.endurance>CommonDatapack::commonDatapack.monsterSkills.value(skill.skill).level.at(skill.level-1).endurance)
                {
                    skill.endurance=CommonDatapack::commonDatapack.monsterSkills.value(skill.skill).level.at(skill.level-1).endurance;
                    normalOutput(QStringLiteral("skill %1 for monsterId: %2 have too hight endurance, lowered to: %3").arg(skill.skill).arg(playerMonster->id).arg(skill.endurance));
                }
            }
            else
                normalOutput(QStringLiteral("skill endurance: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db.value(2)));
        }
        if(ok)
            playerMonster->skills << skill;
    }
    loadPlayerMonsterSkills(index+1);
}

void Client::generateRandomNumber()
{
    if((randomIndex+randomSize)<CATCHCHALLENGER_SERVER_RANDOM_INTERNAL_SIZE)
    {
        //can send the next block
        sendFullPacket(0xE0,0x0009,GlobalServerData::serverPrivateVariables.randomData.mid(randomIndex+randomSize,CATCHCHALLENGER_SERVER_RANDOM_LIST_SIZE));
    }
    else
    {
        //need return to the first block
        sendFullPacket(0xE0,0x0009,GlobalServerData::serverPrivateVariables.randomData.mid(0,CATCHCHALLENGER_SERVER_RANDOM_LIST_SIZE));
    }
    randomSize+=CATCHCHALLENGER_SERVER_RANDOM_INTERNAL_SIZE;
}

quint32 Client::randomSeedsSize() const
{
    return randomSize;
}

quint8 Client::getOneSeed(const quint8 &max)
{
    const quint8 &number=GlobalServerData::serverPrivateVariables.randomData.at(randomIndex);
    randomIndex++;
    randomSize--;
    if(randomSize<CATCHCHALLENGER_SERVER_MIN_RANDOM_LIST_SIZE)
        generateRandomNumber();
    return number%(max+1);
}

void Client::loadBotAlreadyBeaten()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(GlobalServerData::serverPrivateVariables.db_query_select_bot_beaten.isEmpty())
    {
        errorOutput(QStringLiteral("loadBotAlreadyBeaten() Query is empty, bug"));
        return;
    }
    #endif
    const QString &queryText=GlobalServerData::serverPrivateVariables.db_query_select_bot_beaten.arg(character_id);
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db.asyncRead(queryText.toLatin1(),this,&Client::loadBotAlreadyBeaten_static);
    if(callback==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db.errorMessage());
        loadItemOnMap();
        return;
    }
    else
        callbackRegistred << callback;
}

void Client::loadBotAlreadyBeaten_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->loadBotAlreadyBeaten_return();
}

void Client::loadBotAlreadyBeaten_return()
{
    callbackRegistred.removeFirst();
    bool ok;
    //parse the result
    while(GlobalServerData::serverPrivateVariables.db.next())
    {
        const quint32 &id=QString(GlobalServerData::serverPrivateVariables.db.value(0)).toUInt(&ok);
        if(!ok)
        {
            normalOutput(QStringLiteral("wrong value type for quest, skip: %1").arg(id));
            continue;
        }
        if(!CommonDatapack::commonDatapack.botFights.contains(id))
        {
            normalOutput(QStringLiteral("fights is not into the fights list, skip: %1").arg(id));
            continue;
        }
        public_and_private_informations.bot_already_beaten << id;
    }
    loadItemOnMap();
}
