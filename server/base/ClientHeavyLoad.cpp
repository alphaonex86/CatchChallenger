#include "ClientHeavyLoad.h"
#include "GlobalServerData.h"

#include "../../general/base/GeneralVariable.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/FacilityLib.h"
#include "../../general/base/CommonMap.h"
#include "SqlFunction.h"
#include "LocalClientHandler.h"

#include <QProcess>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

using namespace CatchChallenger;

QList<quint16> ClientHeavyLoad::simplifiedIdList;
quint8 ClientHeavyLoad::tempDatapackListReplySize=0;
quint8 ClientHeavyLoad::tempDatapackListReply=0;
QByteArray ClientHeavyLoad::tempDatapackListReplyArray;
QByteArray ClientHeavyLoad::rawFiles;
QByteArray ClientHeavyLoad::compressedFiles;
int ClientHeavyLoad::tempDatapackListReplyTestCount;
int ClientHeavyLoad::rawFilesCount;
int ClientHeavyLoad::compressedFilesCount;
QSet<QString> ClientHeavyLoad::compressedExtension;
QHash<quint32,quint16> ClientHeavyLoad::clanConnectedCount;

ClientHeavyLoad::ClientHeavyLoad()
{
}

ClientHeavyLoad::~ClientHeavyLoad()
{
}

void ClientHeavyLoad::setVariable(Player_internal_informations *player_informations)
{
    this->player_informations=player_informations;
}

void ClientHeavyLoad::askLogin(const quint8 &query_id,const QByteArray &login_org,const QByteArray &pass_org)
{
    QByteArray login,pass;
    {
        QCryptographicHash hash(QCryptographicHash::Sha512);
        hash.addData(login_org);
        login=hash.result();
        QCryptographicHash hash2(QCryptographicHash::Sha512);
        hash2.addData(pass_org);
        pass=hash2.result();
    }
    if(player_informations->isFake)
    {
        askLoginBot(query_id);
        return;
    }
    {
        QString queryText;
        switch(GlobalServerData::serverSettings.database.type)
        {
            default:
            case ServerSettings::Database::DatabaseType_Mysql:
                queryText=QString("SELECT `id`,`password` FROM `account` WHERE `login`='%1'").arg(QString(login.toHex()));
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                queryText=QString("SELECT id,password FROM account WHERE login='%1'").arg(QString(login.toHex()));
            break;
        }
        QSqlQuery accountQuery(*GlobalServerData::serverPrivateVariables.db);
        if(!accountQuery.exec(queryText))
            emit message(accountQuery.lastQuery()+": "+accountQuery.lastError().text());
        bool ok;
        if(!accountQuery.next())
        {
            if(GlobalServerData::serverSettings.automatic_account_creation)
            {
                GlobalServerData::serverPrivateVariables.maxAccountId++;
                player_informations->account_id=GlobalServerData::serverPrivateVariables.maxAccountId;
                switch(GlobalServerData::serverSettings.database.type)
                {
                    default:
                    case ServerSettings::Database::DatabaseType_Mysql:
                        dbQuery(QString("INSERT INTO account(id,login,password,date) VALUES(%1,'%2','%3',%4);").arg(player_informations->account_id).arg(QString(login.toHex())).arg(QString(pass.toHex())).arg(QDateTime::currentMSecsSinceEpoch()/1000));
                    break;
                    case ServerSettings::Database::DatabaseType_SQLite:
                        dbQuery(QString("INSERT INTO account(id,login,password,date) VALUES(%1,'%2','%3',%4);").arg(player_informations->account_id).arg(QString(login.toHex())).arg(QString(pass.toHex())).arg(QDateTime::currentMSecsSinceEpoch()/1000));
                    break;
                }
            }
            else
            {
                loginIsWrong(query_id,"Bad login","Bad login for: "+login.toHex()+", pass: "+pass.toHex());
                return;
            }
        }
        else if(QString(pass.toHex())!=accountQuery.value(1).toString())
        {
            loginIsWrong(query_id,"Wrong login/pass","Password wrong");
            return;
        }
        else
        {
            player_informations->account_id=accountQuery.value(0).toUInt(&ok);
            if(!ok)
            {
                player_informations->account_id=0;
                loginIsWrong(query_id,"Wrong login/pass","Account id is not a number");
                return;
            }
        }
    }

    //send signals into the server
    emit message(QString("Logged the account %1").arg(player_informations->account_id));
    //send the network reply
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)02;
    out << (quint16)GlobalServerData::serverSettings.max_players;
    if(GlobalServerData::serverPrivateVariables.timer_city_capture==NULL)
        out << (quint32)0x00000000;
    else if(GlobalServerData::serverPrivateVariables.timer_city_capture->isActive())
    {
        qint64 time=GlobalServerData::serverPrivateVariables.time_city_capture.toMSecsSinceEpoch()-QDateTime::currentMSecsSinceEpoch();
        out << (quint32)time/1000;
    }
    else
        out << (quint32)0x00000000;
    out << (quint8)GlobalServerData::serverSettings.city.capture.frenquency;

    //common settings
    out << (quint8)CommonSettings::commonSettings.max_character;
    out << (quint8)CommonSettings::commonSettings.min_character;
    out << (quint8)CommonSettings::commonSettings.max_pseudo_size;
    out << (quint32)CommonSettings::commonSettings.character_delete_time;
    out << (float)CommonSettings::commonSettings.rates_xp;
    out << (float)CommonSettings::commonSettings.rates_gold;
    out << (quint8)CommonSettings::commonSettings.chat_allow_all;
    out << (quint8)CommonSettings::commonSettings.chat_allow_local;
    out << (quint8)CommonSettings::commonSettings.chat_allow_private;
    out << (quint8)CommonSettings::commonSettings.chat_allow_clan;
    out << (quint8)CommonSettings::commonSettings.factoryPriceChange;

    {
        quint8 max_character=CommonSettings::commonSettings.max_character;
        if(max_character==0)
            max_character=255;
        QString queryText;
        switch(GlobalServerData::serverSettings.database.type)
        {
            default:
            case ServerSettings::Database::DatabaseType_Mysql:
                queryText=QString("SELECT `id`,`pseudo`,`skin`,`time_to_delete`,`played_time`,`last_connect`,`map` FROM `character` WHERE `account`=%1 LIMIT 0,%2").arg(player_informations->account_id).arg(max_character);
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                queryText=QString("SELECT id,pseudo,skin,time_to_delete,played_time,last_connect,map FROM character WHERE account=%1 LIMIT 0,%2").arg(player_informations->account_id).arg(max_character);
            break;
        }
        QSqlQuery characterQuery(*GlobalServerData::serverPrivateVariables.db);
        if(!characterQuery.exec(queryText))
            emit message(characterQuery.lastQuery()+": "+characterQuery.lastError().text());
        const quint64 current_time=QDateTime::currentMSecsSinceEpoch()/1000;
        QList<CharacterEntry> characterEntryList;
        bool ok;
        while(characterQuery.next())
        {
            CharacterEntry characterEntry;
            characterEntry.character_id=characterQuery.value(0).toUInt(&ok);
            if(ok)
            {
                quint32 time_to_delete=characterQuery.value(3).toUInt(&ok);
                if(!ok)
                {
                    emit message(QString("time_to_delete is not number: %1 for %2 fixed by 0").arg(characterQuery.value(3).toString()).arg(player_informations->account_id));
                    time_to_delete=0;
                }
                characterEntry.played_time=characterQuery.value(4).toUInt(&ok);
                if(!ok)
                {
                    emit message(QString("played_time is not number: %1 for %2 fixed by 0").arg(characterQuery.value(4).toString()).arg(player_informations->account_id));
                    characterEntry.played_time=0;
                }
                characterEntry.last_connect=characterQuery.value(5).toUInt(&ok);
                if(!ok)
                {
                    emit message(QString("last_connect is not number: %1 for %2 fixed by 0").arg(characterQuery.value(5).toString()).arg(player_informations->account_id));
                    characterEntry.last_connect=current_time;
                }
                if(current_time>=time_to_delete && time_to_delete!=0)
                    deleteCharacterNow(characterEntry.character_id);
                else
                {
                    if(time_to_delete==0)
                        characterEntry.delete_time_left=0;
                    else
                        characterEntry.delete_time_left=time_to_delete-current_time;
                    characterEntry.pseudo=characterQuery.value(1).toString();
                    characterEntry.skin=characterQuery.value(2).toString();
                    characterEntry.map=characterQuery.value(6).toString();
                    characterEntryList << characterEntry;
                }
            }
            else
                emit message(QString("Character id is not number: %1 for %2").arg(characterQuery.value(0).toString()).arg(player_informations->account_id));
        }
        if(CommonSettings::commonSettings.max_character==0)
        {
            if(characterEntryList.isEmpty())
            {
                loginIsWrong(query_id,"Can't create character and don't have character","Can't create character and don't have character");
                return;
            }
        }

        player_informations->number_of_character=characterEntryList.size();
        out << (quint8)characterEntryList.size();
        int index=0;
        while(index<characterEntryList.size())
        {
            const CharacterEntry &characterEntry=characterEntryList.at(index);
            out << (quint32)characterEntry.character_id;
            out << characterEntry.pseudo;
            out << characterEntry.skin;
            out << (quint32)characterEntry.delete_time_left;
            out << (quint32)characterEntry.played_time;
            out << (quint32)characterEntry.last_connect;
            out << characterEntry.map;
            index++;
        }
    }

    emit postReply(query_id,outputData);
}

void ClientHeavyLoad::deleteCharacterNow(const quint32 &characterId)
{
    QString queryText;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            queryText=QString("SELECT `id` FROM `monster` WHERE `character`=%1").arg(characterId);
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            queryText=QString("SELECT id FROM monster WHERE character=%1").arg(characterId);
        break;
    }
    bool ok;
    QSqlQuery monstersQuery(*GlobalServerData::serverPrivateVariables.db);
    if(!monstersQuery.exec(queryText))
        emit message(monstersQuery.lastQuery()+": "+monstersQuery.lastError().text());
    while(monstersQuery.next())
    {
        const quint32 &monsterId=monstersQuery.value(0).toUInt(&ok);
        if(ok)
        {
            switch(GlobalServerData::serverSettings.database.type)
            {
                default:
                case ServerSettings::Database::DatabaseType_Mysql:
                    dbQuery(QString("DELETE FROM `monster_buff` WHERE monster=%1")
                                 .arg(monsterId)
                                 );
                break;
                case ServerSettings::Database::DatabaseType_SQLite:
                    dbQuery(QString("DELETE FROM monster_buff WHERE monster=%1")
                                 .arg(monsterId)
                                 );
                break;
            }
            switch(GlobalServerData::serverSettings.database.type)
            {
                default:
                case ServerSettings::Database::DatabaseType_Mysql:
                    dbQuery(QString("DELETE FROM `monster_skill` WHERE monster=%1")
                                 .arg(monsterId)
                                 );
                break;
                case ServerSettings::Database::DatabaseType_SQLite:
                    dbQuery(QString("DELETE FROM monster_skill WHERE monster=%1")
                                 .arg(monsterId)
                                 );
                break;
            }
        }
    }
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            dbQuery(QString("DELETE FROM `bot_already_beaten` WHERE `character`=%1").arg(characterId));
            dbQuery(QString("DELETE FROM `character` WHERE `character`=%1").arg(characterId));
            dbQuery(QString("DELETE FROM `item` WHERE `character`=%1").arg(characterId));
            dbQuery(QString("DELETE FROM `monster` WHERE `character`=%1").arg(characterId));
            dbQuery(QString("DELETE FROM `plant` WHERE `character`=%1").arg(characterId));
            dbQuery(QString("DELETE FROM `quest` WHERE `character`=%1").arg(characterId));
            dbQuery(QString("DELETE FROM `recipes` WHERE `character`=%1").arg(characterId));
            dbQuery(QString("DELETE FROM `reputation` WHERE `character`=%1").arg(characterId));
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            dbQuery(QString("DELETE FROM `bot_already_beaten` WHERE `character`=%1").arg(characterId));
            dbQuery(QString("DELETE FROM `character` WHERE `character`=%1").arg(characterId));
            dbQuery(QString("DELETE FROM `item` WHERE `character`=%1").arg(characterId));
            dbQuery(QString("DELETE FROM `monster` WHERE `character`=%1").arg(characterId));
            dbQuery(QString("DELETE FROM `plant` WHERE `character`=%1").arg(characterId));
            dbQuery(QString("DELETE FROM `quest` WHERE `character`=%1").arg(characterId));
            dbQuery(QString("DELETE FROM `recipes` WHERE `character`=%1").arg(characterId));
            dbQuery(QString("DELETE FROM `reputation` WHERE `character`=%1").arg(characterId));
        break;
    }
}

void ClientHeavyLoad::askLoginBot(const quint8 &query_id)
{
    if(GlobalServerData::serverPrivateVariables.botSpawn.isEmpty())
        loginIsWrong(query_id,"Not bot point","Not bot point");
    else
    {
        if(!GlobalServerData::serverPrivateVariables.map_list.contains(GlobalServerData::serverPrivateVariables.botSpawn.at(GlobalServerData::serverPrivateVariables.botSpawnIndex).map))
            loginIsWrong(query_id,"Bot point not resolved","Bot point not resolved");
        else if(simplifiedIdList.size()<=0)
            loginIsWrong(query_id,"Not free id to login","Not free id to login");
        else
        {
            player_informations->public_and_private_informations.public_informations.simplifiedId = simplifiedIdList.first();
            player_informations->public_and_private_informations.clan=0;
            player_informations->public_and_private_informations.clan_leader=false;
            player_informations->character_id=999999999-GlobalServerData::serverPrivateVariables.number_of_bots_logged;
            player_informations->public_and_private_informations.public_informations.pseudo=QString("bot_%1").arg(player_informations->public_and_private_informations.public_informations.simplifiedId);
            player_informations->public_and_private_informations.public_informations.skinId=0x00;//use the first skin by alaphabetic order
            player_informations->public_and_private_informations.public_informations.type=Player_type_normal;
            player_informations->public_and_private_informations.cash=0;
            player_informations->public_and_private_informations.public_informations.speed=CATCHCHALLENGER_SERVER_NORMAL_SPEED;
            if(!loadTheRawUTF8String())
                loginIsWrong(query_id,"Convert into utf8 have wrong size",QString("Unable to convert the pseudo to utf8 at bot: %1").arg(QString("bot_%1").arg(player_informations->character_id)));
            else
            //all is rights
            {
                loginIsRight(query_id,
                     player_informations->character_id,
                     GlobalServerData::serverPrivateVariables.map_list[GlobalServerData::serverPrivateVariables.botSpawn.at(GlobalServerData::serverPrivateVariables.botSpawnIndex).map],
                     GlobalServerData::serverPrivateVariables.botSpawn.at(GlobalServerData::serverPrivateVariables.botSpawnIndex).x,
                     GlobalServerData::serverPrivateVariables.botSpawn.at(GlobalServerData::serverPrivateVariables.botSpawnIndex).y,
                     Orientation_bottom);

                GlobalServerData::serverPrivateVariables.botSpawnIndex++;
                if(GlobalServerData::serverPrivateVariables.botSpawnIndex>=GlobalServerData::serverPrivateVariables.botSpawn.size())
                    GlobalServerData::serverPrivateVariables.botSpawnIndex=0;
                GlobalServerData::serverPrivateVariables.number_of_bots_logged++;
            }
        }
    }
}

void ClientHeavyLoad::addCharacter(const quint8 &query_id, const quint8 &profileIndex, const QString &pseudo, const QString &skin)
{
    if(player_informations->number_of_character>=CommonSettings::commonSettings.max_character)
    {
        emit error(QString("You can't create more account, you have already %1 on %2 allowed").arg(player_informations->number_of_character).arg(CommonSettings::commonSettings.max_character));
        return;
    }
    if(profileIndex>=CommonDatapack::commonDatapack.profileList.size())
    {
        emit error(QString("profile index: %1 out of range (profileList size: %2)").arg(profileIndex).arg(CommonDatapack::commonDatapack.profileList.size()));
        return;
    }
    if(pseudo.size()>CommonSettings::commonSettings.max_pseudo_size)
    {
        emit error(QString("pseudo size is too big: %1 because is greater than %2").arg(pseudo.size()).arg(CommonSettings::commonSettings.max_pseudo_size));
        return;
    }
    const Profile &profile=CommonDatapack::commonDatapack.profileList.at(profileIndex);
    if(!profile.forcedskin.isEmpty() && !profile.forcedskin.contains(skin))
    {
        emit error(QString("skin provided: %1 is not into profile forced skin list: %2").arg(skin).arg(profile.forcedskin.join(";")));
        return;
    }
    if(!GlobalServerData::serverPrivateVariables.skinList.contains(skin))
    {
        emit error(QString("skin provided: %1 is not into skin listed").arg(skin));
        return;
    }
    player_informations->number_of_character++;
    GlobalServerData::serverPrivateVariables.maxCharacterId++;

    const quint32 &characterId=GlobalServerData::serverPrivateVariables.maxCharacterId;
    int index=0;
    int monster_position=1;
    {
        switch(GlobalServerData::serverSettings.database.type)
        {
            default:
            case ServerSettings::Database::DatabaseType_Mysql:
                dbQuery(QString("INSERT INTO `character`(`id`,`account`,`pseudo`,`skin`,`x`,`y`,`orientation`,`map`,`type`,`clan`,`cash`,`rescue_map`,`rescue_x`,`rescue_y`,`rescue_orientation`,`unvalidated_rescue_map`,`unvalidated_rescue_x`,`unvalidated_rescue_y`,`unvalidated_rescue_orientation`,`market_cash`,`market_bitcoin`,`date`,`warehouse_cash`,`allow`,`clan_leader`,`bitcoin_offset`,`time_to_delete`,`played_time`,`last_connect`) VALUES(%1,'%2','%3','%4',%5,%6,'bottom','%7','normal',0,%8,%9,%9,0,0,"+QString::number(QDateTime::currentMSecsSinceEpoch()/1000)+",0,'',0,0,0,0,"+QString::number(QDateTime::currentMSecsSinceEpoch()/1000)+");")
                        .arg(characterId)
                        .arg(player_informations->account_id)
                        .arg(pseudo)
                        .arg(skin)
                        .arg(profile.x)
                        .arg(profile.y)
                        .arg(profile.map)
                        .arg(profile.cash)
                        .arg(QString("'%1',%2,%3,'bottom'").arg(profile.map).arg(profile.x).arg(profile.y))
                        );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                dbQuery(QString("INSERT INTO `character`(`id`,`account`,`pseudo`,`skin`,`x`,`y`,`orientation`,`map`,`type`,`clan`,`cash`,`rescue_map`,`rescue_x`,`rescue_y`,`rescue_orientation`,`unvalidated_rescue_map`,`unvalidated_rescue_x`,`unvalidated_rescue_y`,`unvalidated_rescue_orientation`,`market_cash`,`market_bitcoin`,`date`,`warehouse_cash`,`allow`,`clan_leader`,`bitcoin_offset`,`time_to_delete`,`played_time`,`last_connect`) VALUES(%1,'%2','%3','%4',%5,%6,'bottom','%7','normal',0,%8,%9,%9,0,0,"+QString::number(QDateTime::currentMSecsSinceEpoch()/1000)+",0,'',0,0,0,0,"+QString::number(QDateTime::currentMSecsSinceEpoch()/1000)+");")
                        .arg(characterId)
                        .arg(player_informations->account_id)
                        .arg(pseudo)
                        .arg(skin)
                        .arg(profile.x)
                        .arg(profile.y)
                        .arg(profile.map)
                        .arg(profile.cash)
                        .arg(QString("'%1',%2,%3,'bottom'").arg(profile.map).arg(profile.x).arg(profile.y))
                        );
            break;
        }
    }
    while(index<profile.monsters.size())
    {
        QString gender="unknown";
        if(CatchChallenger::CommonDatapack::commonDatapack.monsters[profile.monsters.at(index).id].ratio_gender!=-1)
        {
            if(rand()%101<CatchChallenger::CommonDatapack::commonDatapack.monsters[profile.monsters.at(index).id].ratio_gender)
                gender="female";
            else
                gender="male";
        }
        CatchChallenger::Monster::Stat stat=CatchChallenger::CommonFightEngine::getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters[profile.monsters.at(index).id],profile.monsters.at(index).level);
        QList<CatchChallenger::PlayerMonster::PlayerSkill> skills;
        QList<CatchChallenger::Monster::AttackToLearn> attack=CatchChallenger::CommonDatapack::commonDatapack.monsters[profile.monsters.at(index).id].learn;
        int sub_index=0;
        while(sub_index<attack.size())
        {
            if(attack[sub_index].learnAtLevel<=profile.monsters.at(index).level)
            {
                CatchChallenger::PlayerMonster::PlayerSkill temp;
                temp.level=attack[sub_index].learnSkillLevel;
                temp.skill=attack[sub_index].learnSkill;
                temp.endurance=0;
                skills << temp;
            }
            sub_index++;
        }
        quint32 monster_id;
        {
            QMutexLocker(&GlobalServerData::serverPrivateVariables.monsterIdMutex);
            GlobalServerData::serverPrivateVariables.maxMonsterId++;
            monster_id=GlobalServerData::serverPrivateVariables.maxMonsterId;
        }
        while(skills.size()>4)
            skills.removeFirst();
        {
            switch(GlobalServerData::serverSettings.database.type)
            {
                default:
                case ServerSettings::Database::DatabaseType_Mysql:
                    dbQuery(QString("INSERT INTO `monster`(`id`,`hp`,`character`,`monster`,`level`,`xp`,`sp`,`captured_with`,`gender`,`egg_step`,`character_origin`,`place`,`position`,`market_price`,`market_bitcoin`) VALUES(%1,%2,%3,%4,%5,0,0,%6,'%7',0,%3,'wear',%8,0,0);")
                       .arg(monster_id)
                       .arg(stat.hp)
                       .arg(characterId)
                       .arg(profile.monsters.at(index).id)
                       .arg(profile.monsters.at(index).level)
                       .arg(profile.monsters.at(index).captured_with)
                       .arg(gender)
                       .arg(monster_position)
                        );
                break;
                case ServerSettings::Database::DatabaseType_SQLite:
                    dbQuery(QString("INSERT INTO `monster`(`id`,`hp`,`character`,`monster`,`level`,`xp`,`sp`,`captured_with`,`gender`,`egg_step`,`character_origin`,`place`,`position`,`market_price`,`market_bitcoin`) VALUES(%1,%2,%3,%4,%5,0,0,%6,'%7',0,%3,'wear',%8,0,0);")
                       .arg(monster_id)
                       .arg(stat.hp)
                       .arg(characterId)
                       .arg(profile.monsters.at(index).id)
                       .arg(profile.monsters.at(index).level)
                       .arg(profile.monsters.at(index).captured_with)
                       .arg(gender)
                       .arg(monster_position)
                        );
                break;
            }
            monster_position++;
        }
        sub_index=0;
        while(sub_index<skills.size())
        {
            quint8 endurance=0;
            if(CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.contains(skills[sub_index].skill))
                if(skills[sub_index].level<=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills[skills[sub_index].skill].level.size() && skills[sub_index].level>0)
                    endurance=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills[skills[sub_index].skill].level.at(skills[sub_index].level-1).endurance;
            switch(GlobalServerData::serverSettings.database.type)
            {
                default:
                case ServerSettings::Database::DatabaseType_Mysql:
                    dbQuery(QString("INSERT INTO `monster_skill`(`monster`,`skill`,`level`,`endurance`) VALUES(%1,%2,%3,%4);")
                       .arg(monster_id)
                       .arg(skills[sub_index].skill)
                       .arg(skills[sub_index].level)
                       .arg(endurance)
                            );
                break;
                case ServerSettings::Database::DatabaseType_SQLite:
                    dbQuery(QString("INSERT INTO `monster_skill`(`monster`,`skill`,`level`,`endurance`) VALUES(%1,%2,%3,%4);")
                       .arg(monster_id)
                       .arg(skills[sub_index].skill)
                       .arg(skills[sub_index].level)
                       .arg(endurance)
                            );
                break;
            }
            sub_index++;
        }
        index++;
    }
    index=0;
    while(index<profile.reputation.size())
    {
        switch(GlobalServerData::serverSettings.database.type)
        {
            default:
            case ServerSettings::Database::DatabaseType_Mysql:
                dbQuery(QString("INSERT INTO `reputation`(`character`,`type`,`point`,`level`) VALUES(%1,'%2',%3,%4);")
                   .arg(characterId)
                   .arg(profile.reputation.at(index).type)
                   .arg(profile.reputation.at(index).point)
                   .arg(profile.reputation.at(index).level)
                        );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                dbQuery(QString("INSERT INTO `reputation`(`character`,`type`,`point`,`level`) VALUES(%1,'%2',%3,%4);")
                   .arg(characterId)
                   .arg(profile.reputation.at(index).type)
                   .arg(profile.reputation.at(index).point)
                   .arg(profile.reputation.at(index).level)
                        );
            break;
        }
        index++;
    }
    index=0;
    while(index<profile.items.size())
    {
        switch(GlobalServerData::serverSettings.database.type)
        {
            default:
            case ServerSettings::Database::DatabaseType_Mysql:
                dbQuery(QString("INSERT INTO `item`(`item`,`character`,`quantity`,`place`) VALUES(%1,%2,%3,'wear');")
                   .arg(profile.items.at(index).id)
                   .arg(characterId)
                   .arg(profile.items.at(index).quantity)
                        );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                dbQuery(QString("INSERT INTO `item`(`item`,`character`,`quantity`,`place`) VALUES(%1,%2,%3,'wear');")
                   .arg(profile.items.at(index).id)
                   .arg(characterId)
                   .arg(profile.items.at(index).quantity)
                        );
            break;
        }
        index++;
    }

    //send the network reply
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << characterId;
    emit postReply(query_id,outputData);
}

void ClientHeavyLoad::removeCharacter(const quint8 &query_id, const quint32 &characterId)
{
    QString queryText;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            queryText=QString("SELECT `account``time_to_delete` FROM `character` WHERE `id`=%1").arg(characterId);
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            queryText=QString("SELECT account,time_to_delete FROM character WHERE id=%1").arg(characterId);
        break;
    }
    QSqlQuery characterQuery(*GlobalServerData::serverPrivateVariables.db);
    if(!characterQuery.exec(queryText))
    {
        characterSelectionIsWrong(query_id,"Character not found",characterQuery.lastQuery()+": "+characterQuery.lastError().text());
        return;
    }
    if(!characterQuery.next())
    {
        characterSelectionIsWrong(query_id,"Character not found","Result return query wrong");
        return;
    }
    bool ok;
    const quint32 &account_id=characterQuery.value(0).toUInt(&ok);
    if(!ok)
    {
        characterSelectionIsWrong(query_id,"Character not found",QString("Account for character: %1 is not an id").arg(characterQuery.value(0).toString()));
        return;
    }
    if(player_informations->account_id!=account_id)
    {
        characterSelectionIsWrong(query_id,"Character not found",QString("Character: %1 is not owned by the account: %2").arg(characterId).arg(player_informations->account_id));
        return;
    }
    const quint32 &time_to_delete=characterQuery.value(1).toUInt(&ok);
    if(ok && time_to_delete>0)
    {
        characterSelectionIsWrong(query_id,"Already in deleting",QString("Character: %1 is already in deleting for the account: %2").arg(characterId).arg(player_informations->account_id));
        return;
    }
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            dbQuery(QString("UPDATE `character` SET `time_to_delete`=%2 WHERE `id`=%1").arg(characterId).arg(CommonSettings::commonSettings.character_delete_time));
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            dbQuery(QString("UPDATE `character` SET `time_to_delete`=%2 WHERE `id`=%1").arg(characterId).arg(CommonSettings::commonSettings.character_delete_time));
        break;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x02;
    emit postReply(query_id,outputData);
}

void ClientHeavyLoad::selectCharacter(const quint8 &query_id, const quint32 &characterId)
{
    /*account(0),pseudo(1),skin(2),x(3),y(4),orientation(5),map(6),type(7),clan(8),cash(9),
    rescue_map(10),rescue_x(11),rescue_y(12),rescue_orientation(13),unvalidated_rescue_map(14),
    unvalidated_rescue_x(15),unvalidated_rescue_y(16),unvalidated_rescue_orientation(17),
    warehouse_cash(18),allow(19),clan_leader(20),bitcoin_offset(21),market_cash(22),market_bitcoin(23),
    time_to_delete(24),*/
    QString queryText;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            queryText=QString("SELECT `account`,`pseudo`,`skin`,`x`,`y`,`orientation`,`map`,`type`,`clan`,`cash`,`rescue_map`,`rescue_x`,`rescue_y`,`rescue_orientation`,`unvalidated_rescue_map`,`unvalidated_rescue_x`,`unvalidated_rescue_y`,`unvalidated_rescue_orientation`,`warehouse_cash`,`allow`,`clan_leader`,`bitcoin_offset`,`market_cash`,`market_bitcoin`,`time_to_delete` FROM `character` WHERE `id`=%1").arg(characterId);
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            queryText=QString("SELECT account,pseudo,skin,x,y,orientation,map,type,clan,cash,rescue_map,rescue_x,rescue_y,rescue_orientation,unvalidated_rescue_map,unvalidated_rescue_x,unvalidated_rescue_y,unvalidated_rescue_orientation,warehouse_cash,allow,clan_leader,bitcoin_offset,market_cash,market_bitcoin,time_to_delete FROM character WHERE id=%1").arg(characterId);
        break;
    }
    QSqlQuery characterQuery(*GlobalServerData::serverPrivateVariables.db);
    if(!characterQuery.exec(queryText))
    {
        characterSelectionIsWrong(query_id,"Character not found",characterQuery.lastQuery()+": "+characterQuery.lastError().text());
        return;
    }
    if(!characterQuery.next())
    {
        characterSelectionIsWrong(query_id,"Character not found","Result return query wrong");
        return;
    }
    bool ok;
    const quint32 &account_id=characterQuery.value(0).toUInt(&ok);
    if(!ok)
    {
        characterSelectionIsWrong(query_id,"Character not found",QString("Account for character: %1 is not an id").arg(characterQuery.value(0).toString()));
        return;
    }
    if(player_informations->account_id!=account_id)
    {
        characterSelectionIsWrong(query_id,"Character not found",QString("Character: %1 is not owned by the account: %2").arg(characterId).arg(player_informations->account_id));
        return;
    }
    if(player_informations->character_loaded)
    {
        characterSelectionIsWrong(query_id,"Already logged","character_loaded already to true");
        return;
    }
    if(GlobalServerData::serverPrivateVariables.connected_players_id_list.contains(characterId))
    {
        characterSelectionIsWrong(query_id,"Already logged","Already logged");
        return;
    }
    if(simplifiedIdList.size()<=0)
    {
        characterSelectionIsWrong(query_id,"Not free id to login","Not free id to login");
        return;
    }
    if(!loadTheRawUTF8String())
    {
        if(GlobalServerData::serverSettings.anonymous)
            characterSelectionIsWrong(query_id,"Convert into utf8 have wrong size",QString("Unable to convert the pseudo to utf8 for character id: %1").arg(player_informations->character_id));
        else
            characterSelectionIsWrong(query_id,"Convert into utf8 have wrong size",QString("Unable to convert the pseudo to utf8: %1").arg(player_informations->public_and_private_informations.public_informations.pseudo));
        return;
    }
    if(GlobalServerData::serverSettings.anonymous)
        emit message(QString("Charater id is logged: %1").arg(characterId));
    else
        emit message(QString("Charater is logged: %1").arg(characterQuery.value(1).toString()));
    const quint32 &time_to_delete=characterQuery.value(24).toUInt(&ok);
    if(!ok || time_to_delete>0)
    {
        switch(GlobalServerData::serverSettings.database.type)
        {
            default:
            case ServerSettings::Database::DatabaseType_Mysql:
                dbQuery(QString("UPDATE `character` SET `time_to_delete`=0 WHERE `id`=%1").arg(characterId));
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                dbQuery(QString("UPDATE character SET time_to_delete=0 WHERE id=%1").arg(characterId));
            break;
        }
    }
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            dbQuery(QString("UPDATE `character` SET `last_connect`=%2 WHERE `id`=%1").arg(characterId).arg(QDateTime::currentMSecsSinceEpoch()/1000));
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            dbQuery(QString("UPDATE character SET last_connect=%2 WHERE id=%1").arg(characterId).arg(QDateTime::currentMSecsSinceEpoch()/1000));
        break;
    }

    player_informations->public_and_private_informations.clan=characterQuery.value(8).toUInt(&ok);
    if(!ok)
    {
        emit message(QString("clan id is not an number, clan disabled"));
        player_informations->public_and_private_informations.clan=0;//no clan
    }
    player_informations->public_and_private_informations.clan_leader=(characterQuery.value(20).toUInt(&ok)==1);
    if(!ok)
    {
        emit message(QString("clan_leader id is not an number, clan_leader disabled"));
        player_informations->public_and_private_informations.clan_leader=false;//no clan
    }
    player_informations->public_and_private_informations.public_informations.pseudo=characterQuery.value(1).toString();
    QString skinString=characterQuery.value(2).toString();
    if(GlobalServerData::serverPrivateVariables.skinList.contains(skinString))
        player_informations->public_and_private_informations.public_informations.skinId=GlobalServerData::serverPrivateVariables.skinList[skinString];
    else
    {
        emit message(QString("Skin not found, or out of the 255 first folder, default of the first by order alphabetic if have"));
        player_informations->public_and_private_informations.public_informations.skinId=0;
    }
    QString type=characterQuery.value(7).toString();
    if(type=="normal")
        player_informations->public_and_private_informations.public_informations.type=Player_type_normal;
    else if(type=="premium")
        player_informations->public_and_private_informations.public_informations.type=Player_type_premium;
    else if(type=="gm")
        player_informations->public_and_private_informations.public_informations.type=Player_type_gm;
    else if(type=="dev")
        player_informations->public_and_private_informations.public_informations.type=Player_type_dev;
    else
    {
        emit message(QString("Mysql wrong type value").arg(type));
        player_informations->public_and_private_informations.public_informations.type=Player_type_normal;
    }
    player_informations->public_and_private_informations.cash=characterQuery.value(9).toUInt(&ok);
    if(!ok)
    {
        emit message(QString("cash id is not an number, cash set to 0"));
        player_informations->public_and_private_informations.cash=0;
    }
    player_informations->public_and_private_informations.warehouse_cash=characterQuery.value(18).toUInt(&ok);
    if(!ok)
    {
        emit message(QString("warehouse cash id is not an number, warehouse cash set to 0"));
        player_informations->public_and_private_informations.warehouse_cash=0;
    }
    if(GlobalServerData::serverPrivateVariables.bitcoin.enabled)
    {
        double bitcoin_offset=characterQuery.value(21).toDouble(&ok);
        if(!ok)
        {
            emit message(QString("bitcoin offset is not an number (double), bitconi disabled"));
            player_informations->public_and_private_informations.bitcoin=-1.0;
        }
        else
        {
            QProcess process;
            process.start(GlobalServerData::serverSettings.bitcoin.binaryPath,QStringList()
                                                                           << QString("-datadir=%1").arg(GlobalServerData::serverSettings.bitcoin.workingPath)
                                                                           << QString("-port=%1").arg(GlobalServerData::serverSettings.bitcoin.port)
                                                                           << QString("-bind=127.0.0.1:%1").arg(GlobalServerData::serverSettings.bitcoin.port)
                                                                           << QString("-rpcport=%1").arg(GlobalServerData::serverSettings.bitcoin.port+1)
                                                                           << QString("getbalance")
                                                                           << QString("CatchChallenger_player_%1").arg(characterId)
                                                                           << QString("20")//number of confirmation
                   );
            process.waitForStarted();
            process.waitForFinished();
            if(process.state()!=QProcess::NotRunning)
            {
                player_informations->public_and_private_informations.bitcoin=-1.0;
                emit message("Have been to kill the bitcoin requester client");
                process.terminate();
                process.kill();
            }
            else if(process.exitStatus()!=QProcess::NormalExit || process.exitCode()!=0)
            {
                player_informations->public_and_private_informations.bitcoin=-1.0;
                emit message(QString("Bitcoin requester client have wrong exit code or status, exit code: %1, error code: %2").arg(process.exitCode()).arg((int)process.error()));
            }
            else
            {
                QByteArray errorRaw=process.readAllStandardError();
                QByteArray outputRaw=process.readAllStandardOutput();
                if(!errorRaw.isEmpty())
                {
                    player_informations->public_and_private_informations.bitcoin=-1.0;
                    emit message(QString("The bitcoin requester client have reported an error: %1").arg(QString::fromLocal8Bit(errorRaw)));
                }
                else if(outputRaw.isEmpty())
                {
                    player_informations->public_and_private_informations.bitcoin=-1.0;
                    emit message(QString("The bitcoin requester client have empty output"));
                }
                else
                {
                    QString stringOutput=QString::fromLocal8Bit(outputRaw);
                    double bitcoin_real_balance=stringOutput.toDouble(&ok);
                    if(!ok)
                    {
                        player_informations->public_and_private_informations.bitcoin=-1.0;
                        emit message(QString("The bitcoin requester have not returned a double: %1").arg(stringOutput));
                    }
                    else if((bitcoin_real_balance+bitcoin_offset)<0)
                    {
                        player_informations->public_and_private_informations.bitcoin=-1.0;
                        emit message(QString("Bitcoin amount is negative, bitcoind sync?"));
                    }
                    else
                        player_informations->public_and_private_informations.bitcoin=bitcoin_real_balance+bitcoin_offset;
                }
            }
            if(player_informations->public_and_private_informations.bitcoin>=0)
            {
                QProcess process;
                process.start(GlobalServerData::serverSettings.bitcoin.binaryPath,QStringList()
                                                                               << QString("-datadir=%1").arg(GlobalServerData::serverSettings.bitcoin.workingPath)
                                                                               << QString("-port=%1").arg(GlobalServerData::serverSettings.bitcoin.port)
                                                                               << QString("-bind=127.0.0.1:%1").arg(GlobalServerData::serverSettings.bitcoin.port)
                                                                               << QString("-rpcport=%1").arg(GlobalServerData::serverSettings.bitcoin.port+1)
                                                                               << QString("getaccountaddress")
                                                                               << QString("CatchChallenger_player_%1").arg(player_informations->character_id)
                       );
                process.waitForStarted();
                process.waitForFinished();
                if(process.state()!=QProcess::NotRunning)
                {
                    player_informations->public_and_private_informations.bitcoin=-1.0;
                    emit message("Have been to kill the bitcoin requester client");
                    process.terminate();
                    process.kill();
                }
                else if(process.exitStatus()!=QProcess::NormalExit || process.exitCode()!=0)
                {
                    player_informations->public_and_private_informations.bitcoin=-1.0;
                    emit message(QString("Bitcoin requester client have wrong exit code or status, exit code: %1, error code: %2").arg(process.exitCode()).arg((int)process.error()));
                }
                else
                {
                    QByteArray errorRaw=process.readAllStandardError();
                    QByteArray outputRaw=process.readAllStandardOutput();
                    if(!errorRaw.isEmpty())
                    {
                        player_informations->public_and_private_informations.bitcoin=-1.0;
                        emit message(QString("The bitcoin requester client have reported an error: %1").arg(QString::fromLocal8Bit(errorRaw)));
                    }
                    else if(outputRaw.isEmpty())
                    {
                        player_informations->public_and_private_informations.bitcoin=-1.0;
                        emit message(QString("The bitcoin requester client have empty output"));
                    }
                    else
                    {
                        QString stringOutput=QString::fromLocal8Bit(outputRaw);
                        if(stringOutput.contains(QRegularExpression(CATCHCHALLENGER_SERVER_BITCOIN_ADDRESS_REGEX)))
                            player_informations->public_and_private_informations.bitcoinAddress=stringOutput;
                        else
                            player_informations->public_and_private_informations.bitcoin=-1.0;
                    }
                }
            }
        }
    }
    else
        player_informations->public_and_private_informations.bitcoin=-1.0;
    player_informations->market_cash=characterQuery.value(22).toULongLong(&ok);
    if(!ok)
    {
        loginIsWrong(query_id,"Wrong account data",QString("Market cash wrong: %1").arg(characterQuery.value(22).toString()));
        return;
    }
    player_informations->market_bitcoin=0;
    if(player_informations->public_and_private_informations.bitcoin>=0)
    {
        player_informations->market_cash=characterQuery.value(23).toDouble(&ok);
        if(!ok)
        {
            loginIsWrong(query_id,"Wrong account data",QString("Market bitcoin wrong: %1").arg(characterQuery.value(23).toString()));
            return;
        }
    }

    player_informations->public_and_private_informations.public_informations.speed=CATCHCHALLENGER_SERVER_NORMAL_SPEED;
    if(!loadTheRawUTF8String())
    {
        loginIsWrong(query_id,"Convert into utf8 have wrong size","Convert into utf8 have wrong size");
        return;
    }
    QString orientationString=characterQuery.value(5).toString();
    Orientation orentation;
    if(orientationString=="top")
        orentation=Orientation_top;
    else if(orientationString=="bottom")
        orentation=Orientation_bottom;
    else if(orientationString=="left")
        orentation=Orientation_left;
    else if(orientationString=="right")
        orentation=Orientation_right;
    else
    {
        orentation=Orientation_bottom;
        emit message(QString("Wrong orientation corrected with bottom"));
    }
    player_informations->public_and_private_informations.allow=FacilityLib::StringToAllow(characterQuery.value(19).toString());
    //all is rights
    if(GlobalServerData::serverPrivateVariables.map_list.contains(characterQuery.value(6).toString()))
    {
        quint8 x=characterQuery.value(3).toUInt(&ok);
        if(!ok)
        {
            loginIsWrong(query_id,"Wrong account data","x coord is not a number");
            return;
        }
        quint8 y=characterQuery.value(4).toUInt(&ok);
        if(!ok)
        {
            loginIsWrong(query_id,"Wrong account data","y coord is not a number");
            return;
        }
        if(x>=GlobalServerData::serverPrivateVariables.map_list[characterQuery.value(6).toString()]->width)
        {
            loginIsWrong(query_id,"Wrong account data","x to out of map");
            return;
        }
        if(y>=GlobalServerData::serverPrivateVariables.map_list[characterQuery.value(6).toString()]->height)
        {
            loginIsWrong(query_id,"Wrong account data","y to out of map");
            return;
        }
        loginIsRightWithRescue(query_id,
            characterId,
            GlobalServerData::serverPrivateVariables.map_list[characterQuery.value(6).toString()],
            x,
            y,
            (Orientation)orentation,
                characterQuery.value(10),
                characterQuery.value(11),
                characterQuery.value(12),
                characterQuery.value(13),
                characterQuery.value(14),
                characterQuery.value(15),
                characterQuery.value(16),
                characterQuery.value(17)
        );
    }
    else
        loginIsWrong(query_id,"Map not found","Map not found: "+characterQuery.value(6).toString());
}

void ClientHeavyLoad::loginIsRightWithRescue(const quint8 &query_id, quint32 characterId, Map* map, const /*COORD_TYPE*/ quint8 &x, const /*COORD_TYPE*/ quint8 &y, const Orientation &orientation,
                  const QVariant &rescue_map, const QVariant &rescue_x, const QVariant &rescue_y, const QVariant &rescue_orientation,
                  const QVariant &unvalidated_rescue_map, const QVariant &unvalidated_rescue_x, const QVariant &unvalidated_rescue_y, const QVariant &unvalidated_rescue_orientation
                                             )
{
    if(!GlobalServerData::serverPrivateVariables.map_list.contains(rescue_map.toString()))
    {
        emit message(QString("rescue map ,not found"));
        loginIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    bool ok;
    quint8 rescue_new_x=rescue_x.toUInt(&ok);
    if(!ok)
    {
        emit message(QString("rescue x coord is not a number"));
        loginIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    quint8 rescue_new_y=rescue_y.toUInt(&ok);
    if(!ok)
    {
        emit message(QString("rescue y coord is not a number"));
        loginIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    if(rescue_new_x>=GlobalServerData::serverPrivateVariables.map_list[rescue_map.toString()]->width)
    {
        emit message(QString("rescue x to out of map"));
        loginIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    if(rescue_new_y>=GlobalServerData::serverPrivateVariables.map_list[rescue_map.toString()]->height)
    {
        emit message(QString("rescue y to out of map"));
        loginIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    QString orientationString=rescue_orientation.toString();
    Orientation rescue_new_orientation;
    if(orientationString=="top")
        rescue_new_orientation=Orientation_top;
    else if(orientationString=="bottom")
        rescue_new_orientation=Orientation_bottom;
    else if(orientationString=="left")
        rescue_new_orientation=Orientation_left;
    else if(orientationString=="right")
        rescue_new_orientation=Orientation_right;
    else
    {
        rescue_new_orientation=Orientation_bottom;
        emit message(QString("Wrong rescue orientation corrected with bottom"));
    }
    if(!GlobalServerData::serverPrivateVariables.map_list.contains(unvalidated_rescue_map.toString()))
    {
        emit message(QString("unvalidated rescue map ,not found"));
        loginIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    quint8 unvalidated_rescue_new_x=unvalidated_rescue_x.toUInt(&ok);
    if(!ok)
    {
        emit message(QString("unvalidated rescue x coord is not a number"));
        loginIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    quint8 unvalidated_rescue_new_y=unvalidated_rescue_y.toUInt(&ok);
    if(!ok)
    {
        emit message(QString("unvalidated rescue y coord is not a number"));
        loginIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    if(unvalidated_rescue_new_x>=GlobalServerData::serverPrivateVariables.map_list[rescue_map.toString()]->width)
    {
        emit message(QString("unvalidated rescue x to out of map"));
        loginIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    if(unvalidated_rescue_new_y>=GlobalServerData::serverPrivateVariables.map_list[rescue_map.toString()]->height)
    {
        emit message(QString("unvalidated rescue y to out of map"));
        loginIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    QString unvalidated_orientationString=unvalidated_rescue_orientation.toString();
    Orientation unvalidated_rescue_new_orientation;
    if(unvalidated_orientationString=="top")
        unvalidated_rescue_new_orientation=Orientation_top;
    else if(unvalidated_orientationString=="bottom")
        unvalidated_rescue_new_orientation=Orientation_bottom;
    else if(unvalidated_orientationString=="left")
        unvalidated_rescue_new_orientation=Orientation_left;
    else if(unvalidated_orientationString=="right")
        unvalidated_rescue_new_orientation=Orientation_right;
    else
    {
        unvalidated_rescue_new_orientation=Orientation_bottom;
        emit message(QString("Wrong unvalidated rescue orientation corrected with bottom"));
    }
    loginIsRightWithParsedRescue(query_id,characterId,map,x,y,orientation,
                                 GlobalServerData::serverPrivateVariables.map_list[rescue_map.toString()],rescue_new_x,rescue_new_y,rescue_new_orientation,
                                 GlobalServerData::serverPrivateVariables.map_list[unvalidated_rescue_map.toString()],unvalidated_rescue_new_x,unvalidated_rescue_new_y,unvalidated_rescue_new_orientation
            );
}

void ClientHeavyLoad::loginIsRight(const quint8 &query_id,quint32 characterId, Map *map, const quint8 &x, const quint8 &y, const Orientation &orientation)
{
    loginIsRightWithParsedRescue(query_id,characterId,map,x,y,orientation,map,x,y,orientation,map,x,y,orientation);
}

void ClientHeavyLoad::loginIsRightWithParsedRescue(const quint8 &query_id, quint32 characterId, Map* map, const /*COORD_TYPE*/ quint8 &x, const /*COORD_TYPE*/ quint8 &y, const Orientation &orientation,
                  Map* rescue_map, const /*COORD_TYPE*/ quint8 &rescue_x, const /*COORD_TYPE*/ quint8 &rescue_y, const Orientation &rescue_orientation,
                  Map *unvalidated_rescue_map, const quint8 &unvalidated_rescue_x, const quint8 &unvalidated_rescue_y, const Orientation &unvalidated_rescue_orientation
                  )
{
    //load the variables
    player_informations->character_id=characterId;
    GlobalServerData::serverPrivateVariables.connected_players_id_list << characterId;
    player_informations->public_and_private_informations.public_informations.simplifiedId = simplifiedIdList.first();
    simplifiedIdList.removeFirst();
    player_informations->character_loaded=true;
    player_informations->connectedSince=QDateTime::currentDateTime();

    loadLinkedData();

    //send the network reply
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)02;
    if(GlobalServerData::serverSettings.max_players<=255)
        out << (quint8)player_informations->public_and_private_informations.public_informations.simplifiedId;
    else
        out << (quint16)player_informations->public_and_private_informations.public_informations.simplifiedId;
    out << player_informations->public_and_private_informations.public_informations.pseudo;
    out << FacilityLib::allowToString(player_informations->public_and_private_informations.allow);
    out << (quint32)player_informations->public_and_private_informations.clan;
    if(player_informations->public_and_private_informations.clan!=0)
    {
        if(clanConnectedCount.contains(player_informations->public_and_private_informations.clan))
            clanConnectedCount[player_informations->public_and_private_informations.clan]++;
        else
        {
            clanConnectedCount[player_informations->public_and_private_informations.clan]=1;
            emit message(QString("First client of the clan: %1, get the info").arg(player_informations->public_and_private_informations.clan));
            //do the query
            QString queryText;
            switch(GlobalServerData::serverSettings.database.type)
            {
                default:
                case ServerSettings::Database::DatabaseType_Mysql:
                queryText=QString("SELECT `name`,`cash` FROM `clan` WHERE `id`=%1")
                        .arg(player_informations->public_and_private_informations.clan);
                break;
                case ServerSettings::Database::DatabaseType_SQLite:
                queryText=QString("SELECT name,cash FROM clan WHERE id=%1")
                        .arg(player_informations->public_and_private_informations.clan);
                break;
            }
            QSqlQuery clanQuery(*GlobalServerData::serverPrivateVariables.db);
            if(!clanQuery.exec(queryText))
                emit message(clanQuery.lastQuery()+": "+clanQuery.lastError().text());
            //parse the result
            if(clanQuery.next())
            {
                bool ok;
                quint64 cash=clanQuery.value(1).toULongLong(&ok);
                if(!ok)
                {
                    cash=0;
                    emit message("Warning: clan linked: %1 have wrong cash value, then reseted to 0");
                }
                emit haveClanInfo(player_informations->public_and_private_informations.clan,clanQuery.value(0).toString(),cash);
            }
            else
                emit message("Warning: clan linked: %1 is not found into db");
        }
    }
    if(player_informations->public_and_private_informations.clan_leader)
        out << (quint8)0x01;
    else
        out << (quint8)0x00;
    out << (quint64)player_informations->public_and_private_informations.cash;
    out << (quint64)player_informations->public_and_private_informations.warehouse_cash;
    out << (double)player_informations->public_and_private_informations.bitcoin;
    out << player_informations->public_and_private_informations.bitcoinAddress;
    out << (quint32)GlobalServerData::serverPrivateVariables.map_list.size();

    //temporary variable
    quint32 index;
    quint32 size;

    //send recipes
    {
        index=0;
        out << (quint32)player_informations->public_and_private_informations.recipes.size();
        QSetIterator<quint32> k(player_informations->public_and_private_informations.recipes);
        while (k.hasNext())
            out << k.next();
    }

    //send monster
    index=0;
    size=player_informations->public_and_private_informations.playerMonster.size();
    out << (quint32)size;
    while(index<size)
    {
        QByteArray data=FacilityLib::privateMonsterToBinary(player_informations->public_and_private_informations.playerMonster.at(index));
        outputData+=data;
        out.device()->seek(out.device()->pos()+data.size());
        index++;
    }
    index=0;
    size=player_informations->public_and_private_informations.warehouse_playerMonster.size();
    out << (quint32)size;
    while(index<size)
    {
        QByteArray data=FacilityLib::privateMonsterToBinary(player_informations->public_and_private_informations.warehouse_playerMonster.at(index));
        outputData+=data;
        out.device()->seek(out.device()->pos()+data.size());
        index++;
    }

    /// \todo force to 255 max
    //send reputation
    {
        out << (quint8)player_informations->public_and_private_informations.reputation.size();
        QHashIterator<QString,PlayerReputation> i(player_informations->public_and_private_informations.reputation);
        while (i.hasNext()) {
            i.next();
            out << i.key();
            out << i.value().level;
            out << i.value().point;
        }
    }

    /// \todo force to 255 max
    //send quest
    {
        out << (quint8)player_informations->public_and_private_informations.quests.size();
        QHashIterator<quint32,PlayerQuest> j(player_informations->public_and_private_informations.quests);
        while (j.hasNext()) {
            j.next();
            out << j.key();
            out << j.value().step;
            out << j.value().finish_one_time;
        }
    }

    //send bot_already_beaten
    {
        out << (quint32)player_informations->public_and_private_informations.bot_already_beaten.size();
        QSetIterator<quint32> k(player_informations->public_and_private_informations.bot_already_beaten);
        while (k.hasNext())
            out << k.next();
    }

    emit postReply(query_id,outputData);
    sendInventory();

    player_informations->rescue.map=rescue_map;
    player_informations->rescue.x=rescue_x;
    player_informations->rescue.y=rescue_y;
    player_informations->rescue.orientation=rescue_orientation;
    player_informations->unvalidated_rescue.map=unvalidated_rescue_map;
    player_informations->unvalidated_rescue.x=unvalidated_rescue_x;
    player_informations->unvalidated_rescue.y=unvalidated_rescue_y;
    player_informations->unvalidated_rescue.orientation=unvalidated_rescue_orientation;

    //send signals into the server
    emit message(QString("Logged: %1 on the map: %2 (%3,%4)").arg(player_informations->public_and_private_informations.public_informations.pseudo).arg(map->map_file).arg(x).arg(y));
    emit send_player_informations();
    emit isLogged();
    emit put_on_the_map(
                map,//map pointer
        x,
        y,
        orientation
    );
}

void ClientHeavyLoad::loginIsWrong(const quint8 &query_id,const QString &messageToSend,const QString &debugMessage)
{
    //network send
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)01;
    out << QString(messageToSend);
    emit postReply(query_id,outputData);

    //send to server to stop the connection
    emit error(debugMessage);
}

void ClientHeavyLoad::characterSelectionIsWrong(const quint8 &query_id,const QString &messageToSend,const QString &debugMessage)
{
    //network send
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)01;
    out << QString(messageToSend);
    emit postReply(query_id,outputData);

    //send to server to stop the connection
    emit error(debugMessage);
}

//load linked data (like item, quests, ...)
void ClientHeavyLoad::loadLinkedData()
{
    if(player_informations->isFake)
        return;
    loadItems();
    loadRecipes();
    loadMonsters();
    loadReputation();
    loadQuests();
    loadBotAlreadyBeaten();
}

bool ClientHeavyLoad::loadTheRawUTF8String()
{
    player_informations->rawPseudo=FacilityLib::toUTF8(player_informations->public_and_private_informations.public_informations.pseudo);
    if(player_informations->rawPseudo.isEmpty())
    {
        emit message(QString("Unable to convert the pseudo to utf8: %1").arg(player_informations->public_and_private_informations.public_informations.pseudo));
        return false;
    }
    return true;
}

void ClientHeavyLoad::askIfIsReadyToStop()
{
    if(player_informations->character_loaded)
    {
        simplifiedIdList << player_informations->public_and_private_informations.public_informations.simplifiedId;
        GlobalServerData::serverPrivateVariables.connected_players_id_list.remove(player_informations->character_id);
        if(player_informations->public_and_private_informations.clan!=0)
        {
            clanConnectedCount[player_informations->public_and_private_informations.clan]--;
            if(clanConnectedCount[player_informations->public_and_private_informations.clan]==0)
                clanConnectedCount.remove(player_informations->public_and_private_informations.clan);
        }
    }
    emit isReadyToStop();
}

//check each element of the datapack, determine if need be removed, updated, add as new file all the missing file
void ClientHeavyLoad::datapackList(const quint8 &query_id,const QStringList &files,const QList<quint64> &timestamps)
{
    tempDatapackListReplyArray.clear();
    tempDatapackListReplyTestCount=0;
    rawFiles.clear();
    compressedFiles.clear();
    rawFilesCount=0;
    compressedFilesCount=0;
    tempDatapackListReply=0;
    tempDatapackListReplySize=0;
    QHash<QString,quint64> filesList=GlobalServerData::serverPrivateVariables.filesList;

    int loopIndex=0;
    int loop_size=files.size();
    //validate, remove or update the file actualy on the client
    while(loopIndex<loop_size)
    {
        QString fileName=files.at(loopIndex);
        quint32 mtime=timestamps.at(loopIndex);
        if(fileName.contains("./") || fileName.contains("\\") || fileName.contains("//"))
        {
            emit error(QString("file name contains illegale char: %1").arg(fileName));
            return;
        }
        if(fileName.contains(QRegularExpression("^[a-zA-Z]:/")) || fileName.startsWith("/"))
        {
            emit error(QString("start with wrong string: %1").arg(fileName));
            return;
        }
        if(!filesList.contains(fileName))
            addDatapackListReply(true);//to delete
        //the file on the client is already updated
        else
        {
            if(filesList[fileName]==mtime)
                addDatapackListReply(false);//found
            else
            {
                if(sendFile(fileName,filesList[fileName]))
                    addDatapackListReply(false);//found but updated
                else
                {
                    //disconnect to prevent desync of datapack
                    emit error("Unable to open datapack file, disconnect to prevent desync of datapack");
                    return;
                }
            }
            filesList.remove(fileName);
        }
        loopIndex++;
    }
    if(tempDatapackListReplyTestCount!=files.size())
    {
        emit error("Bit count return not match");
        return;
    }
    //send not in the list
    QHashIterator<QString,quint64> i(filesList);
    while (i.hasNext()) {
        i.next();
        sendFile(i.key(),i.value());
    }
    sendFileContent();
    sendCompressedFileContent();
    purgeDatapackListReply(query_id);
}

void ClientHeavyLoad::addDatapackListReply(const bool &fileRemove)
{
    tempDatapackListReplyTestCount++;
    switch(tempDatapackListReplySize)
    {
        case 0:
            if(fileRemove)
                tempDatapackListReply|=0x01;
            else
                tempDatapackListReply&=~0x01;
        break;
        case 1:
            if(fileRemove)
                tempDatapackListReply|=0x02;
            else
                tempDatapackListReply&=~0x02;
        break;
        case 2:
            if(fileRemove)
                tempDatapackListReply|=0x04;
            else
                tempDatapackListReply&=~0x04;
        break;
        case 3:
            if(fileRemove)
                tempDatapackListReply|=0x08;
            else
                tempDatapackListReply&=~0x08;
        break;
        case 4:
            if(fileRemove)
                tempDatapackListReply|=0x10;
            else
                tempDatapackListReply&=~0x10;
        break;
        case 5:
            if(fileRemove)
                tempDatapackListReply|=0x20;
            else
                tempDatapackListReply&=~0x20;
        break;
        case 6:
            if(fileRemove)
                tempDatapackListReply|=0x40;
            else
                tempDatapackListReply&=~0x40;
        break;
        case 7:
            if(fileRemove)
                tempDatapackListReply|=0x80;
            else
                tempDatapackListReply&=~0x80;
        break;
        default:
        break;
    }
    tempDatapackListReplySize++;
    if(tempDatapackListReplySize>=8)
    {
        tempDatapackListReplyArray[tempDatapackListReplyArray.size()]=tempDatapackListReply;
        tempDatapackListReplySize=0;
        tempDatapackListReply=0;
    }
}

void ClientHeavyLoad::purgeDatapackListReply(const quint8 &query_id)
{
    if(tempDatapackListReplySize>0)
    {
        tempDatapackListReplyArray[tempDatapackListReplyArray.size()]=tempDatapackListReply;
        tempDatapackListReplySize=0;
        tempDatapackListReply=0;
    }
    if(tempDatapackListReplyArray.isEmpty())
        tempDatapackListReplyArray[0x00]=0x00;
    emit postReply(query_id,tempDatapackListReplyArray);
    tempDatapackListReplyArray.clear();
}

void ClientHeavyLoad::sendFileContent()
{
    if(rawFiles.size()>0 && rawFilesCount>0)
    {
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);
        out << (quint8)rawFilesCount;
        emit sendFullPacket(0xC2,0x0003,outputData+rawFiles);
        rawFiles.clear();
        rawFilesCount=0;
    }
}

void ClientHeavyLoad::sendCompressedFileContent()
{
    if(compressedFiles.size()>0 && compressedFilesCount>0)
    {
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);
        out << (quint8)compressedFilesCount;
        emit sendFullPacket(0xC2,0x0004,outputData+compressedFiles);
        compressedFiles.clear();
        compressedFilesCount=0;
    }
}

bool ClientHeavyLoad::sendFile(const QString &fileName,const quint64 &mtime)
{
    if(fileName.size()>255 || fileName.isEmpty())
        return false;
    QByteArray fileNameRaw=FacilityLib::toUTF8(fileName);
    if(fileNameRaw.size()>255 || fileNameRaw.isEmpty())
        return false;
    QFile file(GlobalServerData::serverSettings.datapack_basePath+fileName);
    if(file.open(QIODevice::ReadOnly))
    {
        const QByteArray &content=file.readAll();
        /*emit message(QString("send the file: %1, checkMtime: %2, mtime: %3, file server mtime: %4")
                 .arg(fileName)
                 .arg(checkMtime)
                 .arg(mtime)
                 .arg(localMtime)
        );*/
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);
        out << (quint32)content.size();
        out << mtime;
        if(content.size()>CATCHCHALLENGER_SERVER_DATAPACK_FILEPURGE_KB*1024)
        {
            QByteArray outputData2;
            outputData2[0x00]=0x01;
            emit sendFullPacket(0xC2,0x0003,outputData2+fileNameRaw+outputData+content);
        }
        else
        {
            if(compressedExtension.contains(QFileInfo(file).suffix()))
            {
                compressedFiles+=fileNameRaw+outputData+content;
                compressedFilesCount++;
                switch(ProtocolParsing::compressionType)
                {
                    case ProtocolParsing::CompressionType_Xz:
                    if(compressedFiles.size()>CATCHCHALLENGER_SERVER_DATAPACK_XZ_COMPRESSEDFILEPURGE_KB*1024 || compressedFilesCount>=255)
                        sendCompressedFileContent();
                    break;
                    default:
                    case ProtocolParsing::CompressionType_Zlib:
                    if(compressedFiles.size()>CATCHCHALLENGER_SERVER_DATAPACK_ZLIB_COMPRESSEDFILEPURGE_KB*1024 || compressedFilesCount>=255)
                        sendCompressedFileContent();
                    break;
                    case ProtocolParsing::CompressionType_None:
                    if(compressedFiles.size()>CATCHCHALLENGER_SERVER_DATAPACK_FILEPURGE_KB*1024 || compressedFilesCount>=255)
                        sendCompressedFileContent();
                    break;
                }
            }
            else
            {
                rawFiles+=fileNameRaw+outputData+content;
                rawFilesCount++;
                if(rawFiles.size()>CATCHCHALLENGER_SERVER_DATAPACK_FILEPURGE_KB*1024 || rawFilesCount>=255)
                    sendFileContent();
            }
        }
        file.close();
        return true;
    }
    else
        return false;
}

QString ClientHeavyLoad::SQL_text_quote(QString text)
{
    return text.replace("'","\\'");
}

void ClientHeavyLoad::dbQuery(const QString &queryText)
{
    if(player_informations->isFake)
    {
        emit message(QString("Query canceled because is fake: %1").arg(queryText));
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_SQL
    emit message("Do mysql query: "+queryText);
    #endif
    QSqlQuery sqlQuery(*GlobalServerData::serverPrivateVariables.db);
    if(!sqlQuery.exec(queryText))
        emit message(sqlQuery.lastQuery()+": "+sqlQuery.lastError().text());
    GlobalServerData::serverPrivateVariables.db->commit();//to have data coerancy and prevent data lost on crash
}

void ClientHeavyLoad::loadReputation()
{
    //do the query
    QString queryText;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
        queryText=QString("SELECT `type`,`point`,`level` FROM `reputation` WHERE `character`=%1")
                .arg(player_informations->character_id);
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
        queryText=QString("SELECT `type`,`point`,level FROM reputation WHERE character=%1")
                .arg(player_informations->character_id);
        break;
    }
    bool ok;
    QSqlQuery reputationQuery(*GlobalServerData::serverPrivateVariables.db);
    if(!reputationQuery.exec(queryText))
        emit message(reputationQuery.lastQuery()+": "+reputationQuery.lastError().text());
    //parse the result
    while(reputationQuery.next())
    {
        QString type=reputationQuery.value(0).toString();
        qint32 point=reputationQuery.value(1).toInt(&ok);
        if(!ok)
        {
            emit message(QString("point is not a number, skip: %1").arg(type));
            continue;
        }
        qint32 level=reputationQuery.value(2).toInt(&ok);
        if(!ok)
        {
            emit message(QString("level is not a number, skip: %1").arg(type));
            continue;
        }
        if(level<-100 || level>100)
        {
            emit message(QString("level is <100 or >100, skip: %1").arg(type));
            continue;
        }
        if(!CommonDatapack::commonDatapack.reputation.contains(type))
        {
            emit message(QString("The reputation: %1 don't exist").arg(type));
            continue;
        }
        if(level>=0)
        {
            if(level>=CommonDatapack::commonDatapack.reputation[type].reputation_positive.size())
            {
                emit message(QString("The reputation level %1 is wrong because is out of range (reputation level: %2 > max level: %3)").arg(type).arg(level).arg(CommonDatapack::commonDatapack.reputation[type].reputation_positive.size()));
                continue;
            }
        }
        else
        {
            if((-level)>CommonDatapack::commonDatapack.reputation[type].reputation_negative.size())
            {
                emit message(QString("The reputation level %1 is wrong because is out of range (reputation level: %2 < max level: %3)").arg(type).arg(level).arg(CommonDatapack::commonDatapack.reputation[type].reputation_negative.size()));
                continue;
            }
        }
        if(point>0)
        {
            if(CommonDatapack::commonDatapack.reputation[type].reputation_positive.size()==(level+1))//start at level 0 in positive
            {
                emit message(QString("The reputation level is already at max, drop point"));
                point=0;
            }
            if(point>=CommonDatapack::commonDatapack.reputation[type].reputation_positive.at(level+1))//start at level 0 in positive
            {
                emit message(QString("The reputation point %1 is greater than max %2").arg(point).arg(CommonDatapack::commonDatapack.reputation[type].reputation_positive.at(level)));
                continue;
            }
        }
        else if(point<0)
        {
            if(CommonDatapack::commonDatapack.reputation[type].reputation_negative.size()==-level)//start at level -1 in negative
            {
                emit message(QString("The reputation level is already at min, drop point"));
                point=0;
            }
            if(point<CommonDatapack::commonDatapack.reputation[type].reputation_negative.at(-level))//start at level -1 in negative
            {
                emit message(QString("The reputation point %1 is greater than max %2").arg(point).arg(CommonDatapack::commonDatapack.reputation[type].reputation_negative.at(level)));
                continue;
            }
        }
        player_informations->public_and_private_informations.reputation[type].level=level;
        player_informations->public_and_private_informations.reputation[type].point=point;
    }
}

void ClientHeavyLoad::loadQuests()
{
    //do the query
    QString queryText;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
        queryText=QString("SELECT `quest`,`finish_one_time`,`step` FROM `quest` WHERE `character`=%1")
                .arg(player_informations->character_id);
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
        queryText=QString("SELECT quest,finish_one_time,step FROM quest WHERE character=%1")
                .arg(player_informations->character_id);
        break;
    }
    bool ok,ok2;
    QSqlQuery questsQuery(*GlobalServerData::serverPrivateVariables.db);
    if(!questsQuery.exec(queryText))
        emit message(questsQuery.lastQuery()+": "+questsQuery.lastError().text());
    //parse the result
    while(questsQuery.next())
    {
        PlayerQuest playerQuest;
        quint32 id=questsQuery.value(0).toUInt(&ok);
        playerQuest.finish_one_time=questsQuery.value(1).toBool();
        playerQuest.step=questsQuery.value(2).toUInt(&ok2);
        if(!ok || !ok2)
        {
            emit message(QString("wrong value type for quest, skip: %1").arg(id));
            continue;
        }
        if(!CommonDatapack::commonDatapack.quests.contains(id))
        {
            emit message(QString("quest is not into the quests list, skip: %1").arg(id));
            continue;
        }
        if((playerQuest.step<=0 && !playerQuest.finish_one_time) || playerQuest.step>CommonDatapack::commonDatapack.quests[id].steps.size())
        {
            emit message(QString("step out of quest range, skip: %1").arg(id));
            continue;
        }
        if(playerQuest.step<=0 && !playerQuest.finish_one_time)
        {
            emit message(QString("can't be to step 0 if have never finish the quest, skip: %1").arg(id));
            continue;
        }
        player_informations->public_and_private_informations.quests[id]=playerQuest;
        LocalClientHandler::addQuestStepDrop(player_informations,id,playerQuest.step);
    }
}
