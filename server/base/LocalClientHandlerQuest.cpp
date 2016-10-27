
#include "Client.h"
#include "../../general/base/ProtocolParsing.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/CommonDatapackServerSpec.h"
#include "../base/PreparedDBQuery.h"
#include "GlobalServerData.h"

using namespace CatchChallenger;

//quest
void Client::newQuestAction(const QuestAction &action,const uint32_t &questId)
{
    if(CommonDatapackServerSpec::commonDatapackServerSpec.quests.find(questId)==CommonDatapackServerSpec::commonDatapackServerSpec.quests.cend())
    {
        errorOutput("unknown questId: "+std::to_string(questId));
        return;
    }
    const Quest &quest=CommonDatapackServerSpec::commonDatapackServerSpec.quests.at(questId);
    switch(action)
    {
        case QuestAction_Start:
            if(!haveStartQuestRequirement(quest))
            {
                errorOutput("have not quest requirement: "+std::to_string(questId));
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
                errorOutput("have not next step quest requirement: "+std::to_string(questId));
                return;
            }
            nextStepQuest(quest);
        break;
        default:
            errorOutput("newQuestAction unknow: "+std::to_string(action));
        return;
    }
}

void Client::addQuestStepDrop(const uint32_t &questId,const uint8_t &questStep)
{
    #ifdef DEBUG_MESSAGE_CLIENT_QUESTS
    normalOutput("addQuestStepDrop for quest: "+std::to_string(questId)+", step: "+std::to_string(questStep));
    #endif
    if(CommonDatapackServerSpec::commonDatapackServerSpec.quests.find(questId)==CommonDatapackServerSpec::commonDatapackServerSpec.quests.cend())
    {
        errorOutput("Quest not found for drops");
        return;
    }
    const CatchChallenger::Quest &quest=CommonDatapackServerSpec::commonDatapackServerSpec.quests.at(questId);
    if(questStep<=0 || questStep>quest.steps.size())
    {
        errorOutput("Quest step out of range for drops");
        return;
    }
    Quest::Step stepFull=quest.steps.at(questStep-1);
    unsigned int index=0;
    unsigned int sub_index;
    while(index<stepFull.itemsMonster.size())
    {
        const Quest::ItemMonster &itemMonster=stepFull.itemsMonster.at(index);
        sub_index=0;
        const MonsterDrops &monsterDrops=questItemMonsterToMonsterDrops(itemMonster);
        while(sub_index<itemMonster.monsters.size())
        {
            questsDrop[itemMonster.monsters.at(sub_index)].push_back(monsterDrops);
            sub_index++;
        }
        index++;
    }
}

void Client::removeQuestStepDrop(const uint32_t &questId,const uint8_t &questStep)
{
    #ifdef DEBUG_MESSAGE_CLIENT_QUESTS
    normalOutput("removeQuestStepDrop for quest: "+std::to_string(questId)+", step: "+std::to_string(questStep));
    #endif
    if(CommonDatapackServerSpec::commonDatapackServerSpec.quests.find(questId)==CommonDatapackServerSpec::commonDatapackServerSpec.quests.cend())
    {
        errorOutput("Quest not found for drops");
        return;
    }
    const CatchChallenger::Quest &quest=CommonDatapackServerSpec::commonDatapackServerSpec.quests.at(questId);
    if(questStep<=0 || questStep>quest.steps.size())
    {
        errorOutput("Quest step out of range for drops");
        return;
    }
    Quest::Step stepFull=quest.steps.at(questStep-1);
    unsigned int index=0;
    unsigned int sub_index;
    while(index<stepFull.itemsMonster.size())
    {
        const Quest::ItemMonster &itemMonster=stepFull.itemsMonster.at(index);
        sub_index=0;
        const MonsterDrops &monsterDrops=questItemMonsterToMonsterDrops(itemMonster);
        while(sub_index<itemMonster.monsters.size())
        {
            vectorremoveOne(questsDrop[itemMonster.monsters.at(sub_index)],monsterDrops);
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
    normalOutput("check quest step requirement for: "+std::to_string(quest.id));
    #endif
    if(public_and_private_informations.quests.find(quest.id)==public_and_private_informations.quests.cend())
    {
        normalOutput("player quest not found: "+std::to_string(quest.id));
        return false;
    }
    const uint8_t &step=public_and_private_informations.quests.at(quest.id).step;
    if(step<=0 || step>quest.steps.size())
    {
        normalOutput("step out of range for:"+std::to_string(quest.id));
        return false;
    }
    const CatchChallenger::Quest::StepRequirements &requirements=quest.steps.at(step-1).requirements;
    unsigned int index=0;
    while(index<requirements.items.size())
    {
        const CatchChallenger::Quest::Item &item=requirements.items.at(index);
        if(objectQuantity(item.item)<item.quantity)
        {
            normalOutput("quest requirement, have not the quantity for the item: "+std::to_string(item.item));
            return false;
        }
        index++;
    }
    index=0;
    while(index<requirements.fightId.size())
    {
        const uint32_t &fightId=requirements.fightId.at(index);
        if(public_and_private_informations.bot_already_beaten==NULL)
        {
            normalOutput("quest requirement, have not beat list: "+std::to_string(fightId));
            return false;
        }
        if(public_and_private_informations.bot_already_beaten[fightId/8] & (1<<(7-fightId%8)))
        {
            normalOutput("quest requirement, have not beat the bot: "+std::to_string(fightId));
            return false;
        }
        index++;
    }
    return true;
}

bool Client::haveStartQuestRequirement(const CatchChallenger::Quest &quest)
{
    #ifdef DEBUG_MESSAGE_CLIENT_QUESTS
    normalOutput("check quest requirement for: "+std::to_string(quest.id));
    #endif
    if(public_and_private_informations.quests.find(quest.id)!=public_and_private_informations.quests.cend())
    {
        if(public_and_private_informations.quests.at(quest.id).step!=0)
        {
            normalOutput("can start the quest because is already running: "+std::to_string(quest.id));
            return false;
        }
        if(public_and_private_informations.quests.at(quest.id).finish_one_time && !quest.repeatable)
        {
            normalOutput("done one time and no repeatable: "+std::to_string(quest.id));
            return false;
        }
    }
    unsigned int index=0;
    while(index<quest.requirements.quests.size())
    {
        const uint16_t &questId=quest.requirements.quests.at(index).quest;
        if(
                (public_and_private_informations.quests.find(questId)==public_and_private_informations.quests.cend() && !quest.requirements.quests.at(index).inverse)
                ||
                (public_and_private_informations.quests.find(questId)!=public_and_private_informations.quests.cend() && quest.requirements.quests.at(index).inverse)
            )
        {
            normalOutput("have never started the quest: "+std::to_string(questId));
            return false;
        }
        if(!public_and_private_informations.quests.at(questId).finish_one_time)
        {
            normalOutput("quest never finished: "+std::to_string(questId));
            return false;
        }
        index++;
    }
    return haveReputationRequirements(quest.requirements.reputation);
}

void Client::syncDatabaseQuest()
{
    if(public_and_private_informations.quests.size()*(1+1+1)>=sizeof(ProtocolParsingBase::tempBigBufferForOutput))
    {
        uint32_t posOutput=0;
        uint8_t lastQuestId=0;
        char tempBigBufferForOutput[public_and_private_informations.quests.size()*(1+1+1)];
        auto i=public_and_private_informations.quests.begin();
        while(i!=public_and_private_informations.quests.cend())
        {
            #ifdef MAXIMIZEPERFORMANCEOVERDATABASESIZE
            //not ordened
            uint8_t type;
            if(lastQuestId<=i->first)
            {
                type=i->first-lastQuestId;
                lastQuestId=i->first;
            }
            else
            {
                type=256-lastQuestId+i->first;
                lastQuestId=i->first;
            }
            #else
            //ordened
            const uint8_t &type=i->first-lastQuestId;
            lastQuestId=i->first;
            #endif
            const PlayerQuest &quest=i->second;
            tempBigBufferForOutput[posOutput]=type;
            posOutput+=1;
            if(quest.finish_one_time)
                tempBigBufferForOutput[posOutput]=0x01;
            else
                tempBigBufferForOutput[posOutput]=0x00;
            posOutput+=1;
            tempBigBufferForOutput[posOutput]=quest.step;
            posOutput+=1;
            ++i;
        }
        GlobalServerData::serverPrivateVariables.preparedDBQueryServer.db_query_update_character_quests.asyncWrite({
                    binarytoHexa(tempBigBufferForOutput,posOutput),
                    std::to_string(character_id)
                    });
    }
    else
    {
        uint32_t posOutput=0;
        auto i=public_and_private_informations.quests.begin();
        while(i!=public_and_private_informations.quests.cend())
        {
            const uint8_t &type=i->first;
            const PlayerQuest &quest=i->second;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=type;
            posOutput+=1;
            if(quest.finish_one_time)
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x01;
            else
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x00;
            posOutput+=1;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=quest.step;
            posOutput+=1;
            ++i;
        }
        GlobalServerData::serverPrivateVariables.preparedDBQueryServer.db_query_update_character_quests.asyncWrite({
                    binarytoHexa(ProtocolParsingBase::tempBigBufferForOutput,posOutput),
                    std::to_string(character_id)
                    });
    }
}

bool Client::nextStepQuest(const Quest &quest)
{
    #ifdef DEBUG_MESSAGE_CLIENT_QUESTS
    normalOutput("drop quest step requirement for: "+std::to_string(quest.id));
    #endif
    if(public_and_private_informations.quests.find(quest.id)==public_and_private_informations.quests.cend())
    {
        normalOutput("step out of range for: "+std::to_string(quest.id));
        return false;
    }
    const uint8_t step=public_and_private_informations.quests.at(quest.id).step;
    if(step<=0 || step>quest.steps.size())
    {
        normalOutput("step out of range for: "+std::to_string(quest.id));
        return false;
    }
    const CatchChallenger::Quest::StepRequirements &requirements=quest.steps.at(step-1).requirements;
    unsigned int index=0;
    while(index<requirements.items.size())
    {
        const CatchChallenger::Quest::Item &item=requirements.items.at(index);
        removeObject(item.item,item.quantity);
        index++;
    }
    removeQuestStepDrop(quest.id,public_and_private_informations.quests.at(quest.id).step);
    public_and_private_informations.quests[quest.id].step++;
    if(public_and_private_informations.quests.at(quest.id).step>quest.steps.size())
    {
        #ifdef DEBUG_MESSAGE_CLIENT_QUESTS
        normalOutput("finish the quest: "+std::to_string(quest.id));
        #endif
        public_and_private_informations.quests[quest.id].step=0;
        public_and_private_informations.quests[quest.id].finish_one_time=true;
        index=0;
        while(index<quest.rewards.items.size())
        {
            addObjectAndSend(quest.rewards.items.at(index).item,quest.rewards.items.at(index).quantity);
            index++;
        }
        index=0;
        while(index<quest.rewards.reputation.size())
        {
            appendReputationPoint(quest.rewards.reputation.at(index).reputationId,quest.rewards.reputation.at(index).point);
            index++;
        }
        index=0;
        while(index<quest.rewards.allow.size())
        {
            appendAllow(quest.rewards.allow.at(index));
            index++;
        }
        syncDatabaseQuest();
    }
    else
    {
        #ifdef DEBUG_MESSAGE_CLIENT_QUESTS
        normalOutput("next step in the quest: "+std::to_string(quest.id));
        #endif
        addQuestStepDrop(quest.id,public_and_private_informations.quests.at(quest.id).step);
        syncDatabaseQuest();
    }
    return true;
}

bool Client::startQuest(const Quest &quest)
{
    if(public_and_private_informations.quests.find(quest.id)==public_and_private_informations.quests.cend())
    {
        public_and_private_informations.quests[quest.id].step=1;
        public_and_private_informations.quests[quest.id].finish_one_time=false;
    }
    else
        public_and_private_informations.quests[quest.id].step=1;
    addQuestStepDrop(quest.id,1);
    syncDatabaseQuest();
    return true;
}
