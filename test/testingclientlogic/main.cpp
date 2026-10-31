// test/testingclientlogic/main.cpp — client-side game rules shared by every
// client (client/libcatchchallenger/Api_protocol.cpp): quest start / next-step
// requirements and the beaten-bot lookup; then the client logic shared in
// client/libqtcatchchallenger/ (chat text, chat input guard, command line,
// language pick) and the ambiance file pick of DatapackClientLoader. The expected values mirror the
// server (server/base/ClientEvents/LocalClientHandlerQuest.cpp): a client that
// disagrees hides a quest the server would accept, or offers one it refuses.
// Prints "PASS <name>" / "FAIL <name> <detail>" lines for testingclientlogic.py.

#include "../testingmapmanagement/TestApiProtocol.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include "../../client/libcatchchallenger/DatapackClientLoader.hpp"
#include "../../client/libcatchchallenger/SlowServerDetector.hpp"
#include "../../client/libqtcatchchallenger/ChatInputGuard.hpp"
#include "../../client/libqtcatchchallenger/ChatParsing.hpp"
#include "../../client/libqtcatchchallenger/CliClientOptions.hpp"
#include "../../client/libqtcatchchallenger/Language.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <iostream>
#include <string>
#include <vector>

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

static void checkText(const std::string &name,const std::string &got,const std::string &expected)
{
    if(got==expected)
        std::cout << "PASS " << name << std::endl;
    else
    {
        std::cout << "FAIL " << name << " got=\"" << got << "\" expected=\"" << expected << "\"" << std::endl;
        failures++;
    }
}

static bool contains(const std::string &text,const std::string &part)
{
    return text.find(part)!=std::string::npos;
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

static void testChatParsing()
{
    checkText("chat_entities",ChatParsing::toHtmlEntities("<a href=\"x\">&'"),"&lt;a href=&quot;x&quot;&gt;&amp;&#039;");
    check("chat_smilies",contains(ChatParsing::toSmilies("hi :)"),"face-smile.png"),true);
    const std::string line=ChatParsing::new_chat_message("bob",Player_type_normal,Chat_type_all,"<b>x</b>");
    check("chat_player_text_escaped",contains(line,"bob: &lt;b&gt;x&lt;/b&gt;") && !contains(line,"<b>"),true);
    check("chat_pseudo_link",contains(ChatParsing::new_chat_message("bob",Player_type_normal,Chat_type_all,"x",true),"<a href=\"bob\""),true);
    check("chat_system_not_dimmed",contains(ChatParsing::new_chat_message("",Player_type_normal,Chat_type_system,"x"),"color:#707070"),false);
    check("chat_system_dimmed",contains(ChatParsing::new_chat_message("",Player_type_normal,Chat_type_system,"x",false,true),"color:#707070"),true);

    const std::string questText="a<span class=\"quest\">1</span>b<span class=\"quest\">2</span>c";
    checkText("bot_quest_spans_removed",ChatParsing::botTextToDisplay(questText,false),"abc");
    checkText("bot_quest_spans_kept",ChatParsing::botTextToDisplay(questText,true),questText);
    const std::string linkText=ChatParsing::botTextToDisplay("<a href=\"http://x\">site</a>",false);
    check("bot_link_marked",contains(linkText,"style=\"color:#BB9900;\" href=\"http://x\"") && contains(linkText,"site <img src=\":/CC/images/link.png\" alt=\"\" /></a>"),true);
}

static void checkGuard(const std::string &name,ChatInputGuard &guard,const QString &input,
                       const ChatInputGuard::Verdict expected,const std::string &expectedRefusal)
{
    QString text=input;
    const ChatInputGuard::Verdict verdict=guard.check(text);
    if(verdict!=expected)
    {
        std::cout << "FAIL " << name << " verdict=" << (int)verdict << " expected=" << (int)expected << std::endl;
        failures++;
    }
    else if(verdict==ChatInputGuard::Verdict_Refused)
        checkText(name,guard.refusal(),expectedRefusal);
    else
        std::cout << "PASS " << name << std::endl;
}

static void testChatInputGuard()
{
    ChatInputGuard guard;
    checkGuard("guard_empty",guard,"\n\t\r",ChatInputGuard::Verdict_Nothing,"");
    checkGuard("guard_spaces",guard,"   ",ChatInputGuard::Verdict_Refused,"Space text not allowed");
    checkGuard("guard_too_long",guard,QString(257,'a'),ChatInputGuard::Verdict_Refused,"Message too long");
    QString text="hi\n";
    check("guard_strips_newline",guard.check(text)==ChatInputGuard::Verdict_Send && text=="hi",true);
    checkGuard("guard_repeat",guard,"hi",ChatInputGuard::Verdict_Refused,"Send message like as previous");
    checkGuard("guard_command_repeat_1",guard,"/who",ChatInputGuard::Verdict_Send,"");
    checkGuard("guard_command_repeat_2",guard,"/who",ChatInputGuard::Verdict_Send,"");

    guard.reset();
    checkGuard("guard_flood_1",guard,"a",ChatInputGuard::Verdict_Send,"");
    checkGuard("guard_flood_2",guard,"b",ChatInputGuard::Verdict_Send,"");
    checkGuard("guard_flood_3",guard,"c",ChatInputGuard::Verdict_Send,"");
    checkGuard("guard_flood_stop",guard,"d",ChatInputGuard::Verdict_Refused,"Stop flood");
    guard.decayFlood();
    checkGuard("guard_flood_decayed",guard,"e",ChatInputGuard::Verdict_Send,"");

    guard.reset();
    checkGuard("guard_reset_forgets_last",guard,"e",ChatInputGuard::Verdict_Send,"");
}

//argv as the OS gives it: argc strings then a null pointer
static bool parseArguments(const std::vector<std::string> &arguments,std::vector<std::string> &leftForQt)
{
    std::vector<std::vector<char> > storage;
    unsigned int index=0;
    while(index<arguments.size())
    {
        storage.push_back(std::vector<char>(arguments.at(index).cbegin(),arguments.at(index).cend()));
        storage.back().push_back('\0');
        index++;
    }
    std::vector<char *> argv;
    index=0;
    while(index<storage.size())
    {
        argv.push_back(storage.at(index).data());
        index++;
    }
    argv.push_back(nullptr);
    int argc=static_cast<int>(storage.size());
    const bool returnValue=CliClientOptions::parse(argc,argv.data());
    leftForQt.clear();
    int argvIndex=0;
    while(argvIndex<argc)
    {
        leftForQt.push_back(argv.at(argvIndex));
        argvIndex++;
    }
    if(argv.at(argc)!=nullptr)
    {
        std::cout << "FAIL cli_argv_null_terminated" << std::endl;
        failures++;
    }
    return returnValue;
}

static void testCliClientOptions()
{
    std::vector<std::string> left;
    check("cli_parse_ok",parseArguments({"prog","--server","S","-platform","--character=C","offscreen"},left),true);
    check("cli_value_both_forms",CliClientOptions::serverName=="S" && CliClientOptions::characterName=="C",true);
    check("cli_unknown_left_for_qt",left==std::vector<std::string>({"prog","-platform","offscreen"}),true);

    parseArguments({"prog","--host","h","--port=4000"},left);
    check("cli_tcp",CliClientOptions::host=="h" && CliClientOptions::port==4000,true);
    parseArguments({"prog","--host","h","--port","abc"},left);
    check("cli_bad_port",CliClientOptions::port==0,true);

    parseArguments({"prog","--server","S","--url","ws://x"},left);
    check("cli_conflict_clears_all",CliClientOptions::serverName.isEmpty() && CliClientOptions::url.isEmpty() &&
          CliClientOptions::host.isEmpty() && CliClientOptions::port==0,true);
    parseArguments({"prog","--port","4000","--url","ws://x"},left);
    check("cli_conflict_port_url",CliClientOptions::port==0 && CliClientOptions::url.isEmpty(),true);

    parseArguments({"prog","--server","S"},left);
    parseArguments({"prog"},left);
    check("cli_starts_from_defaults",CliClientOptions::serverName.isEmpty(),true);
    parseArguments({"prog","--server"},left);
    check("cli_missing_value",CliClientOptions::serverName.isEmpty() && left.size()==1,true);

    parseArguments({"prog","--autosolo=3,-2","--closewhenonmapafter=0","--fixed","--remote-control"},left);
    check("cli_autosolo_click",CliClientOptions::autosolo && CliClientOptions::autosoloClick &&
          CliClientOptions::autosoloClickDx==3 && CliClientOptions::autosoloClickDy==-2,true);
    check("cli_closewhenonmapafter_min_1",CliClientOptions::closeWhenOnMapAfter==1,true);
    check("cli_flags",CliClientOptions::fixedBackground && CliClientOptions::remoteControl,true);
    parseArguments({"prog","--autosolo"},left);
    check("cli_autosolo_no_click",CliClientOptions::autosolo && !CliClientOptions::autosoloClick,true);

    check("cli_help_stops",parseArguments({"prog","--help"},left),false);
}

static void testPickLanguage()
{
    std::unordered_map<std::string,std::string> languagesByShortName;
    checkText("language_none_available",::Language::pickLanguage("fr",true,languagesByShortName,"French","fr_FR"),"en");
    languagesByShortName["French"]="fr";
    languagesByShortName["pt_BR"]="pt";
    checkText("language_saved",::Language::pickLanguage("pt",true,languagesByShortName,"French","fr_FR"),"pt");
    checkText("language_saved_unavailable",::Language::pickLanguage("de",false,languagesByShortName,"French","fr_FR"),"fr");
    checkText("language_system_locale",::Language::pickLanguage("",false,languagesByShortName,"Portuguese","pt_BR"),"pt");
    checkText("language_fallback_en",::Language::pickLanguage("",false,languagesByShortName,"German","de_DE"),"en");
}

//only the tables a test needs are filled, no datapack is parsed
class TestLoader : public DatapackClientLoader
{
public:
    void addQuestText(const CATCHCHALLENGER_TYPE_QUEST &questId,const uint32_t &textId,const std::string &text,
                      const QuestCondition &type,const uint8_t &value,const bool &withCondition)
    {
        QuestStepWithConditionExtra entry;
        entry.text=text;
        if(withCondition)
        {
            QuestConditionExtra condition;
            condition.type=type;
            condition.value=value;
            entry.conditions.push_back(condition);
        }
        if(questsExtra.find(questId)==questsExtra.cend())
        {
            QuestExtra questExtra;
            questExtra.showRewards=false;
            questExtra.autostep=false;
            questsExtra[questId]=questExtra;
        }
        questsExtra[questId].text[textId].texts.push_back(entry);
    }
    TestLoader()
    {
        ZoneExtra zone;
        zone.id=0;
        zone.audioAmbiance["outdoor"]="zone.opus";
        zonesExtra["town"]=zone;
        audioAmbiance["outdoor"]="general.opus";
    }
protected:
    void emitdatapackParsed() override {}
    void emitdatapackParsedMainSub() override {}
    void emitdatapackChecksumError() override {}
    void parseTopLib() override {}
    std::string getLanguage() override { return "en"; }
};

static bool createFile(const QString &path)
{
    QFile file(path);
    if(!file.open(QIODevice::WriteOnly))
    {
        std::cout << "FAIL ambiance_setup cannot create " << path.toStdString() << std::endl;
        failures++;
        return false;
    }
    file.close();
    return true;
}

static void testAmbianceFile()
{
    QTemporaryDir dir;
    if(!dir.isValid())
    {
        std::cout << "FAIL ambiance_setup no temporary dir" << std::endl;
        failures++;
        return;
    }
    const std::string mainPath=dir.path().toStdString()+"/main/";
    const std::string basePath=dir.path().toStdString()+"/base/";
    if(!QDir().mkpath(dir.path()+"/main") || !QDir().mkpath(dir.path()+"/base") || !QDir().mkpath(dir.path()+"/main/folder.opus"))
    {
        std::cout << "FAIL ambiance_setup cannot create dirs" << std::endl;
        failures++;
        return;
    }
    const TestLoader loader;
    checkText("ambiance_none_exists",loader.ambianceFile("map.opus","town","outdoor",mainPath,basePath),"");
    if(!createFile(dir.path()+"/base/general.opus"))
        return;
    checkText("ambiance_general_from_base",loader.ambianceFile("map.opus","town","outdoor",mainPath,basePath),basePath+"general.opus");
    if(!createFile(dir.path()+"/base/zone.opus"))
        return;
    checkText("ambiance_zone_before_general",loader.ambianceFile("map.opus","town","outdoor",mainPath,basePath),basePath+"zone.opus");
    checkText("ambiance_unknown_zone",loader.ambianceFile("","village","outdoor",mainPath,basePath),basePath+"general.opus");
    checkText("ambiance_unknown_type",loader.ambianceFile("","town","cave",mainPath,basePath),"");
    if(!createFile(dir.path()+"/base/map.opus") || !createFile(dir.path()+"/main/map.opus"))
        return;
    checkText("ambiance_map_main_before_base",loader.ambianceFile("map.opus","town","outdoor",mainPath,basePath),mainPath+"map.opus");
    checkText("ambiance_directory_skipped",loader.ambianceFile("folder.opus","town","outdoor",mainPath,basePath),basePath+"zone.opus");
}

//records what the quest rules take from the inventory
class QuestApi : public TestApiProtocol
{
public:
    std::unordered_map<CATCHCHALLENGER_TYPE_ITEM,uint32_t> removedItems;
    void remove_to_inventory(const std::unordered_map<CATCHCHALLENGER_TYPE_ITEM,uint32_t> &items) override
    {
        std::unordered_map<CATCHCHALLENGER_TYPE_ITEM,uint32_t>::const_iterator i=items.cbegin();
        while(i!=items.cend())
        {
            removedItems[i->first]+=i->second;
            ++i;
        }
    }
};

static Quest::Step makeStep(const CATCHCHALLENGER_TYPE_MAPID &stepMapId,const CATCHCHALLENGER_TYPE_BOTID &stepBotId)
{
    Quest::Step step;
    step.botToTalkMapId=stepMapId;
    step.botToTalkBotId=stepBotId;
    return step;
}

//step 1 at bot (mapId,botId) wants 2 items, step 2 at bot (mapId,botId+1); rewards reputation 0 and clan creation
static Quest makeTwoStepQuest()
{
    Quest quest;
    quest.id=questId;
    quest.repeatable=false;
    quest.rewards.allow_create_clan=true;
    ReputationRewards reputationReward;
    reputationReward.reputationId=0;
    reputationReward.point=150;
    quest.rewards.reputation.push_back(reputationReward);
    Quest::Step first=makeStep(mapId,botId);
    Quest::Item item;
    item.item=itemId;
    item.quantity=2;
    first.requirements.items.push_back(item);
    quest.steps.push_back(first);
    quest.steps.push_back(makeStep(mapId,botId+1));
    return quest;
}

static void testQuestSteps()
{
    //reputation 0: level 1 at 100 points, level 2 at 200
    if(CommonDatapack::commonDatapack.get_reputation().empty())
    {
        Reputation reputation;
        reputation.reputation_positive.push_back(0);
        reputation.reputation_positive.push_back(100);
        reputation.reputation_positive.push_back(200);
        reputation.reputation_negative.push_back(0);
        reputation.reputation_negative.push_back(-100);
        reputation.reverse_database_id=0;
        reputation.name="test";
        CommonDatapack::commonDatapack.get_reputation_rw().push_back(reputation);
    }
    Quest quest=makeTwoStepQuest();
    QuestApi api;
    api.player_informations.allow_create_clan=false;
    std::vector<Api_protocol::ReputationLevelChange> changes;

    check("quest_can_start_at_first_step_bot",api.questForBot(quest,mapId,botId)==Api_protocol::QuestForBot_CanStart,true);
    check("quest_not_at_other_bot",api.questForBot(quest,mapId,botId+1)==Api_protocol::QuestForBot_None,true);
    check("quest_bot_id_on_other_map",api.questForBot(quest,mapId+1,botId)==Api_protocol::QuestForBot_None,true);
    check("quest_start_wrong_bot",api.validateQuestStep(quest,mapId,botId+1,changes)==Api_protocol::QuestStep_CantStart,true);
    check("quest_start",api.validateQuestStep(quest,mapId,botId,changes)==Api_protocol::QuestStep_Started &&
          api.player_informations.quests.at(questId).step==1 && !api.player_informations.quests.at(questId).finish_one_time,true);
    check("quest_step1_in_progress_at_its_bot",api.questForBot(quest,mapId,botId)==Api_protocol::QuestForBot_InProgress,true);
    check("quest_step1_missing_items",api.validateQuestStep(quest,mapId,botId,changes)==Api_protocol::QuestStep_CantContinue &&
          api.player_informations.quests.at(questId).step==1,true);
    api.player_informations.items[itemId]=2;
    check("quest_step1_at_next_step_bot",api.validateQuestStep(quest,mapId,botId+1,changes)==Api_protocol::QuestStep_OtherBot,true);
    check("quest_next_step",api.validateQuestStep(quest,mapId,botId,changes)==Api_protocol::QuestStep_NextStep &&
          api.player_informations.quests.at(questId).step==2,true);
    check("quest_step_items_taken",api.removedItems.size()==1 && api.removedItems[itemId]==2,true);
    check("quest_step2_bot_moved",api.questForBot(quest,mapId,botId)==Api_protocol::QuestForBot_None &&
          api.questForBot(quest,mapId,botId+1)==Api_protocol::QuestForBot_InProgress,true);
    check("quest_step2_at_first_bot",api.validateQuestStep(quest,mapId,botId,changes)==Api_protocol::QuestStep_OtherBot,true);
    check("quest_finish",api.validateQuestStep(quest,mapId,botId+1,changes)==Api_protocol::QuestStep_Finished &&
          api.player_informations.quests.at(questId).step==0 && api.player_informations.quests.at(questId).finish_one_time,true);
    check("quest_reward_reputation",api.player_informations.reputation.at(0).level==1 && api.player_informations.reputation.at(0).point==50 &&
          changes.size()==1 && changes.at(0).reputationId==0 && changes.at(0).better,true);
    check("quest_reward_clan",api.player_informations.allow_create_clan,true);
    check("quest_done_not_repeatable",api.questForBot(quest,mapId,botId)==Api_protocol::QuestForBot_None &&
          api.validateQuestStep(quest,mapId,botId,changes)==Api_protocol::QuestStep_CantStart,true);
    quest.repeatable=true;
    check("quest_done_repeatable",api.validateQuestStep(quest,mapId,botId,changes)==Api_protocol::QuestStep_Started &&
          api.player_informations.quests.at(questId).finish_one_time,true);

    Quest emptyQuest;
    emptyQuest.id=prerequisiteId;
    emptyQuest.repeatable=false;
    check("quest_without_step",api.questForBot(emptyQuest,mapId,botId)==Api_protocol::QuestForBot_None,true);

    Api_protocol::ReputationLevelChange change;
    check("reputation_zero_point",api.appendReputationPoint(0,0,change),false);
    check("reputation_unknown",api.appendReputationPoint(200,10,change),false);
    check("reputation_same_level",!api.appendReputationPoint(0,10,change) && api.player_informations.reputation.at(0).point==60,true);
    check("reputation_level_down",api.appendReputationPoint(0,-200,change) && !change.better && api.player_informations.reputation.at(0).level<1,true);
    const std::vector<Api_protocol::ReputationLevelChange> rewardChanges=api.appendReputationRewards(quest.rewards.reputation);
    check("reputation_rewards_list",rewardChanges.size()==1 && rewardChanges.at(0).better,true);
}

static void testQuestText()
{
    TestLoader loader;
    loader.addQuestText(questId,1,"step2",DatapackClientLoader::QuestCondition_queststep,2,true);
    loader.addQuestText(questId,1,"requirements",DatapackClientLoader::QuestCondition_haverequirements,0,true);
    loader.addQuestText(questId,1,"step1",DatapackClientLoader::QuestCondition_queststep,1,true);
    loader.addQuestText(questId,1,"always",DatapackClientLoader::QuestCondition_queststep,0,false);
    loader.addQuestText(questId,2,"step3",DatapackClientLoader::QuestCondition_queststep,3,true);
    TestApiProtocol api;
    std::string text;
    check("quest_text_no_extra",loader.questText(prerequisiteId,1,api,text)==DatapackClientLoader::QuestText_NoQuestExtra,true);
    check("quest_text_no_entry",loader.questText(questId,9,api,text)==DatapackClientLoader::QuestText_NoEntryPoint,true);
    check("quest_text_not_started_is_step1",loader.questText(questId,1,api,text)==DatapackClientLoader::QuestText_Found && text=="step1",true);
    setQuest(api,questId,2,false);
    check("quest_text_step2",loader.questText(questId,1,api,text)==DatapackClientLoader::QuestText_Found && text=="step2",true);
    setQuest(api,questId,0,true);
    check("quest_text_finished_no_step_text",loader.questText(questId,1,api,text)==DatapackClientLoader::QuestText_Found && text=="always",true);
    check("quest_text_none_matches",loader.questText(questId,2,api,text)==DatapackClientLoader::QuestText_NoText,true);
}

//querySendTime is protected: filled directly, no socket
class QueryTimeApi : public TestApiProtocol
{
public:
    void setQuerySendTime(const uint8_t &queryNumber,const uint64_t &sentAtMs)
    {
        querySendTime[queryNumber]=sentAtMs;
    }
};

static void testSlowServerDetector()
{
    QueryTimeApi api;
    check("query_time_none",api.worstRunningQueryTime(10000)==0,true);
    api.setQuerySendTime(1,9500);
    api.setQuerySendTime(2,8000);
    check("query_time_worst_in_ms",api.worstRunningQueryTime(10000)==2000,true);
    api.setQuerySendTime(3,20000);
    check("query_time_future_ignored",api.worstRunningQueryTime(10000)==2000,true);
    const uint64_t first=Api_protocol::queryClockMs();
    const uint64_t second=Api_protocol::queryClockMs();
    check("query_clock_monotonic",second>=first,true);

    SlowServerDetector detector;
    check("slow_fast",detector.state(0,1000)==SlowServerDetector::State_Fast,true);
    check("slow_query_waiting",detector.state(701,1000)==SlowServerDetector::State_QueryWaiting,true);
    check("slow_query_at_threshold",detector.state(700,1000)==SlowServerDetector::State_Fast,true);
    detector.replyReceived(800,1000);
    check("slow_reply_shown",detector.state(0,1000)==SlowServerDetector::State_LastReplySlow && detector.slowReplyTime()==800,true);
    detector.replyReceived(50,2000);
    check("slow_reply_kept_after_fast_reply",detector.state(0,2000)==SlowServerDetector::State_LastReplySlow && detector.slowReplyTime()==800,true);
    detector.replyReceived(900,3000);
    check("slow_reply_replaced_by_slower",detector.slowReplyTime()==900,true);
    check("slow_reply_still_shown_before_15s",detector.state(0,3000+14999)==SlowServerDetector::State_LastReplySlow,true);
    check("slow_reply_hidden_after_15s",detector.state(0,3000+15000)==SlowServerDetector::State_Fast,true);
    detector.replyReceived(50,3000+15000);
    check("fast_reply_after_15s_replaces",detector.slowReplyTime()==50,true);
    detector.replyReceived(800,40000);
    detector.reset();
    check("slow_reset",detector.state(0,40000)==SlowServerDetector::State_Fast,true);
}

int main(int argc,char *argv[])
{
    //ChatInputGuard owns a QTimer
    QCoreApplication application(argc,argv);
    //packet sizes: the quest rules send their packet
    ProtocolParsing::initialiseTheVariable(ProtocolParsing::InitialiseTheVariableType::AllInOne);
    testBeatBot();
    testNextStepRequirement();
    testStartRequirement();
    testChatParsing();
    testChatInputGuard();
    testCliClientOptions();
    testPickLanguage();
    testAmbianceFile();
    testSlowServerDetector();
    testQuestSteps();
    testQuestText();
    if(failures>0)
    {
        std::cerr << failures << " failure(s)" << std::endl;
        return 1;
    }
    return 0;
}
