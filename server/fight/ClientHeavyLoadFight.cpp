#include "../base/ClientHeavyLoad.h"
#include "../base/GlobalServerData.h"

#include "../../general/base/GeneralVariable.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/fight/CommonFightEngine.h"
#include "../../general/base/FacilityLib.h"

using namespace CatchChallenger;

void ClientHeavyLoad::loadMonsters()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(GlobalServerData::serverPrivateVariables.db_query_select_monsters_by_player_id.isEmpty())
    {
        /*emit */error(QStringLiteral("loadMonsters() Query is empty, bug"));
        return;
    }
    if(GlobalServerData::serverPrivateVariables.db_query_update_monster_place_wearhouse.isEmpty())
    {
        /*emit */error(QStringLiteral("loadMonsters() Query monster place is empty, bug"));
        return;
    }
    #endif

    bool warehouse;
    bool ok;
    Monster monster;
    QSqlQuery monstersQuery(*GlobalServerData::serverPrivateVariables.db);
    if(!monstersQuery.exec(GlobalServerData::serverPrivateVariables.db_query_select_monsters_by_player_id.arg(player_informations->character_id)))
        /*emit */message(monstersQuery.lastQuery()+": "+monstersQuery.lastError().text());
    while(monstersQuery.next())
    {
        monster.give_sp=88889;
        PlayerMonster playerMonster;
        playerMonster.id=monstersQuery.value(0).toUInt(&ok);
        if(!ok)
            /*emit */message(QStringLiteral("monsterId: %1 is not a number").arg(monstersQuery.value(0).toString()));
        if(ok)
        {
            playerMonster.monster=monstersQuery.value(2).toUInt(&ok);
            if(ok)
            {
                if(!CommonDatapack::commonDatapack.monsters.contains(playerMonster.monster))
                {
                    ok=false;
                    /*emit */message(QStringLiteral("monster: %1 is not into monster list").arg(playerMonster.monster));
                }
                else
                    monster=CommonDatapack::commonDatapack.monsters.value(playerMonster.monster);
            }
            else
                /*emit */message(QStringLiteral("monster: %1 is not a number").arg(monstersQuery.value(2).toString()));
        }
        if(ok)
        {
            playerMonster.level=monstersQuery.value(3).toUInt(&ok);
            if(ok)
            {
                if(playerMonster.level>CATCHCHALLENGER_MONSTER_LEVEL_MAX)
                {
                    /*emit */message(QStringLiteral("level: %1 greater than %2, truncated").arg(playerMonster.level).arg(CATCHCHALLENGER_MONSTER_LEVEL_MAX));
                    playerMonster.level=CATCHCHALLENGER_MONSTER_LEVEL_MAX;
                }
            }
            else
                /*emit */message(QStringLiteral("level: %1 is not a number").arg(monstersQuery.value(3).toString()));
        }
        if(ok)
        {
            playerMonster.remaining_xp=monstersQuery.value(4).toUInt(&ok);
            if(ok)
            {
                if(playerMonster.level>=monster.level_to_xp.size())
                {
                    /*emit */message(QStringLiteral("monster level: %1 greater than loaded level %2").arg(playerMonster.level).arg(monster.level_to_xp.size()));
                    ok=false;
                }
                else if(playerMonster.remaining_xp>monster.level_to_xp.at(playerMonster.level-1))
                {
                    /*emit */message(QStringLiteral("monster xp: %1 greater than %2, truncated").arg(playerMonster.remaining_xp).arg(monster.level_to_xp.at(playerMonster.level-1)));
                    playerMonster.remaining_xp=0;
                }
            }
            else
                /*emit */message(QStringLiteral("monster xp: %1 is not a number").arg(monstersQuery.value(4).toString()));
        }
        if(ok)
        {
            playerMonster.sp=monstersQuery.value(5).toUInt(&ok);
            if(!ok)
                /*emit */message(QStringLiteral("monster sp: %1 is not a number").arg(monstersQuery.value(5).toString()));
        }
        if(ok)
        {
            playerMonster.catched_with=monstersQuery.value(6).toUInt(&ok);
            if(ok)
            {
                if(!CommonDatapack::commonDatapack.items.item.contains(playerMonster.catched_with))
                    /*emit */message(QStringLiteral("captured_with: %1 is not is not into items list").arg(playerMonster.catched_with));
            }
            else
                /*emit */message(QStringLiteral("captured_with: %1 is not a number").arg(monstersQuery.value(6).toString()));
        }
        if(ok)
        {
            if(monstersQuery.value(7).toString()==ClientHeavyLoad::text_male)
                playerMonster.gender=Gender_Male;
            else if(monstersQuery.value(7).toString()==ClientHeavyLoad::text_female)
                playerMonster.gender=Gender_Female;
            else if(monstersQuery.value(7).toString()==ClientHeavyLoad::text_unknown)
                playerMonster.gender=Gender_Unknown;
            else
            {
                playerMonster.gender=Gender_Unknown;
                /*emit */message(QStringLiteral("unknown monster gender: %1").arg(monstersQuery.value(7).toString()));
                ok=false;
            }
        }
        if(ok)
        {
            playerMonster.egg_step=monstersQuery.value(8).toUInt(&ok);
            if(!ok)
                /*emit */message(QStringLiteral("monster egg_step: %1 is not a number").arg(monstersQuery.value(8).toString()));
        }
        if(ok)
        {
            if(monstersQuery.value(9).toString()==ClientHeavyLoad::text_warehouse)
                warehouse=true;
            else
            {
                if(monstersQuery.value(9).toString()==ClientHeavyLoad::text_wear)
                    warehouse=false;
                else if(monstersQuery.value(9).toString()==ClientHeavyLoad::text_market)
                    continue;
                else
                {
                    /*emit */message(QStringLiteral("unknow wear type: %1 for monster %2").arg(monstersQuery.value(9).toString()).arg(playerMonster.id));
                    continue;
                }
            }
        }
        if(!warehouse && player_informations->public_and_private_informations.playerMonster.size()>=CATCHCHALLENGER_MONSTER_MAX_WEAR_ON_PLAYER)
        {
            warehouse=true;
            dbQuery(GlobalServerData::serverPrivateVariables.db_query_update_monster_place_wearhouse.arg(playerMonster.id));
        }
        //stats
        if(ok)
        {
            playerMonster.hp=monstersQuery.value(1).toUInt(&ok);
            if(ok)
            {
                const Monster::Stat &stat=CommonFightEngine::getStat(monster,playerMonster.level);
                if(playerMonster.hp>stat.hp)
                {
                    /*emit */message(QStringLiteral("monster hp: %1 greater than max hp %2 for the level %3 of the monster %4, truncated")
                                 .arg(playerMonster.hp)
                                 .arg(stat.hp)
                                 .arg(playerMonster.level)
                                 .arg(playerMonster.monster)
                                 );
                    playerMonster.hp=stat.hp;
                }
            }
            else
                /*emit */message(QStringLiteral("monster hp: %1 is not a number").arg(monstersQuery.value(1).toString()));
        }
        //finish it
        if(ok)
        {
            playerMonster.buffs=loadMonsterBuffs(playerMonster.id);
            playerMonster.skills=loadMonsterSkills(playerMonster.id);
            if(!warehouse)
                player_informations->public_and_private_informations.playerMonster << playerMonster;
            else
                player_informations->public_and_private_informations.warehouse_playerMonster << playerMonster;
        }
    }
}

QList<PlayerBuff> ClientHeavyLoad::loadMonsterBuffs(const quint32 &monsterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(GlobalServerData::serverPrivateVariables.db_query_select_monstersBuff_by_id.isEmpty())
    {
        /*emit */error(QStringLiteral("loadMonsterBuffs() Query is empty, bug"));
        return QList<PlayerBuff>();
    }
    #endif
    QList<PlayerBuff> buffs;

    bool ok;
    QSqlQuery monsterBuffsQuery(*GlobalServerData::serverPrivateVariables.db);
    if(!monsterBuffsQuery.exec(GlobalServerData::serverPrivateVariables.db_query_select_monstersBuff_by_id.arg(monsterId)))
        /*emit */message(monsterBuffsQuery.lastQuery()+": "+monsterBuffsQuery.lastError().text());
    while(monsterBuffsQuery.next())
    {
        PlayerBuff buff;
        buff.buff=monsterBuffsQuery.value(0).toUInt(&ok);
        if(ok)
        {
            if(!CommonDatapack::commonDatapack.monsterBuffs.contains(buff.buff))
            {
                ok=false;
                /*emit */message(QStringLiteral("buff %1 for monsterId: %2 is not found into buff list").arg(buff.buff).arg(monsterId));
            }
        }
        else
            /*emit */message(QStringLiteral("buff id: %1 is not a number").arg(monsterBuffsQuery.value(0).toString()));
        if(ok)
        {
            buff.level=monsterBuffsQuery.value(1).toUInt(&ok);
            if(ok)
            {
                if(buff.level>CommonDatapack::commonDatapack.monsterBuffs.value(buff.buff).level.size())
                {
                    ok=false;
                    /*emit */message(QStringLiteral("buff %1 for monsterId: %2 have not the level: %3").arg(buff.buff).arg(monsterId).arg(buff.level));
                }
            }
            else
                /*emit */message(QStringLiteral("buff level: %1 is not a number").arg(monsterBuffsQuery.value(2).toString()));
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
            buffs << buff;
    }
    return buffs;
}

QList<PlayerMonster::PlayerSkill> ClientHeavyLoad::loadMonsterSkills(const quint32 &monsterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(GlobalServerData::serverPrivateVariables.db_query_select_monstersSkill_by_id.isEmpty())
    {
        /*emit */error(QStringLiteral("loadMonsterSkills() Query is empty, bug"));
        return QList<PlayerMonster::PlayerSkill>();
    }
    #endif
    QList<PlayerMonster::PlayerSkill> skills;

    bool ok;
    QSqlQuery monsterSkillsQuery(*GlobalServerData::serverPrivateVariables.db);
    if(!monsterSkillsQuery.exec(GlobalServerData::serverPrivateVariables.db_query_select_monstersSkill_by_id.arg(monsterId)))
        /*emit */message(monsterSkillsQuery.lastQuery()+QLatin1String(": ")+monsterSkillsQuery.lastError().text());
    while(monsterSkillsQuery.next())
    {
        PlayerMonster::PlayerSkill skill;
        skill.skill=monsterSkillsQuery.value(0).toUInt(&ok);
        if(ok)
        {
            if(!CommonDatapack::commonDatapack.monsterSkills.contains(skill.skill))
            {
                ok=false;
                /*emit */message(QStringLiteral("skill %1 for monsterId: %2 is not found into skill list").arg(skill.skill).arg(monsterId));
            }
        }
        else
            /*emit */message(QStringLiteral("skill id: %1 is not a number").arg(monsterSkillsQuery.value(0).toString()));
        if(ok)
        {
            skill.level=monsterSkillsQuery.value(1).toUInt(&ok);
            if(ok)
            {
                if(skill.level>CommonDatapack::commonDatapack.monsterSkills.value(skill.skill).level.size())
                {
                    ok=false;
                    /*emit */message(QStringLiteral("skill %1 for monsterId: %2 have not the level: %3").arg(skill.skill).arg(monsterId).arg(skill.level));
                }
            }
            else
                /*emit */message(QStringLiteral("skill level: %1 is not a number").arg(monsterSkillsQuery.value(1).toString()));
        }
        if(ok)
        {
            skill.endurance=monsterSkillsQuery.value(2).toUInt(&ok);
            if(ok)
            {
                if(skill.endurance>CommonDatapack::commonDatapack.monsterSkills.value(skill.skill).level.at(skill.level-1).endurance)
                {
                    skill.endurance=CommonDatapack::commonDatapack.monsterSkills.value(skill.skill).level.at(skill.level-1).endurance;
                    /*emit */message(QStringLiteral("skill %1 for monsterId: %2 have too hight endurance, lowered to: %3").arg(skill.skill).arg(monsterId).arg(skill.endurance));
                }
            }
            else
                /*emit */message(QStringLiteral("skill endurance: %1 is not a number").arg(monsterSkillsQuery.value(2).toString()));
        }
        if(ok)
            skills << skill;
    }
    return skills;
}

void ClientHeavyLoad::askedRandomNumber()
{
    QByteArray randomData;
    QDataStream randomDataStream(&randomData, QIODevice::WriteOnly);
    randomDataStream.setVersion(QDataStream::Qt_4_4);
    int index=0;
    while(index<CATCHCHALLENGER_SERVER_RANDOM_LIST_SIZE)
    {
        #if RAND_MAX >= 65535
        randomDataStream << quint16(rand()%65536);
        #else
        randomDataStream << quint8(rand()%256);
        #endif
        index++;
    }
    /*emit */newRandomNumber(randomData);
    /*emit */sendFullPacket(0xD0,0x0009,randomData);
}

void ClientHeavyLoad::loadBotAlreadyBeaten()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(GlobalServerData::serverPrivateVariables.db_query_select_bot_beaten.isEmpty())
    {
        /*emit */error(QStringLiteral("loadBotAlreadyBeaten() Query is empty, bug"));
        return;
    }
    #endif
    //do the query
    bool ok;
    QSqlQuery botAlreadyBeatenQuery(*GlobalServerData::serverPrivateVariables.db);
    if(!botAlreadyBeatenQuery.exec(GlobalServerData::serverPrivateVariables.db_query_select_bot_beaten.arg(player_informations->character_id)))
        /*emit */message(botAlreadyBeatenQuery.lastQuery()+QLatin1String(": ")+botAlreadyBeatenQuery.lastError().text());
    //parse the result
    while(botAlreadyBeatenQuery.next())
    {
        const quint32 &id=botAlreadyBeatenQuery.value(0).toUInt(&ok);
        if(!ok)
        {
            /*emit */message(QStringLiteral("wrong value type for quest, skip: %1").arg(id));
            continue;
        }
        if(!CommonDatapack::commonDatapack.botFights.contains(id))
        {
            /*emit */message(QStringLiteral("fights is not into the fights list, skip: %1").arg(id));
            continue;
        }
        player_informations->public_and_private_informations.bot_already_beaten << id;
    }
}
