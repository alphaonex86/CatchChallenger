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
QSet<QString> ClientHeavyLoad::extensionAllowed;
QHash<quint32,quint16> ClientHeavyLoad::clanConnectedCount;
quint64 ClientHeavyLoad::datapack_list_cache_timestamp;
QHash<QString,quint32> ClientHeavyLoad::datapack_file_list_cache;
QRegularExpression ClientHeavyLoad::fileNameStartStringRegex=QRegularExpression(QLatin1String("^[a-zA-Z]:/"));
QString ClientHeavyLoad::single_quote=QLatin1Literal("'");
QString ClientHeavyLoad::antislash_single_quote=QLatin1Literal("\\'");

QString ClientHeavyLoad::text_dotslash=QLatin1Literal("./");
QString ClientHeavyLoad::text_slash=QLatin1Literal("/");
QString ClientHeavyLoad::text_double_slash=QLatin1Literal("//");
QString ClientHeavyLoad::text_antislash=QLatin1Literal("\\");
QString ClientHeavyLoad::text_dotcomma=QLatin1Literal(";");
QString ClientHeavyLoad::text_normal=QLatin1Literal("normal");
QString ClientHeavyLoad::text_premium=QLatin1Literal("premium");
QString ClientHeavyLoad::text_gm=QLatin1Literal("gm");
QString ClientHeavyLoad::text_dev=QLatin1Literal("dev");
QString ClientHeavyLoad::text_top=QLatin1Literal("top");
QString ClientHeavyLoad::text_bottom=QLatin1Literal("bottom");
QString ClientHeavyLoad::text_left=QLatin1Literal("left");
QString ClientHeavyLoad::text_right=QLatin1Literal("right");
QString ClientHeavyLoad::text_dottmx=QLatin1Literal(".tmx");
QString ClientHeavyLoad::text_unknown=QLatin1Literal("unknown");
QString ClientHeavyLoad::text_female=QLatin1Literal("female");
QString ClientHeavyLoad::text_male=QLatin1Literal("male");
QString ClientHeavyLoad::text_warehouse=QLatin1Literal("warehouse");
QString ClientHeavyLoad::text_wear=QLatin1Literal("wear");
QString ClientHeavyLoad::text_market=QLatin1Literal("market");

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
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(GlobalServerData::serverPrivateVariables.db_query_login.isEmpty())
    {
        emit error(QStringLiteral("askLogin() Query login is empty, bug"));
        return;
    }
    if(GlobalServerData::serverPrivateVariables.db_query_insert_login.isEmpty())
    {
        emit error(QStringLiteral("askLogin() Query inset login is empty, bug"));
        return;
    }
    if(GlobalServerData::serverPrivateVariables.db_query_characters.isEmpty())
    {
        emit error(QStringLiteral("askLogin() Query characters is empty, bug"));
        return;
    }
    #endif
    QByteArray login,pass;
    {
        QCryptographicHash hash(QCryptographicHash::Sha224);
        hash.addData(login_org.toHex());
        login=hash.result();
        QCryptographicHash hash2(QCryptographicHash::Sha224);
        hash2.addData(pass_org.toHex());
        pass=hash2.result();
    }
    {
        QSqlQuery accountQuery(*GlobalServerData::serverPrivateVariables.db);
        if(!accountQuery.exec(GlobalServerData::serverPrivateVariables.db_query_login.arg(QString(login.toHex()))))
            emit message(accountQuery.lastQuery()+": "+accountQuery.lastError().text());
        bool ok;
        if(!accountQuery.next())
        {
            if(GlobalServerData::serverSettings.automatic_account_creation)
            {
                GlobalServerData::serverPrivateVariables.maxAccountId++;
                player_informations->account_id=GlobalServerData::serverPrivateVariables.maxAccountId;
                dbQuery(GlobalServerData::serverPrivateVariables.db_query_insert_login.arg(player_informations->account_id).arg(QString(login.toHex())).arg(QString(pass.toHex())).arg(QDateTime::currentDateTime().toTime_t()));
            }
            else
            {
                loginIsWrong(query_id,"Bad login",QStringLiteral("Bad login for: %1 (%2), pass: %3 (%4)")
                             .arg(QString(login.toHex()))
                             .arg(QString(login_org.toHex()))
                              .arg(QString(pass.toHex()))
                              .arg(QString(pass_org.toHex()))
                              );
                return;
            }
        }
        else if(QString(pass.toHex())!=accountQuery.value(1).toString())
        {
            loginIsWrong(query_id,"Wrong login/pass",QStringLiteral("Password wrong: %1 (%2) for the login: %3").arg(QString(pass.toHex())).arg(QString(pass_org.toHex())).arg(QString(login.toHex())));
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
    emit message(QStringLiteral("Logged the account %1").arg(player_informations->account_id));
    //send the network reply
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)02;
    if(GlobalServerData::serverSettings.sendPlayerNumber)
        out << (quint16)GlobalServerData::serverSettings.max_players;
    else
    {
        if(GlobalServerData::serverSettings.max_players<=255)
            out << (quint16)255;
        else
            out << (quint16)65535;
    }
    if(GlobalServerData::serverPrivateVariables.timer_city_capture==NULL)
        out << (quint32)0x00000000;
    else if(GlobalServerData::serverPrivateVariables.timer_city_capture->isActive())
    {
        const qint64 &time=GlobalServerData::serverPrivateVariables.time_city_capture.toMSecsSinceEpoch()-QDateTime::currentMSecsSinceEpoch();
        out << (quint32)time/1000;
    }
    else
        out << (quint32)0x00000000;
    out << (quint8)GlobalServerData::serverSettings.city.capture.frenquency;
    out << (quint16)GlobalServerData::serverPrivateVariables.maxVisiblePlayerAtSameTime;

    //common settings
    out << (quint8)CommonSettings::commonSettings.forceClientToSendAtMapChange;
    out << (quint8)CommonSettings::commonSettings.forcedSpeed;
    out << (quint8)CommonSettings::commonSettings.dontSendPseudo;
    out << (quint8)CommonSettings::commonSettings.max_character;
    out << (quint8)CommonSettings::commonSettings.min_character;
    out << (quint8)CommonSettings::commonSettings.max_pseudo_size;
    out << (quint32)CommonSettings::commonSettings.character_delete_time;
    out << (float)CommonSettings::commonSettings.rates_xp;
    out << (float)CommonSettings::commonSettings.rates_gold;
    out << (float)CommonSettings::commonSettings.rates_xp_pow;
    out << (float)CommonSettings::commonSettings.rates_drop;
    out << (quint8)CommonSettings::commonSettings.chat_allow_all;
    out << (quint8)CommonSettings::commonSettings.chat_allow_local;
    out << (quint8)CommonSettings::commonSettings.chat_allow_private;
    out << (quint8)CommonSettings::commonSettings.chat_allow_clan;
    out << (quint8)CommonSettings::commonSettings.factoryPriceChange;

    {
        QSqlQuery characterQuery(*GlobalServerData::serverPrivateVariables.db);
        if(!characterQuery.exec(GlobalServerData::serverPrivateVariables.db_query_characters.arg(player_informations->account_id).arg(CommonSettings::commonSettings.max_character)))
            emit message(characterQuery.lastQuery()+": "+characterQuery.lastError().text());
        const quint64 &current_time=QDateTime::currentDateTime().toTime_t();
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
                    emit message(QStringLiteral("time_to_delete is not number: %1 for %2 fixed by 0").arg(characterQuery.value(3).toString()).arg(player_informations->account_id));
                    time_to_delete=0;
                }
                characterEntry.played_time=characterQuery.value(4).toUInt(&ok);
                if(!ok)
                {
                    emit message(QStringLiteral("played_time is not number: %1 for %2 fixed by 0").arg(characterQuery.value(4).toString()).arg(player_informations->account_id));
                    characterEntry.played_time=0;
                }
                characterEntry.last_connect=characterQuery.value(5).toUInt(&ok);
                if(!ok)
                {
                    emit message(QStringLiteral("last_connect is not number: %1 for %2 fixed by 0").arg(characterQuery.value(5).toString()).arg(player_informations->account_id));
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
                emit message(QStringLiteral("Character id is not number: %1 for %2").arg(characterQuery.value(0).toString()).arg(player_informations->account_id));
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
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(GlobalServerData::serverPrivateVariables.db_query_monster_by_character_id.isEmpty())
    {
        emit error(QStringLiteral("deleteCharacterNow() Query is empty, bug"));
        return;
    }
    if(GlobalServerData::serverPrivateVariables.db_query_delete_monster_buff.isEmpty())
    {
        emit error(QStringLiteral("deleteCharacterNow() Query db_query_delete_monster_buff is empty, bug"));
        return;
    }
    if(GlobalServerData::serverPrivateVariables.db_query_delete_monster_skill.isEmpty())
    {
        emit error(QStringLiteral("deleteCharacterNow() Query db_query_delete_monster_skill is empty, bug"));
        return;
    }
    if(GlobalServerData::serverPrivateVariables.db_query_delete_bot_already_beaten.isEmpty())
    {
        emit error(QStringLiteral("deleteCharacterNow() Query db_query_delete_bot_already_beaten is empty, bug"));
        return;
    }
    if(GlobalServerData::serverPrivateVariables.db_query_delete_character.isEmpty())
    {
        emit error(QStringLiteral("deleteCharacterNow() Query db_query_delete_character is empty, bug"));
        return;
    }
    if(GlobalServerData::serverPrivateVariables.db_query_delete_item.isEmpty())
    {
        emit error(QStringLiteral("deleteCharacterNow() Query db_query_delete_item is empty, bug"));
        return;
    }
    if(GlobalServerData::serverPrivateVariables.db_query_delete_monster.isEmpty())
    {
        emit error(QStringLiteral("deleteCharacterNow() Query db_query_delete_monster is empty, bug"));
        return;
    }
    if(GlobalServerData::serverPrivateVariables.db_query_delete_plant.isEmpty())
    {
        emit error(QStringLiteral("deleteCharacterNow() Query db_query_delete_plant is empty, bug"));
        return;
    }
    if(GlobalServerData::serverPrivateVariables.db_query_delete_quest.isEmpty())
    {
        emit error(QStringLiteral("deleteCharacterNow() Query db_query_delete_quest is empty, bug"));
        return;
    }
    if(GlobalServerData::serverPrivateVariables.db_query_delete_recipes.isEmpty())
    {
        emit error(QStringLiteral("deleteCharacterNow() Query db_query_delete_recipes is empty, bug"));
        return;
    }
    if(GlobalServerData::serverPrivateVariables.db_query_delete_reputation.isEmpty())
    {
        emit error(QStringLiteral("deleteCharacterNow() Query db_query_delete_reputation is empty, bug"));
        return;
    }
    #endif
    bool ok;
    QSqlQuery monstersQuery(*GlobalServerData::serverPrivateVariables.db);
    if(!monstersQuery.exec(GlobalServerData::serverPrivateVariables.db_query_monster_by_character_id.arg(characterId)))
        emit message(monstersQuery.lastQuery()+": "+monstersQuery.lastError().text());
    while(monstersQuery.next())
    {
        const quint32 &monsterId=monstersQuery.value(0).toUInt(&ok);
        if(ok)
        {
            dbQuery(GlobalServerData::serverPrivateVariables.db_query_delete_monster_buff.arg(monsterId));
            dbQuery(GlobalServerData::serverPrivateVariables.db_query_delete_monster_skill.arg(monsterId));
        }
    }
    dbQuery(GlobalServerData::serverPrivateVariables.db_query_delete_bot_already_beaten.arg(characterId));
    dbQuery(GlobalServerData::serverPrivateVariables.db_query_delete_character.arg(characterId));
    dbQuery(GlobalServerData::serverPrivateVariables.db_query_delete_item.arg(characterId));
    dbQuery(GlobalServerData::serverPrivateVariables.db_query_delete_monster.arg(characterId));
    dbQuery(GlobalServerData::serverPrivateVariables.db_query_delete_plant.arg(characterId));
    dbQuery(GlobalServerData::serverPrivateVariables.db_query_delete_quest.arg(characterId));
    dbQuery(GlobalServerData::serverPrivateVariables.db_query_delete_recipes.arg(characterId));
    dbQuery(GlobalServerData::serverPrivateVariables.db_query_delete_reputation.arg(characterId));
}

void ClientHeavyLoad::addCharacter(const quint8 &query_id, const quint8 &profileIndex, const QString &pseudo, const QString &skin)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(GlobalServerData::serverPrivateVariables.db_query_select_character_by_pseudo.isEmpty())
    {
        emit error(QStringLiteral("addCharacter() Query is empty, bug"));
        return;
    }
    if(GlobalServerData::serverPrivateVariables.db_query_insert_monster.isEmpty())
    {
        emit error(QStringLiteral("addCharacter() Query db_query_insert_monster is empty, bug"));
        return;
    }
    if(GlobalServerData::serverPrivateVariables.db_query_insert_monster_skill.isEmpty())
    {
        emit error(QStringLiteral("addCharacter() Query db_query_insert_monster_skill is empty, bug"));
        return;
    }
    if(GlobalServerData::serverPrivateVariables.db_query_insert_reputation.isEmpty())
    {
        emit error(QStringLiteral("addCharacter() Query db_query_insert_reputation is empty, bug"));
        return;
    }
    if(GlobalServerData::serverPrivateVariables.db_query_insert_item.isEmpty())
    {
        emit error(QStringLiteral("addCharacter() Query db_query_insert_item is empty, bug"));
        return;
    }
    #endif
    if(player_informations->number_of_character>=CommonSettings::commonSettings.max_character)
    {
        emit error(QStringLiteral("You can't create more account, you have already %1 on %2 allowed").arg(player_informations->number_of_character).arg(CommonSettings::commonSettings.max_character));
        return;
    }
    if(profileIndex>=CommonDatapack::commonDatapack.profileList.size())
    {
        emit error(QStringLiteral("profile index: %1 out of range (profileList size: %2)").arg(profileIndex).arg(CommonDatapack::commonDatapack.profileList.size()));
        return;
    }
    if(pseudo.size()>CommonSettings::commonSettings.max_pseudo_size)
    {
        emit error(QStringLiteral("pseudo size is too big: %1 because is greater than %2").arg(pseudo.size()).arg(CommonSettings::commonSettings.max_pseudo_size));
        return;
    }
    const Profile &profile=CommonDatapack::commonDatapack.profileList.at(profileIndex);
    if(!profile.forcedskin.isEmpty() && !profile.forcedskin.contains(skin))
    {
        emit error(QStringLiteral("skin provided: %1 is not into profile forced skin list: %2").arg(skin).arg(profile.forcedskin.join(";")));
        return;
    }
    if(!GlobalServerData::serverPrivateVariables.skinList.contains(skin))
    {
        emit error(QStringLiteral("skin provided: %1 is not into skin listed").arg(skin));
        return;
    }

    //check
    QSqlQuery monstersQuery(*GlobalServerData::serverPrivateVariables.db);
    if(!monstersQuery.exec(GlobalServerData::serverPrivateVariables.db_query_select_character_by_pseudo.arg(SqlFunction::quoteSqlVariable(pseudo))))
    {
        emit message(monstersQuery.lastQuery()+": "+monstersQuery.lastError().text());
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);
        out << (quint32)0x00000000;
        emit postReply(query_id,outputData);
        return;
    }
    if(monstersQuery.next())
    {
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);
        out << (quint32)0x00000000;
        emit postReply(query_id,outputData);
        return;
    }

    player_informations->number_of_character++;
    GlobalServerData::serverPrivateVariables.maxCharacterId++;

    const quint32 &characterId=GlobalServerData::serverPrivateVariables.maxCharacterId;
    int index=0;
    int monster_position=1;
    {
        const QString &mapQuery=ClientHeavyLoad::single_quote+profile.map+QLatin1String("',")+QString::number(profile.x)+QLatin1String(",")+QString::number(profile.y)+QLatin1String(",'bottom'");
        switch(GlobalServerData::serverSettings.database.type)
        {
            default:
            case ServerSettings::Database::DatabaseType_Mysql:
                dbQuery(QStringLiteral("INSERT INTO `character`(`id`,`account`,`pseudo`,`skin`,`map`,`x`,`y`,`orientation`,`type`,`clan`,`cash`,`rescue_map`,`rescue_x`,`rescue_y`,`rescue_orientation`,`unvalidated_rescue_map`,`unvalidated_rescue_x`,`unvalidated_rescue_y`,`unvalidated_rescue_orientation`,`market_cash`,`market_bitcoin`,`date`,`warehouse_cash`,`allow`,`clan_leader`,`bitcoin_offset`,`time_to_delete`,`played_time`,`last_connect`,`starter`) VALUES(")+
                        QString::number(characterId)+QLatin1String(",")+
                        QString::number(player_informations->account_id)+QLatin1String(",'")+
                        pseudo+QLatin1String("','")+
                        skin+QLatin1String("',")+
                        mapQuery+QLatin1String(",'normal',0,")+
                        QString::number(profile.cash)+QLatin1String(",")+
                        mapQuery+QLatin1String(",")+
                        mapQuery+QLatin1String(",0,0,")+
                        QString::number(QDateTime::currentDateTime().toTime_t())+QLatin1String(",0,'',0,0,0,0,0,")+
                        QString::number(profileIndex)+QLatin1String(");"));
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                dbQuery(QStringLiteral("INSERT INTO `character`(`id`,`account`,`pseudo`,`skin`,`map`,`x`,`y`,`orientation`,`type`,`clan`,`cash`,`rescue_map`,`rescue_x`,`rescue_y`,`rescue_orientation`,`unvalidated_rescue_map`,`unvalidated_rescue_x`,`unvalidated_rescue_y`,`unvalidated_rescue_orientation`,`market_cash`,`market_bitcoin`,`date`,`warehouse_cash`,`allow`,`clan_leader`,`bitcoin_offset`,`time_to_delete`,`played_time`,`last_connect`,`starter`) VALUES(")+
                        QString::number(characterId)+QLatin1String(",")+
                        QString::number(player_informations->account_id)+QLatin1String(",'")+
                        pseudo+QLatin1String("','")+
                        skin+QLatin1String("',")+
                        mapQuery+QLatin1String(",'normal',0,")+
                        QString::number(profile.cash)+QLatin1String(",")+
                        mapQuery+QLatin1String(",")+
                        mapQuery+QLatin1String(",0,0,")+
                        QString::number(QDateTime::currentDateTime().toTime_t())+QLatin1String(",0,'',0,0,0,0,0,")+
                        QString::number(profileIndex)+QLatin1String(");"));
            break;
        }
    }
    while(index<profile.monsters.size())
    {
        const quint32 &monsterId=profile.monsters.at(index).id;
        if(CatchChallenger::CommonDatapack::commonDatapack.monsters.contains(monsterId))
        {
            const Monster &monster=CatchChallenger::CommonDatapack::commonDatapack.monsters.value(monsterId);
            QString gender=ClientHeavyLoad::text_unknown;
            if(monster.ratio_gender!=-1)
            {
                if(rand()%101<monster.ratio_gender)
                    gender=ClientHeavyLoad::text_female;
                else
                    gender=ClientHeavyLoad::text_male;
            }
            CatchChallenger::Monster::Stat stat=CatchChallenger::CommonFightEngine::getStat(monster,profile.monsters.at(index).level);
            QList<CatchChallenger::PlayerMonster::PlayerSkill> skills;
            QList<CatchChallenger::Monster::AttackToLearn> attack=monster.learn;
            int sub_index=0;
            while(sub_index<attack.size())
            {
                if(attack.value(sub_index).learnAtLevel<=profile.monsters.at(index).level)
                {
                    CatchChallenger::PlayerMonster::PlayerSkill temp;
                    temp.level=attack.value(sub_index).learnSkillLevel;
                    temp.skill=attack.value(sub_index).learnSkill;
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
                dbQuery(GlobalServerData::serverPrivateVariables.db_query_insert_monster
                   .arg(monster_id)
                   .arg(stat.hp)
                   .arg(characterId)
                   .arg(monsterId)
                   .arg(profile.monsters.at(index).level)
                   .arg(profile.monsters.at(index).captured_with)
                   .arg(gender)
                   .arg(monster_position)
                    );
                monster_position++;
            }
            sub_index=0;
            while(sub_index<skills.size())
            {
                quint8 endurance=0;
                if(CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.contains(skills.value(sub_index).skill))
                    if(skills.value(sub_index).level<=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.value(skills.value(sub_index).skill).level.size() && skills.value(sub_index).level>0)
                        endurance=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.value(skills.value(sub_index).skill).level.at(skills.value(sub_index).level-1).endurance;
                dbQuery(GlobalServerData::serverPrivateVariables.db_query_insert_monster_skill
                   .arg(monster_id)
                   .arg(skills.value(sub_index).skill)
                   .arg(skills.value(sub_index).level)
                   .arg(endurance)
                        );
                sub_index++;
            }
            index++;
        }
        else
        {
            emit error(QStringLiteral("monster not found to start: %1 is not into profile forced skin list: %2").arg(monsterId));
            return;
        }
    }
    index=0;
    while(index<profile.reputation.size())
    {
        dbQuery(GlobalServerData::serverPrivateVariables.db_query_insert_reputation
           .arg(characterId)
           .arg(profile.reputation.at(index).type)
           .arg(profile.reputation.at(index).point)
           .arg(profile.reputation.at(index).level)
                );
        index++;
    }
    index=0;
    while(index<profile.items.size())
    {
        dbQuery(GlobalServerData::serverPrivateVariables.db_query_insert_item
           .arg(profile.items.at(index).id)
           .arg(characterId)
           .arg(profile.items.at(index).quantity)
                );
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
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(GlobalServerData::serverPrivateVariables.db_query_account_time_to_delete_character_by_id.isEmpty())
    {
        emit error(QStringLiteral("removeCharacter() Query is empty, bug"));
        return;
    }
    if(GlobalServerData::serverPrivateVariables.db_query_update_character_time_to_delete_by_id.isEmpty())
    {
        emit error(QStringLiteral("removeCharacter() Query db_query_update_character_time_to_delete_by_id is empty, bug"));
        return;
    }
    #endif
    QSqlQuery characterQuery(*GlobalServerData::serverPrivateVariables.db);
    if(!characterQuery.exec(GlobalServerData::serverPrivateVariables.db_query_account_time_to_delete_character_by_id.arg(characterId)))
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
        characterSelectionIsWrong(query_id,"Character not found",QStringLiteral("Account for character: %1 is not an id").arg(characterQuery.value(0).toString()));
        return;
    }
    if(player_informations->account_id!=account_id)
    {
        characterSelectionIsWrong(query_id,"Character not found",QStringLiteral("Character: %1 is not owned by the account: %2").arg(characterId).arg(player_informations->account_id));
        return;
    }
    const quint32 &time_to_delete=characterQuery.value(1).toUInt(&ok);
    if(ok && time_to_delete>0)
    {
        characterSelectionIsWrong(query_id,"Already in deleting",QStringLiteral("Character: %1 is already in deleting for the account: %2").arg(characterId).arg(player_informations->account_id));
        return;
    }
    dbQuery(GlobalServerData::serverPrivateVariables.db_query_update_character_time_to_delete_by_id.arg(characterId).arg(CommonSettings::commonSettings.character_delete_time));
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x02;
    emit postReply(query_id,outputData);
}

void ClientHeavyLoad::selectCharacter(const quint8 &query_id, const quint32 &characterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(GlobalServerData::serverPrivateVariables.db_query_character_by_id.isEmpty())
    {
        emit error(QStringLiteral("selectCharacter() Query is empty, bug"));
        return;
    }
    if(GlobalServerData::serverPrivateVariables.db_query_update_character_last_connect.isEmpty())
    {
        emit error(QStringLiteral("selectCharacter() Query db_query_update_character_last_connect is empty, bug"));
        return;
    }
    if(GlobalServerData::serverPrivateVariables.db_query_update_character_time_to_delete.isEmpty())
    {
        emit error(QStringLiteral("selectCharacter() Query db_query_update_character_time_to_delete is empty, bug"));
        return;
    }
    #endif
    /*account(0),pseudo(1),skin(2),x(3),y(4),orientation(5),map(6),type(7),clan(8),cash(9),
    rescue_map(10),rescue_x(11),rescue_y(12),rescue_orientation(13),unvalidated_rescue_map(14),
    unvalidated_rescue_x(15),unvalidated_rescue_y(16),unvalidated_rescue_orientation(17),
    warehouse_cash(18),allow(19),clan_leader(20),bitcoin_offset(21),market_cash(22),market_bitcoin(23),
    time_to_delete(24),*/
    QSqlQuery characterQuery(*GlobalServerData::serverPrivateVariables.db);
    if(!characterQuery.exec(GlobalServerData::serverPrivateVariables.db_query_character_by_id.arg(characterId)))
    {
        characterSelectionIsWrong(query_id,QLatin1String("Character not found"),characterQuery.lastQuery()+QLatin1String(": ")+characterQuery.lastError().text());
        return;
    }
    if(!characterQuery.next())
    {
        characterSelectionIsWrong(query_id,QLatin1String("Character not found"),QLatin1String("Result return query wrong"));
        return;
    }
    bool ok;
    const quint32 &account_id=characterQuery.value(0).toUInt(&ok);
    if(!ok)
    {
        characterSelectionIsWrong(query_id,QLatin1String("Character not found"),QStringLiteral("Account for character: %1 is not an id").arg(characterQuery.value(0).toString()));
        return;
    }
    if(player_informations->account_id!=account_id)
    {
        characterSelectionIsWrong(query_id,QLatin1String("Character not found"),QStringLiteral("Character: %1 is not owned by the account: %2").arg(characterId).arg(player_informations->account_id));
        return;
    }
    if(player_informations->character_loaded)
    {
        characterSelectionIsWrong(query_id,QLatin1String("Already logged"),QLatin1String("character_loaded already to true"));
        return;
    }
    if(GlobalServerData::serverPrivateVariables.connected_players_id_list.contains(characterId))
    {
        characterSelectionIsWrong(query_id,QLatin1String("Already logged"),QLatin1String("Already logged"));
        return;
    }
    if(simplifiedIdList.size()<=0)
    {
        characterSelectionIsWrong(query_id,QLatin1String("Not free id to login"),QLatin1String("Not free id to login"));
        return;
    }
    if(!loadTheRawUTF8String())
    {
        if(GlobalServerData::serverSettings.anonymous)
            characterSelectionIsWrong(query_id,QLatin1String("Convert into utf8 have wrong size"),QStringLiteral("Unable to convert the pseudo to utf8 for character id: %1").arg(player_informations->character_id));
        else
            characterSelectionIsWrong(query_id,QLatin1String("Convert into utf8 have wrong size"),QStringLiteral("Unable to convert the pseudo to utf8: %1").arg(player_informations->public_and_private_informations.public_informations.pseudo));
        return;
    }
    if(GlobalServerData::serverSettings.anonymous)
        emit message(QStringLiteral("Charater id is logged: %1").arg(characterId));
    else
        emit message(QStringLiteral("Charater is logged: %1").arg(characterQuery.value(1).toString()));
    const quint32 &time_to_delete=characterQuery.value(24).toUInt(&ok);
    if(!ok || time_to_delete>0)
        dbQuery(GlobalServerData::serverPrivateVariables.db_query_update_character_time_to_delete.arg(characterId));
    dbQuery(GlobalServerData::serverPrivateVariables.db_query_update_character_last_connect.arg(characterId).arg(QDateTime::currentDateTime().toTime_t()));

    player_informations->public_and_private_informations.clan=characterQuery.value(8).toUInt(&ok);
    if(!ok)
    {
        emit message(QStringLiteral("clan id is not an number, clan disabled"));
        player_informations->public_and_private_informations.clan=0;//no clan
    }
    player_informations->public_and_private_informations.clan_leader=(characterQuery.value(20).toUInt(&ok)==1);
    if(!ok)
    {
        emit message(QStringLiteral("clan_leader id is not an number, clan_leader disabled"));
        player_informations->public_and_private_informations.clan_leader=false;//no clan
    }
    player_informations->public_and_private_informations.public_informations.pseudo=characterQuery.value(1).toString();
    QString skinString=characterQuery.value(2).toString();
    if(GlobalServerData::serverPrivateVariables.skinList.contains(skinString))
        player_informations->public_and_private_informations.public_informations.skinId=GlobalServerData::serverPrivateVariables.skinList.value(skinString);
    else
    {
        emit message(QStringLiteral("Skin not found, or out of the 255 first folder, default of the first by order alphabetic if have"));
        player_informations->public_and_private_informations.public_informations.skinId=0;
    }
    QString type=characterQuery.value(7).toString();
    if(type==ClientHeavyLoad::text_normal)
        player_informations->public_and_private_informations.public_informations.type=Player_type_normal;
    else if(type==ClientHeavyLoad::text_premium)
        player_informations->public_and_private_informations.public_informations.type=Player_type_premium;
    else if(type==ClientHeavyLoad::text_gm)
        player_informations->public_and_private_informations.public_informations.type=Player_type_gm;
    else if(type==ClientHeavyLoad::text_dev)
        player_informations->public_and_private_informations.public_informations.type=Player_type_dev;
    else
    {
        emit message(QStringLiteral("Mysql wrong type value").arg(type));
        player_informations->public_and_private_informations.public_informations.type=Player_type_normal;
    }
    player_informations->public_and_private_informations.cash=characterQuery.value(9).toUInt(&ok);
    if(!ok)
    {
        emit message(QStringLiteral("cash id is not an number, cash set to 0"));
        player_informations->public_and_private_informations.cash=0;
    }
    player_informations->public_and_private_informations.warehouse_cash=characterQuery.value(18).toUInt(&ok);
    if(!ok)
    {
        emit message(QStringLiteral("warehouse cash id is not an number, warehouse cash set to 0"));
        player_informations->public_and_private_informations.warehouse_cash=0;
    }
    if(GlobalServerData::serverPrivateVariables.bitcoin.enabled)
    {
        double bitcoin_offset=characterQuery.value(21).toDouble(&ok);
        if(!ok)
        {
            emit message(QStringLiteral("bitcoin offset is not an number (double), bitconi disabled"));
            player_informations->public_and_private_informations.bitcoin=-1.0;
        }
        else
        {
            QProcess process;
            process.start(GlobalServerData::serverSettings.bitcoin.binaryPath,QStringList()
                                                                           << QStringLiteral("-datadir=%1").arg(GlobalServerData::serverSettings.bitcoin.workingPath)
                                                                           << QStringLiteral("-port=%1").arg(GlobalServerData::serverSettings.bitcoin.port)
                                                                           << QStringLiteral("-bind=127.0.0.1:%1").arg(GlobalServerData::serverSettings.bitcoin.port)
                                                                           << QStringLiteral("-rpcport=%1").arg(GlobalServerData::serverSettings.bitcoin.port+1)
                                                                           << QStringLiteral("getbalance")
                                                                           << QStringLiteral("CatchChallenger_player_%1").arg(characterId)
                                                                           << QStringLiteral("20")//number of confirmation
                   );
            process.waitForStarted();
            process.waitForFinished();
            if(process.state()!=QProcess::NotRunning)
            {
                player_informations->public_and_private_informations.bitcoin=-1.0;
                emit message(QStringLiteral("Have been to kill the bitcoin requester client"));
                process.terminate();
                process.kill();
            }
            else if(process.exitStatus()!=QProcess::NormalExit || process.exitCode()!=0)
            {
                player_informations->public_and_private_informations.bitcoin=-1.0;
                emit message(QStringLiteral("Bitcoin requester client have wrong exit code or status, exit code: %1, error code: %2").arg(process.exitCode()).arg((int)process.error()));
            }
            else
            {
                QByteArray errorRaw=process.readAllStandardError();
                QByteArray outputRaw=process.readAllStandardOutput();
                if(!errorRaw.isEmpty())
                {
                    player_informations->public_and_private_informations.bitcoin=-1.0;
                    emit message(QStringLiteral("The bitcoin requester client have reported an error: %1").arg(QString::fromLocal8Bit(errorRaw)));
                }
                else if(outputRaw.isEmpty())
                {
                    player_informations->public_and_private_informations.bitcoin=-1.0;
                    emit message(QStringLiteral("The bitcoin requester client have empty output"));
                }
                else
                {
                    QString stringOutput=QString::fromLocal8Bit(outputRaw);
                    double bitcoin_real_balance=stringOutput.toDouble(&ok);
                    if(!ok)
                    {
                        player_informations->public_and_private_informations.bitcoin=-1.0;
                        emit message(QStringLiteral("The bitcoin requester have not returned a double: %1").arg(stringOutput));
                    }
                    else if((bitcoin_real_balance+bitcoin_offset)<0)
                    {
                        player_informations->public_and_private_informations.bitcoin=-1.0;
                        emit message(QStringLiteral("Bitcoin amount is negative, bitcoind sync?"));
                    }
                    else
                        player_informations->public_and_private_informations.bitcoin=bitcoin_real_balance+bitcoin_offset;
                }
            }
            if(player_informations->public_and_private_informations.bitcoin>=0)
            {
                QProcess process;
                process.start(GlobalServerData::serverSettings.bitcoin.binaryPath,QStringList()
                                                                               << QStringLiteral("-datadir=%1").arg(GlobalServerData::serverSettings.bitcoin.workingPath)
                                                                               << QStringLiteral("-port=%1").arg(GlobalServerData::serverSettings.bitcoin.port)
                                                                               << QStringLiteral("-bind=127.0.0.1:%1").arg(GlobalServerData::serverSettings.bitcoin.port)
                                                                               << QStringLiteral("-rpcport=%1").arg(GlobalServerData::serverSettings.bitcoin.port+1)
                                                                               << QStringLiteral("getaccountaddress")
                                                                               << QStringLiteral("CatchChallenger_player_%1").arg(player_informations->character_id)
                       );
                process.waitForStarted();
                process.waitForFinished();
                if(process.state()!=QProcess::NotRunning)
                {
                    player_informations->public_and_private_informations.bitcoin=-1.0;
                    emit message(QStringLiteral("Have been to kill the bitcoin requester client"));
                    process.terminate();
                    process.kill();
                }
                else if(process.exitStatus()!=QProcess::NormalExit || process.exitCode()!=0)
                {
                    player_informations->public_and_private_informations.bitcoin=-1.0;
                    emit message(QStringLiteral("Bitcoin requester client have wrong exit code or status, exit code: %1, error code: %2").arg(process.exitCode()).arg((int)process.error()));
                }
                else
                {
                    QByteArray errorRaw=process.readAllStandardError();
                    QByteArray outputRaw=process.readAllStandardOutput();
                    if(!errorRaw.isEmpty())
                    {
                        player_informations->public_and_private_informations.bitcoin=-1.0;
                        emit message(QStringLiteral("The bitcoin requester client have reported an error: %1").arg(QString::fromLocal8Bit(errorRaw)));
                    }
                    else if(outputRaw.isEmpty())
                    {
                        player_informations->public_and_private_informations.bitcoin=-1.0;
                        emit message(QStringLiteral("The bitcoin requester client have empty output"));
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
        loginIsWrong(query_id,"Wrong account data",QStringLiteral("Market cash wrong: %1").arg(characterQuery.value(22).toString()));
        return;
    }
    player_informations->market_bitcoin=0;
    if(player_informations->public_and_private_informations.bitcoin>=0)
    {
        player_informations->market_cash=characterQuery.value(23).toDouble(&ok);
        if(!ok)
        {
            loginIsWrong(query_id,"Wrong account data",QStringLiteral("Market bitcoin wrong: %1").arg(characterQuery.value(23).toString()));
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
    if(orientationString==ClientHeavyLoad::text_top)
        orentation=Orientation_top;
    else if(orientationString==ClientHeavyLoad::text_bottom)
        orentation=Orientation_bottom;
    else if(orientationString==ClientHeavyLoad::text_left)
        orentation=Orientation_left;
    else if(orientationString==ClientHeavyLoad::text_right)
        orentation=Orientation_right;
    else
    {
        orentation=Orientation_bottom;
        emit message(QStringLiteral("Wrong orientation corrected with bottom"));
    }
    player_informations->public_and_private_informations.allow=FacilityLib::StringToAllow(characterQuery.value(19).toString());
    //all is rights
    QString map=characterQuery.value(6).toString();
    if(!map.endsWith(ClientHeavyLoad::text_dottmx))
        map+=ClientHeavyLoad::text_dottmx;
    QString rescue_map=characterQuery.value(10).toString();
    if(!rescue_map.endsWith(ClientHeavyLoad::text_dottmx))
        rescue_map+=ClientHeavyLoad::text_dottmx;
    QString unvalidated_rescue_map=characterQuery.value(14).toString();
    if(!unvalidated_rescue_map.endsWith(ClientHeavyLoad::text_dottmx))
        unvalidated_rescue_map+=ClientHeavyLoad::text_dottmx;
    if(GlobalServerData::serverPrivateVariables.map_list.contains(map))
    {
        const quint8 &x=characterQuery.value(3).toUInt(&ok);
        if(!ok)
        {
            loginIsWrong(query_id,QLatin1String("Wrong account data"),QLatin1String("x coord is not a number"));
            return;
        }
        const quint8 &y=characterQuery.value(4).toUInt(&ok);
        if(!ok)
        {
            loginIsWrong(query_id,QLatin1String("Wrong account data"),QLatin1String("y coord is not a number"));
            return;
        }
        if(x>=GlobalServerData::serverPrivateVariables.map_list.value(map)->width)
        {
            loginIsWrong(query_id,QLatin1String("Wrong account data"),QLatin1String("x to out of map"));
            return;
        }
        if(y>=GlobalServerData::serverPrivateVariables.map_list.value(map)->height)
        {
            loginIsWrong(query_id,QLatin1String("Wrong account data"),QLatin1String("y to out of map"));
            return;
        }
        loginIsRightWithRescue(query_id,
            characterId,
            GlobalServerData::serverPrivateVariables.map_list.value(map),
            x,
            y,
            (Orientation)orentation,
                rescue_map,
                characterQuery.value(11),
                characterQuery.value(12),
                characterQuery.value(13),
                unvalidated_rescue_map,
                characterQuery.value(15),
                characterQuery.value(16),
                characterQuery.value(17)
        );
    }
    else
        loginIsWrong(query_id,QLatin1String("Map not found"),QLatin1String("Map not found: ")+map);
}

void ClientHeavyLoad::loginIsRightWithRescue(const quint8 &query_id, quint32 characterId, CommonMap* map, const /*COORD_TYPE*/ quint8 &x, const /*COORD_TYPE*/ quint8 &y, const Orientation &orientation,
                  const QVariant &rescue_map, const QVariant &rescue_x, const QVariant &rescue_y, const QVariant &rescue_orientation,
                  const QVariant &unvalidated_rescue_map, const QVariant &unvalidated_rescue_x, const QVariant &unvalidated_rescue_y, const QVariant &unvalidated_rescue_orientation
                                             )
{
    if(!GlobalServerData::serverPrivateVariables.map_list.contains(rescue_map.toString()))
    {
        emit message(QStringLiteral("rescue map ,not found"));
        loginIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    bool ok;
    const quint8 &rescue_new_x=rescue_x.toUInt(&ok);
    if(!ok)
    {
        emit message(QStringLiteral("rescue x coord is not a number"));
        loginIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    const quint8 &rescue_new_y=rescue_y.toUInt(&ok);
    if(!ok)
    {
        emit message(QStringLiteral("rescue y coord is not a number"));
        loginIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    if(rescue_new_x>=GlobalServerData::serverPrivateVariables.map_list.value(rescue_map.toString())->width)
    {
        emit message(QStringLiteral("rescue x to out of map"));
        loginIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    if(rescue_new_y>=GlobalServerData::serverPrivateVariables.map_list.value(rescue_map.toString())->height)
    {
        emit message(QStringLiteral("rescue y to out of map"));
        loginIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    const QString &orientationString=rescue_orientation.toString();
    Orientation rescue_new_orientation;
    if(orientationString==ClientHeavyLoad::text_top)
        rescue_new_orientation=Orientation_top;
    else if(orientationString==ClientHeavyLoad::text_bottom)
        rescue_new_orientation=Orientation_bottom;
    else if(orientationString==ClientHeavyLoad::text_left)
        rescue_new_orientation=Orientation_left;
    else if(orientationString==ClientHeavyLoad::text_right)
        rescue_new_orientation=Orientation_right;
    else
    {
        rescue_new_orientation=Orientation_bottom;
        emit message(QStringLiteral("Wrong rescue orientation corrected with bottom"));
    }
    if(!GlobalServerData::serverPrivateVariables.map_list.contains(unvalidated_rescue_map.toString()))
    {
        emit message(QStringLiteral("unvalidated rescue map ,not found"));
        loginIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    const quint8 &unvalidated_rescue_new_x=unvalidated_rescue_x.toUInt(&ok);
    if(!ok)
    {
        emit message(QStringLiteral("unvalidated rescue x coord is not a number"));
        loginIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    const quint8 &unvalidated_rescue_new_y=unvalidated_rescue_y.toUInt(&ok);
    if(!ok)
    {
        emit message(QStringLiteral("unvalidated rescue y coord is not a number"));
        loginIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    if(unvalidated_rescue_new_x>=GlobalServerData::serverPrivateVariables.map_list.value(rescue_map.toString())->width)
    {
        emit message(QStringLiteral("unvalidated rescue x to out of map"));
        loginIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    if(unvalidated_rescue_new_y>=GlobalServerData::serverPrivateVariables.map_list.value(rescue_map.toString())->height)
    {
        emit message(QStringLiteral("unvalidated rescue y to out of map"));
        loginIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    const QString &unvalidated_orientationString=unvalidated_rescue_orientation.toString();
    Orientation unvalidated_rescue_new_orientation;
    if(unvalidated_orientationString==ClientHeavyLoad::text_top)
        unvalidated_rescue_new_orientation=Orientation_top;
    else if(unvalidated_orientationString==ClientHeavyLoad::text_bottom)
        unvalidated_rescue_new_orientation=Orientation_bottom;
    else if(unvalidated_orientationString==ClientHeavyLoad::text_left)
        unvalidated_rescue_new_orientation=Orientation_left;
    else if(unvalidated_orientationString==ClientHeavyLoad::text_right)
        unvalidated_rescue_new_orientation=Orientation_right;
    else
    {
        unvalidated_rescue_new_orientation=Orientation_bottom;
        emit message(QStringLiteral("Wrong unvalidated rescue orientation corrected with bottom"));
    }
    loginIsRightWithParsedRescue(query_id,characterId,map,x,y,orientation,
                                 GlobalServerData::serverPrivateVariables.map_list.value(rescue_map.toString()),rescue_new_x,rescue_new_y,rescue_new_orientation,
                                 GlobalServerData::serverPrivateVariables.map_list.value(unvalidated_rescue_map.toString()),unvalidated_rescue_new_x,unvalidated_rescue_new_y,unvalidated_rescue_new_orientation
            );
}

void ClientHeavyLoad::loginIsRight(const quint8 &query_id,quint32 characterId, CommonMap *map, const quint8 &x, const quint8 &y, const Orientation &orientation)
{
    loginIsRightWithParsedRescue(query_id,characterId,map,x,y,orientation,map,x,y,orientation,map,x,y,orientation);
}

void ClientHeavyLoad::loginIsRightWithParsedRescue(const quint8 &query_id, quint32 characterId, CommonMap* map, const /*COORD_TYPE*/ quint8 &x, const /*COORD_TYPE*/ quint8 &y, const Orientation &orientation,
                  CommonMap* rescue_map, const /*COORD_TYPE*/ quint8 &rescue_x, const /*COORD_TYPE*/ quint8 &rescue_y, const Orientation &rescue_orientation,
                  CommonMap *unvalidated_rescue_map, const quint8 &unvalidated_rescue_x, const quint8 &unvalidated_rescue_y, const Orientation &unvalidated_rescue_orientation
                  )
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(GlobalServerData::serverPrivateVariables.db_query_clan.isEmpty())
    {
        emit error(QStringLiteral("loginIsRightWithParsedRescue() Query is empty, bug"));
        return;
    }
    #endif
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
            emit message(QStringLiteral("First client of the clan: %1, get the info").arg(player_informations->public_and_private_informations.clan));
            //do the query
            QSqlQuery clanQuery(*GlobalServerData::serverPrivateVariables.db);
            if(!clanQuery.exec(GlobalServerData::serverPrivateVariables.db_query_clan.arg(player_informations->public_and_private_informations.clan)))
                emit message(clanQuery.lastQuery()+": "+clanQuery.lastError().text());
            //parse the result
            if(clanQuery.next())
            {
                bool ok;
                quint64 cash=clanQuery.value(1).toULongLong(&ok);
                if(!ok)
                {
                    cash=0;
                    emit message(QStringLiteral("Warning: clan linked: %1 have wrong cash value, then reseted to 0"));
                }
                emit haveClanInfo(player_informations->public_and_private_informations.clan,clanQuery.value(0).toString(),cash);
            }
            else
                emit message(QStringLiteral("Warning: clan linked: %1 is not found into db"));
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
    emit message(QStringLiteral("Logged: %1 on the map: %2 (%3,%4)").arg(player_informations->public_and_private_informations.public_informations.pseudo).arg(map->map_file).arg(x).arg(y));
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
        emit message(QStringLiteral("Unable to convert the pseudo to utf8: %1").arg(player_informations->public_and_private_informations.public_informations.pseudo));
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
            if(clanConnectedCount.value(player_informations->public_and_private_informations.clan)==0)
                clanConnectedCount.remove(player_informations->public_and_private_informations.clan);
        }
    }
    emit isReadyToStop();
}

QHash<QString, quint32> ClientHeavyLoad::datapack_file_list_cached()
{
    if(GlobalServerData::serverSettings.datapackCache==-1)
        return datapack_file_list();
    else if(GlobalServerData::serverSettings.datapackCache==0)
    {
        if(ClientHeavyLoad::datapack_list_cache_timestamp==0)
        {
            ClientHeavyLoad::datapack_list_cache_timestamp=QDateTime::currentDateTime().toTime_t();
            ClientHeavyLoad::datapack_file_list_cache=datapack_file_list();
        }
        return ClientHeavyLoad::datapack_file_list_cache;
    }
    else
    {
        const quint64 &currentTime=QDateTime::currentDateTime().toTime_t();
        if(ClientHeavyLoad::datapack_list_cache_timestamp<(currentTime-GlobalServerData::serverSettings.datapackCache))
        {
            ClientHeavyLoad::datapack_list_cache_timestamp=currentTime;
            ClientHeavyLoad::datapack_file_list_cache=datapack_file_list();
        }
        return ClientHeavyLoad::datapack_file_list_cache;
    }
}

QHash<QString,quint32> ClientHeavyLoad::datapack_file_list()
{
    QHash<QString,quint32> filesList;

    const QStringList &returnList=FacilityLib::listFolder(GlobalServerData::serverSettings.datapack_basePath);
    int index=0;
    const int &size=returnList.size();
    while(index<size)
    {
        #ifdef Q_OS_WIN32
        QString fileName=returnList.at(index);
        #else
        const QString &fileName=returnList.at(index);
        #endif
        if(fileName.contains(GlobalServerData::serverPrivateVariables.datapack_rightFileName))
        {
            if(!QFileInfo(fileName).suffix().isEmpty() && extensionAllowed.contains(QFileInfo(fileName).suffix()))
            {
                QFile file(GlobalServerData::serverSettings.datapack_basePath+returnList.at(index));
                if(file.size()<=8*1024*1024)
                {
                    if(file.open(QIODevice::ReadOnly))
                    {
                        #ifdef Q_OS_WIN32
                        fileName.replace(ClientHeavyLoad::text_antislash,ClientHeavyLoad::text_slash);//remplace if is under windows server
                        #endif
                        if(GlobalServerData::serverSettings.datapackCacheMtime)
                            filesList[fileName]=QFileInfo(file).lastModified().toTime_t();
                        else
                            filesList[fileName]=0;
                        file.close();
                    }
                }
            }
        }
        index++;
    }
    return filesList;
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
    QHash<QString,quint32> filesList=datapack_file_list_cached();
    QHash<QString,quint32> filesListInfo;
    QStringList fileToSend;

    const int &loop_size=files.size();
    //send the size to download on the client
    {
        QHash<QString,quint32> filesListForSize(filesList);
        int index=0;
        quint32 datapckFileNumber=0;
        quint32 datapckFileSize=0;
        while(index<loop_size)
        {
            QString fileName=files.at(index);
            quint32 mtime=timestamps.at(index);
            if(fileName.contains(ClientHeavyLoad::text_dotslash) || fileName.contains(ClientHeavyLoad::text_antislash) || fileName.contains(ClientHeavyLoad::text_double_slash))
            {
                emit error(QStringLiteral("file name contains illegale char: %1").arg(fileName));
                return;
            }
            if(fileName.contains(fileNameStartStringRegex) || fileName.startsWith(ClientHeavyLoad::text_slash))
            {
                emit error(QStringLiteral("start with wrong string: %1").arg(fileName));
                return;
            }
            if(filesListForSize.contains(fileName))
            {
                quint32 file_mtime;
                if(GlobalServerData::serverSettings.datapackCacheMtime)
                    file_mtime=filesListForSize.value(fileName);
                else
                    file_mtime=QFileInfo(GlobalServerData::serverSettings.datapack_basePath+fileName).lastModified().toTime_t();
                filesListInfo[fileName]=file_mtime;
                if(file_mtime==mtime)
                    addDatapackListReply(false);//found
                else
                {
                    addDatapackListReply(false);//found but updated
                    datapckFileNumber++;
                    datapckFileSize+=QFile(GlobalServerData::serverSettings.datapack_basePath+fileName).size();
                    fileToSend << fileName;
                }
                filesListForSize.remove(fileName);
            }
            else
                addDatapackListReply(true);//to delete
            index++;
        }
        QHashIterator<QString,quint32> i(filesListForSize);
        while (i.hasNext()) {
            i.next();
            datapckFileNumber++;
            datapckFileSize+=QFile(GlobalServerData::serverSettings.datapack_basePath+i.key()).size();
            fileToSend << i.key();
        }
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);
        out << (quint32)datapckFileNumber;
        out << (quint32)datapckFileSize;
        emit sendFullPacket(0xC2,0x000C,outputData);
    }
    fileToSend.sort();
    if(GlobalServerData::serverSettings.httpDatapackMirror.isEmpty())
    {
        //validate, remove or update the file actualy on the client
        if(tempDatapackListReplyTestCount!=files.size())
        {
            emit error("Bit count return not match");
            return;
        }
        //send not in the list
        {
            int index=0;
            while(index<fileToSend.size())
            {
                sendFile(fileToSend.at(index),filesListInfo.value(fileToSend.at(index)));
                index++;
            }
        }
        sendFileContent();
        sendCompressedFileContent();
        purgeDatapackListReply(query_id);
    }
    else
    {
        QByteArray outputData(FacilityLib::toUTF8(GlobalServerData::serverSettings.httpDatapackMirror));
        if(outputData.size()>255 || outputData.isEmpty())
        {
            emit error(QLatin1Literal("httpDatapackMirror too big or not compatible with utf8"));
            return;
        }

        QList<QString> fileHttpListName;
        QList<quint64> fileHttpListDate;
        //validate, remove or update the file actualy on the client
        if(tempDatapackListReplyTestCount!=files.size())
        {
            emit error("Bit count return not match");
            return;
        }
        //send not in the list
        {
            int index=0;
            while(index<fileToSend.size())
            {
                fileHttpListName << fileToSend.at(index);
                fileHttpListDate << filesListInfo.value(fileToSend.at(index));
                index++;
            }
        }
        if(!fileHttpListName.isEmpty())
        {
            QDataStream out(&outputData, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_4_4);
            out.device()->seek(out.device()->size());
            out << (quint32)fileHttpListName.size();
            quint32 index=0;
            const quint32 &fileHttpListNameSize=fileHttpListName.size();
            while(index<fileHttpListNameSize)
            {
                const QByteArray &rawFileName=FacilityLib::toUTF8(fileHttpListName.at(index));
                if(rawFileName.size()>255 || rawFileName.isEmpty())
                {
                    emit error(QLatin1Literal("file path too big or not compatible with utf8"));
                    return;
                }
                const quint64 &fileInfoModTime=fileHttpListDate.at(index);
                outputData+=rawFileName;
                out.device()->seek(out.device()->size());
                out << (quint64)fileInfoModTime;
                index++;
            }
            emit sendFullPacket(0xC2,0x000D,outputData);
        }
        purgeDatapackListReply(query_id);
    }
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
    const QByteArray &fileNameRaw=FacilityLib::toUTF8(fileName);
    if(fileNameRaw.size()>255 || fileNameRaw.isEmpty())
        return false;
    QFile file(GlobalServerData::serverSettings.datapack_basePath+fileName);
    if(file.open(QIODevice::ReadOnly))
    {
        const QByteArray &content=file.readAll();
        /*emit message(QStringLiteral("send the file: %1, checkMtime: %2, mtime: %3, file server mtime: %4")
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
        if(compressedExtension.contains(QFileInfo(file).suffix()) && ProtocolParsing::compressionType!=ProtocolParsing::CompressionType_None && content.size()<CATCHCHALLENGER_SERVER_DATAPACK_DONT_COMPRESS_GREATER_THAN_KB*1024)
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
            }
        }
        else
        {
            if(content.size()>CATCHCHALLENGER_SERVER_DATAPACK_MIN_FILEPURGE_KB*1024)
            {
                QByteArray outputData2;
                outputData2[0x00]=0x01;
                emit sendFullPacket(0xC2,0x0003,outputData2+fileNameRaw+outputData+content);
            }
            else
            {
                rawFiles+=fileNameRaw+outputData+content;
                rawFilesCount++;
                if(rawFiles.size()>CATCHCHALLENGER_SERVER_DATAPACK_MAX_FILEPURGE_KB*1024 || rawFilesCount>=255)
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
    return text.replace(ClientHeavyLoad::single_quote,ClientHeavyLoad::antislash_single_quote);
}

void ClientHeavyLoad::dbQuery(const QString &queryText)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(queryText.isEmpty())
    {
        emit error(QStringLiteral("dbQuery() Query is empty, bug"));
        return;
    }
    #endif
    #ifdef DEBUG_MESSAGE_CLIENT_SQL
    emit message(QStringLiteral("Do mysql query: ")+queryText);
    #endif
    QSqlQuery sqlQuery(*GlobalServerData::serverPrivateVariables.db);
    if(!sqlQuery.exec(queryText))
        emit message(sqlQuery.lastQuery()+QLatin1String(": ")+sqlQuery.lastError().text());
    GlobalServerData::serverPrivateVariables.db->commit();//to have data coerancy and prevent data lost on crash
}

void ClientHeavyLoad::loadReputation()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(GlobalServerData::serverPrivateVariables.db_query_select_reputation_by_id.isEmpty())
    {
        emit error(QStringLiteral("loadReputation() Query is empty, bug"));
        return;
    }
    #endif
    //do the query
    bool ok;
    QSqlQuery reputationQuery(*GlobalServerData::serverPrivateVariables.db);
    if(!reputationQuery.exec(GlobalServerData::serverPrivateVariables.db_query_select_reputation_by_id.arg(player_informations->character_id)))
        emit message(reputationQuery.lastQuery()+QLatin1String(": ")+reputationQuery.lastError().text());
    //parse the result
    while(reputationQuery.next())
    {
        const QString &type=reputationQuery.value(0).toString();
        qint32 point=reputationQuery.value(1).toInt(&ok);
        if(!ok)
        {
            emit message(QStringLiteral("point is not a number, skip: %1").arg(type));
            continue;
        }
        const qint32 &level=reputationQuery.value(2).toInt(&ok);
        if(!ok)
        {
            emit message(QStringLiteral("level is not a number, skip: %1").arg(type));
            continue;
        }
        if(level<-100 || level>100)
        {
            emit message(QStringLiteral("level is <100 or >100, skip: %1").arg(type));
            continue;
        }
        if(!CommonDatapack::commonDatapack.reputation.contains(type))
        {
            emit message(QStringLiteral("The reputation: %1 don't exist").arg(type));
            continue;
        }
        if(level>=0)
        {
            if(level>=CommonDatapack::commonDatapack.reputation.value(type).reputation_positive.size())
            {
                emit message(QStringLiteral("The reputation level %1 is wrong because is out of range (reputation level: %2 > max level: %3)").arg(type).arg(level).arg(CommonDatapack::commonDatapack.reputation.value(type).reputation_positive.size()));
                continue;
            }
        }
        else
        {
            if((-level)>CommonDatapack::commonDatapack.reputation.value(type).reputation_negative.size())
            {
                emit message(QStringLiteral("The reputation level %1 is wrong because is out of range (reputation level: %2 < max level: %3)").arg(type).arg(level).arg(CommonDatapack::commonDatapack.reputation.value(type).reputation_negative.size()));
                continue;
            }
        }
        if(point>0)
        {
            if(CommonDatapack::commonDatapack.reputation.value(type).reputation_positive.size()==(level+1))//start at level 0 in positive
            {
                emit message(QStringLiteral("The reputation level is already at max, drop point"));
                point=0;
            }
            if(point>=CommonDatapack::commonDatapack.reputation.value(type).reputation_positive.at(level+1))//start at level 0 in positive
            {
                emit message(QStringLiteral("The reputation point %1 is greater than max %2").arg(point).arg(CommonDatapack::commonDatapack.reputation.value(type).reputation_positive.at(level)));
                continue;
            }
        }
        else if(point<0)
        {
            if(CommonDatapack::commonDatapack.reputation.value(type).reputation_negative.size()==-level)//start at level -1 in negative
            {
                emit message(QStringLiteral("The reputation level is already at min, drop point"));
                point=0;
            }
            if(point<CommonDatapack::commonDatapack.reputation.value(type).reputation_negative.at(-level))//start at level -1 in negative
            {
                emit message(QStringLiteral("The reputation point %1 is greater than max %2").arg(point).arg(CommonDatapack::commonDatapack.reputation.value(type).reputation_negative.at(level)));
                continue;
            }
        }
        player_informations->public_and_private_informations.reputation[type].level=level;
        player_informations->public_and_private_informations.reputation[type].point=point;
    }
}

void ClientHeavyLoad::loadQuests()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(GlobalServerData::serverPrivateVariables.db_query_select_quest_by_id.isEmpty())
    {
        emit error(QStringLiteral("loadQuests() Query is empty, bug"));
        return;
    }
    #endif
    //do the query
    bool ok,ok2;
    QSqlQuery questsQuery(*GlobalServerData::serverPrivateVariables.db);
    if(!questsQuery.exec(GlobalServerData::serverPrivateVariables.db_query_select_quest_by_id.arg(player_informations->character_id)))
        emit message(questsQuery.lastQuery()+": "+questsQuery.lastError().text());
    //parse the result
    while(questsQuery.next())
    {
        PlayerQuest playerQuest;
        const quint32 &id=questsQuery.value(0).toUInt(&ok);
        playerQuest.finish_one_time=questsQuery.value(1).toBool();
        playerQuest.step=questsQuery.value(2).toUInt(&ok2);
        if(!ok || !ok2)
        {
            emit message(QStringLiteral("wrong value type for quest, skip: %1").arg(id));
            continue;
        }
        if(!CommonDatapack::commonDatapack.quests.contains(id))
        {
            emit message(QStringLiteral("quest is not into the quests list, skip: %1").arg(id));
            continue;
        }
        if((playerQuest.step<=0 && !playerQuest.finish_one_time) || playerQuest.step>CommonDatapack::commonDatapack.quests.value(id).steps.size())
        {
            emit message(QStringLiteral("step out of quest range, skip: %1").arg(id));
            continue;
        }
        if(playerQuest.step<=0 && !playerQuest.finish_one_time)
        {
            emit message(QStringLiteral("can't be to step 0 if have never finish the quest, skip: %1").arg(id));
            continue;
        }
        player_informations->public_and_private_informations.quests[id]=playerQuest;
        LocalClientHandler::addQuestStepDrop(player_informations,id,playerQuest.step);
    }
}
