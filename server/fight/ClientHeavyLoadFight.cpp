#include "../base/ClientHeavyLoad.h"
#include "../base/GlobalServerData.h"

#include "../../general/base/GeneralVariable.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/fight/CommonFightEngine.h"
#include "../../general/base/FacilityLib.h"

using namespace CatchChallenger;

void ClientHeavyLoad::loadMonsters()
{
    QString queryText;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            queryText=QString("SELECT `id`,`hp`,`monster`,`level`,`xp`,`sp`,`captured_with`,`gender`,`egg_step`,`place` FROM `monster` WHERE `character`=%1 ORDER BY `position` ASC").arg(player_informations->character_id);
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            queryText=QString("SELECT id,hp,monster,level,xp,sp,captured_with,gender,egg_step,place FROM monster WHERE character=%1 ORDER BY position ASC").arg(player_informations->character_id);
        break;
    }

    bool warehouse;
    bool ok;
    QSqlQuery monstersQuery(*GlobalServerData::serverPrivateVariables.db);
    if(!monstersQuery.exec(queryText))
        emit message(monstersQuery.lastQuery()+": "+monstersQuery.lastError().text());
    while(monstersQuery.next())
    {
        PlayerMonster playerMonster;
        playerMonster.id=monstersQuery.value(0).toUInt(&ok);
        if(!ok)
            emit message(QString("monsterId: %1 is not a number").arg(monstersQuery.value(0).toString()));
        if(ok)
        {
            playerMonster.monster=monstersQuery.value(2).toUInt(&ok);
            if(ok)
            {
                if(!CommonDatapack::commonDatapack.monsters.contains(playerMonster.monster))
                {
                    ok=false;
                    emit message(QString("monster: %1 is not into monster list").arg(playerMonster.monster));
                }
            }
            else
                emit message(QString("monster: %1 is not a number").arg(monstersQuery.value(2).toString()));
        }
        if(ok)
        {
            playerMonster.level=monstersQuery.value(3).toUInt(&ok);
            if(ok)
            {
                if(playerMonster.level>CATCHCHALLENGER_MONSTER_LEVEL_MAX)
                {
                    emit message(QString("level: %1 greater than %2, truncated").arg(playerMonster.level).arg(CATCHCHALLENGER_MONSTER_LEVEL_MAX));
                    playerMonster.level=CATCHCHALLENGER_MONSTER_LEVEL_MAX;
                }
            }
            else
                emit message(QString("level: %1 is not a number").arg(monstersQuery.value(3).toString()));
        }
        if(ok)
        {
            playerMonster.remaining_xp=monstersQuery.value(4).toUInt(&ok);
            if(ok)
            {
                if(playerMonster.remaining_xp>CommonDatapack::commonDatapack.monsters[playerMonster.monster].level_to_xp.at(playerMonster.level-1))
                {
                    emit message(QString("monster xp: %1 greater than %2, truncated").arg(playerMonster.remaining_xp).arg(CommonDatapack::commonDatapack.monsters[playerMonster.monster].level_to_xp.at(playerMonster.level-1)));
                    playerMonster.remaining_xp=0;
                }
            }
            else
                emit message(QString("monster xp: %1 is not a number").arg(monstersQuery.value(4).toString()));
        }
        if(ok)
        {
            playerMonster.sp=monstersQuery.value(5).toUInt(&ok);
            if(!ok)
                emit message(QString("monster sp: %1 is not a number").arg(monstersQuery.value(5).toString()));
        }
        if(ok)
        {
            playerMonster.captured_with=monstersQuery.value(6).toUInt(&ok);
            if(ok)
            {
                if(!CommonDatapack::commonDatapack.items.item.contains(playerMonster.captured_with))
                    emit message(QString("captured_with: %1 is not is not into items list").arg(playerMonster.captured_with));
            }
            else
                emit message(QString("captured_with: %1 is not a number").arg(monstersQuery.value(6).toString()));
        }
        if(ok)
        {
            if(monstersQuery.value(7).toString()=="male")
                playerMonster.gender=Gender_Male;
            else if(monstersQuery.value(7).toString()=="female")
                playerMonster.gender=Gender_Female;
            else if(monstersQuery.value(7).toString()=="unknown")
                playerMonster.gender=Gender_Unknown;
            else
            {
                playerMonster.gender=Gender_Unknown;
                emit message(QString("unknown monster gender: %1").arg(monstersQuery.value(7).toString()));
                ok=false;
            }
        }
        if(ok)
        {
            playerMonster.egg_step=monstersQuery.value(8).toUInt(&ok);
            if(!ok)
                emit message(QString("monster egg_step: %1 is not a number").arg(monstersQuery.value(8).toString()));
        }
        if(ok)
        {
            if(monstersQuery.value(9).toString()=="warehouse")
                warehouse=true;
            else
            {
                if(monstersQuery.value(9).toString()=="wear")
                    warehouse=false;
                else if(monstersQuery.value(9).toString()=="market")
                    continue;
                else
                {
                    emit message(QString("unknow wear type: %1 for monster %2").arg(monstersQuery.value(9).toString()).arg(playerMonster.id));
                    continue;
                }
            }
        }
        if(!warehouse && player_informations->public_and_private_informations.playerMonster.size()>=CATCHCHALLENGER_MONSTER_MAX_WEAR_ON_PLAYER)
        {
            warehouse=true;
            switch(GlobalServerData::serverSettings.database.type)
            {
                default:
                case ServerSettings::Database::DatabaseType_Mysql:
                    dbQuery(QString("UPDATE `monster` SET `place`='warehouse' WHERE `id`=%1;")
                                 .arg(playerMonster.id)
                                 );
                break;
                case ServerSettings::Database::DatabaseType_SQLite:
                    dbQuery(QString("UPDATE monster SET place='warehouse' WHERE id=%1;")
                                 .arg(playerMonster.id)
                                 );
                break;
            }
        }
        //stats
        if(ok)
        {
            playerMonster.hp=monstersQuery.value(1).toUInt(&ok);
            if(ok)
            {
                const Monster::Stat &stat=CommonFightEngine::getStat(CommonDatapack::commonDatapack.monsters[playerMonster.monster],playerMonster.level);
                if(playerMonster.hp>stat.hp)
                {
                    emit message(QString("monster hp: %1 greater than max hp %2 for the level %3 of the monster %4, truncated")
                                 .arg(playerMonster.hp)
                                 .arg(stat.hp)
                                 .arg(playerMonster.level)
                                 .arg(playerMonster.monster)
                                 );
                    playerMonster.hp=stat.hp;
                }
            }
            else
                emit message(QString("monster hp: %1 is not a number").arg(monstersQuery.value(1).toString()));
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
    QList<PlayerBuff> buffs;
    QString queryText;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            queryText=QString("SELECT `buff`,`level` FROM `monster_buff` WHERE `monster`=%1").arg(monsterId);
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            queryText=QString("SELECT buff,level FROM monster_buff WHERE monster=%1").arg(monsterId);
        break;
    }

    bool ok;
    QSqlQuery monsterBuffsQuery(*GlobalServerData::serverPrivateVariables.db);
    if(!monsterBuffsQuery.exec(queryText))
        emit message(monsterBuffsQuery.lastQuery()+": "+monsterBuffsQuery.lastError().text());
    while(monsterBuffsQuery.next())
    {
        PlayerBuff buff;
        buff.buff=monsterBuffsQuery.value(0).toUInt(&ok);
        if(ok)
        {
            if(!CommonDatapack::commonDatapack.monsterBuffs.contains(buff.buff))
            {
                ok=false;
                emit message(QString("buff %1 for monsterId: %2 is not found into buff list").arg(buff.buff).arg(monsterId));
            }
        }
        else
            emit message(QString("buff id: %1 is not a number").arg(monsterBuffsQuery.value(0).toString()));
        if(ok)
        {
            buff.level=monsterBuffsQuery.value(1).toUInt(&ok);
            if(ok)
            {
                if(buff.level>CommonDatapack::commonDatapack.monsterBuffs[buff.buff].level.size())
                {
                    ok=false;
                    emit message(QString("buff %1 for monsterId: %2 have not the level: %3").arg(buff.buff).arg(monsterId).arg(buff.level));
                }
            }
            else
                emit message(QString("buff level: %1 is not a number").arg(monsterBuffsQuery.value(2).toString()));
        }
        if(ok)
        {
            if(CommonDatapack::commonDatapack.monsterBuffs[buff.buff].level.at(buff.level-1).duration!=Buff::Duration_Always)
            {
                ok=false;
                DebugClass::debugConsole(QString("buff %1 for monsterId: %2 can't be loaded from the db if is not permanent").arg(buff.buff).arg(monsterId));
            }
        }
        if(ok)
            buffs << buff;
    }
    return buffs;
}

QList<PlayerMonster::PlayerSkill> ClientHeavyLoad::loadMonsterSkills(const quint32 &monsterId)
{
    QList<PlayerMonster::PlayerSkill> skills;
    QString queryText;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            queryText=QString("SELECT `skill`,`level`,`endurance` FROM `monster_skill` WHERE `monster`=%1").arg(monsterId);
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            queryText=QString("SELECT skill,level,endurance FROM monster_skill WHERE monster=%1").arg(monsterId);
        break;
    }

    bool ok;
    QSqlQuery monsterSkillsQuery(*GlobalServerData::serverPrivateVariables.db);
    if(!monsterSkillsQuery.exec(queryText))
        emit message(monsterSkillsQuery.lastQuery()+": "+monsterSkillsQuery.lastError().text());
    while(monsterSkillsQuery.next())
    {
        PlayerMonster::PlayerSkill skill;
        skill.skill=monsterSkillsQuery.value(0).toUInt(&ok);
        if(ok)
        {
            if(!CommonDatapack::commonDatapack.monsterSkills.contains(skill.skill))
            {
                ok=false;
                emit message(QString("skill %1 for monsterId: %2 is not found into skill list").arg(skill.skill).arg(monsterId));
            }
        }
        else
            emit message(QString("skill id: %1 is not a number").arg(monsterSkillsQuery.value(0).toString()));
        if(ok)
        {
            skill.level=monsterSkillsQuery.value(1).toUInt(&ok);
            if(ok)
            {
                if(skill.level>CommonDatapack::commonDatapack.monsterSkills[skill.skill].level.size())
                {
                    ok=false;
                    emit message(QString("skill %1 for monsterId: %2 have not the level: %3").arg(skill.skill).arg(monsterId).arg(skill.level));
                }
            }
            else
                emit message(QString("skill level: %1 is not a number").arg(monsterSkillsQuery.value(1).toString()));
        }
        if(ok)
        {
            skill.endurance=monsterSkillsQuery.value(2).toUInt(&ok);
            if(ok)
            {
                if(skill.endurance>CommonDatapack::commonDatapack.monsterSkills[skill.skill].level.at(skill.level-1).endurance)
                {
                    skill.endurance=CommonDatapack::commonDatapack.monsterSkills[skill.skill].level.at(skill.level-1).endurance;
                    emit message(QString("skill %1 for monsterId: %2 have too hight endurance, lowered to: %3").arg(skill.skill).arg(monsterId).arg(skill.endurance));
                }
            }
            else
                emit message(QString("skill endurance: %1 is not a number").arg(monsterSkillsQuery.value(2).toString()));
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
        //randomDataStream << quint8(rand()%256);
        randomDataStream << quint8(0x00);
        index++;
    }
    emit newRandomNumber(randomData);
    emit sendFullPacket(0xD0,0x0009,randomData);
}

void ClientHeavyLoad::loadBotAlreadyBeaten()
{
    //do the query
    QString queryText;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
        queryText=QString("SELECT `botfight_id` FROM `bot_already_beaten` WHERE `character`=%1")
                .arg(player_informations->character_id);
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
        queryText=QString("SELECT botfight_id FROM bot_already_beaten WHERE character=%1")
                .arg(player_informations->character_id);
        break;
    }
    bool ok;
    QSqlQuery botAlreadyBeatenQuery(*GlobalServerData::serverPrivateVariables.db);
    if(!botAlreadyBeatenQuery.exec(queryText))
        emit message(botAlreadyBeatenQuery.lastQuery()+": "+botAlreadyBeatenQuery.lastError().text());
    //parse the result
    while(botAlreadyBeatenQuery.next())
    {
        quint32 id=botAlreadyBeatenQuery.value(0).toUInt(&ok);
        if(!ok)
        {
            emit message(QString("wrong value type for quest, skip: %1").arg(id));
            continue;
        }
        if(!CommonDatapack::commonDatapack.botFights.contains(id))
        {
            emit message(QString("fights is not into the fights list, skip: %1").arg(id));
            continue;
        }
        player_informations->public_and_private_informations.bot_already_beaten << id;
    }
}
