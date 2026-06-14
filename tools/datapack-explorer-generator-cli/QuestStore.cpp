#include "QuestStore.hpp"
#include "Helper.hpp"

#include "../../client/libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include "../../general/base/cpp11addition.hpp"
#include "../../general/base/customtinyxml2.hpp"

#include <algorithm>
#include <iostream>
#include <set>

#include <sys/types.h>
#include <dirent.h>

namespace QuestStore {

static std::vector<MainCodeSet> g_sets;

// First <element> child of root with no lang attribute or lang="en"
static std::string firstEnText(const tinyxml2::XMLElement *root, const char *elementName)
{
    const tinyxml2::XMLElement *el=root->FirstChildElement(elementName);
    while(el!=NULL)
    {
        if(el->Attribute("lang")==NULL || strcmp(el->Attribute("lang"),"en")==0)
            if(el->GetText()!=NULL)
                return el->GetText();
        el=el->NextSiblingElement(elementName);
    }
    return std::string();
}

// Resolve a map= attribute value against the loader map list of the
// current main ("house1" or "house1.tmx" -> "house1.tmx"); empty if unknown.
static std::string resolveMapPath(const std::string &mapAttr, const std::set<std::string> &mapPaths)
{
    if(mapAttr.empty())
        return std::string();
    std::string p=mapAttr;
    std::replace(p.begin(),p.end(),'\\','/');
    if(mapPaths.find(p)!=mapPaths.end())
        return p;
    if(mapPaths.find(p+".tmx")!=mapPaths.end())
        return p+".tmx";
    return std::string();
}

// Resolve an item id= attribute (item name or numeric id)
static bool resolveItemId(const char *raw, uint16_t &itemId)
{
    if(raw==NULL)
        return false;
    const std::string lower=str_tolower(std::string(raw));
    if(CatchChallenger::CommonDatapack::commonDatapack.has_tempNameToItemId(lower))
    {
        itemId=CatchChallenger::CommonDatapack::commonDatapack.get_tempNameToItemId(lower);
        return true;
    }
    bool ok=false;
    const uint16_t id=stringtouint16(std::string(raw),&ok);
    if(ok)
    {
        itemId=id;
        return true;
    }
    return false;
}

// Resolve a monster reference (monster name or numeric id); the attribute
// can hold a ";" separated list, only the first entry is kept (like the
// old PHP which displayed a single source monster per item).
static bool resolveMonsterId(const char *raw, uint16_t &monsterId)
{
    if(raw==NULL)
        return false;
    const std::vector<std::string> parts=stringsplit(std::string(raw),';');
    unsigned int index=0;
    while(index<parts.size())
    {
        const std::string lower=str_tolower(parts.at(index));
        if(CatchChallenger::CommonDatapack::commonDatapack.has_tempNameToMonsterId(lower))
        {
            monsterId=CatchChallenger::CommonDatapack::commonDatapack.get_tempNameToMonsterId(lower);
            return true;
        }
        bool ok=false;
        const uint16_t id=stringtouint16(parts.at(index),&ok);
        if(ok)
        {
            monsterId=id;
            return true;
        }
        index++;
    }
    return false;
}

static void parseStepItems(const tinyxml2::XMLElement *step, QuestStep &stepData)
{
    const tinyxml2::XMLElement *item=step->FirstChildElement("item");
    while(item!=NULL)
    {
        QuestItem qi;
        qi.itemId=0;
        qi.quantity=1;
        qi.hasMonster=false;
        qi.monsterId=0;
        qi.rate=0;
        qi.itemOk=resolveItemId(item->Attribute("id"),qi.itemId);
        if(item->Attribute("quantity")!=NULL)
        {
            bool ok=false;
            const uint32_t q=stringtouint32(std::string(item->Attribute("quantity")),&ok);
            if(ok)
                qi.quantity=q;
        }
        if(item->Attribute("monster")!=NULL && item->Attribute("rate")!=NULL)
        {
            uint16_t monsterId=0;
            if(resolveMonsterId(item->Attribute("monster"),monsterId))
            {
                std::string rateString=item->Attribute("rate");
                stringreplaceOne(rateString,"%","");
                bool ok=false;
                const uint8_t rate=stringtouint8(rateString,&ok);
                if(ok)
                {
                    qi.hasMonster=true;
                    qi.monsterId=monsterId;
                    qi.rate=rate;
                }
            }
        }
        stepData.items.push_back(qi);
        item=item->NextSiblingElement("item");
    }
}

static bool parseQuestDefinition(const std::string &file, const std::set<std::string> &mapPaths, Quest &quest)
{
    tinyxml2::XMLDocument domDocument;
    if(domDocument.LoadFile(file.c_str())!=tinyxml2::XML_SUCCESS)
    {
        std::cerr << "QuestStore: " << file << ", " << tinyxml2errordoc(&domDocument) << std::endl;
        return false;
    }
    const tinyxml2::XMLElement *root=domDocument.RootElement();
    if(root==NULL || root->Name()==NULL || strcmp(root->Name(),"quest")!=0)
    {
        std::cerr << "QuestStore: \"quest\" root balise not found for the xml file: " << file << std::endl;
        return false;
    }

    quest.name=firstEnText(root,"name");
    quest.repeatable=false;
    if(root->Attribute("repeatable")!=NULL)
        if(strcmp(root->Attribute("repeatable"),"yes")==0 ||
           strcmp(root->Attribute("repeatable"),"true")==0)
            quest.repeatable=true;

    // Starting bot: bot= + map= attributes, bot="map/botid" old format,
    // or bot="<datapack global bot id>" without any map information.
    quest.hasBot=false;
    quest.botId=0;
    if(root->Attribute("bot")!=NULL)
    {
        bool ok=false;
        const std::string botAttr=root->Attribute("bot");
        if(root->Attribute("map")!=NULL)
        {
            const uint32_t botId=stringtouint32(botAttr,&ok);
            if(ok)
            {
                quest.hasBot=true;
                quest.botId=botId;
                quest.mapTmxPath=resolveMapPath(root->Attribute("map"),mapPaths);
            }
        }
        else
        {
            const size_t slashPos=botAttr.rfind('/');
            if(slashPos!=std::string::npos)
            {
                const uint32_t botId=stringtouint32(botAttr.substr(slashPos+1),&ok);
                if(ok)
                {
                    quest.hasBot=true;
                    quest.botId=botId;
                    quest.mapTmxPath=resolveMapPath(botAttr.substr(0,slashPos),mapPaths);
                }
            }
            else
            {
                const uint32_t botId=stringtouint32(botAttr,&ok);
                if(ok)
                {
                    quest.hasBot=true;
                    quest.botId=botId;
                }
            }
        }
    }

    // Requirements
    const tinyxml2::XMLElement *requirements=root->FirstChildElement("requirements");
    while(requirements!=NULL)
    {
        const tinyxml2::XMLElement *questReq=requirements->FirstChildElement("quest");
        while(questReq!=NULL)
        {
            if(questReq->Attribute("id")!=NULL)
            {
                bool ok=false;
                const uint16_t questId=stringtouint16(std::string(questReq->Attribute("id")),&ok);
                if(ok)
                    quest.requirementQuests.push_back(questId);
            }
            questReq=questReq->NextSiblingElement("quest");
        }
        const tinyxml2::XMLElement *repReq=requirements->FirstChildElement("reputation");
        while(repReq!=NULL)
        {
            if(repReq->Attribute("type")!=NULL && repReq->Attribute("level")!=NULL)
            {
                ReputationRequirement rr;
                rr.type=repReq->Attribute("type");
                std::string stringLevel=repReq->Attribute("level");
                rr.positif=stringLevel.empty() || stringLevel[0]!='-';
                if(!rr.positif)
                    stringLevel.erase(0,1);
                bool ok=false;
                rr.level=stringtouint8(stringLevel,&ok);
                if(ok)
                    quest.requirementReputation.push_back(rr);
            }
            repReq=repReq->NextSiblingElement("reputation");
        }
        requirements=requirements->NextSiblingElement("requirements");
    }

    // Steps, ordered by their id= attribute (1..n)
    {
        std::map<uint8_t,QuestStep> steps;
        const tinyxml2::XMLElement *step=root->FirstChildElement("step");
        while(step!=NULL)
        {
            bool ok=true;
            uint8_t stepId=1;
            if(step->Attribute("id")!=NULL)
                stepId=stringtouint8(std::string(step->Attribute("id")),&ok);
            if(ok)
            {
                QuestStep stepData;
                stepData.hasBot=false;
                stepData.botId=0;
                stepData.text=firstEnText(step,"text");
                if(step->Attribute("bot")!=NULL)
                {
                    bool botOk=false;
                    const uint32_t botId=stringtouint32(std::string(step->Attribute("bot")),&botOk);
                    if(botOk)
                    {
                        stepData.hasBot=true;
                        stepData.botId=botId;
                        if(step->Attribute("map")!=NULL)
                            stepData.mapTmxPath=resolveMapPath(step->Attribute("map"),mapPaths);
                    }
                }
                parseStepItems(step,stepData);
                steps[stepId]=stepData;
            }
            step=step->NextSiblingElement("step");
        }
        unsigned int indexLoop=1;
        while(indexLoop<(steps.size()+1))
        {
            if(steps.find(indexLoop)==steps.end())
                break;
            quest.steps.push_back(steps.at(indexLoop));
            indexLoop++;
        }
    }

    // Rewards
    quest.allowCreateClan=false;
    const tinyxml2::XMLElement *rewards=root->FirstChildElement("rewards");
    while(rewards!=NULL)
    {
        const tinyxml2::XMLElement *item=rewards->FirstChildElement("item");
        while(item!=NULL)
        {
            RewardItem ri;
            ri.itemId=0;
            ri.quantity=1;
            ri.itemOk=resolveItemId(item->Attribute("id"),ri.itemId);
            if(item->Attribute("quantity")!=NULL)
            {
                bool ok=false;
                const uint32_t q=stringtouint32(std::string(item->Attribute("quantity")),&ok);
                if(ok)
                    ri.quantity=q;
            }
            quest.rewardItems.push_back(ri);
            item=item->NextSiblingElement("item");
        }
        const tinyxml2::XMLElement *reputation=rewards->FirstChildElement("reputation");
        while(reputation!=NULL)
        {
            if(reputation->Attribute("type")!=NULL && reputation->Attribute("point")!=NULL)
            {
                ReputationReward rr;
                rr.type=reputation->Attribute("type");
                bool ok=false;
                rr.point=stringtoint32(std::string(reputation->Attribute("point")),&ok);
                if(ok)
                    quest.rewardReputation.push_back(rr);
            }
            reputation=reputation->NextSiblingElement("reputation");
        }
        const tinyxml2::XMLElement *allow=rewards->FirstChildElement("allow");
        while(allow!=NULL)
        {
            if(allow->Attribute("type")!=NULL && strcmp(allow->Attribute("type"),"clan")==0)
                quest.allowCreateClan=true;
            allow=allow->NextSiblingElement("allow");
        }
        rewards=rewards->NextSiblingElement("rewards");
    }

    return true;
}

// Scan the map xml files of the main for <bot id="..."> definitions
// matching the bot ids referenced by the quests. Ids reused by several
// maps are flagged ambiguous (bot ids are only unique per map since the
// per-map renumbering of the datapack).
static void scanBots(MainCodeSet &set, const std::vector<std::string> &mapPaths)
{
    std::set<uint32_t> wanted;
    for(const Quest &quest : set.quests)
    {
        if(quest.hasBot)
            wanted.insert(quest.botId);
        for(const QuestStep &step : quest.steps)
            if(step.hasBot)
                wanted.insert(step.botId);
    }
    if(wanted.empty())
        return;

    const std::string mapRoot=Helper::datapackPath()+"map/main/"+set.mainCode+"/";
    for(const std::string &mapPath : mapPaths)
    {
        std::string xmlPath=mapPath;
        if(xmlPath.size()>4 && xmlPath.compare(xmlPath.size()-4,4,".tmx")==0)
            xmlPath=xmlPath.substr(0,xmlPath.size()-4);
        xmlPath=mapRoot+xmlPath+".xml";
        if(!Helper::fileExists(xmlPath))
            continue;
        tinyxml2::XMLDocument doc;
        if(doc.LoadFile(xmlPath.c_str())!=tinyxml2::XML_SUCCESS)
            continue;
        const tinyxml2::XMLElement *root=doc.RootElement();
        if(root==NULL)
            continue;
        const tinyxml2::XMLElement *bot=root->FirstChildElement("bot");
        while(bot!=NULL)
        {
            if(bot->Attribute("id")!=NULL)
            {
                bool ok=false;
                const uint32_t botId=stringtouint32(std::string(bot->Attribute("id")),&ok);
                if(ok && wanted.find(botId)!=wanted.end())
                {
                    std::map<uint32_t,BotInfo>::iterator it=set.bots.find(botId);
                    if(it!=set.bots.end())
                        it->second.ambiguous=true;
                    else
                    {
                        BotInfo info;
                        info.name=firstEnText(bot,"name");
                        info.mapTmxPath=mapPath;
                        info.ambiguous=false;
                        set.bots[botId]=info;
                    }
                }
            }
            bot=bot->NextSiblingElement("bot");
        }
    }
}

void addFromCurrentLoader(const std::string &mainCode)
{
    for(const MainCodeSet &set : g_sets)
        if(set.mainCode==mainCode)
            return;

    MainCodeSet set;
    set.mainCode=mainCode;

    const std::vector<std::string> &loaderMaps=QtDatapackClientLoader::datapackLoader->get_maps();
    std::set<std::string> mapPathSet(loaderMaps.begin(),loaderMaps.end());

    // List quests/<id>/definition.xml folders
    const std::string questsRoot=Helper::datapackPath()+"map/main/"+mainCode+"/quests/";
    DIR *d=::opendir(questsRoot.c_str());
    if(d!=nullptr)
    {
        struct dirent *ent;
        while((ent=::readdir(d))!=nullptr)
        {
            std::string n=ent->d_name;
            if(n=="." || n=="..")
                continue;
            if(!Helper::isDir(questsRoot+n))
                continue;
            bool ok=false;
            const uint16_t questId=stringtouint16(n,&ok);
            if(!ok)
            {
                std::cerr << "QuestStore: quest folder name is not a number: " << questsRoot+n << std::endl;
                continue;
            }
            const std::string file=questsRoot+n+"/definition.xml";
            if(!Helper::fileExists(file))
                continue;
            Quest quest;
            quest.id=questId;
            if(parseQuestDefinition(file,mapPathSet,quest))
                set.quests.push_back(quest);
        }
        ::closedir(d);
    }

    std::sort(set.quests.begin(),set.quests.end(),
              [](const Quest &a, const Quest &b) { return a.id<b.id; });

    scanBots(set,loaderMaps);

    std::cout << set.quests.size() << " quest(s) stored for main " << mainCode << std::endl;
    g_sets.push_back(std::move(set));
}

const std::vector<MainCodeSet> &sets()
{
    return g_sets;
}

const MainCodeSet *setForMainCode(const std::string &mainCode)
{
    for(const MainCodeSet &set : g_sets)
        if(set.mainCode==mainCode)
            return &set;
    return nullptr;
}

const Quest *questById(const MainCodeSet &set, uint16_t id)
{
    for(const Quest &quest : set.quests)
        if(quest.id==id)
            return &quest;
    return nullptr;
}

} // namespace QuestStore
