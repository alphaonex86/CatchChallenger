#include "CharactersGroupForLogin.h"
#include "EpollServerLoginSlave.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "../base/PreparedDBQuery.h"
#include <iostream>
#include <QDebug>

using namespace CatchChallenger;

void CharactersGroupForLogin::character_list(EpollClientLoginSlave * const client,const quint32 &account_id)
{
    const QString &queryText=QString(PreparedDBQuery::db_query_characters).arg(account_id).arg(EpollClientLoginSlave::max_character*2);
    CatchChallenger::DatabaseBase::CallBack *callback=databaseBaseCommon->asyncRead(queryText.toLatin1(),this,&EpollClientLoginSlave::character_list_static);
    if(callback==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(databaseBaseCommon->errorMessage());
        client->askLogin_cancel();
        return;
    }
    else
        clientQueryForReadReturn << client;
}

void CharactersGroupForLogin::character_list_static(void *object)
{
    if(object!=NULL)
        static_cast<CharactersGroupForLogin *>(object)->character_list_object();
}

void CharactersGroupForLogin::character_list_object()
{
    EpollClientLoginSlave * const client=clientQueryForReadReturn.takeFirst();

    char * const tempRawData=new char[4*1024];
    //memset(tempRawData,0x00,sizeof(4*1024));
    int tempRawDataSize=0x01;

    const quint64 &current_time=QDateTime::currentDateTime().toTime_t();
    bool ok;
    quint8 validCharaterCount=0;
    while(databaseBaseCommon->next() && validCharaterCount<EpollClientLoginSlave::max_character)
    {
        unsigned int character_id=QString(databaseBaseCommon->value(0)).toUInt(&ok);
        if(ok)
        {
            //delete
            quint32 time_to_delete=QString(databaseBaseCommon->value(3)).toUInt(&ok);
            if(!ok)
            {
                qDebug() << (QStringLiteral("time_to_delete is not number: %1 for %2 fixed by 0").arg(QString(databaseBaseCommon->value(3))).arg(character_id));
                time_to_delete=0;
            }
            if(current_time<time_to_delete && time_to_delete!=0)
            {
                validCharaterCount++;

                //Character id
                *reinterpret_cast<quint32 *>(tempRawData+tempRawDataSize)=htole32(character_id);
                tempRawDataSize+=sizeof(quint32);

                //pseudo
                tempRawDataSize+=FacilityLibGeneral::toUTF8WithHeader(databaseBaseCommon->value(1),tempRawData);

                //skin
                tempRawData[tempRawDataSize]=QString(databaseBaseCommon->value(2)).toUInt(&ok);
                if(!ok)//if not number
                {
                    qDebug() << (QStringLiteral("character return skin is not number: %1 for %2 fixed by 0").arg(databaseBaseCommon->value(5)).arg(character_id));
                    tempRawData[tempRawDataSize]=0;
                    ok=true;
                }
                else
                {
                    if(tempRawData[tempRawDataSize]>=EpollServerLoginSlave::epollServerLoginSlave->dictionary_skin_internal_to_database.size())//out of range
                    {
                        qDebug() << (QStringLiteral("character return skin out of range: %1 for %2 fixed by 0").arg(databaseBaseCommon->value(5)).arg(character_id));
                        tempRawData[tempRawDataSize]=0;
                        ok=true;
                    }
                    //else all is good
                }
                tempRawDataSize+=1;

                //delete register
                unsigned int delete_time_left=0;
                if(time_to_delete==0)
                    delete_time_left=0;
                else
                    delete_time_left=time_to_delete-current_time;
                *reinterpret_cast<quint32 *>(tempRawData+tempRawDataSize)=htole32(delete_time_left);
                tempRawDataSize+=sizeof(quint32);

                //played_time
                unsigned int played_time=QString(databaseBaseCommon->value(4)).toUInt(&ok);
                if(!ok)
                {
                    qDebug() << (QStringLiteral("played_time is not number: %1 for %2 fixed by 0").arg(databaseBaseCommon->value(4)).arg(character_id));
                    played_time=0;
                }
                *reinterpret_cast<quint32 *>(tempRawData+tempRawDataSize)=htole32(played_time);
                tempRawDataSize+=sizeof(quint32);

                //last_connect
                unsigned int last_connect=QString(databaseBaseCommon->value(5)).toUInt(&ok);
                if(!ok)
                {
                    qDebug() << (QStringLiteral("last_connect is not number: %1 for %2 fixed by 0").arg(databaseBaseCommon->value(5)).arg(character_id));
                    last_connect=current_time;
                }
                *reinterpret_cast<quint32 *>(tempRawData+tempRawDataSize)=htole32(last_connect);
                tempRawDataSize+=sizeof(quint32);
            }
            else
                deleteCharacterNow(character_id);
        }
        else
            qDebug() << (QStringLiteral("Character id is not number: %1 for %2").arg(databaseBaseCommon->value(0)).arg(character_id));
    }
    tempRawData[0]=validCharaterCount;

    client->character_list_return(this->index,tempRawData,tempRawDataSize);
    delete tempRawData;

    //get server list
    server_list(client,client->account_id);
}

void CharactersGroupForLogin::server_list(EpollClientLoginSlave * const client,const quint32 &account_id)
{
    const QString &queryText=QString(PreparedDBQuery::db_query_servers).arg(account_id);
    CatchChallenger::DatabaseBase::CallBack *callback=databaseBaseCommon->asyncRead(queryText.toLatin1(),this,&CharactersGroupForLogin::server_list_static);
    if(callback==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(databaseBaseCommon->errorMessage());
        client->askLogin_cancel();
        return;
    }
    else
        clientQueryForReadReturn << client;
}

void CharactersGroupForLogin::server_list_static(void *object)
{
    if(object!=NULL)
        static_cast<CharactersGroupForLogin *>(object)->server_list_object();
}

void CharactersGroupForLogin::server_list_object()
{
    EpollClientLoginSlave * const client=clientQueryForReadReturn.takeFirst();

    char * const tempRawData=new char[4*1024];
    //memset(tempRawData,0x00,sizeof(4*1024));
    int tempRawDataSize=0x01;

    const quint64 &current_time=QDateTime::currentDateTime().toTime_t();
    bool ok;
    quint8 validServerCount=0;
    while(databaseBaseCommon->next() && validServerCount<EpollClientLoginSlave::max_character)
    {
        unsigned int server_id=QString(databaseBaseCommon->value(0)).toUInt(&ok);
        if(ok)
        {
            qint16 serverIndex=-1;
            if(server_id<(unsigned int)CharactersGroupForLogin::dictionary_server_database_to_index.size())
                if(CharactersGroupForLogin::dictionary_server_database_to_index.at(server_id)!=-1)
                    serverIndex=CharactersGroupForLogin::dictionary_server_database_to_index.at(server_id);
            if(serverIndex!=-1)
            {
                //server index
                tempRawData[tempRawDataSize]=serverIndex;
                tempRawDataSize+=1;

                //played_time
                unsigned int played_time=QString(databaseBaseCommon->value(1)).toUInt(&ok);
                if(!ok)
                {
                    qDebug() << (QStringLiteral("played_time is not number: %1 fixed by 0").arg(databaseBaseCommon->value(4)));
                    played_time=0;
                }
                *reinterpret_cast<quint32 *>(tempRawData+tempRawDataSize)=htole32(played_time);
                tempRawDataSize+=sizeof(quint32);

                //last_connect
                unsigned int last_connect=QString(databaseBaseCommon->value(2)).toUInt(&ok);
                if(!ok)
                {
                    qDebug() << (QStringLiteral("last_connect is not number: %1 fixed by 0").arg(databaseBaseCommon->value(5)));
                    last_connect=current_time;
                }
                *reinterpret_cast<quint32 *>(tempRawData+tempRawDataSize)=htole32(last_connect);
                tempRawDataSize+=sizeof(quint32);

                validServerCount++;
            }
        }
        else
            qDebug() << (QStringLiteral("Character id is not number: %1").arg(databaseBaseCommon->value(0)));
    }
    tempRawData[0]=validServerCount;

    client->server_list_return(validServerCount,tempRawData,tempRawDataSize);
    delete tempRawData;
}

void CharactersGroupForLogin::deleteCharacterNow(const quint32 &characterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQuery::db_query_monster_by_character_id==0)
    {
        qDebug() << (QStringLiteral("deleteCharacterNow() Query is empty, bug"));
        return;
    }
    if(PreparedDBQuery::db_query_delete_monster_buff==0)
    {
        qDebug() << (QStringLiteral("deleteCharacterNow() Query db_query_delete_monster_buff is empty, bug"));
        return;
    }
    if(PreparedDBQuery::db_query_delete_monster_skill==0)
    {
        qDebug() << (QStringLiteral("deleteCharacterNow() Query db_query_delete_monster_skill is empty, bug"));
        return;
    }
    if(PreparedDBQuery::db_query_delete_bot_already_beaten==0)
    {
        qDebug() << (QStringLiteral("deleteCharacterNow() Query db_query_delete_bot_already_beaten is empty, bug"));
        return;
    }
    if(PreparedDBQuery::db_query_delete_character==0)
    {
        qDebug() << (QStringLiteral("deleteCharacterNow() Query db_query_delete_character is empty, bug"));
        return;
    }
    if(PreparedDBQuery::db_query_delete_all_item==0)
    {
        qDebug() << (QStringLiteral("deleteCharacterNow() Query db_query_delete_item is empty, bug"));
        return;
    }
    if(PreparedDBQuery::db_query_delete_monster_by_character==0)
    {
        qDebug() << (QStringLiteral("deleteCharacterNow() Query db_query_delete_monster is empty, bug"));
        return;
    }
    if(PreparedDBQuery::db_query_delete_plant==0)
    {
        qDebug() << (QStringLiteral("deleteCharacterNow() Query db_query_delete_plant is empty, bug"));
        return;
    }
    if(PreparedDBQuery::db_query_delete_quest==0)
    {
        qDebug() << (QStringLiteral("deleteCharacterNow() Query db_query_delete_quest is empty, bug"));
        return;
    }
    if(PreparedDBQuery::db_query_delete_recipes==0)
    {
        qDebug() << (QStringLiteral("deleteCharacterNow() Query db_query_delete_recipes is empty, bug"));
        return;
    }
    if(PreparedDBQuery::db_query_delete_reputation==0)
    {
        qDebug() << (QStringLiteral("deleteCharacterNow() Query db_query_delete_reputation is empty, bug"));
        return;
    }
    #endif

    const QString &queryText=QString(PreparedDBQuery::db_query_monster_by_character_id).arg(characterId);
    CatchChallenger::DatabaseBase::CallBack *callback=databaseBaseCommon->asyncRead(queryText.toLatin1(),this,&CharactersGroupForLogin::deleteCharacterNow_static);
    if(callback==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(databaseBaseCommon->errorMessage());
        return;
    }
    else
        deleteCharacterNowCharacterIdList << characterId;
}

void CharactersGroupForLogin::deleteCharacterNow_static(void *object)
{
    if(object!=NULL)
        static_cast<CharactersGroupForLogin *>(object)->deleteCharacterNow_object();
}

void CharactersGroupForLogin::deleteCharacterNow_object()
{
    deleteCharacterNow_return(deleteCharacterNowCharacterIdList.takeFirst());
    databaseBaseCommon->clear();
}

void CharactersGroupForLogin::deleteCharacterNow_return(const quint32 &characterId)
{
    bool ok;
    while(databaseBaseCommon->next())
    {
        const quint32 &monsterId=QString(databaseBaseCommon->value(0)).toUInt(&ok);
        if(ok)
        {
            dbQueryWriteCommon(QString(PreparedDBQuery::db_query_delete_monster_buff).arg(monsterId).toUtf8().constData());
            dbQueryWriteCommon(QString(PreparedDBQuery::db_query_delete_monster_skill).arg(monsterId).toUtf8().constData());
        }
    }
    dbQueryWriteCommon(QString(PreparedDBQuery::db_query_delete_bot_already_beaten).arg(characterId).toUtf8().constData());
    dbQueryWriteCommon(QString(PreparedDBQuery::db_query_delete_character).arg(characterId).toUtf8().constData());
    dbQueryWriteCommon(QString(PreparedDBQuery::db_query_delete_all_item).arg(characterId).toUtf8().constData());
    dbQueryWriteCommon(QString(PreparedDBQuery::db_query_delete_all_item_warehouse).arg(characterId).toUtf8().constData());
    dbQueryWriteCommon(QString(PreparedDBQuery::db_query_delete_all_item_market).arg(characterId).toUtf8().constData());
    dbQueryWriteCommon(QString(PreparedDBQuery::db_query_delete_monster_by_character).arg(characterId).toUtf8().constData());
    dbQueryWriteCommon(QString(PreparedDBQuery::db_query_delete_monster_warehouse_by_character).arg(characterId).toUtf8().constData());
    dbQueryWriteCommon(QString(PreparedDBQuery::db_query_delete_monster_market_by_character).arg(characterId).toUtf8().constData());
    dbQueryWriteCommon(QString(PreparedDBQuery::db_query_delete_plant).arg(characterId).toUtf8().constData());
    dbQueryWriteCommon(QString(PreparedDBQuery::db_query_delete_quest).arg(characterId).toUtf8().constData());
    dbQueryWriteCommon(QString(PreparedDBQuery::db_query_delete_recipes).arg(characterId).toUtf8().constData());
    dbQueryWriteCommon(QString(PreparedDBQuery::db_query_delete_reputation).arg(characterId).toUtf8().constData());
    dbQueryWriteCommon(QString(PreparedDBQuery::db_query_delete_allow).arg(characterId).toUtf8().constData());
}

bool CharactersGroupForLogin::addCharacter(void * const client,const quint8 &query_id, const quint8 &profileIndex, const QString &pseudo, const quint8 &skinId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQuery::db_query_select_character_by_pseudo==NULL)
    {
        qDebug() << (QStringLiteral("addCharacter() Query is empty, bug"));
        return false;
    }
    #endif
    if(skinList.isEmpty())
    {
        qDebug() << QStringLiteral("Skin list is empty, unable to add charaters");
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
        out << (quint8)0x02;
        out << (quint32)0x00000000;
        postReply(query_id,outputData);
        return false;
    }
    if(number_of_character>=EpollClientLoginSlave::max_character)
    {
        qDebug() << (QStringLiteral("You can't create more account, you have already %1 on %2 allowed").arg(number_of_character).arg(EpollClientLoginSlave::max_character));
        return false;
    }
    if(profileIndex>=CommonDatapack::commonDatapack.profileList.size())
    {
        qDebug() << (QStringLiteral("profile index: %1 out of range (profileList size: %2)").arg(profileIndex).arg(CommonDatapack::commonDatapack.profileList.size()));
        return false;
    }
    if(!serverProfileList.at(profileIndex).valid)
    {
        qDebug() << (QStringLiteral("profile index: %1 profil not valid").arg(profileIndex));
        return false;
    }
    if(pseudo.size()>max_pseudo_size)
    {
        qDebug() << (QStringLiteral("pseudo size is too big: %1 because is greater than %2").arg(pseudo.size()).arg(max_pseudo_size));
        return false;
    }
    if(skinId>=skinList.size())
    {
        qDebug() << (QStringLiteral("skin provided: %1 is not into skin listed").arg(skinId));
        return false;
    }
    const Profile &profile=CommonDatapack::commonDatapack.profileList.at(profileIndex);
    if(!profile.forcedskin.isEmpty() && !profile.forcedskin.contains(skinId))
    {
        qDebug() << (QStringLiteral("skin provided: %1 is not into profile %2 forced skin list").arg(skinId).arg(profileIndex));
        return false;
    }
    AddCharacterParam *addCharacterParam=new AddCharacterParam();
    addCharacterParam->query_id=query_id;
    addCharacterParam->profileIndex=profileIndex;
    addCharacterParam->pseudo=pseudo;
    addCharacterParam->skinId=skinId;
    addCharacterParam->client=client;

    const QString &queryText=QString(PreparedDBQuery::db_query_select_character_by_pseudo).arg(SqlFunction::quoteSqlVariable(pseudo));
    CatchChallenger::DatabaseBase::CallBack *callback=databaseBaseCommon->asyncRead(queryText.toLatin1(),this,&CharactersGroupForLogin::addCharacter_static);
    if(callback==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(databaseBaseCommon->errorMessage());

        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
        out << (quint8)0x02;
        out << (quint32)0x00000000;
        postReply(query_id,outputData);
        delete addCharacterParam;
        return false;
    }
    else
    {
        addCharacterParamList << addCharacterParam;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        paramToPassToCallBackType << QStringLiteral("AddCharacterParam");
        #endif
        callbackRegistred << callback;
        return true;
    }
}

void CharactersGroupForLogin::addCharacter_static(void *object)
{
    if(object!=NULL)
        static_cast<CharactersGroupForLogin *>(object)->addCharacter_object();
}

void CharactersGroupForLogin::addCharacter_object()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBack.isEmpty())
    {
        qDebug() << "paramToPassToCallBack.isEmpty()" << __FILE__ << __LINE__;
        abort();
    }
    #endif
    AddCharacterParam *addCharacterParam=static_cast<AddCharacterParam *>(paramToPassToCallBack.takeFirst());
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(addCharacterParam==NULL)
        abort();
    #endif
    addCharacter_return(addCharacterParam->query_id,addCharacterParam->profileIndex,addCharacterParam->pseudo,addCharacterParam->skinId);
    delete addCharacterParam;
    databaseBaseCommon->clear();
}

void CharactersGroupForLogin::addCharacter_return(const quint8 &query_id,const quint8 &profileIndex,const QString &pseudo,const quint8 &skinId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBackType.takeFirst()!=QStringLiteral("AddCharacterParam"))
    {
        qDebug() << "is not AddCharacterParam" << paramToPassToCallBackType.join(";") << __FILE__ << __LINE__;
        abort();
    }
    #endif
    callbackRegistred.removeFirst();
    if(databaseBaseCommon->next())
    {
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
        out << (quint8)0x01;
        out << (quint32)0x00000000;
        postReply(query_id,outputData);
        return;
    }
    const Profile &profile=CommonDatapack::commonDatapack.profileList.at(profileIndex);
    const ServerProfile &serverProfile=serverProfileList.at(profileIndex);

    number_of_character++;
    maxCharacterId++;

    const quint32 &characterId=maxCharacterId;
    int index=0;
    int monster_position=1;
    dbQueryWriteCommon(serverProfile.preparedQuery.at(0)+QString::number(characterId)+serverProfile.preparedQuery.at(1)+QString::number(account_id)+serverProfile.preparedQuery.at(2)+pseudo+serverProfile.preparedQuery.at(3)+QString::number(dictionary_skin_reverse.at(skinId))+serverProfile.preparedQuery.at(4).toUtf8().constData());
    while(index<profile.monsters.size())
    {
        const quint32 &monsterId=profile.monsters.at(index).id;
        if(CatchChallenger::CommonDatapack::commonDatapack.monsters.contains(monsterId))
        {
            const Monster &monster=CatchChallenger::CommonDatapack::commonDatapack.monsters.value(monsterId);
            quint32 gender=Gender_Unknown;
            if(monster.ratio_gender!=-1)
            {
                if(rand()%101<monster.ratio_gender)
                    gender=Gender_Female;
                else
                    gender=Gender_Male;
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
                QMutexLocker(&monsterIdMutex);
                maxMonsterId++;
                monster_id=maxMonsterId;
            }
            while(skills.size()>4)
                skills.removeFirst();
            {
                dbQueryWriteCommon(PreparedDBQuery::db_query_insert_monster
                   .arg(monster_id)
                   .arg(stat.hp)
                   .arg(characterId)
                   .arg(monsterId)
                   .arg(profile.monsters.at(index).level)
                   .arg(profile.monsters.at(index).captured_with)
                   .arg(gender)
                   .arg(monster_position)
                   .toUtf8().constData());
                monster_position++;
            }
            sub_index=0;
            while(sub_index<skills.size())
            {
                quint8 endurance=0;
                if(CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.contains(skills.value(sub_index).skill))
                    if(skills.value(sub_index).level<=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.value(skills.value(sub_index).skill).level.size() && skills.value(sub_index).level>0)
                        endurance=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.value(skills.value(sub_index).skill).level.at(skills.value(sub_index).level-1).endurance;
                dbQueryWriteCommon(PreparedDBQuery::db_query_insert_monster_skill
                   .arg(monster_id)
                   .arg(skills.value(sub_index).skill)
                   .arg(skills.value(sub_index).level)
                   .arg(endurance)
                   .toUtf8().constData());
                sub_index++;
            }
            index++;
        }
        else
        {
            qDebug() << (QStringLiteral("monster not found to start: %1 is not into profile forced skin list: %2").arg(monsterId));
            return;
        }
    }
    index=0;
    while(index<profile.reputation.size())
    {
        dbQueryWriteCommon(PreparedDBQuery::db_query_insert_reputation
           .arg(characterId)
           .arg(CommonDatapack::commonDatapack.reputation.at(profile.reputation.at(index).reputationId).reverse_database_id)
           .arg(profile.reputation.at(index).point)
           .arg(profile.reputation.at(index).level)
           .toUtf8().constData());
        index++;
    }
    index=0;
    while(index<profile.items.size())
    {
        dbQueryWriteCommon(PreparedDBQuery::db_query_insert_item
           .arg(profile.items.at(index).id)
           .arg(characterId)
           .arg(profile.items.at(index).quantity)
           .toUtf8().constData());
        index++;
    }

    //send the network reply
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (quint8)0x00;
    out << characterId;
    postReply(query_id,outputData);
}

void CharactersGroupForLogin::removeCharacter(void * const client,const quint8 &query_id, const quint32 &characterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQuery::db_query_account_time_to_delete_character_by_id==NULL)
    {
        qDebug() << (QStringLiteral("removeCharacter() Query is empty, bug"));
        return;
    }
    #endif
    RemoveCharacterParam *removeCharacterParam=new RemoveCharacterParam;
    removeCharacterParam->query_id=query_id;
    removeCharacterParam->characterId=characterId;
    removeCharacterParam->client=client;

    const QString &queryText=PreparedDBQuery::db_query_account_time_to_delete_character_by_id.arg(characterId);
    CatchChallenger::DatabaseBase::CallBack *callback=databaseBaseCommon->asyncRead(queryText.toLatin1(),this,&CharactersGroupForLogin::removeCharacter_static);
    if(callback==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(databaseBaseCommon->errorMessage());
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
        out << (quint8)0x02;
        postReply(query_id,outputData);
        delete removeCharacterParam;
        return;
    }
    else
    {
        callbackRegistred << callback;
        removeCharacterParamList << removeCharacterParam;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        paramToPassToCallBackType << QStringLiteral("RemoveCharacterParam");
        #endif
    }
}

void CharactersGroupForLogin::removeCharacter_static(void *object)
{
    if(object!=NULL)
        static_cast<CharactersGroupForLogin *>(object)->removeCharacter_object();
}

void CharactersGroupForLogin::removeCharacter_object()
{
    RemoveCharacterParam *removeCharacterParam=static_cast<RemoveCharacterParam *>(paramToPassToCallBack.takeFirst());
    removeCharacter_return(removeCharacterParam->query_id,removeCharacterParam->characterId);
    delete removeCharacterParam;
    databaseBaseCommon->clear();
}

void CharactersGroupForLogin::removeCharacter_return(const quint8 &query_id,const quint32 &characterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBackType.takeFirst()!=QStringLiteral("RemoveCharacterParam"))
    {
        qDebug() << "is not RemoveCharacterParam" << paramToPassToCallBackType.join(";") << __FILE__ << __LINE__;
        abort();
    }
    #endif
    if(!databaseBaseCommon->next())
    {
        characterSelectionIsWrong(query_id,0x02,"Result return query to remove wrong");
        return;
    }
    bool ok;
    const quint32 &account_id=QString(databaseBaseCommon->value(0)).toUInt(&ok);
    if(!ok)
    {
        characterSelectionIsWrong(query_id,0x02,QStringLiteral("Account for character: %1 is not an id").arg(databaseBaseCommon->value(0)));
        return;
    }
    if(this->account_id!=account_id)
    {
        characterSelectionIsWrong(query_id,0x02,QStringLiteral("Character: %1 is not owned by the account: %2").arg(characterId).arg(account_id));
        return;
    }
    const quint32 &time_to_delete=QString(databaseBaseCommon->value(1)).toUInt(&ok);
    if(ok && time_to_delete>0)
    {
        characterSelectionIsWrong(query_id,0x02,QStringLiteral("Character: %1 is already in deleting for the account: %2").arg(characterId).arg(account_id));
        return;
    }
    dbQueryWriteCommon(PreparedDBQuery::db_query_update_character_time_to_delete_by_id.arg(characterId).arg(character_delete_time).toUtf8().constData());
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (quint8)0x02;
    postReply(query_id,outputData);
}

void CharactersGroupForLogin::dbQueryWriteCommon(const char * const queryText)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(queryText==NULL || queryText[0]=='\0')
    {
        qDebug() << (QStringLiteral("dbQuery() Query is empty, bug"));
        return;
    }
    #endif
    #ifdef DEBUG_MESSAGE_CLIENT_SQL
    qDebug() << (QStringLiteral("Do db write: ")+queryText);
    #endif
    databaseBaseCommon->asyncWrite(queryText);
}
