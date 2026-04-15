#include "GeneratorQuests.hpp"
#include "GeneratorItems.hpp"
#include "GeneratorMonsters.hpp"
#include "Helper.hpp"
#include "MapStore.hpp"

#include "../../client/libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include "../../general/base/CommonDatapackServerSpec.hpp"

#include <sstream>

namespace GeneratorQuests {

std::string relativePathForQuest(uint16_t id)
{
    const auto &extras=QtDatapackClientLoader::datapackLoader->get_questsExtra();
    auto it=extras.find(id);
    std::string name;
    if(it!=extras.cend() && !it->second.name.empty())
        name=it->second.name;
    return "quests/"+Helper::toStringUint(id)+"-"+Helper::textForUrl(name)+".html";
}

// Get reputation name by id (from CommonDatapack reputation vector)
static std::string reputationName(uint8_t reputationId)
{
    const auto &repList=CatchChallenger::CommonDatapack::commonDatapack.get_reputation();
    if(reputationId<repList.size())
    {
        const auto &repExtra=QtDatapackClientLoader::datapackLoader->get_reputationExtra();
        auto it=repExtra.find(repList[reputationId].name);
        if(it!=repExtra.cend() && !it->second.name.empty())
            return it->second.name;
        if(!repList[reputationId].name.empty())
            return repList[reputationId].name;
    }
    return "Reputation #"+Helper::toStringUint(reputationId);
}

// reputationLevelToText equivalent
static std::string reputationLevelToText(uint8_t reputationId, uint8_t level, bool positif)
{
    const auto &repList=CatchChallenger::CommonDatapack::commonDatapack.get_reputation();
    std::string repName=reputationName(reputationId);

    if(reputationId<repList.size())
    {
        const auto &repExtra=QtDatapackClientLoader::datapackLoader->get_reputationExtra();
        auto it=repExtra.find(repList[reputationId].name);
        if(it!=repExtra.cend())
        {
            const auto &re=it->second;
            if(positif && level<re.reputation_positive.size())
                return "Level "+Helper::toStringUint(level)+" in "+repName+" ("+re.reputation_positive[level]+")";
            else if(!positif && level<re.reputation_negative.size())
                return "Level "+Helper::toStringUint(level)+" in "+repName+" ("+re.reputation_negative[level]+")";
        }
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
    const auto &itemsExtra=QtDatapackClientLoader::datapackLoader->get_itemsExtra();
    auto it=itemsExtra.find(itemId);
    if(it!=itemsExtra.cend())
    {
        info.name=it->second.name;
        info.link=Helper::relUrl(GeneratorItems::relativePathForItem(itemId));
        if(!it->second.imagePath.empty())
        {
            std::string rel=Helper::relativeFromDatapack(it->second.imagePath);
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

// Get map path from mapId using MapStore
static std::string mapPathForId(CATCHCHALLENGER_TYPE_MAPID mapId)
{
    const auto &allSets=MapStore::sets();
    for(size_t s=0;s<allSets.size();s++)
    {
        if(mapId<allSets[s].mapPaths.size())
            return allSets[s].mapPaths[mapId];
    }
    return std::string();
}

// Get mainCode for a mapId
static std::string mainCodeForMapId(CATCHCHALLENGER_TYPE_MAPID mapId)
{
    const auto &allSets=MapStore::sets();
    for(size_t s=0;s<allSets.size();s++)
    {
        if(mapId<allSets[s].mapPaths.size())
            return allSets[s].mainCode;
    }
    return std::string();
}

// Get map display name from path
static std::string mapDisplayName(CATCHCHALLENGER_TYPE_MAPID mapId)
{
    std::string path=mapPathForId(mapId);
    if(path.empty())
        return "Unknown map";
    std::string stem=path;
    if(stem.size()>4 && stem.substr(stem.size()-4)==".tmx")
        stem=stem.substr(0,stem.size()-4);
    return stem;
}

// Strip .tmx from a path
static std::string stripTmx(const std::string &path)
{
    if(path.size()>4 && path.substr(path.size()-4)==".tmx")
        return path.substr(0,path.size()-4);
    return path;
}

// Write bot location columns (map + zone) given a mapId
static void writeBotMapColumns(std::ostringstream &body, CATCHCHALLENGER_TYPE_MAPID botMapId)
{
    std::string mapPath=mapPathForId(botMapId);
    if(!mapPath.empty())
    {
        std::string mapName=mapDisplayName(botMapId);
        std::string mainCode=mainCodeForMapId(botMapId);
        if(!mainCode.empty())
        {
            std::string mapRel="map/"+mainCode+"/"+stripTmx(mapPath)+".html";
            body << "<td><a href=\"" << Helper::htmlEscape(Helper::relUrl(mapRel))
                 << "\" title=\"" << Helper::htmlEscape(mapName) << "\">"
                 << Helper::htmlEscape(mapName) << "</a></td>\n";
            body << "<td>&nbsp;</td>\n";
        }
        else
            body << "<td colspan=\"2\">" << Helper::htmlEscape(mapName) << "</td>\n";
    }
    else
        body << "<td colspan=\"2\">&nbsp;</td>\n";
}

void generate()
{
    const auto &extras=QtDatapackClientLoader::datapackLoader->get_questsExtra();
    const auto &quests=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_quests();
    const auto &monsterExtra=QtDatapackClientLoader::datapackLoader->get_monsterExtra();

    // ── Individual quest pages ──────────────────────────────────────────
    // Iterate over questsExtra (always populated) and optionally enrich
    // with Quest data from CommonDatapackServerSpec (may be empty if
    // quest definitions use old format without map= attribute).

    for(const auto &ep : extras)
    {
        const uint16_t id=ep.first;
        const auto &qe=ep.second;

        std::string name=qe.name.empty()
            ? ("Quest #"+Helper::toStringUint(id))
            : qe.name;

        // Try to get full quest data (may not exist)
        const CatchChallenger::Quest *quest=nullptr;
        auto qIt=quests.find(id);
        if(qIt!=quests.cend())
            quest=&qIt->second;

        std::string rel=relativePathForQuest(id);
        Helper::setCurrentPage(rel);

        std::ostringstream body;

        // Header: name + repeatable/one time + #ID
        body << "<div class=\"map item_details\">\n";
        body << "<div class=\"subblock\"><h1>" << Helper::htmlEscape(name);
        if(quest!=nullptr)
        {
            if(quest->repeatable)
                body << " (repeatable)\n";
            else
                body << " (one time)\n";
        }
        body << "</h1>\n";
        body << "<h2>#" << id << "</h2>\n";
        body << "</div>\n";
        body << "</div>\n";

        // Starting bot (from first step if quest data available)
        if(quest!=nullptr && !quest->steps.empty())
        {
            uint8_t botId=quest->steps[0].botToTalkBotId;
            CATCHCHALLENGER_TYPE_MAPID botMapId=quest->steps[0].botToTalkMapId;

            body << "<center><table class=\"item_list item_list_type_normal\"><tr class=\"value\">\n";

            std::string botLabel="Bot #"+Helper::toStringUint(botId);
            body << "<td colspan=\"2\">" << botLabel << "</td>\n";

            writeBotMapColumns(body,botMapId);

            body << "</tr></table></center>\n";
        }

        body << "</div>\n";

        // Requirements (only if quest data available)
        if(quest!=nullptr && (!quest->requirements.quests.empty() || !quest->requirements.reputation.empty()))
        {
            body << "<div class=\"subblock\"><div class=\"valuetitle\">Requirements</div><div class=\"value\">\n";

            for(const auto &qr : quest->requirements.quests)
            {
                std::string reqName;
                auto reqIt=extras.find(qr.quest);
                if(reqIt!=extras.cend() && !reqIt->second.name.empty())
                    reqName=reqIt->second.name;
                else
                    reqName="Quest #"+Helper::toStringUint(qr.quest);

                std::string reqRel=relativePathForQuest(qr.quest);
                body << "Quest: <a href=\"" << Helper::htmlEscape(Helper::relUrl(reqRel))
                     << "\" title=\"" << Helper::htmlEscape(reqName) << "\">\n";
                body << reqName;
                body << "</a><br />\n";
            }

            for(const auto &rr : quest->requirements.reputation)
                body << reputationLevelToText(rr.reputationId,rr.level,rr.positif) << "<br />\n";

            body << "</div></div>\n";
        }

        // Steps
        if(quest!=nullptr)
        {
            // Full quest data: show steps with items/monsters
            for(size_t stepIdx=0;stepIdx<quest->steps.size();stepIdx++)
            {
                const CatchChallenger::Quest::Step &step=quest->steps[stepIdx];
                unsigned stepNum=stepIdx+1;

                body << "<div class=\"subblock\"><div class=\"valuetitle\">Step #" << stepNum;

                // If step bot differs from first step's bot, show bot info
                if(stepIdx>0 && step.botToTalkBotId!=quest->steps[0].botToTalkBotId)
                {
                    std::string stepBotLabel="Bot #"+Helper::toStringUint(step.botToTalkBotId);

                    body << "<center><table class=\"item_list item_list_type_normal\"><tr class=\"value\">\n";
                    body << "<td colspan=\"2\">" << stepBotLabel << "</td>\n";
                    writeBotMapColumns(body,step.botToTalkMapId);
                    body << "</tr></table></center>\n";
                }

                body << "</div><div class=\"value\">\n";

                // Step text from QuestExtra
                if(stepIdx<qe.steps.size())
                    body << qe.steps[stepIdx];

                // Build unified item list
                struct StepItem {
                    CATCHCHALLENGER_TYPE_ITEM item;
                    uint32_t quantity;
                    bool hasMonster;
                    CATCHCHALLENGER_TYPE_MONSTER monster;
                    uint8_t rate;
                };
                std::vector<StepItem> stepItems;

                for(const auto &it : step.requirements.items)
                    stepItems.push_back({it.item,it.quantity,false,0,0});
                for(const auto &im : step.itemsMonster)
                    stepItems.push_back({im.item,1,!im.monsters.empty(),
                        im.monsters.empty()?static_cast<CATCHCHALLENGER_TYPE_MONSTER>(0):im.monsters[0],im.rate});

                if(!stepItems.empty())
                {
                    bool show_full=false;
                    for(const auto &si : stepItems)
                    {
                        if(si.hasMonster && monsterExtra.find(si.monster)!=monsterExtra.cend())
                            show_full=true;
                    }

                    body << "<table class=\"item_list item_list_type_outdoor\"><tr class=\"item_list_title item_list_title_type_outdoor\">\n";
                    if(show_full)
                        body << "<th colspan=\"2\">Item</th><th colspan=\"2\">Monster</th><th>Luck</th></tr>\n";
                    else
                        body << "<th colspan=\"2\">Item</th></tr>\n";

                    for(const auto &si : stepItems)
                    {
                        writeItemCells(body,si.item,si.quantity);

                        if(si.hasMonster)
                        {
                            auto mIt=monsterExtra.find(si.monster);
                            if(mIt!=monsterExtra.cend())
                            {
                                const auto &me=mIt->second;
                                std::string monLink=Helper::relUrl(GeneratorMonsters::relativePathForMonster(si.monster));

                                body << "<td>\n";
                                std::string monSmallRel="monsters/"+Helper::toStringUint(si.monster)+"/small.png";
                                if(Helper::fileExists(Helper::datapackPath()+monSmallRel))
                                {
                                    std::string monImg=Helper::relUrl(Helper::publishDatapackFile(monSmallRel));
                                    body << "<div class=\"monstericon\"><a href=\"" << Helper::htmlEscape(monLink)
                                         << "\"><img src=\"" << monImg << "\" width=\"32\" height=\"32\" alt=\""
                                         << Helper::htmlEscape(me.name) << "\" title=\"" << Helper::htmlEscape(me.name) << "\" /></a></div>\n";
                                }
                                else
                                {
                                    monSmallRel="monsters/"+Helper::toStringUint(si.monster)+"/small.gif";
                                    if(Helper::fileExists(Helper::datapackPath()+monSmallRel))
                                    {
                                        std::string monImg=Helper::relUrl(Helper::publishDatapackFile(monSmallRel));
                                        body << "<div class=\"monstericon\"><a href=\"" << Helper::htmlEscape(monLink)
                                             << "\"><img src=\"" << monImg << "\" width=\"32\" height=\"32\" alt=\""
                                             << Helper::htmlEscape(me.name) << "\" title=\"" << Helper::htmlEscape(me.name) << "\" /></a></div>\n";
                                    }
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
        }
        else if(!qe.steps.empty())
        {
            // No full quest data, but we have step texts from extras
            for(size_t stepIdx=0;stepIdx<qe.steps.size();stepIdx++)
            {
                body << "<div class=\"subblock\"><div class=\"valuetitle\">Step #" << (stepIdx+1);
                body << "</div><div class=\"value\">\n";
                body << qe.steps[stepIdx];
                body << "</div></div>\n";
            }
        }

        // Rewards (only if quest data available)
        if(quest!=nullptr && (!quest->rewards.items.empty() || !quest->rewards.reputation.empty() || quest->rewards.allowCreateClan))
        {
            body << "<div class=\"subblock\"><div class=\"valuetitle\">Rewards</div><div class=\"value\">\n";

            if(!quest->rewards.items.empty())
            {
                body << "<table class=\"item_list item_list_type_outdoor\"><tr class=\"item_list_title item_list_title_type_outdoor\">\n"
                     << "<th colspan=\"2\">Item</th></tr>\n";

                for(const auto &ri : quest->rewards.items)
                {
                    writeItemCells(body,ri.item,ri.quantity);
                    body << "</tr>\n";
                }

                body << "<tr>\n<td colspan=\"2\" class=\"item_list_endline item_list_title_type_outdoor\"></td>\n</tr></table>\n";
            }

            for(const auto &rr : quest->rewards.reputation)
            {
                if(rr.point<0)
                    body << "Less reputation in: " << reputationName(rr.reputationId);
                else
                    body << "More reputation in: " << reputationName(rr.reputationId);
            }

            if(quest->rewards.allowCreateClan)
                body << "Able to create clan";

            body << "</div></div>\n";
        }

        Helper::writeHtml(rel,name,body.str());
    }

    // ── Index page (questList) ──────────────────────────────────────────

    Helper::setCurrentPage("quests.html");

    std::ostringstream indexBody;

    indexBody << "<table class=\"item_list item_list_type_normal\">\n"
              << "<tr class=\"item_list_title item_list_title_type_normal\">\n"
              << "\t<th>Quests</th>\n"
              << "\t<th>Repeatable</th>\n"
              << "\t<th colspan=\"4\">Starting bot</th>\n"
              << "\t<th>Rewards</th>\n"
              << "</tr>\n";

    for(const auto &ep : extras)
    {
        const uint16_t id=ep.first;
        const auto &qe=ep.second;

        std::string name=qe.name.empty()
            ? ("Quest #"+Helper::toStringUint(id))
            : qe.name;

        const CatchChallenger::Quest *quest=nullptr;
        auto qIt=quests.find(id);
        if(qIt!=quests.cend())
            quest=&qIt->second;

        std::string questLink=Helper::relUrl(relativePathForQuest(id));

        indexBody << "<tr class=\"value\">\n";

        // Quest name column
        indexBody << "<td><a href=\"" << Helper::htmlEscape(questLink) << "\" title=\""
                  << Helper::htmlEscape(name) << "\">" << Helper::htmlEscape(name) << "</a></td>\n";

        // Repeatable
        if(quest!=nullptr)
        {
            if(quest->repeatable)
                indexBody << "<td>Yes</td>\n";
            else
                indexBody << "<td>No</td>\n";
        }
        else
            indexBody << "<td>&nbsp;</td>\n";

        // Starting bot (4 columns: skin, name, map, zone)
        if(quest!=nullptr && !quest->steps.empty())
        {
            std::string botLabel="Bot #"+Helper::toStringUint(quest->steps[0].botToTalkBotId);

            indexBody << "<td colspan=\"2\">" << botLabel << "</td>\n";

            std::string mapPath=mapPathForId(quest->steps[0].botToTalkMapId);
            if(!mapPath.empty())
            {
                std::string mapName=mapDisplayName(quest->steps[0].botToTalkMapId);
                std::string mainCode=mainCodeForMapId(quest->steps[0].botToTalkMapId);
                if(!mainCode.empty())
                {
                    std::string mapRel="map/"+mainCode+"/"+stripTmx(mapPath)+".html";
                    indexBody << "<td><a href=\"" << Helper::htmlEscape(Helper::relUrl(mapRel))
                              << "\" title=\"" << Helper::htmlEscape(mapName) << "\">"
                              << Helper::htmlEscape(mapName) << "</a></td>\n";
                    indexBody << "<td>&nbsp;</td>\n";
                }
                else
                    indexBody << "<td colspan=\"2\">" << Helper::htmlEscape(mapName) << "</td>\n";
            }
            else
                indexBody << "<td colspan=\"2\">&nbsp;</td>\n";
        }
        else
            indexBody << "<td colspan=\"4\">&nbsp;</td>\n";

        // Rewards column
        indexBody << "<td>\n";
        if(quest!=nullptr && (!quest->rewards.items.empty() || !quest->rewards.reputation.empty() || quest->rewards.allowCreateClan))
        {
            indexBody << "<div class=\"subblock\"><div class=\"value\">\n";

            if(!quest->rewards.items.empty())
            {
                indexBody << "<table>\n";
                for(const auto &ri : quest->rewards.items)
                {
                    ItemInfo info=getItemInfo(ri.item);
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

            for(const auto &rr : quest->rewards.reputation)
            {
                if(rr.point<0)
                    indexBody << "Less reputation in: " << reputationName(rr.reputationId);
                else
                    indexBody << "More reputation in: " << reputationName(rr.reputationId);
            }

            if(quest->rewards.allowCreateClan)
                indexBody << "Able to create clan";

            indexBody << "</div></div>\n";
        }
        indexBody << "</td>\n";

        indexBody << "</tr>\n";
    }

    indexBody << "<tr>\n\t<td colspan=\"7\" class=\"item_list_endline item_list_title_type_normal\"></td>\n</tr>\n</table>\n";

    Helper::writeHtml("quests.html","Quests list",indexBody.str());
}

} // namespace GeneratorQuests
