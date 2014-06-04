#include "LocalClientHandler.h"
#include "../../general/base/ProtocolParsing.h"
#include "../../general/base/CommonDatapack.h"
#include "GlobalServerData.h"
#ifdef EPOLLCATCHCHALLENGERSERVER
#include "Client.h"
#endif

using namespace CatchChallenger;

//quest
void LocalClientHandler::newQuestAction(const QuestAction &action,const quint32 &questId)
{
    if(!CommonDatapack::commonDatapack.quests.contains(questId))
    {
        /*emit */error(QStringLiteral("unknown questId: %1").arg(questId));
        return;
    }
    const Quest &quest=CommonDatapack::commonDatapack.quests.value(questId);
    switch(action)
    {
        case QuestAction_Start:
            if(!haveStartQuestRequirement(quest))
            {
                /*emit */error(QStringLiteral("have not quest requirement: %1").arg(questId));
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
                /*emit */error(QStringLiteral("have not next step quest requirement: %1").arg(questId));
                return;
            }
            nextStepQuest(quest);
        break;
        default:
            /*emit */error(QStringLiteral("newQuestAction unknow: %1").arg(action));
        return;
    }
}

void LocalClientHandler::addQuestStepDrop(const quint32 &questId,const quint8 &questStep)
{
    #ifdef DEBUG_MESSAGE_CLIENT_QUESTS
    /*emit */message(QStringLiteral("addQuestStepDrop for quest: %1, step: %2").arg(questId).arg(questStep));
    #endif
    if(!addQuestStepDrop(player_informations,questId,questStep))
        /*emit */message(QStringLiteral("error append drop for quest have failed: %1").arg(questId));
}

void LocalClientHandler::removeQuestStepDrop(const quint32 &questId,const quint8 &questStep)
{
    #ifdef DEBUG_MESSAGE_CLIENT_QUESTS
    /*emit */message(QStringLiteral("removeQuestStepDrop for quest: %1, step: %2").arg(questId).arg(questStep));
    #endif
    if(!removeQuestStepDrop(player_informations,questId,questStep))
        /*emit */message(QStringLiteral("error append drop for quest have failed: %1").arg(questId));
}

bool LocalClientHandler::addQuestStepDrop(Player_internal_informations *player_informations,const quint32 &questId,const quint8 &step)
{

    if(!CommonDatapack::commonDatapack.quests.contains(questId))
        return false;
    const CatchChallenger::Quest &quest=CommonDatapack::commonDatapack.quests.value(questId);
    if(step<=0 || step>quest.steps.size())
        return false;
    Quest::Step stepFull=quest.steps.value(step-1);
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
    if(!CommonDatapack::commonDatapack.quests.contains(questId))
        return false;
    const CatchChallenger::Quest &quest=CommonDatapack::commonDatapack.quests.value(questId);
    if(step<=0 || step>quest.steps.size())
        return false;
    Quest::Step stepFull=quest.steps.value(step-1);
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
    /*emit */message(QStringLiteral("check quest step requirement for: %1").arg(quest.id));
    #endif
    if(!player_informations->public_and_private_informations.quests.contains(quest.id))
    {
        /*emit */message(QStringLiteral("player quest not found: %1").arg(quest.id));
        return false;
    }
    const quint8 &step=player_informations->public_and_private_informations.quests.value(quest.id).step;
    if(step<=0 || step>quest.steps.size())
    {
        /*emit */message(QStringLiteral("step out of range for: %1").arg(quest.id));
        return false;
    }
    const CatchChallenger::Quest::StepRequirements &requirements=quest.steps.at(step-1).requirements;
    int index=0;
    while(index<requirements.items.size())
    {
        const CatchChallenger::Quest::Item &item=requirements.items.at(index);
        if(objectQuantity(item.item)<item.quantity)
        {
            /*emit */message(QStringLiteral("quest requirement, have not the quantity for the item: %1").arg(item.item));
            return false;
        }
        index++;
    }
    index=0;
    while(index<requirements.fightId.size())
    {
        const quint32 &fightId=requirements.fightId.at(index);
        if(!player_informations->public_and_private_informations.bot_already_beaten.contains(fightId))
        {
            /*emit */message(QStringLiteral("quest requirement, have not beat the bot: %1").arg(fightId));
            return false;
        }
        index++;
    }
    return true;
}

bool LocalClientHandler::haveStartQuestRequirement(const CatchChallenger::Quest &quest)
{
    #ifdef DEBUG_MESSAGE_CLIENT_QUESTS
    /*emit */message(QStringLiteral("check quest requirement for: %1").arg(quest.id));
    #endif
    if(player_informations->public_and_private_informations.quests.contains(quest.id))
    {
        if(player_informations->public_and_private_informations.quests.value(quest.id).step!=0)
        {
            /*emit */message(QStringLiteral("can start the quest because is already running: %1").arg(quest.id));
            return false;
        }
        if(player_informations->public_and_private_informations.quests.value(quest.id).finish_one_time && !quest.repeatable)
        {
            /*emit */message(QStringLiteral("done one time and no repeatable: %1").arg(quest.id));
            return false;
        }
    }
    int index=0;
    while(index<quest.requirements.quests.size())
    {
        const quint32 &questId=quest.requirements.quests.at(index);
        if(!player_informations->public_and_private_informations.quests.contains(questId))
        {
            /*emit */message(QStringLiteral("have never started the quest: %1").arg(questId));
            return false;
        }
        if(!player_informations->public_and_private_informations.quests.value(questId).finish_one_time)
        {
            /*emit */message(QStringLiteral("quest never finished: %1").arg(questId));
            return false;
        }
        index++;
    }
    return haveReputationRequirements(quest.requirements.reputation);
}

bool LocalClientHandler::nextStepQuest(const Quest &quest)
{
    #ifdef DEBUG_MESSAGE_CLIENT_QUESTS
    /*emit */message(QStringLiteral("drop quest step requirement for: %1").arg(quest.id));
    #endif
    if(!player_informations->public_and_private_informations.quests.contains(quest.id))
    {
        /*emit */message(QStringLiteral("step out of range for: %1").arg(quest.id));
        return false;
    }
    quint8 step=player_informations->public_and_private_informations.quests.value(quest.id).step;
    if(step<=0 || step>quest.steps.size())
    {
        /*emit */message(QStringLiteral("step out of range for: %1").arg(quest.id));
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
    removeQuestStepDrop(quest.id,player_informations->public_and_private_informations.quests.value(quest.id).step);
    player_informations->public_and_private_informations.quests[quest.id].step++;
    if(player_informations->public_and_private_informations.quests.value(quest.id).step>quest.steps.size())
    {
        #ifdef DEBUG_MESSAGE_CLIENT_QUESTS
        /*emit */message(QStringLiteral("finish the quest: %1").arg(quest.id));
        #endif
        switch(GlobalServerData::serverSettings.database.type)
        {
            default:
            case ServerSettings::Database::DatabaseType_Mysql:
                /*emit */dbQuery(QStringLiteral("UPDATE `quest` SET `step`=0,`finish_one_time`=1 WHERE `character`=%1 AND quest=%2;")
                             .arg(player_informations->character_id)
                             .arg(quest.id)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                /*emit */dbQuery(QStringLiteral("UPDATE quest SET step=0,finish_one_time=1 WHERE character=%1 AND quest=%2;")
                             .arg(player_informations->character_id)
                             .arg(quest.id)
                             );
            break;
            case ServerSettings::Database::DatabaseType_PostgreSQL:
                /*emit */dbQuery(QStringLiteral("UPDATE quest SET step=0,finish_one_time=true WHERE character=%1 AND quest=%2;")
                             .arg(player_informations->character_id)
                             .arg(quest.id)
                             );
            break;
        }
        player_informations->public_and_private_informations.quests[quest.id].step=0;
        player_informations->public_and_private_informations.quests[quest.id].finish_one_time=true;
        index=0;
        while(index<quest.rewards.items.size())
        {
            addObjectAndSend(quest.rewards.items.value(index).item,quest.rewards.items.value(index).quantity);
            index++;
        }
        index=0;
        while(index<quest.rewards.reputation.size())
        {
            appendReputationPoint(quest.rewards.reputation.value(index).type,quest.rewards.reputation.value(index).point);
            index++;
        }
        index=0;
        while(index<quest.rewards.allow.size())
        {
            appendAllow(quest.rewards.allow.at(index));
            index++;
        }
    }
    else
    {
        #ifdef DEBUG_MESSAGE_CLIENT_QUESTS
        /*emit */message(QStringLiteral("next step in the quest: %1").arg(quest.id));
        #endif
        switch(GlobalServerData::serverSettings.database.type)
        {
            default:
            case ServerSettings::Database::DatabaseType_Mysql:
                /*emit */dbQuery(QStringLiteral("UPDATE `quest` SET `step`=%3 WHERE `character`=%1 AND `quest`=%2;")
                             .arg(player_informations->character_id)
                             .arg(quest.id)
                             .arg(player_informations->public_and_private_informations.quests.value(quest.id).step)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                /*emit */dbQuery(QStringLiteral("UPDATE quest SET step=%3 WHERE character=%1 AND quest=%2;")
                             .arg(player_informations->character_id)
                             .arg(quest.id)
                             .arg(player_informations->public_and_private_informations.quests.value(quest.id).step)
                             );
            break;
            case ServerSettings::Database::DatabaseType_PostgreSQL:
                /*emit */dbQuery(QStringLiteral("UPDATE quest SET step=%3 WHERE character=%1 AND quest=%2;")
                             .arg(player_informations->character_id)
                             .arg(quest.id)
                             .arg(player_informations->public_and_private_informations.quests.value(quest.id).step)
                             );
            break;
        }
        addQuestStepDrop(quest.id,player_informations->public_and_private_informations.quests.value(quest.id).step);
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
                /*emit */dbQuery(QStringLiteral("INSERT INTO `quest`(`character`,`quest`,`finish_one_time`,`step`) VALUES(%1,%2,%3,%4);")
                             .arg(player_informations->character_id)
                             .arg(quest.id)
                             .arg(0)
                             .arg(1)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                /*emit */dbQuery(QStringLiteral("INSERT INTO quest(character,quest,finish_one_time,step) VALUES(%1,%2,%3,%4);")
                             .arg(player_informations->character_id)
                             .arg(quest.id)
                             .arg(0)
                             .arg(1)
                             );
            break;
            case ServerSettings::Database::DatabaseType_PostgreSQL:
                /*emit */dbQuery(QStringLiteral("INSERT INTO quest(character,quest,finish_one_time,step) VALUES(%1,%2,false,%3);")
                             .arg(player_informations->character_id)
                             .arg(quest.id)
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
                /*emit */dbQuery(QStringLiteral("UPDATE `quest` SET `step`=%3 WHERE `character`=%1 AND quest=%2;")
                             .arg(player_informations->character_id)
                             .arg(quest.id)
                             .arg(1)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                /*emit */dbQuery(QStringLiteral("UPDATE quest SET step=%3 WHERE character=%1 AND quest=%2;")
                             .arg(player_informations->character_id)
                             .arg(quest.id)
                             .arg(1)
                             );
            break;
            case ServerSettings::Database::DatabaseType_PostgreSQL:
                /*emit */dbQuery(QStringLiteral("UPDATE quest SET step=%3 WHERE character=%1 AND quest=%2;")
                             .arg(player_informations->character_id)
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
