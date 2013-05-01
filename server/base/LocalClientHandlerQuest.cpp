#include "LocalClientHandler.h"
#include "../../general/base/ProtocolParsing.h"
#include "GlobalServerData.h"

using namespace CatchChallenger;

//quest
void LocalClientHandler::newQuestAction(const QuestAction &action,const quint32 &questId)
{
    if(!GlobalServerData::serverPrivateVariables.quests.contains(questId))
    {
        emit error(QString("unknow questId: %1").arg(questId));
        return;
    }
    const Quest &quest=GlobalServerData::serverPrivateVariables.quests[questId];
    switch(action)
    {
        case QuestAction_Start:
            if(!haveStartQuestRequirement(quest))
            {
                emit error(QString("have not quest requirement: %1").arg(questId));
                return;
            }
            startQuest(quest);
        break;
        case QuestAction_Cancel:
        break;
        case QuestAction_Finish:
        case QuestAction_NextStep:
            if(!haveNextStepQuestRequirements(quest))
            {
                emit error(QString("have not next step quest requirement: %1").arg(questId));
                return;
            }
            nextStepQuest(quest);
        break;
        default:
            emit error(QString("newQuestAction unknow: %1").arg(action));
        return;
    }
}

void LocalClientHandler::addQuestStepDrop(const quint32 &questId,const quint8 &questStep)
{
    #ifdef DEBUG_MESSAGE_CLIENT_QUESTS
    emit message(QString("addQuestStepDrop for quest: %1, step: %2").arg(questId).arg(questStep));
    #endif
    if(!addQuestStepDrop(player_informations,questId,questStep))
        emit message(QString("error append drop for quest have failed: %1").arg(questId));
}

void LocalClientHandler::removeQuestStepDrop(const quint32 &questId,const quint8 &questStep)
{
    #ifdef DEBUG_MESSAGE_CLIENT_QUESTS
    emit message(QString("removeQuestStepDrop for quest: %1, step: %2").arg(questId).arg(questStep));
    #endif
    if(!removeQuestStepDrop(player_informations,questId,questStep))
        emit message(QString("error append drop for quest have failed: %1").arg(questId));
}

bool LocalClientHandler::addQuestStepDrop(Player_internal_informations *player_informations,const quint32 &questId,const quint8 &step)
{

    if(!GlobalServerData::serverPrivateVariables.quests.contains(questId))
        return false;
    const CatchChallenger::Quest &quest=GlobalServerData::serverPrivateVariables.quests[questId];
    if(step<=0 || step>quest.steps.size())
        return false;
    Quest::Step stepFull=quest.steps[step-1];
    int index=0;
    int sub_index;
    while(index<stepFull.itemsMonster.size())
    {
        const Quest::ItemMonster &itemMonster=stepFull.itemsMonster.at(index);
        sub_index=0;
        const MonsterDrops &monsterDrops=questItemMonsterToMonsterDrops(itemMonster);
        while(sub_index<itemMonster.monsters.size())
        {
            player_informations->questsDrop.insert(itemMonster.monsters.at(sub_index),monsterDrops);
            sub_index++;
        }
        index++;
    }
    return true;
}

bool LocalClientHandler::removeQuestStepDrop(Player_internal_informations *player_informations, const quint32 &questId, const quint8 &step)
{
    if(!GlobalServerData::serverPrivateVariables.quests.contains(questId))
        return false;
    const CatchChallenger::Quest &quest=GlobalServerData::serverPrivateVariables.quests[questId];
    if(step<=0 || step>quest.steps.size())
        return false;
    Quest::Step stepFull=quest.steps[step-1];
    int index=0;
    int sub_index;
    while(index<stepFull.itemsMonster.size())
    {
        const Quest::ItemMonster &itemMonster=stepFull.itemsMonster.at(index);
        sub_index=0;
        const MonsterDrops &monsterDrops=questItemMonsterToMonsterDrops(itemMonster);
        while(sub_index<itemMonster.monsters.size())
        {
            player_informations->questsDrop.remove(itemMonster.monsters.at(sub_index),monsterDrops);
            sub_index++;
        }
        index++;
    }
    return true;
}

MonsterDrops LocalClientHandler::questItemMonsterToMonsterDrops(const Quest::ItemMonster &questItemMonster)
{
    MonsterDrops monsterDrops;
    monsterDrops.item=questItemMonster.item;
    monsterDrops.luck=questItemMonster.rate;
    monsterDrops.quantity_min=1;
    monsterDrops.quantity_max=1;
    return monsterDrops;
}

bool LocalClientHandler::haveNextStepQuestRequirements(const CatchChallenger::Quest &quest)
{
    #ifdef DEBUG_MESSAGE_CLIENT_QUESTS
    emit message(QString("check quest step requirement for: %1").arg(quest.id));
    #endif
    if(!player_informations->public_and_private_informations.quests.contains(quest.id))
    {
        emit message(QString("player quest not found: %1").arg(quest.id));
        return false;
    }
    quint8 step=player_informations->public_and_private_informations.quests[quest.id].step;
    if(step<=0 || step>quest.steps.size())
    {
        emit message(QString("step out of range for: %1").arg(quest.id));
        return false;
    }
    const CatchChallenger::Quest::StepRequirements &requirements=quest.steps.at(step-1).requirements;
    int index=0;
    while(index<requirements.items.size())
    {
        const CatchChallenger::Quest::Item &item=requirements.items.at(index);
        if(objectQuantity(item.item)<item.quantity)
        {
            emit message(QString("have not the quantity for the item: %1").arg(item.item));
            return false;
        }
        index++;
    }
    return true;
}

bool LocalClientHandler::haveStartQuestRequirement(const CatchChallenger::Quest &quest)
{
    #ifdef DEBUG_MESSAGE_CLIENT_QUESTS
    emit message(QString("check quest requirement for: %1").arg(quest.id));
    #endif
    if(player_informations->public_and_private_informations.quests.contains(quest.id))
    {
        if(player_informations->public_and_private_informations.quests[quest.id].step!=0)
        {
            emit message(QString("can start the quest because is already running: %1").arg(quest.id));
            return false;
        }
        if(player_informations->public_and_private_informations.quests[quest.id].finish_one_time && !quest.repeatable)
        {
            emit message(QString("done one time and no repeatable: %1").arg(quest.id));
            return false;
        }
    }
    int index=0;
    while(index<quest.requirements.quests.size())
    {
        const quint32 &questId=quest.requirements.quests.at(index);
        if(!player_informations->public_and_private_informations.quests.contains(questId))
        {
            emit message(QString("have never started the quest: %1").arg(questId));
            return false;
        }
        if(!player_informations->public_and_private_informations.quests[questId].finish_one_time)
        {
            emit message(QString("quest never finished: %1").arg(questId));
            return false;
        }
        index++;
    }
    index=0;
    while(index<quest.requirements.reputation.size())
    {
        const CatchChallenger::Quest::ReputationRequirements &reputation=quest.requirements.reputation.at(index);
        if(player_informations->public_and_private_informations.reputation.contains(reputation.type))
        {
            const PlayerReputation &playerReputation=player_informations->public_and_private_informations.reputation[reputation.type];
            if(!reputation.positif)
            {
                if(-reputation.level<playerReputation.level)
                {
                    emit message(QString("reputation.level(%1)<playerReputation.level(%2)").arg(reputation.level).arg(playerReputation.level));
                    return false;
                }
            }
            else
            {
                if(reputation.level>playerReputation.level || playerReputation.point<0)
                {
                    emit message(QString("reputation.level(%1)>playerReputation.level(%2) || playerReputation.point(%3)<0").arg(reputation.level).arg(playerReputation.level).arg(playerReputation.point));
                    return false;
                }
            }
        }
        else
            if(!reputation.positif)//default level is 0, but required level is negative
            {
                emit message(QString("reputation.level(%1)<0 and no reputation.type=%2").arg(reputation.level).arg(reputation.type));
                return false;
            }
        index++;
    }
    return true;
}

bool LocalClientHandler::nextStepQuest(const Quest &quest)
{
    #ifdef DEBUG_MESSAGE_CLIENT_QUESTS
    emit message(QString("drop quest step requirement for: %1").arg(quest.id));
    #endif
    if(!player_informations->public_and_private_informations.quests.contains(quest.id))
    {
        emit message(QString("step out of range for: %1").arg(quest.id));
        return false;
    }
    quint8 step=player_informations->public_and_private_informations.quests[quest.id].step;
    if(step<=0 || step>quest.steps.size())
    {
        emit message(QString("step out of range for: %1").arg(quest.id));
        return false;
    }
    const CatchChallenger::Quest::StepRequirements &requirements=quest.steps.at(step-1).requirements;
    int index=0;
    while(index<requirements.items.size())
    {
        const CatchChallenger::Quest::Item &item=requirements.items.at(index);
        removeObject(item.item,item.quantity);
        index++;
    }
    removeQuestStepDrop(quest.id,player_informations->public_and_private_informations.quests[quest.id].step);
    player_informations->public_and_private_informations.quests[quest.id].step++;
    if(player_informations->public_and_private_informations.quests[quest.id].step>quest.steps.size())
    {
        #ifdef DEBUG_MESSAGE_CLIENT_QUESTS
        emit message(QString("finish the quest: %1").arg(quest.id));
        #endif
        switch(GlobalServerData::serverSettings.database.type)
        {
            default:
            case ServerSettings::Database::DatabaseType_Mysql:
                emit dbQuery(QString("UPDATE quest SET step=0,finish_one_time=1 WHERE player=%1 AND quest=%2;")
                             .arg(player_informations->id)
                             .arg(quest.id)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                emit dbQuery(QString("UPDATE quest SET step=0,finish_one_time=1 WHERE player=%1 AND quest=%2;")
                             .arg(player_informations->id)
                             .arg(quest.id)
                             );
            break;
        }
        player_informations->public_and_private_informations.quests[quest.id].step=0;
        player_informations->public_and_private_informations.quests[quest.id].finish_one_time=true;
        index=0;
        while(index<quest.rewards.items.size())
        {
            addObjectAndSend(quest.rewards.items[index].item,quest.rewards.items[index].quantity);
            index++;
        }
        index=0;
        while(index<quest.rewards.reputation.size())
        {
            appendReputationPoint(quest.rewards.reputation[index].type,quest.rewards.reputation[index].point);
            index++;
        }
    }
    else
    {
        #ifdef DEBUG_MESSAGE_CLIENT_QUESTS
        emit message(QString("next step in the quest: %1").arg(quest.id));
        #endif
        switch(GlobalServerData::serverSettings.database.type)
        {
            default:
            case ServerSettings::Database::DatabaseType_Mysql:
                emit dbQuery(QString("UPDATE quest SET step=%3 WHERE player=%1 AND quest=%2;")
                             .arg(player_informations->id)
                             .arg(quest.id)
                             .arg(player_informations->public_and_private_informations.quests[quest.id].step)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                emit dbQuery(QString("UPDATE quest SET step=%3 WHERE player=%1 AND quest=%2;")
                             .arg(player_informations->id)
                             .arg(quest.id)
                             .arg(player_informations->public_and_private_informations.quests[quest.id].step)
                             );
            break;
        }
        addQuestStepDrop(quest.id,player_informations->public_and_private_informations.quests[quest.id].step);
    }
    return true;
}

bool LocalClientHandler::startQuest(const Quest &quest)
{
    if(!player_informations->public_and_private_informations.quests.contains(quest.id))
    {
        switch(GlobalServerData::serverSettings.database.type)
        {
            default:
            case ServerSettings::Database::DatabaseType_Mysql:
                emit dbQuery(QString("INSERT INTO quest(player,quest,finish_one_time,step) VALUES(%1,%2,%3,%4);")
                             .arg(player_informations->id)
                             .arg(quest.id)
                             .arg(0)
                             .arg(1)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                emit dbQuery(QString("INSERT INTO quest(player,quest,finish_one_time,step) VALUES(%1,%2,%3,%4);")
                             .arg(player_informations->id)
                             .arg(quest.id)
                             .arg(0)
                             .arg(1)
                             );
            break;
        }
        player_informations->public_and_private_informations.quests[quest.id].step=1;
        player_informations->public_and_private_informations.quests[quest.id].finish_one_time=false;
    }
    else
    {
        switch(GlobalServerData::serverSettings.database.type)
        {
            default:
            case ServerSettings::Database::DatabaseType_Mysql:
                emit dbQuery(QString("UPDATE quest SET step=%3 WHERE player=%1 AND quest=%2;")
                             .arg(player_informations->id)
                             .arg(quest.id)
                             .arg(1)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                emit dbQuery(QString("UPDATE quest SET step=%3 WHERE player=%1 AND quest=%2;")
                             .arg(player_informations->id)
                             .arg(quest.id)
                             .arg(1)
                             );
            break;
        }
        player_informations->public_and_private_informations.quests[quest.id].step=1;
    }
    addQuestStepDrop(quest.id,1);
    return true;
}
