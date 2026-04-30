#include "ActionsAction.h"
#include "../../client/libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../general/base/CommonDatapackServerSpec.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include "../../general/base/FacilityLib.hpp"

bool ActionsAction::nextStepQuest(CatchChallenger::Api_protocol_Qt *api,const CatchChallenger::Quest &quest)
{
    CatchChallenger::Player_private_and_public_informations &player=api->get_player_informations();
    std::map<CATCHCHALLENGER_TYPE_QUEST, CatchChallenger::PlayerQuest> &quests=player.quests;
    #ifdef DEBUG_CLIENT_QUEST
    qDebug() << "drop quest step requirement for: " << quest.id;
    #endif
    if(quests.find(quest.id)==quests.cend())
    {
        qDebug() << "step out of range for: " << quest.id;
        return false;
    }
    uint8_t step=quests.at(quest.id).step;
    if(step<=0 || step>quest.steps.size())
    {
        qDebug() << "step out of range for: " << quest.id;
        return false;
    }
    const CatchChallenger::Quest::StepRequirements &requirements=quest.steps.at(step-1).requirements;
    unsigned int index=0;
    while(index<requirements.items.size())
    {
        const CatchChallenger::Quest::Item &item=requirements.items.at(index);
        std::unordered_map<uint16_t,uint32_t> items;
        items[item.item]=item.quantity;
        remove_to_inventory(api,items);
        index++;
    }
    quests[quest.id].step++;
    if(quests.at(quest.id).step>quest.steps.size())
    {
        #ifdef DEBUG_CLIENT_QUEST
        qDebug() << "finish the quest: " << quest.id;
        #endif
        quests[quest.id].step=0;
        quests[quest.id].finish_one_time=true;
        index=0;
        while(index<quest.rewards.reputation.size())
        {
            appendReputationPoint(api,
                            CatchChallenger::CommonDatapack::commonDatapack.get_reputation().at(quest.rewards.reputation.at(index).reputationId).name,
                        quest.rewards.reputation.at(index).point
                        );
            index++;
        }
        //show_reputation();
        if(quest.rewards.allow_create_clan)
        {
            //player can now create clan
        }
    }
    return true;
}

//reputation
void ActionsAction::appendReputationPoint(CatchChallenger::Api_protocol_Qt *api,const std::string &type,const int32_t &point)
{
    if(point==0)
        return;
    if(!QtDatapackClientLoader::datapackLoader->has_reputationNameToId(type))
    {
        //emit error(QStringLiteral("Unknow reputation: %1").arg(type));
        abort();
        return;
    }
    const uint8_t reputatioId=QtDatapackClientLoader::datapackLoader->get_reputationNameToId(type);
    CatchChallenger::PlayerReputation playerReputation;
    if(api->player_informations.reputation.find(reputatioId)!=api->player_informations.reputation.cend())
        playerReputation=api->player_informations.reputation.at(reputatioId);
    else
    {
        playerReputation.point=0;
        playerReputation.level=0;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_REPUTATION
    emit message(QStringLiteral("Reputation %1 at level: %2 with point: %3").arg(type).arg(playerReputation.level).arg(playerReputation.point));
    #endif
    CatchChallenger::PlayerReputation oldPlayerReputation=playerReputation;
    int32_t old_level=playerReputation.level;
    CatchChallenger::FacilityLib::appendReputationPoint(&playerReputation,point,CatchChallenger::CommonDatapack::commonDatapack.get_reputation().at(reputatioId));
    if(oldPlayerReputation.level==playerReputation.level && oldPlayerReputation.point==playerReputation.point)
        return;
    if(api->player_informations.reputation.find(reputatioId)!=api->player_informations.reputation.cend())
        api->player_informations.reputation[reputatioId]=playerReputation;
    else
        api->player_informations.reputation[reputatioId]=playerReputation;
    const std::string &reputationCodeName=CatchChallenger::CommonDatapack::commonDatapack.get_reputation().at(reputatioId).name;
    if(old_level<playerReputation.level)
    {
        if(QtDatapackClientLoader::datapackLoader->has_reputationExtra(reputationCodeName))
            showTip(tr("You have better reputation into %1")
                    .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_reputationExtra(reputationCodeName).name)));
        else
            showTip(tr("You have better reputation into %1")
                    .arg(QStringLiteral("???")));
    }
    else if(old_level>playerReputation.level)
    {
        if(QtDatapackClientLoader::datapackLoader->has_reputationExtra(reputationCodeName))
            showTip(tr("You have worse reputation into %1")
                    .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_reputationExtra(reputationCodeName).name)));
        else
            showTip(tr("You have worse reputation into %1")
                    .arg(QStringLiteral("???")));
    }
    #ifdef DEBUG_MESSAGE_CLIENT_REPUTATION
    emit message(QStringLiteral("New reputation %1 at level: %2 with point: %3").arg(type).arg(playerReputation.level).arg(playerReputation.point));
    #endif
}

bool ActionsAction::botHaveQuest(const CatchChallenger::Api_protocol_Qt *api,const uint16_t &botId)
{
    const CatchChallenger::Player_private_and_public_informations &player=api->get_player_informations_ro();
    const std::map<CATCHCHALLENGER_TYPE_QUEST, CatchChallenger::PlayerQuest> &quests=player.quests;
    #ifdef DEBUG_CLIENT_QUEST
    qDebug() << "check bot quest for: " << botId;
    #endif
    //do the not started quest here, collect quests for this botId across all maps
    std::vector<CATCHCHALLENGER_TYPE_QUEST> botQuests;
    for(CATCHCHALLENGER_TYPE_MAPID mapId=0;mapId<(CATCHCHALLENGER_TYPE_MAPID)QtDatapackClientLoader::datapackLoader->get_maps().size();mapId++) {
        if(!QtDatapackClientLoader::datapackLoader->has_botToQuestStart(mapId))
            continue;
        if(!QtDatapackClientLoader::datapackLoader->has_botToQuestStartForBot(mapId,static_cast<CATCHCHALLENGER_TYPE_BOTID>(botId)))
            continue;
        const std::vector<CATCHCHALLENGER_TYPE_QUEST> &mapBotQuests=QtDatapackClientLoader::datapackLoader->get_botToQuestStartForBot(mapId,static_cast<CATCHCHALLENGER_TYPE_BOTID>(botId));
        for(const CATCHCHALLENGER_TYPE_QUEST &q : mapBotQuests)
            botQuests.push_back(q);
    }
    unsigned int index=0;
    while(index<botQuests.size())
    {
        const uint32_t &questId=botQuests.at(index);
        #ifdef CATCHCHALLENGER_HARDENED
        if(questId!=botQuests.at(index))
            qDebug() << "cast error for questId at ActionsAction::getQuestList()";
        #endif
        if(!CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.has_quest(questId))
        {
            index++;
            continue;
        }
        const CatchChallenger::Quest &currentQuest=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_quest(questId);
        if(quests.find(botQuests.at(index))==quests.cend())
        {
            //quest not started
            if(haveStartQuestRequirement(api,currentQuest))
                return true;
            else
                {}//have not the requirement
        }
        else
        {
            if(!CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.has_quest(botQuests.at(index)))
                qDebug() << "internal bug: have quest registred, but no quest found with this id";
            else
            {
                if(quests.at(botQuests.at(index)).step==0)
                {
                    if(currentQuest.repeatable)
                    {
                        if(quests.at(botQuests.at(index)).finish_one_time)
                        {
                            //quest already done but repeatable
                            if(haveStartQuestRequirement(api,currentQuest))
                                return true;
                            else
                                {}//have not the requirement
                        }
                        else
                            {}//bug: can't be !finish_one_time && currentQuest.steps==0
                    }
                    else
                        {}//quest already done
                }
                else
                {
                    const CatchChallenger::Quest::Step &step=currentQuest.steps.at(quests.at(questId).step-1);
                    if(step.botToTalkBotId==static_cast<CATCHCHALLENGER_TYPE_BOTID>(botId))
                        return true;//in progress
                    else
                        {}//Need got to another bot to progress, this it's just the starting bot
                }
            }
        }
        index++;
    }
    //do the started quest here
    for(const std::pair<const CATCHCHALLENGER_TYPE_QUEST, CatchChallenger::PlayerQuest> &n : quests) {
        if(!vectorcontainsAtLeastOne(botQuests,n.first) && n.second.step>0)
        {
            if(!CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.has_quest(n.first))
                continue;
            const CatchChallenger::Quest &currentQuest=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_quest(n.first);
            const CatchChallenger::Quest::Step &step=currentQuest.steps.at(n.second.step-1);
            if(step.botToTalkBotId==static_cast<CATCHCHALLENGER_TYPE_BOTID>(botId))
                return true;//in progress, but not the starting bot
            else
                {}//it's another bot
        }
    }
    return false;
}

bool ActionsAction::tryValidateQuestStep(CatchChallenger::Api_protocol_Qt *api, const uint16_t &questId, const uint16_t &botId, bool silent)
{
    CatchChallenger::Player_private_and_public_informations &player=api->get_player_informations();
    const std::map<CATCHCHALLENGER_TYPE_QUEST, CatchChallenger::PlayerQuest> &quests=player.quests;
    if(!CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.has_quest(questId))
    {
        if(!silent)
            showTip(tr("Quest not found"));
        return
                false;
    }
    const CatchChallenger::Quest &quest=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_quest(questId);

    if(quests.find(questId)==quests.cend())
    {
        //start for the first time the quest
        if(quest.steps.at(0).botToTalkBotId==static_cast<CATCHCHALLENGER_TYPE_BOTID>(botId)
                && haveStartQuestRequirement(api,quest))
        {
            api->startQuest(questId);
            startQuest(api,quest);
            //updateDisplayedQuests();
            return true;
        }
        else
        {
            if(!silent)
                showTip(tr("You don't have the requirement to start this quest"));
            return false;
        }
    }
    else if(quests.at(questId).step==0)
    {
        //start again the quest if can be repeated
        if(quest.repeatable &&
                quest.steps.at(0).botToTalkBotId==static_cast<CATCHCHALLENGER_TYPE_BOTID>(botId)
                && haveStartQuestRequirement(api,quest))
        {
            api->startQuest(questId);
            startQuest(api,quest);
            //updateDisplayedQuests();
            return true;
        }
        else
        {
            if(!silent)
                showTip(tr("You don't have the requirement to start this quest"));
            return false;
        }
    }
    if(!haveNextStepQuestRequirements(api,quest))
    {
        if(!silent)
            showTip(tr("You don't have the requirement to continue this quest"));
        return false;
    }
    if(quests.at(questId).step>=(quest.steps.size()))
    {
        if(!silent)
        {
            std::string questName="???";
            if(QtDatapackClientLoader::datapackLoader->has_questExtra(questId))
                questName=QtDatapackClientLoader::datapackLoader->get_questExtra(questId).name;
            showTip(tr("You have finish the quest <b>%1</b>").arg(QString::fromStdString(questName)));
        }
        api->finishQuest(questId);
        nextStepQuest(api,quest);
        //updateDisplayedQuests();
        return true;
    }
    if(quest.steps.at(quests.at(questId).step).botToTalkBotId==static_cast<CATCHCHALLENGER_TYPE_BOTID>(botId))
    {
        if(!silent)
            showTip(tr("You need talk to another bot"));
        return false;
    }
    api->nextQuestStep(questId);
    nextStepQuest(api,quest);
    //updateDisplayedQuests();
    return true;
}

bool ActionsAction::startQuest(CatchChallenger::Api_protocol_Qt *api,const CatchChallenger::Quest &quest)
{
    (void)api;
    (void)quest;
    return true;
}

bool ActionsAction::haveNextStepQuestRequirements(const CatchChallenger::Api_protocol_Qt *api,const CatchChallenger::Quest &quest)
{
    const CatchChallenger::Player_private_and_public_informations &player=api->get_player_informations_ro();
    const std::map<CATCHCHALLENGER_TYPE_QUEST, CatchChallenger::PlayerQuest> &quests=player.quests;
    #ifdef DEBUG_CLIENT_QUEST
    qDebug() << QStringLiteral("haveNextStepQuestRequirements for quest: %1").arg(questId);
    #endif
    if(quests.find(quest.id)==quests.cend())
    {
        qDebug() << "step out of range for: " << quest.id;
        return false;
    }
    uint8_t step=quests.at(quest.id).step;
    if(step<=0 || step>quest.steps.size())
    {
        qDebug() << "step out of range for: " << quest.id;
        return false;
    }
    #ifdef DEBUG_CLIENT_QUEST
    qDebug() << QStringLiteral("haveNextStepQuestRequirements for quest: %1, step: %2").arg(questId).arg(step);
    #endif
    const CatchChallenger::Quest::StepRequirements &requirements=quest.steps.at(step-1).requirements;
    unsigned int index=0;
    while(index<requirements.items.size())
    {
        const CatchChallenger::Quest::Item &item=requirements.items.at(index);
        if(itemQuantity(api,item.item)<item.quantity)
        {
            #ifdef DEBUG_CLIENT_QUEST
            qDebug() << "quest requirement, have not the quantity for the item: " << item.item;
            #endif
            return false;
        }
        index++;
    }
    for(const std::pair<const uint16_t, catchchallenger_datapack_set<uint8_t>> &fightEntry : requirements.fights)
    {
        for(const uint8_t &fightBotId : fightEntry.second)
        {
            if(!haveBeatBot(api,fightBotId))
            {
                #ifdef DEBUG_CLIENT_QUEST
                qDebug() << "quest requirement, have not beat the bot: " << fightBotId;
                #endif
                return false;
            }
        }
    }
    return true;
}

bool ActionsAction::haveStartQuestRequirement(const CatchChallenger::Api_protocol_Qt *api,const CatchChallenger::Quest &quest)
{
    const CatchChallenger::Player_private_and_public_informations &informations=api->get_player_informations_ro();
    const std::map<CATCHCHALLENGER_TYPE_QUEST, CatchChallenger::PlayerQuest> &quests=informations.quests;
    #ifdef DEBUG_CLIENT_QUEST
    qDebug() << "check quest requirement for: " << quest.id;
    #endif
    if(quests.find(quest.id)!=quests.cend())
    {
        const CatchChallenger::PlayerQuest &playerquest=quests.at(quest.id);
        if(playerquest.step!=0)
        {
            #ifdef DEBUG_CLIENT_QUEST
            qDebug() << "can start the quest because is already running: " << quest.id;
            #endif
            return false;
        }
        if(playerquest.finish_one_time && !quest.repeatable)
        {
            #ifdef DEBUG_CLIENT_QUEST
            qDebug() << "done one time and no repeatable: " << quest.id;
            #endif
            return false;
        }
    }
    unsigned int index=0;
    while(index<quest.requirements.quests.size())
    {
        const uint16_t &questId=quest.requirements.quests.at(index).quest;
        if(
                (quests.find(questId)==quests.cend() && !quest.requirements.quests.at(index).inverse)
                ||
                (quests.find(questId)!=quests.cend() && quest.requirements.quests.at(index).inverse)
        )
        {
            #ifdef DEBUG_CLIENT_QUEST
            qDebug() << "quest requirement not met for: " << questId;
            #endif
            return false;
        }
        index++;
    }
    index=0;
    while(index<quest.requirements.reputation.size())
    {
        const CatchChallenger::ReputationRequirements &repReq=quest.requirements.reputation.at(index);
        if(informations.reputation.find(repReq.reputationId)==informations.reputation.cend())
        {
            if(repReq.level>0)
            {
                #ifdef DEBUG_CLIENT_QUEST
                qDebug() << "quest requirement, reputation not found: " << repReq.reputationId;
                #endif
                return false;
            }
        }
        else
        {
            const CatchChallenger::PlayerReputation &playerRep=informations.reputation.at(repReq.reputationId);
            if(repReq.positif)
            {
                if(playerRep.level<(int32_t)repReq.level)
                {
                    #ifdef DEBUG_CLIENT_QUEST
                    qDebug() << "quest requirement, reputation too low: " << repReq.reputationId;
                    #endif
                    return false;
                }
            }
            else
            {
                if(playerRep.level>(-(int32_t)repReq.level))
                {
                    #ifdef DEBUG_CLIENT_QUEST
                    qDebug() << "quest requirement, reputation too high: " << repReq.reputationId;
                    #endif
                    return false;
                }
            }
        }
        index++;
    }
    return true;
}

std::vector<std::pair<uint16_t,std::string> > ActionsAction::getQuestList(CatchChallenger::Api_protocol_Qt *api, const uint16_t &botId)
{
    const CatchChallenger::Player_private_and_public_informations &player=api->get_player_informations_ro();
    const std::map<CATCHCHALLENGER_TYPE_QUEST, CatchChallenger::PlayerQuest> &quests=player.quests;
    std::vector<std::pair<uint16_t,std::string> > entryList;
    std::pair<uint16_t,std::string> oneEntry;
    //do the not started quest here, collect quests for this botId across all maps
    std::vector<CATCHCHALLENGER_TYPE_QUEST> botQuests;
    const std::vector<std::string> &maps=QtDatapackClientLoader::datapackLoader->get_maps();
    for(CATCHCHALLENGER_TYPE_MAPID mapId=0;mapId<static_cast<CATCHCHALLENGER_TYPE_MAPID>(maps.size());mapId++) {
        if(!QtDatapackClientLoader::datapackLoader->has_botToQuestStartForBot(mapId,static_cast<CATCHCHALLENGER_TYPE_BOTID>(botId)))
            continue;
        const std::vector<CATCHCHALLENGER_TYPE_QUEST> &mapBotQuests=QtDatapackClientLoader::datapackLoader->get_botToQuestStartForBot(mapId,static_cast<CATCHCHALLENGER_TYPE_BOTID>(botId));
        for(const CATCHCHALLENGER_TYPE_QUEST &q : mapBotQuests)
            botQuests.push_back(q);
    }
    unsigned int index=0;
    while(index<botQuests.size())
    {
        const uint16_t &questId=botQuests.at(index);
        #ifdef CATCHCHALLENGER_HARDENED
        if(questId!=botQuests.at(index))
            qDebug() << "cast error for questId at ActionsAction::getQuestList()";
        #endif
        if(!CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.has_quest(questId))
        {
            index++;
            continue;
        }
        const CatchChallenger::Quest &currentQuest=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_quest(questId);
        if(quests.find(botQuests.at(index))==quests.cend())
        {
            //quest not started
            if(haveStartQuestRequirement(api,currentQuest))
            {
                oneEntry.first=questId;
                if(QtDatapackClientLoader::datapackLoader->has_questExtra(questId))
                    oneEntry.second=QtDatapackClientLoader::datapackLoader->get_questExtra(questId).name;
                else
                {
                    qDebug() << "internal bug: quest extra not found";
                    oneEntry.second="???";
                }
                entryList.push_back(oneEntry);
            }
            else
                {}//have not the requirement
        }
        else
        {
            if(!CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.has_quest(botQuests.at(index)))
                qDebug() << "internal bug: have quest registred, but no quest found with this id";
            else
            {
                if(quests.at(botQuests.at(index)).step==0)
                {
                    if(currentQuest.repeatable)
                    {
                        if(quests.at(botQuests.at(index)).finish_one_time)
                        {
                            //quest already done but repeatable
                            if(haveStartQuestRequirement(api,currentQuest))
                            {
                                oneEntry.first=questId;
                                if(QtDatapackClientLoader::datapackLoader->has_questExtra(questId))
                                    oneEntry.second=QtDatapackClientLoader::datapackLoader->get_questExtra(questId).name;
                                else
                                {
                                    qDebug() << "internal bug: quest extra not found";
                                    oneEntry.second="???";
                                }
                                entryList.push_back(oneEntry);
                            }
                            else
                                {}//have not the requirement
                        }
                        else
                            {}//bug: can't be !finish_one_time && currentQuest.steps==0
                    }
                    else
                        {}//quest already done
                }
                else
                {
                    const CatchChallenger::Quest::Step &step=currentQuest.steps.at(quests.at(questId).step-1);
                    if(step.botToTalkBotId==static_cast<CATCHCHALLENGER_TYPE_BOTID>(botId))
                    {
                        oneEntry.first=questId;
                        if(QtDatapackClientLoader::datapackLoader->has_questExtra(questId))
                            oneEntry.second=tr("%1 (in progress)").arg(QString::fromStdString(
                                                                           QtDatapackClientLoader::datapackLoader->get_questExtra(questId).name)).toStdString();
                        else
                        {
                            qDebug() << "internal bug: quest extra not found";
                            oneEntry.second=tr("??? (in progress)").toStdString();
                        }
                        entryList.push_back(oneEntry);
                    }
                    else
                        {}//Need got to another bot to progress, this it's just the starting bot
                }
            }
        }
        index++;
    }
    //do the started quest here
    for(const std::pair<const CATCHCHALLENGER_TYPE_QUEST, CatchChallenger::PlayerQuest> &n : quests) {
        if(!vectorcontainsAtLeastOne(botQuests,n.first) && n.second.step>0)
        {
            if(!CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.has_quest(n.first))
                continue;
            const CatchChallenger::Quest &currentQuest=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_quest(n.first);
            const CatchChallenger::Quest::Step &step=currentQuest.steps.at(n.second.step-1);
            if(step.botToTalkBotId==static_cast<CATCHCHALLENGER_TYPE_BOTID>(botId))
            {
                //in progress, but not the starting bot
                oneEntry.first=n.first;
                if(QtDatapackClientLoader::datapackLoader->has_questExtra(n.first))
                    oneEntry.second=tr("%1 (in progress)")
                            .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_questExtra(n.first).name))
                            .toStdString();
                else
                    oneEntry.second=tr("??? (in progress)").toStdString();
                entryList.push_back(oneEntry);
            }
            else
                {}//it's another bot
        }
    }
    return entryList;
}
