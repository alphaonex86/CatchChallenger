// test/testingclientlogic/main.cpp — client-side game rules shared by every
// client (client/libcatchchallenger/Api_protocol.cpp): quest start / next-step
// requirements and the beaten-bot lookup. The expected values mirror the
// server (server/base/ClientEvents/LocalClientHandlerQuest.cpp): a client that
// disagrees hides a quest the server would accept, or offers one it refuses.
// Prints "PASS <name>" / "FAIL <name> <detail>" lines for testingclientlogic.py.

#include "../testingmapmanagement/TestApiProtocol.hpp"

#include <iostream>
#include <string>

using namespace CatchChallenger;

static int failures=0;

static void check(const std::string &name,const bool got,const bool expected)
{
    if(got==expected)
        std::cout << "PASS " << name << std::endl;
    else
    {
        std::cout << "FAIL " << name << " got=" << got << " expected=" << expected << std::endl;
        failures++;
    }
}

static const uint16_t questId=1;
static const uint16_t prerequisiteId=2;
static const CATCHCHALLENGER_TYPE_ITEM itemId=10;
static const CATCHCHALLENGER_TYPE_MAPID mapId=3;
static const CATCHCHALLENGER_TYPE_BOTID botId=4;

static Quest makeQuest()
{
    Quest quest;
    quest.id=questId;
    quest.repeatable=false;
    Quest::Step step;
    step.botToTalkBotId=0;
    step.botToTalkMapId=0;
    quest.steps.push_back(step);
    return quest;
}

static Quest makeQuestWithPrerequisite(const bool inverse)
{
    Quest quest=makeQuest();
    QuestRequirements requirement;
    requirement.quest=prerequisiteId;
    requirement.inverse=inverse;
    quest.requirements.quests.push_back(requirement);
    return quest;
}

static void setQuest(TestApiProtocol &api,const uint16_t &id,const uint8_t &step,const bool &finishOneTime)
{
    PlayerQuest playerQuest;
    playerQuest.step=step;
    playerQuest.finish_one_time=finishOneTime;
    api.player_informations.quests[id]=playerQuest;
}

static void testStartRequirement()
{
    {
        TestApiProtocol api;
        check("start_no_requirement",api.haveStartQuestRequirement(makeQuest()),true);
    }
    {
        TestApiProtocol api;
        setQuest(api,questId,1,false);
        check("start_already_running",api.haveStartQuestRequirement(makeQuest()),false);
    }
    {
        TestApiProtocol api;
        setQuest(api,questId,0,true);
        check("start_done_not_repeatable",api.haveStartQuestRequirement(makeQuest()),false);
    }
    {
        TestApiProtocol api;
        setQuest(api,questId,0,true);
        Quest quest=makeQuest();
        quest.repeatable=true;
        check("start_done_repeatable",api.haveStartQuestRequirement(quest),true);
    }
    {
        TestApiProtocol api;
        check("start_prerequisite_absent",api.haveStartQuestRequirement(makeQuestWithPrerequisite(false)),false);
    }
    {
        TestApiProtocol api;
        setQuest(api,prerequisiteId,1,false);
        check("start_prerequisite_running",api.haveStartQuestRequirement(makeQuestWithPrerequisite(false)),false);
    }
    {
        TestApiProtocol api;
        setQuest(api,prerequisiteId,0,true);
        check("start_prerequisite_finished",api.haveStartQuestRequirement(makeQuestWithPrerequisite(false)),true);
    }
    {
        TestApiProtocol api;
        setQuest(api,prerequisiteId,0,true);
        check("start_inverse_prerequisite_present",api.haveStartQuestRequirement(makeQuestWithPrerequisite(true)),false);
    }
    {
        TestApiProtocol api;
        check("start_inverse_prerequisite_absent",api.haveStartQuestRequirement(makeQuestWithPrerequisite(true)),true);
    }
}

static void testBeatBot()
{
    TestApiProtocol api;
    check("beatbot_unknown_map",api.haveBeatBot(mapId,botId),false);
    api.addBeatenBotFight(mapId,botId);
    check("beatbot_beaten",api.haveBeatBot(mapId,botId),true);
    check("beatbot_other_bot",api.haveBeatBot(mapId,botId+1),false);
    check("beatbot_other_map",api.haveBeatBot(mapId+1,botId),false);
}

static void testNextStepRequirement()
{
    Quest quest=makeQuest();
    Quest::Item item;
    item.item=itemId;
    item.quantity=2;
    quest.steps[0].requirements.items.push_back(item);
    quest.steps[0].requirements.fights[mapId].insert(botId);
    {
        TestApiProtocol api;
        check("nextstep_not_started",api.haveNextStepQuestRequirements(quest),false);
    }
    {
        TestApiProtocol api;
        setQuest(api,questId,1,false);
        api.player_informations.items[itemId]=1;
        api.addBeatenBotFight(mapId,botId);
        check("nextstep_missing_item",api.haveNextStepQuestRequirements(quest),false);
    }
    {
        TestApiProtocol api;
        setQuest(api,questId,1,false);
        api.player_informations.items[itemId]=2;
        check("nextstep_bot_not_beaten",api.haveNextStepQuestRequirements(quest),false);
    }
    {
        TestApiProtocol api;
        setQuest(api,questId,1,false);
        api.player_informations.items[itemId]=2;
        api.addBeatenBotFight(mapId,botId);
        check("nextstep_all_done",api.haveNextStepQuestRequirements(quest),true);
    }
}

int main()
{
    testBeatBot();
    testNextStepRequirement();
    testStartRequirement();
    if(failures>0)
    {
        std::cerr << failures << " failure(s)" << std::endl;
        return 1;
    }
    return 0;
}
