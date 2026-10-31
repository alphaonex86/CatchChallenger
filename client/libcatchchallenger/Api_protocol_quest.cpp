#include "Api_protocol.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include "../../general/base/CommonDatapackServerSpec.hpp"
#include "../../general/base/FacilityLib.hpp"

#include <iostream>

#if ! defined (CATCHCHALLENGER_ONLYMAPRENDER)
using namespace CatchChallenger;

static bool isStepBot(const Quest::Step &step,const CATCHCHALLENGER_TYPE_MAPID &mapId,const CATCHCHALLENGER_TYPE_BOTID &botId)
{
    return step.botToTalkMapId==mapId && step.botToTalkBotId==botId;
}

bool Api_protocol::appendReputationPoint(const uint8_t &reputationId,const int32_t &point,ReputationLevelChange &change)
{
    if(point==0)
        return false;
    const std::vector<Reputation> &reputations=CommonDatapack::commonDatapack.get_reputation();
    if(reputationId>=reputations.size())
    {
        std::cerr << "Api_protocol::appendReputationPoint(): unknown reputation " << std::to_string(reputationId) << std::endl;
        return false;
    }
    if(player_informations.reputation.find(reputationId)==player_informations.reputation.cend())
    {
        PlayerReputation newReputation;
        newReputation.level=0;
        newReputation.point=0;
        player_informations.reputation[reputationId]=newReputation;
    }
    PlayerReputation &playerReputation=player_informations.reputation[reputationId];
    const int8_t oldLevel=playerReputation.level;
    FacilityLib::appendReputationPoint(&playerReputation,point,reputations.at(reputationId));
    if(playerReputation.level==oldLevel)
        return false;
    change.reputationId=reputationId;
    change.better=playerReputation.level>oldLevel;
    return true;
}

std::vector<Api_protocol::ReputationLevelChange> Api_protocol::appendReputationRewards(const std::vector<ReputationRewards> &reputationList)
{
    std::vector<ReputationLevelChange> changes;
    unsigned int index=0;
    while(index<reputationList.size())
    {
        ReputationLevelChange change;
        if(appendReputationPoint(reputationList.at(index).reputationId,reputationList.at(index).point,change))
            changes.push_back(change);
        index++;
    }
    return changes;
}

Api_protocol::QuestForBot Api_protocol::questForBot(const Quest &quest,const CATCHCHALLENGER_TYPE_MAPID &mapId,const CATCHCHALLENGER_TYPE_BOTID &botId) const
{
    if(quest.steps.empty())
        return QuestForBot_None;
    const std::map<CATCHCHALLENGER_TYPE_QUEST,PlayerQuest>::const_iterator playerQuest=player_informations.quests.find(quest.id);
    if(playerQuest==player_informations.quests.cend() || playerQuest->second.step==0)
    {
        if(isStepBot(quest.steps.front(),mapId,botId) && haveStartQuestRequirement(quest))
            return QuestForBot_CanStart;
        return QuestForBot_None;
    }
    const uint8_t step=playerQuest->second.step;
    if(step<=quest.steps.size() && isStepBot(quest.steps.at(step-1),mapId,botId))
        return QuestForBot_InProgress;
    return QuestForBot_None;
}

std::vector<std::pair<CATCHCHALLENGER_TYPE_QUEST,bool> > Api_protocol::questsForBot(const CATCHCHALLENGER_TYPE_MAPID &mapId,const CATCHCHALLENGER_TYPE_BOTID &botId) const
{
    std::vector<std::pair<CATCHCHALLENGER_TYPE_QUEST,bool> > quests;
    const CommonDatapackServerSpec &datapack=CommonDatapackServerSpec::commonDatapackServerSpec;
    unsigned int questId=0;
    while(questId<=255)
    {
        if(datapack.has_quest(static_cast<CATCHCHALLENGER_TYPE_QUEST>(questId)))
        {
            const QuestForBot questState=questForBot(datapack.get_quest(static_cast<CATCHCHALLENGER_TYPE_QUEST>(questId)),mapId,botId);
            if(questState!=QuestForBot_None)
                quests.push_back(std::pair<CATCHCHALLENGER_TYPE_QUEST,bool>(static_cast<CATCHCHALLENGER_TYPE_QUEST>(questId),questState==QuestForBot_InProgress));
        }
        questId++;
    }
    return quests;
}

bool Api_protocol::botHaveQuest(const CATCHCHALLENGER_TYPE_MAPID &mapId,const CATCHCHALLENGER_TYPE_BOTID &botId) const
{
    return !questsForBot(mapId,botId).empty();
}

Api_protocol::QuestStepResult Api_protocol::validateQuestStep(const CATCHCHALLENGER_TYPE_QUEST &questId,const CATCHCHALLENGER_TYPE_MAPID &mapId,
                                                              const CATCHCHALLENGER_TYPE_BOTID &botId,std::vector<ReputationLevelChange> &reputationChanges)
{
    reputationChanges.clear();
    if(!CommonDatapackServerSpec::commonDatapackServerSpec.has_quest(questId))
        return QuestStep_UnknownQuest;
    return validateQuestStep(CommonDatapackServerSpec::commonDatapackServerSpec.get_quest(questId),mapId,botId,reputationChanges);
}

Api_protocol::QuestStepResult Api_protocol::validateQuestStep(const Quest &quest,const CATCHCHALLENGER_TYPE_MAPID &mapId,
                                                              const CATCHCHALLENGER_TYPE_BOTID &botId,std::vector<ReputationLevelChange> &reputationChanges)
{
    reputationChanges.clear();
    const CATCHCHALLENGER_TYPE_QUEST questId=static_cast<CATCHCHALLENGER_TYPE_QUEST>(quest.id);
    const std::map<CATCHCHALLENGER_TYPE_QUEST,PlayerQuest>::const_iterator playerQuest=player_informations.quests.find(questId);
    if(playerQuest==player_informations.quests.cend() || playerQuest->second.step==0)
    {
        if(questForBot(quest,mapId,botId)!=QuestForBot_CanStart)
            return QuestStep_CantStart;
        startQuest(questId);
        if(playerQuest==player_informations.quests.cend())
            player_informations.quests[questId].finish_one_time=false;
        player_informations.quests[questId].step=1;
        return QuestStep_Started;
    }
    const uint8_t step=playerQuest->second.step;
    if(step>quest.steps.size())
    {
        std::cerr << "Api_protocol::validateQuestStep(): step " << std::to_string(step) << " out of range for quest " << std::to_string(questId) << std::endl;
        return QuestStep_CantContinue;
    }
    if(!isStepBot(quest.steps.at(step-1),mapId,botId))
        return QuestStep_OtherBot;
    if(!haveNextStepQuestRequirements(quest))
        return QuestStep_CantContinue;
    //the server takes the step items without telling the client
    const std::vector<Quest::Item> &items=quest.steps.at(step-1).requirements.items;
    if(!items.empty())
    {
        std::unordered_map<CATCHCHALLENGER_TYPE_ITEM,uint32_t> itemsToRemove;
        unsigned int index=0;
        while(index<items.size())
        {
            itemsToRemove[items.at(index).item]+=items.at(index).quantity;
            index++;
        }
        remove_to_inventory(itemsToRemove);
    }
    if(step<quest.steps.size())
    {
        nextQuestStep(questId);
        player_informations.quests[questId].step++;
        return QuestStep_NextStep;
    }
    //the reward items come from the server
    finishQuest(questId);
    player_informations.quests[questId].step=0;
    player_informations.quests[questId].finish_one_time=true;
    reputationChanges=appendReputationRewards(quest.rewards.reputation);
    if(quest.rewards.allow_create_clan)
        player_informations.allow_create_clan=true;
    return QuestStep_Finished;
}
#endif
