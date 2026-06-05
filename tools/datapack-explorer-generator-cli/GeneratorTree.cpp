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
#include <cstdlib>
#include <cstring>
#include <dirent.h>

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

// Wild-encounter layer tags that hold "monster on map" lists.
static const char *const s_wildLayers[]={"grass","water","cave","lava","waterRod","waterSuperRod",nullptr};

// get_maps() returns map paths WITHOUT extension (e.g. "kanto/pallet/pallet"),
// but some callers pass ".tmx" paths — drop a trailing ".tmx" so we never chop
// real characters when appending ".xml".
static std::string dropTmx(const std::string &rel)
{
    if(rel.size()>=4 && rel.compare(rel.size()-4,4,".tmx")==0)
        return rel.substr(0,rel.size()-4);
    return rel;
}

// Resolve a <monster id="..."> attribute to a numeric monster id.  The datapack
// references a monster by NUMERIC id or by NAME (case-insensitive); gba2cc-built
// datapacks use names, so a plain atoi() would silently drop every entry.
static uint16_t resolveMonsterId(const char *idAttr)
{
    if(idAttr==nullptr || idAttr[0]=='\0')
        return 0;
    bool numeric=true;
    const char *p=idAttr;
    while(*p!='\0')
    {
        if(*p<'0' || *p>'9')
        {
            numeric=false;
            break;
        }
        p++;
    }
    if(numeric)
        return (uint16_t)std::atoi(idAttr);
    std::string low(idAttr);
    size_t i=0;
    while(i<low.size())
    {
        if(low[i]>='A' && low[i]<='Z')
            low[i]=(char)(low[i]-'A'+'a');
        i++;
    }
    if(CatchChallenger::CommonDatapack::commonDatapack.has_tempNameToMonsterId(low))
        return CatchChallenger::CommonDatapack::commonDatapack.get_tempNameToMonsterId(low);
    return 0;
}

// Read a map xml's wild layers into `layers` (layer tag -> set of monster ids).
// A layer PRESENT in this file REPLACES the same layer already in `layers` — this
// is exactly the sub-overlay semantics (a sub's <grass> overrides the main's
// <grass>, sections it omits inherit from the main).  Missing file = no change.
static void readWildLayers(const std::string &xmlPath,
                           std::map<std::string,std::set<uint16_t>> &layers)
{
    tinyxml2::XMLDocument doc;
    if(doc.LoadFile(xmlPath.c_str())!=tinyxml2::XML_SUCCESS)
        return;
    if(doc.RootElement()==nullptr)
        return;
    int t=0;
    while(s_wildLayers[t]!=nullptr)
    {
        const tinyxml2::XMLElement *el=doc.RootElement()->FirstChildElement(s_wildLayers[t]);
        if(el!=nullptr)
        {
            std::set<uint16_t> ids;
            while(el!=nullptr)
            {
                const tinyxml2::XMLElement *mon=el->FirstChildElement("monster");
                while(mon!=nullptr)
                {
                    uint16_t mid=resolveMonsterId(mon->Attribute("id"));
                    if(mid>0)
                        ids.insert(mid);
                    mon=mon->NextSiblingElement("monster");
                }
                el=el->NextSiblingElement(s_wildLayers[t]);
            }
            layers[s_wildLayers[t]]=ids; // override this layer
        }
        t++;
    }
}

// Sub-datapack codes under map/main/<mainCode>/sub/ (sorted, "" base excluded).
static std::vector<std::string> listSubCodes(const std::string &mainCode)
{
    std::vector<std::string> subs;
    const std::string subRoot=Helper::datapackPath()+"map/main/"+mainCode+"/sub/";
    DIR *d=::opendir(subRoot.c_str());
    if(d!=nullptr)
    {
        struct dirent *ent;
        while((ent=::readdir(d))!=nullptr)
        {
            std::string n=ent->d_name;
            if(n=="." || n=="..")
                continue;
            if(Helper::isDir(subRoot+n))
                subs.push_back(n);
        }
        ::closedir(d);
    }
    std::sort(subs.begin(),subs.end());
    return subs;
}

// ── Compute exclusive monsters per mainCode ──────────────────────────
// A monster is "exclusive" to a mainCode if all maps where it appears
// as a wild monster are within that single mainCode.  (Base maps only; the
// per-sub version-exclusives are handled by computeSubExclusiveMonsters.)

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
            std::string xmlPath=Helper::datapackPath()+"map/main/"+mc+"/"+dropTmx(sets[si].mapPaths[mi])+".xml";
            std::map<std::string,std::set<uint16_t>> layers;
            readWildLayers(xmlPath,layers);
            for(const std::pair<const std::string,std::set<uint16_t>> &lp : layers)
                for(uint16_t mid : lp.second)
                    monsterToMainCodes[mid].insert(mc);
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

// ── Compute version-exclusive monsters per sub overlay ───────────────
// A sub (e.g. ruby/sub/sapphire) overlays per-map wild encounters on its main.
// Its "exclusive monsters" are the species that appear once the sub overlay is
// applied but NOT anywhere in the main's base wild data — i.e. the monster
// CHANGES the sub introduces (the version exclusives).  Returns mainCode ->
// subCode -> sorted ids.
static std::map<std::string,std::map<std::string,std::vector<uint16_t>>>
computeSubExclusiveMonsters()
{
    std::map<std::string,std::map<std::string,std::vector<uint16_t>>> result;
    const std::vector<MapStore::MainCodeSet> &sets=MapStore::sets();
    for(size_t si=0; si<sets.size(); ++si)
    {
        const std::string &mc=sets[si].mainCode;
        const std::vector<std::string> subCodes=listSubCodes(mc);
        if(subCodes.empty())
            continue;
        const std::vector<std::string> &mapPaths=sets[si].mapPaths;
        // Cache each map's base wild layers once, and the base monster union.
        std::vector<std::map<std::string,std::set<uint16_t>>> mainLayers(mapPaths.size());
        std::set<uint16_t> baseSet;
        size_t mi=0;
        while(mi<mapPaths.size())
        {
            std::string base=Helper::datapackPath()+"map/main/"+mc+"/"+dropTmx(mapPaths[mi])+".xml";
            readWildLayers(base,mainLayers[mi]);
            for(const std::pair<const std::string,std::set<uint16_t>> &lp : mainLayers[mi])
                for(uint16_t mid : lp.second)
                    baseSet.insert(mid);
            mi++;
        }
        size_t sc=0;
        while(sc<subCodes.size())
        {
            const std::string &sub=subCodes[sc];
            std::set<uint16_t> subSet;
            mi=0;
            while(mi<mapPaths.size())
            {
                // Start from the main's layers, then apply the sub overlay file.
                std::map<std::string,std::set<uint16_t>> layers=mainLayers[mi];
                std::string subXml=Helper::datapackPath()+"map/main/"+mc+"/sub/"+sub+"/"+dropTmx(mapPaths[mi])+".xml";
                readWildLayers(subXml,layers);
                for(const std::pair<const std::string,std::set<uint16_t>> &lp : layers)
                    for(uint16_t mid : lp.second)
                        subSet.insert(mid);
                mi++;
            }
            std::vector<uint16_t> excl;
            for(uint16_t mid : subSet)
                if(baseSet.find(mid)==baseSet.cend())
                    excl.push_back(mid);
            std::sort(excl.begin(),excl.end());
            if(!excl.empty())
                result[mc][sub]=excl;
            sc++;
        }
    }
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

    // Compute exclusive monsters (per main, and the version-exclusives per sub)
    std::map<std::string, std::vector<uint16_t>> exclusiveMonsters=computeExclusiveMonsters();
    std::map<std::string, std::map<std::string,std::vector<uint16_t>>> subExclusiveMonsters=computeSubExclusiveMonsters();

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

        // Sub-datapacks (if any): each sub's own heading + the version-exclusive
        // monsters it introduces (its per-map wild overlay vs this main's base).
        std::vector<std::string> subCodes=listSubCodes(mc);
        if(!subCodes.empty())
        {
            body << "<div class=\"subblock\"><div class=\"valuetitle\">Sub part(s)</div><div class=\"value\">\n";
            for(const std::string &sub : subCodes)
            {
                InfoData subInfo=parseInformations(Helper::datapackPath()+"map/main/"+mc+"/sub/"+sub+"/informations.xml");
                body << "<div class=\"map map_type_city\">\n";
                body << "<div class=\"subblock\"><h1";
                if(subInfo.initial.empty() && !subInfo.color.empty())
                    body << " style=\"color:" << subInfo.color << "\"";
                body << ">\n";
                body << Helper::htmlEscape(subInfo.name.empty()?sub:subInfo.name);
                if(!subInfo.initial.empty())
                {
                    if(!subInfo.color.empty())
                        body << "&nbsp;<span style=\"background-color:" << subInfo.color << ";\" class=\"datapackinital\">"
                             << subInfo.initial << "</span>\n";
                    else
                        body << "&nbsp;<span class=\"datapackinital\">" << subInfo.initial << "</span>\n";
                }
                body << "</h1></div>\n";
                if(!subInfo.description.empty())
                    body << "<div class=\"type_label_list\">" << Helper::htmlEscape(Helper::firstLetterUpper(subInfo.description)) << "</div>\n";
                if(subExclusiveMonsters.count(mc) && subExclusiveMonsters[mc].count(sub))
                    writeExclusiveMonsters(body,subExclusiveMonsters[mc][sub]);
                body << "</div>\n";
            }
            body << "</div></div>\n";
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
