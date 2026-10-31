// test/testingclientlogic/main.cpp — client-side game rules shared by every
// client (client/libcatchchallenger/Api_protocol.cpp): quest start / next-step
// requirements and the beaten-bot lookup; then the client logic shared in
// client/libqtcatchchallenger/ (chat text, chat input guard, command line,
// language pick) and the ambiance file pick of DatapackClientLoader. The expected values mirror the
// server (server/base/ClientEvents/LocalClientHandlerQuest.cpp): a client that
// disagrees hides a quest the server would accept, or offers one it refuses.
// Prints "PASS <name>" / "FAIL <name> <detail>" lines for testingclientlogic.py.

#include "../testingmapmanagement/TestApiProtocol.hpp"
#include "../../client/libcatchchallenger/DatapackClientLoader.hpp"
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

//only the ambiance tables are filled, no datapack is parsed
class AmbianceLoader : public DatapackClientLoader
{
public:
    AmbianceLoader()
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
    const AmbianceLoader loader;
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

int main(int argc,char *argv[])
{
    //ChatInputGuard owns a QTimer
    QCoreApplication application(argc,argv);
    testBeatBot();
    testNextStepRequirement();
    testStartRequirement();
    testChatParsing();
    testChatInputGuard();
    testCliClientOptions();
    testPickLanguage();
    testAmbianceFile();
    if(failures>0)
    {
        std::cerr << failures << " failure(s)" << std::endl;
        return 1;
    }
    return 0;
}
