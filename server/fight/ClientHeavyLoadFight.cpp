#include "../base/ClientHeavyLoad.h"
#include "../base/GlobalData.h"

#include "../../general/base/GeneralVariable.h"
#include "../../general/base/FacilityLib.h"

using namespace Pokecraft;

void ClientHeavyLoad::loadMonsters()
{
    player_informations->ableToFight=false;
    QString queryText;
    switch(GlobalData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            queryText=QString("SELECT id,hp,monster,level,xp,sp,captured_with,gender,egg_step FROM monster WHERE player=%1").arg(player_informations->id);
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            queryText=QString("SELECT id,hp,monster,level,xp,sp,captured_with,gender,egg_step FROM monster WHERE player=%1").arg(player_informations->id);
        break;
    }

    bool ok;
    QSqlQuery monstersQuery(queryText);
    while(monstersQuery.next())
    {
        PlayerMonster playerMonster;
        quint32 monsterId=monstersQuery.value(0).toUInt(&ok);
        if(!ok)
            emit message(QString("monsterId: %1 is not a number").arg(monstersQuery.value(0).toString()));
        if(ok)
        {
            playerMonster.monster=monstersQuery.value(2).toUInt(&ok);
            if(ok)
            {
                if(!GlobalData::serverPrivateVariables.monsters.contains(playerMonster.monster))
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
                if(playerMonster.level>POKECRAFT_MONSTER_LEVEL_MAX)
                {
                    emit message(QString("level: %1 greater than %2, truncated").arg(playerMonster.level).arg(POKECRAFT_MONSTER_LEVEL_MAX));
                    playerMonster.level=POKECRAFT_MONSTER_LEVEL_MAX;
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
                if(playerMonster.remaining_xp>GlobalData::serverPrivateVariables.monsters[monsterId].level_to_xp.at(playerMonster.level-1))
                {
                    emit message(QString("monster xp: %1 greater than %2, truncated").arg(playerMonster.remaining_xp).arg(GlobalData::serverPrivateVariables.monsters[monsterId].level_to_xp.at(playerMonster.level-1)));
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
                if(!GlobalData::serverPrivateVariables.items.contains(playerMonster.captured_with))
                    emit message(QString("captured_with: %1 is not is not into items list").arg(playerMonster.captured_with));
            }
            else
                emit message(QString("captured_with: %1 is not a number").arg(monstersQuery.value(6).toString()));
        }
        if(ok)
        {
            if(monstersQuery.value(7).toString()=="male")
                playerMonster.gender=PlayerMonster::Male;
            if(monstersQuery.value(7).toString()=="female")
                playerMonster.gender=PlayerMonster::Female;
            if(monstersQuery.value(7).toString()=="unknown")
                playerMonster.gender=PlayerMonster::Unknown;
            else
            {
                emit message(QString("unknown monster gender"));
                ok=false;
            }
        }
        if(ok)
        {
            playerMonster.egg_step=monstersQuery.value(8).toUInt(&ok);
            if(!ok)
                emit message(QString("monster egg_step: %1 is not a number").arg(monstersQuery.value(8).toString()));
        }
        //stats
        if(ok)
        {
            playerMonster.hp=monstersQuery.value(1).toUInt(&ok);
            if(ok)
            {
                if(playerMonster.hp>((GlobalData::serverPrivateVariables.monsters[monsterId].stat.hp*playerMonster.level)/POKECRAFT_MONSTER_LEVEL_MAX))
                {
                    emit message(QString("monster hp: %1 greater than max hp %2 for the level %3 of the monster %4, truncated")
                                 .arg(playerMonster.hp)
                                 .arg(((GlobalData::serverPrivateVariables.monsters[monsterId].stat.hp*playerMonster.level)/POKECRAFT_MONSTER_LEVEL_MAX))
                                 .arg(playerMonster.level)
                                 .arg(playerMonster.monster)
                                 );
                    playerMonster.hp=((GlobalData::serverPrivateVariables.monsters[monsterId].stat.hp*playerMonster.level)/POKECRAFT_MONSTER_LEVEL_MAX);
                }
            }
            else
                emit message(QString("monster hp: %1 is not a number").arg(monstersQuery.value(1).toString()));
        }
        //finish it
        if(ok)
        {
            if(playerMonster.hp>0 && playerMonster.egg_step==0)
                player_informations->ableToFight=true;
            playerMonster.buffs=loadMonsterBuffs(monsterId);
            playerMonster.skills=loadMonsterSkills(monsterId);
            player_informations->public_and_private_informations.playerMonster << playerMonster;
        }
    }
}

QList<PlayerMonster::Buff> ClientHeavyLoad::loadMonsterBuffs(const quint32 &monsterId)
{
    QList<PlayerMonster::Buff> buffs;
    QString queryText;
    switch(GlobalData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            queryText=QString("SELECT buff,level FROM monster_buff WHERE monster=%1").arg(monsterId);
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            queryText=QString("SELECT buff,level FROM monster_buff WHERE monster=%1").arg(monsterId);
        break;
    }

    bool ok;
    QSqlQuery monsterBuffsQuery(queryText);
    while(monsterBuffsQuery.next())
    {
        PlayerMonster::Buff buff;
        buff.buff=monsterBuffsQuery.value(0).toUInt(&ok);
        if(ok)
        {
            if(!GlobalData::serverPrivateVariables.monsterBuffs.contains(buff.buff))
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
                if(buff.level>GlobalData::serverPrivateVariables.monsterBuffs[buff.buff].level.size())
                {
                    ok=false;
                    emit message(QString("buff %1 for monsterId: %2 have not the level: %3").arg(buff.buff).arg(monsterId).arg(buff.level));
                }
            }
            else
                emit message(QString("buff level: %1 is not a number").arg(monsterBuffsQuery.value(2).toString()));
        }
        if(ok)
            buffs << buff;
    }
    return buffs;
}

QList<PlayerMonster::Skill> ClientHeavyLoad::loadMonsterSkills(const quint32 &monsterId)
{
    QList<PlayerMonster::Skill> skills;
    QString queryText;
    switch(GlobalData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            queryText=QString("SELECT skill,level FROM monster_skill WHERE monster=%1").arg(monsterId);
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            queryText=QString("SELECT skill,level FROM monster_skill WHERE monster=%1").arg(monsterId);
        break;
    }

    bool ok;
    QSqlQuery monsterSkillsQuery(queryText);
    while(monsterSkillsQuery.next())
    {
        PlayerMonster::Skill skill;
        skill.skill=monsterSkillsQuery.value(0).toUInt(&ok);
        if(ok)
        {
            if(!GlobalData::serverPrivateVariables.monsterSkills.contains(skill.skill))
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
                if(skill.level>GlobalData::serverPrivateVariables.monsterSkills[skill.skill].level.size())
                {
                    ok=false;
                    emit message(QString("skill %1 for monsterId: %2 have not the level: %3").arg(skill.skill).arg(monsterId).arg(skill.level));
                }
            }
            else
                emit message(QString("skill level: %1 is not a number").arg(monsterSkillsQuery.value(2).toString()));
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
    while(index<POKECRAFT_SERVER_RANDOM_LIST_SIZE)
    {
        randomDataStream << quint8(rand()%256);
        index++;
    }
    emit newRandomNumber(randomData);
    emit sendPacket(0xD0,0x0009,randomData);
}
