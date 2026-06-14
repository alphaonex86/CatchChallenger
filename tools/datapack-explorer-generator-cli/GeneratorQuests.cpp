#include "GeneratorQuests.hpp"
#include "GeneratorItems.hpp"
#include "GeneratorMonsters.hpp"
#include "Helper.hpp"
#include "QuestStore.hpp"

#include "../../client/libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../general/base/customtinyxml2.hpp"

#include <map>
#include <sstream>

namespace GeneratorQuests {

std::string relativePathForQuest(const std::string &mainCode, uint16_t id)
{
    std::string name;
    const QuestStore::MainCodeSet *set=QuestStore::setForMainCode(mainCode);
    if(set!=nullptr)
    {
        const QuestStore::Quest *quest=QuestStore::questById(*set,id);
        if(quest!=nullptr)
            name=quest->name;
    }
    return "quests/"+mainCode+"/"+Helper::toStringUint(id)+"-"+Helper::textForUrl(name)+".html";
}

std::string relativePathForQuest(uint16_t id)
{
    // The per-main loaders only keep the LAST parsed main when the
    // generators run, so legacy callers passing a bare id (GeneratorItems)
    // are in fact referencing the last main that defines this quest.
    const std::vector<QuestStore::MainCodeSet> &allSets=QuestStore::sets();
    size_t s=allSets.size();
    while(s>0)
    {
        s--;
        if(QuestStore::questById(allSets[s],id)!=nullptr)
            return relativePathForQuest(allSets[s].mainCode,id);
    }
    // Unknown quest: keep the old single-main scheme as last resort
    return "quests/"+Helper::toStringUint(id)+"-"+Helper::textForUrl(std::string())+".html";
}

// Reputation display name from its datapack code ("nation", ...). The
// reputation extras are part of the base datapack and stay loaded.
static std::string reputationName(const std::string &type)
{
    if(QtDatapackClientLoader::datapackLoader->has_reputationExtra(type))
    {
        const DatapackClientLoader::ReputationExtra &repExEntry=QtDatapackClientLoader::datapackLoader->get_reputationExtra(type);
        if(!repExEntry.name.empty())
            return repExEntry.name;
    }
    return type;
}

// reputationLevelToText equivalent
static std::string reputationLevelToText(const std::string &type, uint8_t level, bool positif)
{
    std::string repName=reputationName(type);
    if(QtDatapackClientLoader::datapackLoader->has_reputationExtra(type))
    {
        const DatapackClientLoader::ReputationExtra &re=QtDatapackClientLoader::datapackLoader->get_reputationExtra(type);
        if(positif && level<re.reputation_positive.size())
            return "Level "+Helper::toStringUint(level)+" in "+repName+" ("+re.reputation_positive[level]+")";
        else if(!positif && level<re.reputation_negative.size())
            return "Level "+Helper::toStringUint(level)+" in "+repName+" ("+re.reputation_negative[level]+")";
    }
    return "Level "+Helper::toStringUint(level)+" in "+repName;
}

// Get item name/link/image for an item id
struct ItemInfo {
    std::string name;
    std::string link;
    std::string image;
};

static ItemInfo getItemInfo(CATCHCHALLENGER_TYPE_ITEM itemId)
{
    ItemInfo info;
    if(QtDatapackClientLoader::datapackLoader->has_itemExtra(itemId))
    {
        const DatapackClientLoader::ItemExtra &ie=QtDatapackClientLoader::datapackLoader->get_itemExtra(itemId);
        info.name=ie.name;
        info.link=Helper::relUrl(GeneratorItems::relativePathForItem(itemId));
        if(!ie.imagePath.empty())
        {
            std::string rel=Helper::relativeFromDatapack(ie.imagePath);
            if(!rel.empty() && Helper::fileExists(Helper::datapackPath()+rel))
                info.image=Helper::relUrl(Helper::publishDatapackFile(rel));
        }
    }
    return info;
}

// Write item icon+name cell pair (used for step items and reward items)
static void writeItemCells(std::ostringstream &body, CATCHCHALLENGER_TYPE_ITEM itemId, uint32_t quantity)
{
    ItemInfo info=getItemInfo(itemId);

    body << "<tr class=\"value\"><td>\n";

    // Icon
    if(!info.image.empty())
    {
        if(!info.link.empty())
            body << "<a href=\"" << Helper::htmlEscape(info.link) << "\">\n";
        body << "<img src=\"" << info.image << "\" width=\"24\" height=\"24\" alt=\""
             << Helper::htmlEscape(info.name) << "\" title=\"" << Helper::htmlEscape(info.name) << "\" />\n";
        if(!info.link.empty())
            body << "</a>\n";
    }
    body << "</td><td>\n";

    // Name with quantity
    if(!info.link.empty())
        body << "<a href=\"" << Helper::htmlEscape(info.link) << "\">\n";
    std::string qtext;
    if(quantity>1)
        qtext=Helper::toStringUint(quantity)+" ";
    if(!info.name.empty())
        body << qtext << info.name;
    else
        body << qtext << "Unknown item";
    if(!info.link.empty())
        body << "</a>\n";
    body << "</td>\n";
}

// ── Map name / zone helpers (from the map xml, like the old PHP) ───────

static std::string stripTmx(const std::string &path)
{
    if(path.size()>4 && path.substr(path.size()-4)==".tmx")
        return path.substr(0,path.size()-4);
    return path;
}

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

struct MapMetaLite {
    std::string name;       // display name, never empty
    std::string zoneName;   // zone display name, may be empty
};

static std::map<std::pair<std::string,std::string>,MapMetaLite> s_mapMeta;

static std::string zoneDisplayName(const std::string &mainCode, const std::string &zoneCode)
{
    if(zoneCode.empty())
        return std::string();
    std::string xmlPath=Helper::datapackPath()+"map/main/"+mainCode+"/zone/"+zoneCode+".xml";
    tinyxml2::XMLDocument doc;
    if(doc.LoadFile(xmlPath.c_str())==tinyxml2::XML_SUCCESS && doc.RootElement()!=NULL)
    {
        std::string n=firstEnText(doc.RootElement(),"name");
        if(!n.empty())
            return n;
    }
    return zoneCode;
}

static const MapMetaLite &mapMeta(const std::string &mainCode, const std::string &mapTmxPath)
{
    std::pair<std::string,std::string> key(mainCode,mapTmxPath);
    std::map<std::pair<std::string,std::string>,MapMetaLite>::const_iterator it=s_mapMeta.find(key);
    if(it!=s_mapMeta.end())
        return it->second;

    MapMetaLite meta;
    std::string xmlPath=Helper::datapackPath()+"map/main/"+mainCode+"/"+stripTmx(mapTmxPath)+".xml";
    tinyxml2::XMLDocument doc;
    if(doc.LoadFile(xmlPath.c_str())==tinyxml2::XML_SUCCESS && doc.RootElement()!=NULL)
    {
        meta.name=firstEnText(doc.RootElement(),"name");
        const char *zone=doc.RootElement()->Attribute("zone");
        if(zone!=NULL)
            meta.zoneName=zoneDisplayName(mainCode,zone);
    }
    if(meta.name.empty())
    {
        // Old PHP fallback: "Unknown name (house1.tmx)"
        size_t pos=mapTmxPath.find_last_of('/');
        std::string fileName=(pos!=std::string::npos)?mapTmxPath.substr(pos+1):mapTmxPath;
        if(fileName.size()<=4 || fileName.substr(fileName.size()-4)!=".tmx")
            fileName+=".tmx";
        meta.name="Unknown name ("+fileName+")";
    }
    s_mapMeta[key]=meta;
    return s_mapMeta[key];
}

// ── Starting-bot resolution ─────────────────────────────────────────────

struct BotDisplay {
    bool hasBot;            // quest references a bot at all
    uint32_t botId;
    std::string name;       // bot display name, may be empty
    std::string mapTmxPath; // bot map, may be empty
};

static BotDisplay resolveBot(const QuestStore::MainCodeSet &set, bool hasBot, uint32_t botId, const std::string &explicitMap)
{
    BotDisplay bd;
    bd.hasBot=hasBot;
    bd.botId=botId;
    bd.mapTmxPath=explicitMap;
    if(!hasBot)
        return bd;
    std::map<uint32_t,QuestStore::BotInfo>::const_iterator it=set.bots.find(botId);
    if(it!=set.bots.end() && !it->second.ambiguous)
    {
        bd.name=it->second.name;
        if(bd.mapTmxPath.empty())
            bd.mapTmxPath=it->second.mapTmxPath;
    }
    return bd;
}

// Starting bot of the quest, the old PHP way: the first step's bot when it
// has one, otherwise the quest's bot.
static BotDisplay startingBot(const QuestStore::MainCodeSet &set, const QuestStore::Quest &quest)
{
    if(!quest.steps.empty() && quest.steps[0].hasBot)
        return resolveBot(set,true,quest.steps[0].botId,
                          !quest.steps[0].mapTmxPath.empty()?quest.steps[0].mapTmxPath:quest.mapTmxPath);
    return resolveBot(set,quest.hasBot,quest.botId,quest.mapTmxPath);
}

// Write the bot name + map + zone cells (3 columns worth: name colspan=2
// since we have no skin, then map/zone like the old PHP).
static void writeBotCells(std::ostringstream &body, const std::string &mainCode, const BotDisplay &bd)
{
    std::string label=bd.name;
    if(label.empty())
        label="Bot #"+Helper::toStringUint(bd.botId);
    body << "<td colspan=\"2\">" << Helper::htmlEscape(label) << "</td>\n";

    if(!bd.mapTmxPath.empty())
    {
        const MapMetaLite &meta=mapMeta(mainCode,bd.mapTmxPath);
        std::string mapRel="map/"+mainCode+"/"+stripTmx(bd.mapTmxPath)+".html";
        if(!meta.zoneName.empty())
        {
            body << "<td><a href=\"" << Helper::htmlEscape(Helper::relUrl(mapRel))
                 << "\" title=\"" << Helper::htmlEscape(meta.name) << "\">"
                 << Helper::htmlEscape(meta.name) << "</a></td>\n";
            body << "<td>" << Helper::htmlEscape(meta.zoneName) << "</td>\n";
        }
        else
            body << "<td colspan=\"2\"><a href=\"" << Helper::htmlEscape(Helper::relUrl(mapRel))
                 << "\" title=\"" << Helper::htmlEscape(meta.name) << "\">"
                 << Helper::htmlEscape(meta.name) << "</a></td>\n";
    }
    else
        body << "<td colspan=\"2\">&nbsp;</td>\n";
}

// ── Quest page ──────────────────────────────────────────────────────────

static void generateQuestPage(const QuestStore::MainCodeSet &set, const QuestStore::Quest &quest)
{
    std::string name=quest.name.empty()
        ? ("Quest #"+Helper::toStringUint(quest.id))
        : quest.name;

    std::string rel=relativePathForQuest(set.mainCode,quest.id);
    Helper::setCurrentPage(rel);

    std::ostringstream body;

    // Header: name + repeatable/one time + #ID
    body << "<div class=\"map item_details\">\n";
    body << "<div class=\"subblock\"><h1>" << Helper::htmlEscape(name);
    if(quest.repeatable)
        body << " (repeatable)\n";
    else
        body << " (one time)\n";
    body << "</h1>\n";
    body << "<h2>#" << quest.id << "</h2>\n";
    body << "</div>\n";
    body << "</div>\n";

    // Starting bot, only when something is known about it (the old PHP
    // skipped the whole block for bots absent from bots_meta)
    BotDisplay startBot=startingBot(set,quest);
    if(startBot.hasBot && (!startBot.name.empty() || !startBot.mapTmxPath.empty()))
    {
        body << "<center><table class=\"item_list item_list_type_normal\"><tr class=\"value\">\n";
        writeBotCells(body,set.mainCode,startBot);
        body << "</tr></table></center>\n";
    }

    body << "</div>\n";

    // Requirements
    if(!quest.requirementQuests.empty() || !quest.requirementReputation.empty())
    {
        body << "<div class=\"subblock\"><div class=\"valuetitle\">Requirements</div><div class=\"value\">\n";

        for(const uint16_t reqId : quest.requirementQuests)
        {
            const QuestStore::Quest *reqQuest=QuestStore::questById(set,reqId);
            std::string reqName;
            if(reqQuest!=nullptr && !reqQuest->name.empty())
                reqName=reqQuest->name;
            else
                reqName="Quest #"+Helper::toStringUint(reqId);

            std::string reqRel=relativePathForQuest(set.mainCode,reqId);
            body << "Quest: <a href=\"" << Helper::htmlEscape(Helper::relUrl(reqRel))
                 << "\" title=\"" << Helper::htmlEscape(reqName) << "\">\n";
            body << Helper::htmlEscape(reqName);
            body << "</a><br />\n";
        }

        for(const QuestStore::ReputationRequirement &rr : quest.requirementReputation)
            body << reputationLevelToText(rr.type,rr.level,rr.positif) << "<br />\n";

        body << "</div></div>\n";
    }

    // Steps
    for(size_t stepIdx=0;stepIdx<quest.steps.size();stepIdx++)
    {
        const QuestStore::QuestStep &step=quest.steps[stepIdx];
        unsigned stepNum=stepIdx+1;

        body << "<div class=\"subblock\"><div class=\"valuetitle\">Step #" << stepNum;

        // If the step has its own bot, show it (the old PHP only showed it
        // when it differs from the quest's starting bot)
        if(step.hasBot && step.botId!=startBot.botId)
        {
            BotDisplay stepBot=resolveBot(set,true,step.botId,step.mapTmxPath);
            if(!stepBot.name.empty() || !stepBot.mapTmxPath.empty())
            {
                body << "<center><table class=\"item_list item_list_type_normal\"><tr class=\"value\">\n";
                writeBotCells(body,set.mainCode,stepBot);
                body << "</tr></table></center>\n";
            }
        }

        body << "</div><div class=\"value\">\n";

        // Step text (raw datapack html, like the old PHP)
        body << step.text;

        if(!step.items.empty())
        {
            bool show_full=false;
            for(const QuestStore::QuestItem &si : step.items)
            {
                if(si.hasMonster && QtDatapackClientLoader::datapackLoader->has_monsterExtra(si.monsterId))
                    show_full=true;
            }

            body << "<table class=\"item_list item_list_type_outdoor\"><tr class=\"item_list_title item_list_title_type_outdoor\">\n";
            if(show_full)
                body << "<th colspan=\"2\">Item</th><th colspan=\"2\">Monster</th><th>Luck</th></tr>\n";
            else
                body << "<th colspan=\"2\">Item</th></tr>\n";

            for(const QuestStore::QuestItem &si : step.items)
            {
                writeItemCells(body,si.itemId,si.quantity);

                if(si.hasMonster)
                {
                    if(QtDatapackClientLoader::datapackLoader->has_monsterExtra(si.monsterId))
                    {
                        const DatapackClientLoader::MonsterExtra &me=QtDatapackClientLoader::datapackLoader->get_monsterExtra(si.monsterId);
                        std::string monLink=Helper::relUrl(GeneratorMonsters::relativePathForMonster(si.monsterId));

                        body << "<td>\n";
                        std::string monSmallRel="monsters/"+Helper::toStringUint(si.monsterId)+"/small.png";
                        if(!Helper::fileExists(Helper::datapackPath()+monSmallRel))
                            monSmallRel="monsters/"+Helper::toStringUint(si.monsterId)+"/small.gif";
                        if(Helper::fileExists(Helper::datapackPath()+monSmallRel))
                        {
                            std::string monImg=Helper::relUrl(Helper::publishDatapackFile(monSmallRel));
                            body << "<div class=\"monstericon\"><a href=\"" << Helper::htmlEscape(monLink)
                                 << "\"><img src=\"" << monImg << "\" width=\"32\" height=\"32\" alt=\""
                                 << Helper::htmlEscape(me.name) << "\" title=\"" << Helper::htmlEscape(me.name) << "\" /></a></div>\n";
                        }
                        body << "</td>\n<td><a href=\"" << Helper::htmlEscape(monLink) << "\">"
                             << Helper::htmlEscape(me.name) << "</a></td>\n";
                        body << "<td>" << (unsigned)si.rate << "%</td>\n";
                    }
                    else if(show_full)
                        body << "<td></td><td></td><td></td>\n";
                }
                else if(show_full)
                    body << "<td></td><td></td><td></td>\n";

                body << "</tr>\n";
            }

            if(show_full)
                body << "<tr>\n\t\t\t\t\t\t<td colspan=\"5\" class=\"item_list_endline item_list_title_type_outdoor\"></td>\n\t\t\t\t\t\t</tr></table>\n";
            else
                body << "<tr>\n\t\t\t\t\t\t<td colspan=\"2\" class=\"item_list_endline item_list_title_type_outdoor\"></td>\n\t\t\t\t\t\t</tr></table>\n";

            body << "<br />\n";
        }

        body << "</div></div>\n";
    }

    // Rewards
    if(!quest.rewardItems.empty() || !quest.rewardReputation.empty() || quest.allowCreateClan)
    {
        body << "<div class=\"subblock\"><div class=\"valuetitle\">Rewards</div><div class=\"value\">\n";

        if(!quest.rewardItems.empty())
        {
            body << "<table class=\"item_list item_list_type_outdoor\"><tr class=\"item_list_title item_list_title_type_outdoor\">\n"
                 << "<th colspan=\"2\">Item</th></tr>\n";

            for(const QuestStore::RewardItem &ri : quest.rewardItems)
            {
                writeItemCells(body,ri.itemId,ri.quantity);
                body << "</tr>\n";
            }

            body << "<tr>\n<td colspan=\"2\" class=\"item_list_endline item_list_title_type_outdoor\"></td>\n</tr></table>\n";
        }

        for(const QuestStore::ReputationReward &rr : quest.rewardReputation)
        {
            if(rr.point<0)
                body << "Less reputation in: " << reputationName(rr.type);
            else
                body << "More reputation in: " << reputationName(rr.type);
        }

        if(quest.allowCreateClan)
            body << "Able to create clan";

        body << "</div></div>\n";
    }

    Helper::writeHtml(rel,name,body.str());
}

// ── Index page (one questList table per main, like the old PHP) ─────────

static void writeIndexTable(std::ostringstream &indexBody, const QuestStore::MainCodeSet &set)
{
    indexBody << "<table class=\"item_list item_list_type_normal\">\n"
              << "<tr class=\"item_list_title item_list_title_type_normal\">\n"
              << "\t<th>Quests</th>\n"
              << "\t<th>Repeatable</th>\n"
              << "\t<th colspan=\"4\">Starting bot</th>\n"
              << "\t<th>Rewards</th>\n"
              << "</tr>\n";

    for(const QuestStore::Quest &quest : set.quests)
    {
        std::string name=quest.name.empty()
            ? ("Quest #"+Helper::toStringUint(quest.id))
            : quest.name;

        std::string questLink=Helper::relUrl(relativePathForQuest(set.mainCode,quest.id));

        indexBody << "<tr class=\"value\">\n";

        // Quest name column
        indexBody << "<td><a href=\"" << Helper::htmlEscape(questLink) << "\" title=\""
                  << Helper::htmlEscape(name) << "\">" << Helper::htmlEscape(name) << "</a></td>\n";

        // Repeatable
        if(quest.repeatable)
            indexBody << "<td>Yes</td>\n";
        else
            indexBody << "<td>No</td>\n";

        // Starting bot (4 columns: skin, name, map, zone). No skin data is
        // available so the name takes the first two columns like the old
        // PHP "skin not found" fallback.
        BotDisplay bd=startingBot(set,quest);
        if(bd.hasBot)
            writeBotCells(indexBody,set.mainCode,bd);
        else
            indexBody << "<td colspan=\"4\">&nbsp;</td>\n";

        // Rewards column
        indexBody << "<td>\n";
        if(!quest.rewardItems.empty() || !quest.rewardReputation.empty() || quest.allowCreateClan)
        {
            indexBody << "<div class=\"subblock\"><div class=\"value\">\n";

            if(!quest.rewardItems.empty())
            {
                indexBody << "<table>\n";
                for(const QuestStore::RewardItem &ri : quest.rewardItems)
                {
                    ItemInfo info=getItemInfo(ri.itemId);
                    indexBody << "<tr class=\"value\"><td>\n";
                    if(!info.image.empty())
                    {
                        if(!info.link.empty())
                            indexBody << "<a href=\"" << Helper::htmlEscape(info.link) << "\">\n";
                        indexBody << "<img src=\"" << info.image << "\" width=\"24\" height=\"24\" alt=\""
                                  << Helper::htmlEscape(info.name) << "\" title=\"" << Helper::htmlEscape(info.name) << "\" />\n";
                        if(!info.link.empty())
                            indexBody << "</a>\n";
                    }
                    indexBody << "</td><td>\n";
                    if(!info.link.empty())
                        indexBody << "<a href=\"" << Helper::htmlEscape(info.link) << "\">\n";
                    std::string qtext;
                    if(ri.quantity>1)
                        qtext=Helper::toStringUint(ri.quantity)+" ";
                    if(!info.name.empty())
                        indexBody << qtext << info.name;
                    else
                        indexBody << qtext << "Unknown item";
                    if(!info.link.empty())
                        indexBody << "</a>\n";
                    indexBody << "</td></tr>\n";
                }
                indexBody << "</table>\n";
            }

            for(const QuestStore::ReputationReward &rr : quest.rewardReputation)
            {
                if(rr.point<0)
                    indexBody << "Less reputation in: " << reputationName(rr.type);
                else
                    indexBody << "More reputation in: " << reputationName(rr.type);
            }

            if(quest.allowCreateClan)
                indexBody << "Able to create clan";

            indexBody << "</div></div>\n";
        }
        indexBody << "</td>\n";

        indexBody << "</tr>\n";
    }

    indexBody << "<tr>\n\t<td colspan=\"7\" class=\"item_list_endline item_list_title_type_normal\"></td>\n</tr>\n</table>\n";
}

void generate()
{
    // ── Individual quest pages, per main ────────────────────────────────
    for(const QuestStore::MainCodeSet &set : QuestStore::sets())
        for(const QuestStore::Quest &quest : set.quests)
            generateQuestPage(set,quest);

    // ── Index page (questList) ──────────────────────────────────────────
    Helper::setCurrentPage("quests.html");

    std::ostringstream indexBody;

    for(const QuestStore::MainCodeSet &set : QuestStore::sets())
    {
        if(set.quests.empty())
            continue;
        std::string label=(set.mainCode=="official")
            ? std::string("Official datapack")
            : set.mainCode;
        indexBody << "<h2>" << Helper::htmlEscape(label) << "</h2>\n";
        writeIndexTable(indexBody,set);
    }

    Helper::writeHtml("quests.html","Quests list",indexBody.str());
}

} // namespace GeneratorQuests
