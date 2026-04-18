#include "GeneratorTree.hpp"
#include "GeneratorMaps.hpp"
#include "GeneratorMonsters.hpp"
#include "Helper.hpp"
#include "MapStore.hpp"

#include "../../client/libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include "../../general/tinyXML2/tinyxml2.hpp"

#include <sstream>
#include <map>
#include <set>
#include <vector>
#include <algorithm>

namespace GeneratorTree {

// ── Data structures ──────────────────────────────────────────────────

struct InfoData
{
    std::string name;
    std::string description;
    std::string initial;
    std::string color;
};

// ── Parse informations.xml ───────────────────────────────────────────

static InfoData parseInformations(const std::string &path)
{
    InfoData info;
    tinyxml2::XMLDocument doc;
    if(doc.LoadFile(path.c_str())!=tinyxml2::XML_SUCCESS)
        return info;

    const tinyxml2::XMLElement *root=doc.RootElement();
    if(!root) return info;

    // Color attribute on root
    const char *colorAttr=root->Attribute("color");
    if(colorAttr) info.color=colorAttr;

    // Name (default = no lang attribute, or lang="en")
    const tinyxml2::XMLElement *el=root->FirstChildElement("name");
    while(el)
    {
        const char *lang=el->Attribute("lang");
        if(!lang || strcmp(lang,"en")==0)
        {
            if(el->GetText())
                info.name=el->GetText();
            if(!lang) break; // prefer no-lang over lang="en"
        }
        el=el->NextSiblingElement("name");
    }

    // Description
    el=root->FirstChildElement("description");
    while(el)
    {
        const char *lang=el->Attribute("lang");
        if(!lang || strcmp(lang,"en")==0)
        {
            if(el->GetText())
                info.description=el->GetText();
            if(!lang) break;
        }
        el=el->NextSiblingElement("description");
    }

    // Initial
    el=root->FirstChildElement("initial");
    if(el && el->GetText())
        info.initial=el->GetText();

    return info;
}

// ── Parse start.xml for start maps ───────────────────────────────────

static std::vector<std::string> parseStartMaps(const std::string &path)
{
    std::vector<std::string> maps;
    tinyxml2::XMLDocument doc;
    if(doc.LoadFile(path.c_str())!=tinyxml2::XML_SUCCESS)
        return maps;

    const tinyxml2::XMLElement *root=doc.RootElement();
    if(!root) return maps;

    const tinyxml2::XMLElement *start=root->FirstChildElement("start");
    while(start)
    {
        const tinyxml2::XMLElement *mapEl=start->FirstChildElement("map");
        while(mapEl)
        {
            const char *file=mapEl->Attribute("file");
            if(file)
            {
                std::string f=file;
                if(f.size()<4 || f.compare(f.size()-4,4,".tmx")!=0)
                    f+=".tmx";
                maps.push_back(f);
            }
            mapEl=mapEl->NextSiblingElement("map");
        }
        start=start->NextSiblingElement("start");
    }
    return maps;
}

// ── Compute exclusive monsters per mainCode ──────────────────────────
// A monster is "exclusive" to a mainCode if all maps where it appears
// as a wild monster are within that single mainCode.

static std::map<std::string,std::vector<uint16_t>>
computeExclusiveMonsters()
{
    // monsterToMainCodes[monsterId] = set of mainCodes where it appears wild
    std::map<uint16_t,std::set<std::string>> monsterToMainCodes;

    const std::vector<MapStore::MainCodeSet> &sets=MapStore::sets();
    for(size_t si=0; si<sets.size(); ++si)
    {
        const std::string &mc=sets[si].mainCode;
        for(size_t mi=0; mi<sets[si].mapPaths.size(); ++mi)
        {
            // Parse map XML for wild monsters
            std::string xmlPath=Helper::datapackPath()+"map/main/"+mc+"/"+sets[si].mapPaths[mi];
            xmlPath=xmlPath.substr(0,xmlPath.size()-4)+".xml";

            tinyxml2::XMLDocument doc;
            if(doc.LoadFile(xmlPath.c_str())!=tinyxml2::XML_SUCCESS)
                continue;
            if(!doc.RootElement()) continue;

            static const char *layers[]={"grass","water","cave","lava","waterRod","waterSuperRod"};
            for(const char *layer : layers)
            {
                const tinyxml2::XMLElement *el=doc.RootElement()->FirstChildElement(layer);
                while(el)
                {
                    const tinyxml2::XMLElement *mon=el->FirstChildElement("monster");
                    while(mon)
                    {
                        const char *idAttr=mon->Attribute("id");
                        if(idAttr)
                        {
                            uint16_t mid=(uint16_t)std::atoi(idAttr);
                            if(mid>0)
                                monsterToMainCodes[mid].insert(mc);
                        }
                        mon=mon->NextSiblingElement("monster");
                    }
                    el=el->NextSiblingElement(layer);
                }
            }
        }
    }

    // Collect monsters exclusive to a single mainCode
    std::map<std::string,std::vector<uint16_t>> result;
    for(const std::pair<const uint16_t, std::set<std::string>> &p : monsterToMainCodes)
    {
        if(p.second.size()==1)
            result[*p.second.begin()].push_back(p.first);
    }
    // Sort monster IDs
    for(std::pair<const std::string, std::vector<uint16_t>> &p : result)
        std::sort(p.second.begin(),p.second.end());
    return result;
}

// ── Write exclusive monsters table ───────────────────────────────────

static void writeExclusiveMonsters(std::ostringstream &body,
                                   const std::vector<uint16_t> &monsters)
{
    if(monsters.empty()) return;

    const std::vector<CatchChallenger::Type> &types=CatchChallenger::CommonDatapack::commonDatapack.get_types();

    body << "<div class=\"subblock\"><div class=\"valuetitle\">Exclusive monsters</div><div class=\"value\">\n";
    body << "<br style=\"clear:both\" />";
    body << "<table class=\"item_list item_list_type_normal monster_list\">\n"
         << "<tr class=\"item_list_title item_list_title_type_normal\">\n"
         << "\t<th colspan=\"3\">Monster</th>\n"
         << "</tr>\n";

    int count=0;
    for(uint16_t id : monsters)
    {
        count++;
        if(count>20)
        {
            body << "<tr>\n\t<td colspan=\"3\" class=\"item_list_endline item_list_title_type_normal\"></td>\n</tr>\n"
                 << "</table>\n";
            body << "<table class=\"item_list item_list_type_normal monster_list\">\n"
                 << "<tr class=\"item_list_title item_list_title_type_normal\">\n"
                 << "\t<th colspan=\"3\">Monster</th>\n"
                 << "</tr>\n";
            count=1;
        }

        std::string name;
        if(QtDatapackClientLoader::datapackLoader->has_monsterExtra(id))
            name=QtDatapackClientLoader::datapackLoader->get_monsterExtra(id).name;
        else
            name="Monster #"+std::to_string(id);
        std::string link=Helper::relUrl(GeneratorMonsters::relativePathForMonster(id));

        body << "<tr class=\"value\">\n<td>\n";
        // Small icon
        std::string smallRel="monsters/"+std::to_string(id)+"/small.png";
        if(Helper::fileExists(Helper::datapackPath()+smallRel))
        {
            std::string pub=Helper::publishDatapackFile(smallRel);
            body << "<div class=\"monstericon\"><a href=\"" << Helper::htmlEscape(link)
                 << "\"><img src=\"" << Helper::relUrl(pub) << "\" width=\"32\" height=\"32\" alt=\""
                 << Helper::htmlEscape(name) << "\" title=\"" << Helper::htmlEscape(name) << "\" /></a></div>\n";
        }
        body << "</td>\n";

        body << "<td><a href=\"" << Helper::htmlEscape(link) << "\">" << Helper::htmlEscape(name) << "</a>";
        body << "</td>\n";

        // Types
        body << "<td>\n";
        if(CatchChallenger::CommonDatapack::commonDatapack.has_monster(id))
        {
            const CatchChallenger::Monster &monDef=CatchChallenger::CommonDatapack::commonDatapack.get_monster(id);
            body << "<div class=\"type_label_list\">";
            bool first=true;
            for(uint8_t t : monDef.type)
            {
                if(!first) body << " ";
                first=false;
                std::string typeName;
                if(t<types.size())
                    typeName=Helper::firstLetterUpper(types[t].name);
                else
                    typeName="Type "+std::to_string(t);
                body << "<span class=\"type_label type_label_" << (int)t << "\">"
                     << "<a href=\"" << Helper::htmlEscape(Helper::relUrl("monsters/type-"+std::to_string(t)+".html"))
                     << "\">" << Helper::htmlEscape(typeName) << "</a></span>\n";
            }
            body << "</div>\n";
        }
        body << "</td>\n</tr>\n";
    }

    body << "<tr>\n\t<td colspan=\"3\" class=\"item_list_endline item_list_title_type_normal\"></td>\n</tr>\n"
         << "</table>\n";
    body << "<br style=\"clear:both;\" /></div></div>\n";
}

// ── Write start maps table ───────────────────────────────────────────

static void writeStartMaps(std::ostringstream &body,
                           const std::string &mainCode,
                           const std::vector<std::string> &startMaps)
{
    if(startMaps.empty()) return;

    body << "<div class=\"subblock\"><div class=\"value\">\n";
    body << "<table class=\"item_list item_list_type_normal\">\n"
         << "<tr class=\"item_list_title item_list_title_type_normal\">\n"
         << "\t<th colspan=\"1\">Start map</th>\n"
         << "</tr>\n";

    for(const std::string &mapFile : startMaps)
    {
        std::string stem=mapFile;
        if(stem.size()>=4 && stem.compare(stem.size()-4,4,".tmx")==0)
            stem=stem.substr(0,stem.size()-4);

        // Parse map XML for display name
        std::string mapDisplayName;
        std::string xmlPath=Helper::datapackPath()+"map/main/"+mainCode+"/"+stem+".xml";
        {
            tinyxml2::XMLDocument doc;
            if(doc.LoadFile(xmlPath.c_str())==tinyxml2::XML_SUCCESS && doc.RootElement())
            {
                const tinyxml2::XMLElement *n=doc.RootElement()->FirstChildElement("name");
                if(n && n->GetText())
                    mapDisplayName=n->GetText();
            }
        }
        if(mapDisplayName.empty())
        {
            // Use last path component with first letter upper
            size_t pos=stem.rfind('/');
            mapDisplayName=(pos!=std::string::npos)?stem.substr(pos+1):stem;
            if(!mapDisplayName.empty())
                mapDisplayName[0]=(char)toupper((unsigned char)mapDisplayName[0]);
        }
        std::string mapHtml=mainCode+"/"+stem+".html";

        body << "<tr class=\"value\"><td><a href=\""
             << Helper::htmlEscape(Helper::relUrl("map/"+mapHtml))
             << "\" title=\"" << Helper::htmlEscape(mapDisplayName) << "\">"
             << Helper::htmlEscape(mapDisplayName) << "</a></td></tr>\n";
    }

    body << "<tr>\n\t<td colspan=\"1\" class=\"item_list_endline item_list_title_type_normal\"></td>\n</tr>\n"
         << "</table>\n";
    body << "</div></div>\n";
}

// ── Generate ─────────────────────────────────────────────────────────

void generate()
{
    // Parse root informations
    InfoData rootInfo=parseInformations(Helper::datapackPath()+"informations.xml");

    // Compute exclusive monsters
    std::map<std::string, std::vector<uint16_t>> exclusiveMonsters=computeExclusiveMonsters();

    std::ostringstream body;
    body << "<div class=\"map map_type_city\">\n";

    // Root name
    body << "<div class=\"subblock\"><h1>\n";
    if(!rootInfo.name.empty())
        body << Helper::htmlEscape(rootInfo.name);
    else
        body << "Informations";
    body << "</h1></div>\n";

    // Root description
    if(!rootInfo.description.empty())
        body << "<div class=\"type_label_list\">" << Helper::htmlEscape(Helper::firstLetterUpper(rootInfo.description)) << "</div>\n";

    // For each mainDatapackCode
    const std::vector<MapStore::MainCodeSet> &sets=MapStore::sets();
    for(size_t si=0; si<sets.size(); ++si)
    {
        const std::string &mc=sets[si].mainCode;
        InfoData mainInfo=parseInformations(Helper::datapackPath()+"map/main/"+mc+"/informations.xml");

        body << "<div class=\"map map_type_city\">\n";
        body << "<div class=\"subblock\"><h1";
        if(mainInfo.initial.empty() && !mainInfo.color.empty())
            body << " style=\"color:" << mainInfo.color << "\"";
        body << ">\n";
        if(!mainInfo.name.empty())
            body << Helper::htmlEscape(mainInfo.name);
        else
            body << "Informations";
        if(!mainInfo.initial.empty())
        {
            if(!mainInfo.color.empty())
                body << "&nbsp;<span style=\"background-color:" << mainInfo.color << ";\" class=\"datapackinital\">"
                     << mainInfo.initial << "</span>\n";
            else
                body << "&nbsp;<span class=\"datapackinital\">" << mainInfo.initial << "</span>\n";
        }
        body << "</h1></div>\n";

        if(!mainInfo.description.empty())
            body << "<div class=\"type_label_list\">" << Helper::htmlEscape(Helper::firstLetterUpper(mainInfo.description)) << "</div>\n";

        // Start maps
        std::string startPath=Helper::datapackPath()+"map/main/"+mc+"/start.xml";
        std::vector<std::string> startMaps=parseStartMaps(startPath);
        writeStartMaps(body,mc,startMaps);

        // Exclusive monsters
        if(exclusiveMonsters.count(mc))
            writeExclusiveMonsters(body,exclusiveMonsters[mc]);

        // Sub-datapacks (if any)
        std::string subDir=Helper::datapackPath()+"map/main/"+mc+"/sub/";
        if(Helper::isDir(subDir))
        {
            // List sub directories
            std::vector<std::string> subs;
            // Use getXmlList approach - list directories manually
            // For now, just check for informations.xml in potential sub dirs
            // This is a simplified approach; full implementation would enumerate dirs
        }

        body << "</div>\n";
    }

    body << "</div>\n";

    std::string title;
    if(!rootInfo.name.empty())
        title=rootInfo.name;
    else
        title="Informations";

    Helper::writeHtml("tree.html",title,body.str());
}

} // namespace GeneratorTree
