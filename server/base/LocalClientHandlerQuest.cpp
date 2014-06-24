#include "Client.h"
#include "../../general/base/ProtocolParsing.h"
#include "../../general/base/CommonDatapack.h"
#include "GlobalServerData.h"

using namespace CatchChallenger;

//quest
void Client::newQuestAction(const QuestAction &action,const quint32 &questId)
{
    if(!CommonDatapack::commonDatapack.quests.contains(questId))
    {
        errorOutput(QStringLiteral("unknown questId: %1").arg(questId));
        return;
    }
    const Quest &quest=CommonDatapack::commonDatapack.quests.value(questId);
    switch(action)
    {
        case QuestAction_Start:
            if(!haveStartQuestRequirement(quest))
            {
                errorOutput(QStringLiteral("have not quest requirement: %1").arg(questId));
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
                errorOutput(QStringLiteral("have not next step quest requirement: %1").arg(questId));
                return;
            }
            nextStepQuest(quest);
        break;
        default:
            errorOutput(QStringLiteral("newQuestAction unknow: %1").arg(action));
        return;
    }
}

void Client::addQuestStepDrop(const quint32 &questId,const quint8 &questStep)
{
    #ifdef DEBUG_MESSAGE_CLIENT_QUESTS
    normalOutput(QStringLiteral("addQuestStepDrop for quest: %1, step: %2").arg(questId).arg(questStep));
    #endif
    if(!CommonDatapack::commonDatapack.quests.contains(questId))
    {
        errorOutput("Quest not found for drops");
        return;
    }
    const CatchChallenger::Quest &quest=CommonDatapack::commonDatapack.quests.value(questId);
    if(questStep<=0 || questStep>quest.steps.size())
    {
        errorOutput("Quest step out of range for drops");
        return;
    }
    Quest::Step stepFull=quest.steps.value(questStep-1);
    int index=0;
    int sub_index;
    while(index<stepFull.itemsMonster.size())
    {
        const Quest::ItemMonster &itemMonster=stepFull.itemsMonster.at(index);
        sub_index=0;
        const MonsterDrops &monsterDrops=questItemMonsterToMonsterDrops(itemMonster);
        while(sub_index<itemMonster.monsters.size())
        {
            questsDrop.insert(itemMonster.monsters.at(sub_index),monsterDrops);
            sub_index++;
        }
        index++;
    }
}

void Client::removeQuestStepDrop(const quint32 &questId,const quint8 &questStep)
{
    #ifdef DEBUG_MESSAGE_CLIENT_QUESTS
    normalOutput(QStringLiteral("removeQuestStepDrop for quest: %1, step: %2").arg(questId).arg(questStep));
    #endif
    if(!CommonDatapack::commonDatapack.quests.contains(questId))
    {
        errorOutput("Quest not found for drops");
        return;
    }
    const CatchChallenger::Quest &quest=CommonDatapack::commonDatapack.quests.value(questId);
    if(questStep<=0 || questStep>quest.steps.size())
    {
        errorOutput("Quest step out of range for drops");
        return;
    }
    Quest::Step stepFull=quest.steps.value(questStep-1);
    int index=0;
    int sub_index;
    while(index<stepFull.itemsMonster.size())
    {
        const Quest::ItemMonster &itemMonster=stepFull.itemsMonster.at(index);
        sub_index=0;
        const MonsterDrops &monsterDrops=questItemMonsterToMonsterDrops(itemMonster);
        while(sub_index<itemMonster.monsters.size())
        {
            questsDrop.remove(itemMonster.monsters.at(sub_index),monsterDrops);
            sub_index++;
        }
        index++;
    }
}

MonsterDrops Client::questItemMonsterToMonsterDrops(const Quest::ItemMonster &questItemMonster)
{
    MonsterDrops monsterDrops;
    monsterDrops.item=questItemMonster.item;
    monsterDrops.luck=questItemMonster.rate;
    monsterDrops.quantity_min=1;
    monsterDrops.quantity_max=1;
    return monsterDrops;
}

bool Client::haveNextStepQuestRequirements(const CatchChallenger::Quest &quest)
{
    #ifdef DEBUG_MESSAGE_CLIENT_QUESTS
    normalOutput(QStringLiteral("check quest step requirement for: %1").arg(quest.id));
    #endif
    if(!public_and_private_informations.quests.contains(quest.id))
    {
        normalOutput(QStringLiteral("player quest not found: %1").arg(quest.id));
        return false;
    }
    const quint8 &step=public_and_private_informations.quests.value(quest.id).step;
    if(step<=0 || step>quest.steps.size())
    {
        normalOutput(QStringLiteral("step out of range for: %1").arg(quest.id));
        return false;
    }
    const CatchChallenger::Quest::StepRequirements &requirements=quest.steps.at(step-1).requirements;
    int index=0;
    while(index<requirements.items.size())
    {
        const CatchChallenger::Quest::Item &item=requirements.items.at(index);
        if(objectQuantity(item.item)<item.quantity)
        {
            normalOutput(QStringLiteral("quest requirement, have not the quantity for the item: %1").arg(item.item));
            return false;
        }
        index++;
    }
    index=0;
    while(index<requirements.fightId.size())
    {
        const quint32 &fightId=requirements.fightId.at(index);
        if(!public_and_private_informations.bot_already_beaten.contains(fightId))
        {
            normalOutput(QStringLiteral("quest requirement, have not beat the bot: %1").arg(fightId));
            return false;
        }
        index++;
    }
    return true;
}

bool Client::haveStartQuestRequirement(const CatchChallenger::Quest &quest)
{
    #ifdef DEBUG_MESSAGE_CLIENT_QUESTS
    normalOutput(QStringLiteral("check quest requirement for: %1").arg(quest.id));
    #endif
    if(public_and_private_informations.quests.contains(quest.id))
    {
        if(public_and_private_informations.quests.value(quest.id).step!=0)
        {
            normalOutput(QStringLiteral("can start the quest because is already running: %1").arg(quest.id));
            return false;
        }
        if(public_and_private_informations.quests.value(quest.id).finish_one_time && !quest.repeatable)
        {
            normalOutput(QStringLiteral("done one time and no repeatable: %1").arg(quest.id));
            return false;
        }
    }
    int index=0;
    while(index<quest.requirements.quests.size())
    {
        const quint32 &questId=quest.requirements.quests.at(index);
        if(!public_and_private_informations.quests.contains(questId))
        {
            normalOutput(QStringLiteral("have never started the quest: %1").arg(questId));
            return false;
        }
        if(!public_and_private_informations.quests.value(questId).finish_one_time)
        {
            normalOutput(QStringLiteral("quest never finished: %1").arg(questId));
            return false;
        }
        index++;
    }
    return haveReputationRequirements(quest.requirements.reputation);
}

bool Client::nextStepQuest(const Quest &quest)
{
    #ifdef DEBUG_MESSAGE_CLIENT_QUESTS
    normalOutput(QStringLiteral("drop quest step requirement for: %1").arg(quest.id));
    #endif
    if(!public_and_private_informations.quests.contains(quest.id))
    {
        normalOutput(QStringLiteral("step out of range for: %1").arg(quest.id));
        return false;
    }
    quint8 step=public_and_private_informations.quests.value(quest.id).step;
    if(step<=0 || step>quest.steps.size())
    {
        normalOutput(QStringLiteral("step out of range for: %1").arg(quest.id));
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
    removeQuestStepDrop(quest.id,public_and_private_informations.quests.value(quest.id).step);
    public_and_private_informations.quests[quest.id].step++;
    if(public_and_private_informations.quests.value(quest.id).step>quest.steps.size())
    {
        #ifdef DEBUG_MESSAGE_CLIENT_QUESTS
        normalOutput(QStringLiteral("finish the quest: %1").arg(quest.id));
        #endif
        switch(GlobalServerData::serverSettings.database.type)
        {
            default:
            case ServerSettings::Database::DatabaseType_Mysql:
                dbQueryWrite(QStringLiteral("UPDATE `quest` SET `step`=0,`finish_one_time`=1 WHERE `character`=%1 AND quest=%2;")
                             .arg(character_id)
                             .arg(quest.id)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                dbQueryWrite(QStringLiteral("UPDATE quest SET step=0,finish_one_time=1 WHERE character=%1 AND quest=%2;")
                             .arg(character_id)
                             .arg(quest.id)
                             );
            break;
            case ServerSettings::Database::DatabaseType_PostgreSQL:
                dbQueryWrite(QStringLiteral("UPDATE quest SET step=0,finish_one_time=true WHERE character=%1 AND quest=%2;")
                             .arg(character_id)
                             .arg(quest.id)
                             );
            break;
        }
        public_and_private_informations.quests[quest.id].step=0;
        public_and_private_informations.quests[quest.id].finish_one_time=true;
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
        normalOutput(QStringLiteral("next step in the quest: %1").arg(quest.id));
        #endif
        switch(GlobalServerData::serverSettings.database.type)
        {
            default:
            case ServerSettings::Database::DatabaseType_Mysql:
                dbQueryWrite(QStringLiteral("UPDATE `quest` SET `step`=%3 WHERE `character`=%1 AND `quest`=%2;")
                             .arg(character_id)
                             .arg(quest.id)
                             .arg(public_and_private_informations.quests.value(quest.id).step)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                dbQueryWrite(QStringLiteral("UPDATE quest SET step=%3 WHERE character=%1 AND quest=%2;")
                             .arg(character_id)
                             .arg(quest.id)
                             .arg(public_and_private_informations.quests.value(quest.id).step)
                             );
            break;
            case ServerSettings::Database::DatabaseType_PostgreSQL:
                dbQueryWrite(QStringLiteral("UPDATE quest SET step=%3 WHERE character=%1 AND quest=%2;")
                             .arg(character_id)
                             .arg(quest.id)
                             .arg(public_and_private_informations.quests.value(quest.id).step)
                             );
            break;
        }
        addQuestStepDrop(quest.id,public_and_private_informations.quests.value(quest.id).step);
    }
    return true;
}

bool Client::startQuest(const Quest &quest)
{
    if(!public_and_private_informations.quests.contains(quest.id))
    {
        switch(GlobalServerData::serverSettings.database.type)
        {
            default:
            case ServerSettings::Database::DatabaseType_Mysql:
                dbQueryWrite(QStringLiteral("INSERT INTO `quest`(`character`,`quest`,`finish_one_time`,`step`) VALUES(%1,%2,%3,%4);")
                             .arg(character_id)
                             .arg(quest.id)
                             .arg(0)
                             .arg(1)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                dbQueryWrite(QStringLiteral("INSERT INTO quest(character,quest,finish_one_time,step) VALUES(%1,%2,%3,%4);")
                             .arg(character_id)
                             .arg(quest.id)
                             .arg(0)
                             .arg(1)
                             );
            break;
            case ServerSettings::Database::DatabaseType_PostgreSQL:
                dbQueryWrite(QStringLiteral("INSERT INTO quest(character,quest,finish_one_time,step) VALUES(%1,%2,false,%3);")
                             .arg(character_id)
                             .arg(quest.id)
                             .arg(1)
                             );
            break;
        }
        public_and_private_informations.quests[quest.id].step=1;
        public_and_private_informations.quests[quest.id].finish_one_time=false;
    }
    else
    {
        switch(GlobalServerData::serverSettings.database.type)
        {
            default:
            case ServerSettings::Database::DatabaseType_Mysql:
                dbQueryWrite(QStringLiteral("UPDATE `quest` SET `step`=%3 WHERE `character`=%1 AND quest=%2;")
                             .arg(character_id)
                             .arg(quest.id)
                             .arg(1)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                dbQueryWrite(QStringLiteral("UPDATE quest SET step=%3 WHERE character=%1 AND quest=%2;")
                             .arg(character_id)
                             .arg(quest.id)
                             .arg(1)
                             );
            break;
            case ServerSettings::Database::DatabaseType_PostgreSQL:
                dbQueryWrite(QStringLiteral("UPDATE quest SET step=%3 WHERE character=%1 AND quest=%2;")
                             .arg(character_id)
                             .arg(quest.id)
                             .arg(1)
                             );
            break;
        }
        public_and_private_informations.quests[quest.id].step=1;
    }
    addQuestStepDrop(quest.id,1);
    return true;
}
