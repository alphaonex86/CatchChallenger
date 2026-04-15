#include "GeneratorIndustries.hpp"
#include "GeneratorItems.hpp"
#include "Helper.hpp"
#include "MapStore.hpp"

#include "../../client/libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include "../../general/tinyXML2/tinyxml2.hpp"

#include <sstream>
#include <map>
#include <vector>
#include <cstdlib>
#include <cmath>
#include <iostream>

namespace GeneratorIndustries {

// ── Data structures ──────────────────────────────────────────────────

struct IndustryResource { uint16_t item; uint32_t quantity; };
struct IndustryProduct  { uint16_t item; uint32_t quantity; };

struct IndustryData
{
    std::string id;           // string ID from XML
    uint64_t time;
    uint32_t cycletobefull;
    std::vector<IndustryResource> resources;
    std::vector<IndustryProduct>  products;
};

struct BotLocation
{
    std::string mainCode;
    std::string mapStem;
    std::string mapDisplayName;
    std::string zoneCode;
    std::string zoneName;
    std::string botName;
    uint8_t botLocalId;
    std::string skinName;
};

// ── Helpers ──────────────────────────────────────────────────────────

static std::string itemImageUrl(uint16_t itemId)
{
    const auto &extras=QtDatapackClientLoader::datapackLoader->get_itemsExtra();
    auto it=extras.find(itemId);
    if(it==extras.cend() || it->second.imagePath.empty())
        return std::string();
    std::string rel=Helper::relativeFromDatapack(it->second.imagePath);
    if(rel.empty() || !Helper::fileExists(Helper::datapackPath()+rel))
        return std::string();
    return Helper::relUrl(Helper::publishDatapackFile(rel));
}

static void writeItemIconAndNameDiv(std::ostringstream &body, uint16_t itemId, const std::string &textAlign)
{
    const auto &extras=QtDatapackClientLoader::datapackLoader->get_itemsExtra();
    auto it=extras.find(itemId);
    std::string name=(it!=extras.cend())?it->second.name:"Unknown item";
    std::string link=Helper::relUrl(GeneratorItems::relativePathForItem(itemId));
    std::string image=itemImageUrl(itemId);

    body << "<div style=\"float:left;text-align:" << textAlign << ";\">\n";
    if(!image.empty())
    {
        if(!link.empty())
            body << "<a href=\"" << Helper::htmlEscape(link) << "\">\n";
        body << "<img src=\"" << image << "\" width=\"24\" height=\"24\" alt=\""
             << Helper::htmlEscape(name) << "\" title=\"" << Helper::htmlEscape(name) << "\" />\n";
        if(!link.empty())
            body << "</a>\n";
    }
    if(!link.empty())
        body << "<a href=\"" << Helper::htmlEscape(link) << "\">\n";
    if(!name.empty())
        body << name;
    else
        body << "Unknown item";
    if(!link.empty())
        body << "</a></div>\n";
}

static std::string skinTrainerUrl(const std::string &skinName)
{
    // Check skin/bot/ then skin/fighter/ for trainer.png then trainer.gif
    for(const char *dir : {"skin/bot/","skin/fighter/"})
    {
        for(const char *ext : {"trainer.png","trainer.gif"})
        {
            std::string rel=std::string(dir)+skinName+"/"+ext;
            if(Helper::fileExists(Helper::datapackPath()+rel))
                return Helper::publishDatapackFile(rel);
        }
    }
    return std::string();
}

// ── Parse industrialrecipe.xml ───────────────────────────────────────

static std::map<std::string,IndustryData> parseIndustrialRecipe(const std::string &mainCode)
{
    std::map<std::string,IndustryData> result;
    std::string xmlPath=Helper::datapackPath()+"map/main/"+mainCode+"/industries/industrialrecipe.xml";
    if(!Helper::fileExists(xmlPath))
        return result;

    tinyxml2::XMLDocument doc;
    if(doc.LoadFile(xmlPath.c_str())!=tinyxml2::XML_SUCCESS)
        return result;

    const tinyxml2::XMLElement *root=doc.RootElement();
    if(!root) return result;

    const tinyxml2::XMLElement *el=root->FirstChildElement("industrialrecipe");
    while(el)
    {
        IndustryData ind;
        const char *idAttr=el->Attribute("id");
        if(!idAttr) { el=el->NextSiblingElement("industrialrecipe"); continue; }
        ind.id=idAttr;
        {
            const char *v=el->Attribute("time");
            ind.time=v?(uint64_t)std::strtoull(v,nullptr,10):0;
        }
        {
            const char *v=el->Attribute("cycletobefull");
            ind.cycletobefull=v?(uint32_t)std::strtoul(v,nullptr,10):0;
        }

        const tinyxml2::XMLElement *res=el->FirstChildElement("resource");
        while(res)
        {
            IndustryResource r;
            const char *rid=res->Attribute("id");
            r.item=rid?(uint16_t)std::atoi(rid):0;
            const char *rq=res->Attribute("quantity");
            r.quantity=rq?(uint32_t)std::strtoul(rq,nullptr,10):0;
            ind.resources.push_back(r);
            res=res->NextSiblingElement("resource");
        }

        const tinyxml2::XMLElement *prod=el->FirstChildElement("product");
        while(prod)
        {
            IndustryProduct p;
            const char *pid=prod->Attribute("id");
            p.item=pid?(uint16_t)std::atoi(pid):0;
            const char *pq=prod->Attribute("quantity");
            p.quantity=pq?(uint32_t)std::strtoul(pq,nullptr,10):0;
            ind.products.push_back(p);
            prod=prod->NextSiblingElement("product");
        }

        result[ind.id]=std::move(ind);
        el=el->NextSiblingElement("industrialrecipe");
    }
    return result;
}

// ── Build industry-to-bot mapping ────────────────────────────────────

static std::map<std::string/*industryId*/,std::vector<BotLocation>>
buildIndustryToBotMap(const std::string &mainCode, size_t setIdx)
{
    std::map<std::string,std::vector<BotLocation>> result;
    const auto &set=MapStore::sets()[setIdx];

    for(size_t mi=0; mi<set.mapPaths.size(); ++mi)
    {
        std::string stem=set.mapPaths[mi];
        if(stem.size()>=4 && stem.compare(stem.size()-4,4,".tmx")==0)
            stem=stem.substr(0,stem.size()-4);

        // Parse the map XML for bot data (same approach as GeneratorMaps)
        std::string xmlPath=Helper::datapackPath()+"map/main/"+mainCode+"/"+set.mapPaths[mi];
        xmlPath=xmlPath.substr(0,xmlPath.size()-4)+".xml";

        tinyxml2::XMLDocument doc;
        if(doc.LoadFile(xmlPath.c_str())!=tinyxml2::XML_SUCCESS)
            continue;

        // Get map name from XML
        std::string mapName;
        std::string zoneCode;
        if(doc.RootElement())
        {
            const tinyxml2::XMLElement *nameEl=doc.RootElement()->FirstChildElement("name");
            if(nameEl && nameEl->GetText())
                mapName=nameEl->GetText();
            const tinyxml2::XMLElement *zoneEl=doc.RootElement()->FirstChildElement("zone");
            if(zoneEl && zoneEl->GetText())
                zoneCode=zoneEl->GetText();
        }
        if(mapName.empty())
        {
            // Use last path component
            size_t pos=stem.rfind('/');
            mapName=(pos!=std::string::npos)?stem.substr(pos+1):stem;
            mapName[0]=(char)toupper((unsigned char)mapName[0]);
        }

        // Get zone display name
        std::string zoneName;
        if(!zoneCode.empty())
        {
            std::string zoneXml=Helper::datapackPath()+"map/main/"+mainCode+"/zone/"+zoneCode+".xml";
            tinyxml2::XMLDocument zdoc;
            if(zdoc.LoadFile(zoneXml.c_str())==tinyxml2::XML_SUCCESS && zdoc.RootElement())
            {
                const tinyxml2::XMLElement *zn=zdoc.RootElement()->FirstChildElement("name");
                if(zn && zn->GetText())
                    zoneName=zn->GetText();
            }
        }

        // Parse TMX for bot skins
        std::map<int,std::string> botIdToSkin;
        {
            std::string tmxPath=Helper::datapackPath()+"map/main/"+mainCode+"/"+set.mapPaths[mi];
            tinyxml2::XMLDocument tmxDoc;
            if(tmxDoc.LoadFile(tmxPath.c_str())==tinyxml2::XML_SUCCESS && tmxDoc.RootElement())
            {
                const tinyxml2::XMLElement *og=tmxDoc.RootElement()->FirstChildElement("objectgroup");
                while(og)
                {
                    const tinyxml2::XMLElement *obj=og->FirstChildElement("object");
                    while(obj)
                    {
                        const char *t=obj->Attribute("type");
                        if(t && strcmp(t,"bot")==0)
                        {
                            const tinyxml2::XMLElement *props=obj->FirstChildElement("properties");
                            if(props)
                            {
                                int bid=-1;
                                std::string skin;
                                const tinyxml2::XMLElement *prop=props->FirstChildElement("property");
                                while(prop)
                                {
                                    const char *pn=prop->Attribute("name");
                                    const char *pv=prop->Attribute("value");
                                    if(pn && pv)
                                    {
                                        if(strcmp(pn,"id")==0) bid=std::atoi(pv);
                                        else if(strcmp(pn,"skin")==0) skin=pv;
                                    }
                                    prop=prop->NextSiblingElement("property");
                                }
                                if(bid>=0 && !skin.empty())
                                    botIdToSkin[bid]=skin;
                            }
                        }
                        obj=obj->NextSiblingElement("object");
                    }
                    og=og->NextSiblingElement("objectgroup");
                }
            }
        }

        // Parse bot elements for industry steps
        if(!doc.RootElement()) continue;
        const tinyxml2::XMLElement *botEl=doc.RootElement()->FirstChildElement("bot");
        while(botEl)
        {
            const char *idAttr=botEl->Attribute("id");
            if(!idAttr) { botEl=botEl->NextSiblingElement("bot"); continue; }
            uint8_t botId=(uint8_t)std::atoi(idAttr);

            std::string botName;
            const tinyxml2::XMLElement *bnEl=botEl->FirstChildElement("name");
            if(bnEl && bnEl->GetText())
                botName=bnEl->GetText();

            std::string skinName;
            const char *skinAttr=botEl->Attribute("skin");
            if(skinAttr)
                skinName=skinAttr;
            else if(botIdToSkin.count(botId))
                skinName=botIdToSkin[botId];

            const tinyxml2::XMLElement *step=botEl->FirstChildElement("step");
            while(step)
            {
                const char *type=step->Attribute("type");
                if(type && strcmp(type,"industry")==0)
                {
                    const char *indAttr=step->Attribute("industry");
                    if(indAttr)
                    {
                        BotLocation loc;
                        loc.mainCode=mainCode;
                        loc.mapStem=stem;
                        loc.mapDisplayName=mapName;
                        loc.zoneCode=zoneCode;
                        loc.zoneName=zoneName;
                        loc.botName=botName;
                        loc.botLocalId=botId;
                        loc.skinName=skinName;
                        result[indAttr].push_back(std::move(loc));
                    }
                }
                step=step->NextSiblingElement("step");
            }
            botEl=botEl->NextSiblingElement("bot");
        }
    }
    return result;
}

std::string relativePathForIndustry(const std::string &mainCode, const std::string &id)
{
    return "industries/"+mainCode+"/"+id+".html";
}

// ── Format time duration (matches PHP) ───────────────────────────────

static std::string formatTime(uint64_t seconds)
{
    if(seconds<(60*2))
        return std::to_string(seconds)+"s";
    else if(seconds<(60*60*2))
        return std::to_string(seconds/60)+"mins";
    else if(seconds<(60*60*24*2))
        return std::to_string(seconds/(60*60))+"hours";
    else
        return std::to_string(seconds/(60*60*24))+"days";
}

// ── Generate ─────────────────────────────────────────────────────────

void generate()
{
    const auto &sets=MapStore::sets();

    // Collect all industries per mainCode
    // industryMap[mainCode][id] = IndustryData
    std::map<std::string,std::map<std::string,IndustryData>> allIndustries;
    // industryBots[mainCode][id] = vector<BotLocation>
    std::map<std::string,std::map<std::string,std::vector<BotLocation>>> allBots;

    for(size_t si=0; si<sets.size(); ++si)
    {
        const std::string &mc=sets[si].mainCode;
        auto recipes=parseIndustrialRecipe(mc);
        if(!recipes.empty())
        {
            allIndustries[mc]=std::move(recipes);
            allBots[mc]=buildIndustryToBotMap(mc,si);
        }
    }

    if(allIndustries.empty())
        return;

    // ── Individual industry pages ────────────────────────────────────

    for(const auto &mcPair : allIndustries)
    {
        const std::string &mc=mcPair.first;
        Helper::mkpath(Helper::localPath()+"industries/"+mc+"/");

        for(const auto &idPair : mcPair.second)
        {
            const std::string &id=idPair.first;
            const IndustryData &ind=idPair.second;
            std::string rel=relativePathForIndustry(mc,id);

            std::ostringstream body;
            body << "<div class=\"map item_details\">\n";
            body << "<div class=\"subblock\"><h1>Industry " << id << "</h1>\n</div>\n";

            // Time
            body << "<div class=\"subblock\"><div class=\"valuetitle\">Time to complet a cycle</div><div class=\"value\">\n"
                 << formatTime(ind.time)
                 << "</div></div>\n";

            // Cycle to be full
            body << "<div class=\"subblock\"><div class=\"valuetitle\">Cycle to be full</div><div class=\"value\">"
                 << ind.cycletobefull << "</div></div>\n";

            // Resources
            body << "<div class=\"subblock\"><div class=\"valuetitle\">Resources</div><div class=\"value\">\n";
            for(const auto &res : ind.resources)
            {
                const auto &extras=QtDatapackClientLoader::datapackLoader->get_itemsExtra();
                auto it=extras.find(res.item);
                if(it!=extras.cend())
                {
                    std::string name=it->second.name;
                    std::string link=Helper::relUrl(GeneratorItems::relativePathForItem(res.item));
                    std::string image=itemImageUrl(res.item);

                    body << "<a href=\"" << Helper::htmlEscape(link) << "\" title=\"" << Helper::htmlEscape(name) << "\">\n";
                    body << "<table><tr><td>\n";
                    if(!image.empty() && Helper::fileExists(Helper::datapackPath()+Helper::relativeFromDatapack(it->second.imagePath)))
                        body << "<img src=\"" << image << "\" width=\"24\" height=\"24\" alt=\""
                             << Helper::htmlEscape(name) << "\" title=\"" << Helper::htmlEscape(name) << "\" />\n";
                    body << "</td><td>\n";
                    if(res.quantity>1)
                        body << res.quantity << "x ";
                    body << name << "</td></tr></table>\n";
                    body << "</a>\n";
                }
                else
                    body << "Unknown material: " << res.item;
            }
            body << "</div></div>\n";

            // Location (bot links)
            if(allBots.count(mc) && allBots[mc].count(id))
            {
                const auto &bots=allBots[mc][id];
                body << "<div class=\"subblock\"><div class=\"valuetitle\">Location</div><div class=\"value\">\n";
                body << "<table class=\"item_list item_list_type_normal map_list\">\n"
                     << "\t\t\t\t<tr class=\"item_list_title item_list_title_type_normal\">\n"
                     << "\t\t\t\t\t<th colspan=\"2\">Bot</th>\n"
                     << "\t\t\t\t\t<th colspan=\"2\">Map</th>\n"
                     << "\t\t\t\t\t</tr>\n";

                for(const auto &loc : bots)
                {
                    body << "<tr class=\"value\">\n";
                    std::string finalName=loc.botName.empty()?("Bot #"+std::to_string(loc.botLocalId)):loc.botName;

                    // Skin column
                    bool skinFound=false;
                    if(!loc.skinName.empty())
                    {
                        std::string trainerUrl=skinTrainerUrl(loc.skinName);
                        if(!trainerUrl.empty())
                        {
                            skinFound=true;
                            body << "<td><center><div style=\"width:16px;height:24px;background-image:url('"
                                 << Helper::relUrl(trainerUrl)
                                 << "');background-repeat:no-repeat;background-position:-16px -48px;\"></div></center></td>\n";
                        }
                    }

                    // Bot name column
                    std::string botPageRel="map/"+loc.mainCode+"/"+loc.mapStem+"/bots/bot-"+std::to_string(loc.botLocalId)+".html";
                    body << "<td";
                    if(!skinFound) body << " colspan=\"2\"";
                    body << "><a href=\"" << Helper::htmlEscape(Helper::relUrl(botPageRel))
                         << "\" title=\"" << Helper::htmlEscape(finalName) << "\">"
                         << Helper::htmlEscape(finalName) << "</a></td>\n";

                    // Map + zone columns
                    std::string mapPageRel="map/"+loc.mainCode+"/"+loc.mapStem+".html";
                    if(!loc.zoneName.empty())
                    {
                        body << "<td><a href=\"" << Helper::htmlEscape(Helper::relUrl(mapPageRel))
                             << "\" title=\"" << Helper::htmlEscape(loc.mapDisplayName) << "\">"
                             << Helper::htmlEscape(loc.mapDisplayName) << "</a></td>\n";
                        body << "<td>" << Helper::htmlEscape(loc.zoneName) << "</td>\n";
                    }
                    else
                    {
                        body << "<td colspan=\"2\"><a href=\"" << Helper::htmlEscape(Helper::relUrl(mapPageRel))
                             << "\" title=\"" << Helper::htmlEscape(loc.mapDisplayName) << "\">"
                             << Helper::htmlEscape(loc.mapDisplayName) << "</a></td>\n";
                    }
                    body << "</tr>\n";
                }
                body << "<tr>\n\t<td colspan=\"4\" class=\"item_list_endline item_list_title_type_normal\"></td>\n</tr>\n"
                     << "</table><br style=\"clear:both;\" />\n";
                body << "</div></div>\n";
            }

            // Products
            body << "<div class=\"subblock\"><div class=\"valuetitle\">Products</div><div class=\"value\">\n";
            for(const auto &prod : ind.products)
            {
                const auto &extras=QtDatapackClientLoader::datapackLoader->get_itemsExtra();
                auto it=extras.find(prod.item);
                if(it!=extras.cend())
                {
                    std::string name=it->second.name;
                    std::string link=Helper::relUrl(GeneratorItems::relativePathForItem(prod.item));
                    std::string image=itemImageUrl(prod.item);

                    body << "<a href=\"" << Helper::htmlEscape(link) << "\" title=\"" << Helper::htmlEscape(name) << "\">\n";
                    if(!image.empty() && Helper::fileExists(Helper::datapackPath()+Helper::relativeFromDatapack(it->second.imagePath)))
                        body << "<img src=\"" << image << "\" width=\"24\" height=\"24\" alt=\""
                             << Helper::htmlEscape(name) << "\" title=\"" << Helper::htmlEscape(name) << "\" />\n";
                    body << "</td><td>\n";
                    if(prod.quantity>1)
                        body << prod.quantity << "x ";
                    body << name << "</td></tr></table>\n";
                    body << "</a>\n";
                }
                else
                    body << "Unknown material: " << prod.item;
            }
            body << "</div></div>\n";
            body << "</div>\n";

            Helper::writeHtml(rel,"Industry #"+id,body.str());
        }
    }

    // ── Industries index page ────────────────────────────────────────

    std::ostringstream idx;
    idx << "<table class=\"item_list item_list_type_normal\">\n"
        << "<tr class=\"item_list_title item_list_title_type_normal\">\n"
        << "\t<th>Industry</th>\n"
        << "\t<th>Resources</th>\n"
        << "\t<th>Products</th>\n"
        << "\t<th>Location</th>\n"
        << "</tr>\n";

    Helper::setCurrentPage("industries.html");

    for(const auto &mcPair : allIndustries)
    {
        const std::string &mc=mcPair.first;
        for(const auto &idPair : mcPair.second)
        {
            const std::string &id=idPair.first;
            const IndustryData &ind=idPair.second;

            idx << "<tr class=\"value\">\n";
            // Industry ID link
            idx << "<td>\n<a href=\"" << Helper::htmlEscape(Helper::relUrl(relativePathForIndustry(mc,id)))
                << "\">#" << id << "</a>\n</td>\n";

            // Resources
            idx << "<td><center>\n";
            for(const auto &res : ind.resources)
                writeItemIconAndNameDiv(idx,res.item,"middle");
            idx << "</center></td>\n";

            // Products
            idx << "<td><center>\n";
            for(const auto &prod : ind.products)
                writeItemIconAndNameDiv(idx,prod.item,"middle");
            idx << "</center></td><td>\n";

            // Location
            if(allBots.count(mc) && allBots[mc].count(id))
            {
                const auto &bots=allBots[mc][id];
                idx << "<table class=\"item_list item_list_type_normal map_list\">\n";
                for(const auto &loc : bots)
                {
                    idx << "<tr class=\"value\">\n";
                    std::string finalName=loc.botName.empty()?("Bot #"+std::to_string(loc.botLocalId)):loc.botName;

                    bool skinFound=false;
                    if(!loc.skinName.empty())
                    {
                        std::string trainerUrl=skinTrainerUrl(loc.skinName);
                        if(!trainerUrl.empty())
                        {
                            skinFound=true;
                            idx << "<td><center><div style=\"width:16px;height:24px;background-image:url('"
                                << Helper::relUrl(trainerUrl)
                                << "');background-repeat:no-repeat;background-position:-16px -48px;\"></div></center></td>\n";
                        }
                    }

                    std::string botPageRel="map/"+loc.mainCode+"/"+loc.mapStem+"/bots/bot-"+std::to_string(loc.botLocalId)+".html";
                    idx << "<td";
                    if(!skinFound) idx << " colspan=\"2\"";
                    idx << "><a href=\"" << Helper::htmlEscape(Helper::relUrl(botPageRel))
                        << "\" title=\"" << Helper::htmlEscape(finalName) << "\">"
                        << Helper::htmlEscape(finalName) << "</a></td>\n";

                    std::string mapPageRel="map/"+loc.mainCode+"/"+loc.mapStem+".html";
                    if(!loc.zoneName.empty())
                    {
                        idx << "<td><a href=\"" << Helper::htmlEscape(Helper::relUrl(mapPageRel))
                            << "\" title=\"" << Helper::htmlEscape(loc.mapDisplayName) << "\">"
                            << Helper::htmlEscape(loc.mapDisplayName) << "</a></td>\n";
                        idx << "<td>" << Helper::htmlEscape(loc.zoneName) << "</td>\n";
                    }
                    else
                    {
                        idx << "<td colspan=\"2\"><a href=\"" << Helper::htmlEscape(Helper::relUrl(mapPageRel))
                            << "\" title=\"" << Helper::htmlEscape(loc.mapDisplayName) << "\">"
                            << Helper::htmlEscape(loc.mapDisplayName) << "</a></td>\n";
                    }
                    idx << "</tr>\n";
                }
                idx << "</table>\n";
            }
            idx << "</td>\n</tr>\n";
        }
    }
    idx << "<tr>\n\t<td colspan=\"4\" class=\"item_list_endline item_list_title_type_normal\"></td>\n</tr>\n"
        << "</table>\n";

    Helper::writeHtml("industries.html","Industries list",idx.str());
}

} // namespace GeneratorIndustries
