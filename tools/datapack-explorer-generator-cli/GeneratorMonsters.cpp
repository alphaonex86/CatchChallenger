#include "GeneratorMonsters.hpp"
#include "GeneratorItems.hpp"
#include "GeneratorSkills.hpp"
#include "GeneratorMaps.hpp"
#include "GeneratorTypes.hpp"
#include "Helper.hpp"
#include "MapStore.hpp"

#include "../../client/libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include "../../general/base/CommonDatapackServerSpec.hpp"
#include "../../general/base/CommonMap/CommonMap.hpp"

#include "../../general/tinyXML2/tinyxml2.hpp"

#include <sstream>
#include <set>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <cstdlib>
#include <algorithm>

namespace GeneratorMonsters {

std::string relativePathForMonster(uint16_t id)
{
    std::string stem;
    if(QtDatapackClientLoader::datapackLoader->has_monsterExtra(id) && !QtDatapackClientLoader::datapackLoader->get_monsterExtra(id).name.empty())
        stem=Helper::textForUrl(QtDatapackClientLoader::datapackLoader->get_monsterExtra(id).name);
    if(stem.empty())
        stem=Helper::toStringUint(id);
    return "monsters/"+stem+".html";
}

static std::string frontImageAbsPath(uint16_t id)
{
    if(QtDatapackClientLoader::datapackLoader->has_monsterExtra(id) && !QtDatapackClientLoader::datapackLoader->get_monsterExtra(id).frontPath.empty())
        return QtDatapackClientLoader::datapackLoader->get_monsterExtra(id).frontPath;
    const std::string base=Helper::datapackPath()+"monsters/"+Helper::toStringUint(id)+"/";
    if(Helper::fileExists(base+"front.png"))
        return base+"front.png";
    if(Helper::fileExists(base+"front.gif"))
        return base+"front.gif";
    return std::string();
}

static std::string frontImageUrlFrom(const std::string &fromPage, uint16_t id)
{
    std::string abs=frontImageAbsPath(id);
    if(abs.empty())
        return std::string();
    std::string rel=Helper::relativeFromDatapack(abs);
    std::string published=Helper::publishDatapackFile(rel);
    return Helper::relUrlFrom(fromPage,published);
}

static std::string smallImageAbsPath(uint16_t id)
{
    const std::string base=Helper::datapackPath()+"monsters/"+Helper::toStringUint(id)+"/";
    if(Helper::fileExists(base+"small.png"))
        return base+"small.png";
    if(Helper::fileExists(base+"small.gif"))
        return base+"small.gif";
    return std::string();
}

static std::string smallImageUrlFrom(const std::string &fromPage, uint16_t id)
{
    std::string abs=smallImageAbsPath(id);
    if(abs.empty())
        return std::string();
    std::string rel=Helper::relativeFromDatapack(abs);
    std::string published=Helper::publishDatapackFile(rel);
    return Helper::relUrlFrom(fromPage,published);
}

static std::string typeName(uint8_t typeId)
{
    if(QtDatapackClientLoader::datapackLoader->has_typeExtra(typeId) && !QtDatapackClientLoader::datapackLoader->get_typeExtra(typeId).name.empty())
        return QtDatapackClientLoader::datapackLoader->get_typeExtra(typeId).name;
    const auto &types=CatchChallenger::CommonDatapack::commonDatapack.get_types();
    if(typeId<types.size() && !types[typeId].name.empty())
        return types[typeId].name;
    return "#"+Helper::toStringUint(typeId);
}

static std::string typeNameLower(uint8_t typeId)
{
    std::string n=typeName(typeId);
    std::string lower;
    lower.reserve(n.size());
    for(char c : n)
        lower.push_back(std::tolower(static_cast<unsigned char>(c)));
    return lower;
}

static std::string ucfirst(const std::string &s)
{
    if(s.empty()) return s;
    std::string r=s;
    r[0]=std::toupper(static_cast<unsigned char>(r[0]));
    return r;
}

// type_label span with link (used in PHP: <span class="type_label type_label_{type}"><a href="...">Name</a></span>)
static std::string typeLabelHtml(uint8_t typeId, const std::string &prefix="")
{
    std::ostringstream oss;
    oss << "<span class=\"type_label type_label_" << typeNameLower(typeId) << "\">";
    if(!prefix.empty())
        oss << prefix;
    oss << "<a href=\"" << Helper::htmlEscape(Helper::relUrl(GeneratorTypes::relativePathForType(typeId)))
        << "\">" << Helper::htmlEscape(ucfirst(typeName(typeId))) << "</a></span>";
    return oss.str();
}

// Compute defensive effectiveness
static std::map<uint8_t,double> computeMonsterEffectiveness(const std::vector<uint8_t> &defenderTypes)
{
    std::map<uint8_t,double> result;
    const auto &types=CatchChallenger::CommonDatapack::commonDatapack.get_types();
    if(types.empty())
        return result;
    for(uint8_t atk=0; atk<(uint8_t)types.size(); ++atk)
    {
        double mult=1.0;
        for(const uint8_t defType : defenderTypes)
        {
            if(defType>=types.size())
                continue;
            const auto &mp=types[atk].multiplicator;
            auto mit=mp.find(defType);
            if(mit==mp.cend())
                continue;
            int8_t m=mit->second;
            if(m==0)
            { mult=0.0; break; }
            if(m<0)
                mult/=(double)(-m);
            else
                mult*=(double)m;
        }
        result[atk]=mult;
    }
    return result;
}

// ── Reverse lookup structures ──

using MapKey=std::pair<size_t,size_t>;

struct WildMonsterMapEntry {
    size_t setIdx;
    size_t mapIdx;
    std::string layerType; // "Grass", "Water", "Cave", etc.
    uint8_t minLevel;
    uint8_t maxLevel;
    uint8_t luck;
};

struct BotFightRef {
    size_t setIdx;
    size_t mapIdx;
    uint16_t botLocalId;
    std::string botSkin;
};

struct ReverseEvolutionEntry {
    uint16_t evolveFrom;
    CatchChallenger::Monster::EvolutionType type;
    union { int8_t level; uint16_t item; } data;
};

static std::unordered_map<uint16_t,std::vector<WildMonsterMapEntry>> s_monsterToMapEntries;
static std::unordered_map<uint16_t,std::set<MapKey>> s_monsterToMaps; // just MapKeys for index
static std::unordered_map<uint16_t,std::set<MapKey>> s_monsterInTrainerOnMaps;
static std::unordered_map<uint16_t,std::vector<BotFightRef>> s_monsterToFight;

static std::unordered_map<uint16_t,uint32_t> s_monsterRarityPosition;
static size_t s_monsterRarityCount=0;

static std::unordered_map<uint16_t,std::vector<ReverseEvolutionEntry>> s_reverseEvolution;

static void buildReverseLookups()
{
    s_monsterToMapEntries.clear();
    s_monsterToMaps.clear();
    s_monsterInTrainerOnMaps.clear();
    s_monsterToFight.clear();

    std::unordered_map<std::string,uint16_t> nameToId;
    for(CATCHCHALLENGER_TYPE_MONSTER mid=1;mid<=CatchChallenger::CommonDatapack::commonDatapack.get_monstersMaxId();mid++) {
        if(!QtDatapackClientLoader::datapackLoader->has_monsterExtra(mid))
            continue;
        const DatapackClientLoader::MonsterExtra &mex=QtDatapackClientLoader::datapackLoader->get_monsterExtra(mid);
        if(!mex.name.empty())
            nameToId[mex.name]=mid;
    }

    auto resolveMonster=[&](const char *idStr) -> uint16_t {
        if(!idStr) return 0;
        int n=std::atoi(idStr);
        if(n>0 && CatchChallenger::CommonDatapack::commonDatapack.has_monster((uint16_t)n))
            return (uint16_t)n;
        auto it=nameToId.find(idStr);
        if(it!=nameToId.end())
            return it->second;
        return 0;
    };

    static const struct { const char *xmlName; const char *displayName; } s_wildTypes[]={
        {"grass","Grass"},{"water","Water"},{"cave","Cave"},{"lava","Lava"},
        {"waterRod","Water rod"},{"waterSuperRod","Super rod"},{nullptr,nullptr}
    };
    std::unordered_map<uint16_t,uint32_t> monsterLuck;

    const auto &sets=MapStore::sets();
    for(size_t si=0;si<sets.size();++si)
    {
        const auto &set=sets[si];
        for(size_t mi=0;mi<set.mapPaths.size();++mi)
        {
            const MapKey key{si,mi};

            std::string mapPath=set.mapPaths[mi];
            if(mapPath.size()>4 && mapPath.substr(mapPath.size()-4)==".tmx")
                mapPath=mapPath.substr(0,mapPath.size()-4);
            std::string xmlPath=Helper::datapackPath()+"map/main/"+set.mainCode+"/"+mapPath+".xml";

            tinyxml2::XMLDocument doc;
            if(doc.LoadFile(xmlPath.c_str())!=tinyxml2::XML_SUCCESS)
                continue;
            const tinyxml2::XMLElement *root=doc.RootElement();
            if(!root) continue;

            // Parse wild monsters
            for(int t=0;s_wildTypes[t].xmlName;t++)
            {
                const tinyxml2::XMLElement *layer=root->FirstChildElement(s_wildTypes[t].xmlName);
                if(!layer) continue;
                const tinyxml2::XMLElement *mon=layer->FirstChildElement("monster");
                while(mon)
                {
                    uint16_t id=resolveMonster(mon->Attribute("id"));
                    if(id>0)
                    {
                        s_monsterToMaps[id].insert(key);
                        uint8_t minLv=0,maxLv=0;
                        if(mon->Attribute("level"))
                        {
                            minLv=maxLv=(uint8_t)std::atoi(mon->Attribute("level"));
                        }
                        else
                        {
                            if(mon->Attribute("minLevel"))
                                minLv=(uint8_t)std::atoi(mon->Attribute("minLevel"));
                            if(mon->Attribute("maxLevel"))
                                maxLv=(uint8_t)std::atoi(mon->Attribute("maxLevel"));
                        }
                        uint8_t luck=0;
                        if(mon->Attribute("luck"))
                            luck=(uint8_t)std::atoi(mon->Attribute("luck"));

                        WildMonsterMapEntry e;
                        e.setIdx=si;
                        e.mapIdx=mi;
                        e.layerType=s_wildTypes[t].displayName;
                        e.minLevel=minLv;
                        e.maxLevel=maxLv;
                        e.luck=luck;
                        s_monsterToMapEntries[id].push_back(e);

                        if(luck>0)
                            monsterLuck[id]+=luck;
                    }
                    mon=mon->NextSiblingElement("monster");
                }
            }

            // Parse bot fight monsters
            const tinyxml2::XMLElement *bot=root->FirstChildElement("bot");
            while(bot)
            {
                uint16_t botLocalId=0;
                if(bot->Attribute("id"))
                    botLocalId=(uint16_t)std::atoi(bot->Attribute("id"));
                std::string botSkin;
                if(bot->Attribute("skin"))
                    botSkin=bot->Attribute("skin");

                const tinyxml2::XMLElement *step=bot->FirstChildElement("step");
                while(step)
                {
                    const char *type=step->Attribute("type");
                    if(type && std::string(type)=="fight")
                    {
                        const tinyxml2::XMLElement *mon=step->FirstChildElement("monster");
                        while(mon)
                        {
                            uint16_t id=resolveMonster(mon->Attribute("id"));
                            if(id>0)
                            {
                                s_monsterInTrainerOnMaps[id].insert(key);
                                BotFightRef bf;
                                bf.setIdx=si;
                                bf.mapIdx=mi;
                                bf.botLocalId=botLocalId;
                                bf.botSkin=botSkin;
                                s_monsterToFight[id].push_back(bf);
                            }
                            mon=mon->NextSiblingElement("monster");
                        }
                    }
                    step=step->NextSiblingElement("step");
                }
                bot=bot->NextSiblingElement("bot");
            }
        }
    }

    // Sort ascending by luck, assign positions (PHP rarity algorithm)
    s_monsterRarityPosition.clear();
    s_monsterRarityCount=0;
    std::vector<std::pair<uint32_t,uint16_t>> rarityList;
    for(const auto &p : monsterLuck)
        if(p.second>0)
            rarityList.push_back({p.second,p.first});
    std::sort(rarityList.begin(),rarityList.end());

    uint32_t pos=1;
    for(const auto &entry : rarityList)
    {
        uint16_t id=entry.second;
        auto it=s_monsterToMaps.find(id);
        if(it!=s_monsterToMaps.end() && it->second.size()>=2)
            s_monsterRarityPosition[id]=pos;
        pos++;
    }
    s_monsterRarityCount=s_monsterRarityPosition.size();

    // Build reverse evolution map
    s_reverseEvolution.clear();
    for(CATCHCHALLENGER_TYPE_MONSTER mrid=1;mrid<=CatchChallenger::CommonDatapack::commonDatapack.get_monstersMaxId();mrid++) {
        if(!CatchChallenger::CommonDatapack::commonDatapack.has_monster(mrid))
            continue;
        const CatchChallenger::Monster &mrMonster=CatchChallenger::CommonDatapack::commonDatapack.get_monster(mrid);
        for(const auto &ev : mrMonster.evolutions)
        {
            ReverseEvolutionEntry re;
            re.evolveFrom=mrid;
            re.type=ev.type;
            re.data.item=ev.data.item; // union covers both level and item
            s_reverseEvolution[ev.evolveTo].push_back(re);
        }
    }
}

// BFS up the reverse evolution chain: returns true if any ancestor is wild
static bool isEvolvedFromWild(uint16_t id)
{
    if(s_reverseEvolution.find(id)==s_reverseEvolution.end())
        return false;
    std::vector<uint16_t> toScan={id};
    std::unordered_set<uint16_t> visited;
    while(!toScan.empty())
    {
        std::vector<uint16_t> nextScan;
        for(uint16_t scanId : toScan)
        {
            if(visited.count(scanId))
                continue;
            visited.insert(scanId);
            auto it=s_reverseEvolution.find(scanId);
            if(it==s_reverseEvolution.end())
                continue;
            for(const auto &re : it->second)
            {
                if(s_monsterToMaps.find(re.evolveFrom)!=s_monsterToMaps.end())
                    return true;
                nextScan.push_back(re.evolveFrom);
            }
        }
        toScan=nextScan;
    }
    return false;
}

// Cache for map meta parsed from XML
struct MapMetaCache {
    std::string name;
    std::string zoneCode;
    bool parsed;
};
static std::map<std::pair<size_t,size_t>,MapMetaCache> s_mapMetaCache;

static void ensureMapMeta(size_t si, size_t mi)
{
    auto key=std::make_pair(si,mi);
    if(s_mapMetaCache.find(key)!=s_mapMetaCache.end())
        return;
    MapMetaCache mc;
    mc.parsed=false;
    const auto &sets=MapStore::sets();
    if(si<sets.size() && mi<sets[si].mapPaths.size())
    {
        std::string mapPath=sets[si].mapPaths[mi];
        if(mapPath.size()>4 && mapPath.substr(mapPath.size()-4)==".tmx")
            mapPath=mapPath.substr(0,mapPath.size()-4);
        std::string xmlPath=Helper::datapackPath()+"map/main/"+sets[si].mainCode+"/"+mapPath+".xml";
        tinyxml2::XMLDocument doc;
        if(doc.LoadFile(xmlPath.c_str())==tinyxml2::XML_SUCCESS)
        {
            const tinyxml2::XMLElement *root=doc.RootElement();
            if(root)
            {
                const char *zone=root->Attribute("zone");
                if(zone) mc.zoneCode=zone;
                // Parse <name> element
                const tinyxml2::XMLElement *nameEl=root->FirstChildElement("name");
                if(nameEl && nameEl->GetText())
                    mc.name=nameEl->GetText();
                mc.parsed=true;
            }
        }
        if(mc.name.empty())
        {
            // fallback: last component of map path
            auto slash=mapPath.rfind('/');
            mc.name=(slash!=std::string::npos)?mapPath.substr(slash+1):mapPath;
        }
    }
    s_mapMetaCache[key]=mc;
}

static std::string mapDisplayName(size_t si, size_t mi)
{
    ensureMapMeta(si,mi);
    auto it=s_mapMetaCache.find(std::make_pair(si,mi));
    if(it!=s_mapMetaCache.end() && !it->second.name.empty())
        return it->second.name;
    return "Unknown map";
}

// Zone name cache
static std::map<std::pair<std::string,std::string>,std::string> s_zoneNameCache;

static std::string getZoneName(const std::string &mainCode, const std::string &zoneCode)
{
    if(zoneCode.empty()) return std::string();
    auto key=std::make_pair(mainCode,zoneCode);
    auto it=s_zoneNameCache.find(key);
    if(it!=s_zoneNameCache.end()) return it->second;

    std::string result=zoneCode;
    std::string xmlPath=Helper::datapackPath()+"map/main/"+mainCode+"/zone/"+zoneCode+".xml";
    tinyxml2::XMLDocument doc;
    if(doc.LoadFile(xmlPath.c_str())==tinyxml2::XML_SUCCESS)
    {
        const tinyxml2::XMLElement *root=doc.RootElement();
        if(root)
        {
            const tinyxml2::XMLElement *nameEl=root->FirstChildElement("name");
            if(nameEl && nameEl->GetText())
                result=nameEl->GetText();
        }
    }
    s_zoneNameCache[key]=result;
    return result;
}

static std::string mapZoneName(size_t si, size_t mi)
{
    ensureMapMeta(si,mi);
    auto it=s_mapMetaCache.find(std::make_pair(si,mi));
    if(it!=s_mapMetaCache.end() && !it->second.zoneCode.empty())
    {
        const auto &sets=MapStore::sets();
        if(si<sets.size())
            return getZoneName(sets[si].mainCode,it->second.zoneCode);
    }
    return std::string();
}

void generate()
{
    buildReverseLookups();
    s_mapMetaCache.clear();
    s_zoneNameCache.clear();

    for(CATCHCHALLENGER_TYPE_MONSTER id=1;id<=CatchChallenger::CommonDatapack::commonDatapack.get_monstersMaxId();id++)
    {
        if(!CatchChallenger::CommonDatapack::commonDatapack.has_monster(id))
            continue;
        const CatchChallenger::Monster &m=CatchChallenger::CommonDatapack::commonDatapack.get_monster(id);

        std::string name;
        std::string description;
        std::string kind;
        std::string habitat;
        if(QtDatapackClientLoader::datapackLoader->has_monsterExtra(id))
        {
            const DatapackClientLoader::MonsterExtra &itxExtra=QtDatapackClientLoader::datapackLoader->get_monsterExtra(id);
            name=itxExtra.name;
            description=itxExtra.description;
            kind=itxExtra.kind;
            habitat=itxExtra.habitat;
        }
        if(name.empty())
            name="Monster #"+Helper::toStringUint(id);

        std::string rel=relativePathForMonster(id);
        Helper::setCurrentPage(rel);
        std::string image=frontImageUrlFrom(rel,id);

        // Determine primary type name for CSS class (PHP: monster_type_{typename})
        std::string resolved_type="normal";
        if(!m.type.empty())
            resolved_type=typeNameLower(m.type[0]);

        // Compute effectiveness
        std::map<std::string,std::vector<uint8_t>> effectiveness_list;
        {
            auto eff=computeMonsterEffectiveness(m.type);
            for(const auto &e : eff)
            {
                if(e.second!=1.0)
                {
                    std::ostringstream key;
                    // Format like PHP: "4", "2", "0.5", "0.25", "0"
                    if(e.second==0.0) key << "0";
                    else if(e.second>=4.0) key << "4";
                    else if(e.second>=2.0) key << "2";
                    else if(e.second<=0.25 && e.second>0) key << "0.25";
                    else if(e.second<=0.5 && e.second>0) key << "0.5";
                    else continue;
                    effectiveness_list[key.str()].push_back(e.first);
                }
            }
        }

        std::ostringstream body;

        // ── 1. Opening div with type class ──
        body << "<div class=\"map monster_type_" << resolved_type << "\">\n";

        // ── 2. Header ──
        body << "<div class=\"subblock\"><h1>" << Helper::htmlEscape(name) << "</h1>\n";
        body << "<h2>#" << id << "</h2>\n";
        body << "</div>\n";

        // ── 3. Front image ──
        body << "<div class=\"value datapackscreenshot\"><center>\n";
        if(!image.empty())
            body << "<img src=\"" << Helper::htmlEscape(image) << "\" width=\"80\" height=\"80\" alt=\""
                 << Helper::htmlEscape(name) << "\" title=\"" << Helper::htmlEscape(name) << "\" />\n";
        body << "</center></div>\n";

        // ── 4. Type ──
        body << "<div class=\"subblock\"><div class=\"valuetitle\">Type</div><div class=\"value\">\n";
        {
            std::vector<std::string> type_labels;
            for(const uint8_t tId : m.type)
                type_labels.push_back(typeLabelHtml(tId)+"\n");
            std::string joined;
            for(size_t i=0;i<type_labels.size();i++)
            {
                if(i>0) joined+=" ";
                joined+=type_labels[i];
            }
            body << "<div class=\"type_label_list\">" << joined << "</div></div></div>\n";
        }

        // ── 5. Gender ratio ──
        body << "<div class=\"subblock\"><div class=\"valuetitle\">Gender ratio</div><div class=\"value\">\n";
        if(m.ratio_gender<0 || m.ratio_gender>100)
        {
            body << "<center><table class=\"genderbar\"><tr><td class=\"genderbarunknown\" style=\"width:100%\"></td></tr></table></center>\n";
            body << "Unknown gender";
        }
        else
        {
            int male=(int)m.ratio_gender;
            int female=100-male;
            body << "<center><table class=\"genderbar\"><tr><td class=\"genderbarmale\" style=\"width:" << male << "%\"></td><td class=\"genderbarfemale\" style=\"width:" << female << "%\"></td></tr></table></center>\n";
            body << male << "% male, " << female << "% female";
        }
        body << "</div></div>\n";

        // ── 6. Description ──
        if(!description.empty())
            body << "<div class=\"subblock\"><div class=\"valuetitle\">Description</div><div class=\"value\">" << Helper::htmlEscape(description) << "</div></div>\n";

        // ── 7. Kind ──
        if(!kind.empty())
            body << "<div class=\"subblock\"><div class=\"valuetitle\">Kind</div><div class=\"value\">" << Helper::htmlEscape(kind) << "</div></div>\n";

        // ── 8. Habitat ──
        if(!habitat.empty())
            body << "<div class=\"subblock\"><div class=\"valuetitle\">Habitat</div><div class=\"value\">" << Helper::htmlEscape(habitat) << "</div></div>\n";

        // ── 9. Catch rate ──
        body << "<div class=\"subblock\"><div class=\"valuetitle\">Catch rate</div><div class=\"value\">" << (unsigned)m.catch_rate << "</div></div>\n";

        // ── 10. Rarity ──
        {
            body << "<div class=\"subblock\"><div class=\"valuetitle\">Rarity</div><div class=\"value\">\n";
            bool isWild=s_monsterToMaps.find(id)!=s_monsterToMaps.end();
            if(!isWild)
                body << "Not found on any map";
            else
            {
                auto rit=s_monsterRarityPosition.find(id);
                if(rit==s_monsterRarityPosition.end())
                    body << "Very rare";
                else if(s_monsterRarityCount>0)
                {
                    double percent=100.0*rit->second/s_monsterRarityCount;
                    if(percent>90) body << "Very common";
                    else if(percent>70) body << "Common";
                    else if(percent>40) body << "Less common";
                    else if(percent>10) body << "Rare";
                    else body << "Very rare";
                }
                else
                    body << "Very rare";
            }
            body << "</div></div>\n";
        }

        // ── 11. Steps for hatching ──
        body << "<div class=\"subblock\"><div class=\"valuetitle\">Steps for hatching</div><div class=\"value\">" << m.egg_step << "</div></div>\n";

        // ── 12. Stat (inline text like PHP) ──
        body << "<div class=\"subblock\"><div class=\"valuetitle\">Stat</div><div class=\"value\">"
             << "Hp: <i>" << m.stat.hp << "</i>, "
             << "Attack: <i>" << m.stat.attack << "</i>, "
             << "Defense: <i>" << m.stat.defense << "</i>, "
             << "Special attack: <i>" << m.stat.special_attack << "</i>, "
             << "Special defense: <i>" << m.stat.special_defense << "</i>, "
             << "Speed: <i>" << m.stat.speed << "</i>"
             << "</div></div>\n";

        // ── 13. Weak to ──
        {
            bool hasWeak2=effectiveness_list.find("2")!=effectiveness_list.end();
            bool hasWeak4=effectiveness_list.find("4")!=effectiveness_list.end();
            if(hasWeak4 || hasWeak2)
            {
                body << "<div class=\"subblock\"><div class=\"valuetitle\">Weak to</div><div class=\"value\">\n";
                std::vector<std::string> labels;
                if(hasWeak2)
                    for(uint8_t t : effectiveness_list["2"])
                        labels.push_back(typeLabelHtml(t,"2x: ")+"\n");
                if(hasWeak4)
                    for(uint8_t t : effectiveness_list["4"])
                        labels.push_back(typeLabelHtml(t,"4x: ")+"\n");
                for(size_t i=0;i<labels.size();i++)
                {
                    if(i>0) body << " ";
                    body << labels[i];
                }
                body << "</div></div>\n";
            }
        }

        // ── 14. Resistant to ──
        {
            bool hasRes25=effectiveness_list.find("0.25")!=effectiveness_list.end();
            bool hasRes50=effectiveness_list.find("0.5")!=effectiveness_list.end();
            if(hasRes25 || hasRes50)
            {
                body << "<div class=\"subblock\"><div class=\"valuetitle\">Resistant to</div><div class=\"value\">\n";
                std::vector<std::string> labels;
                if(hasRes25)
                    for(uint8_t t : effectiveness_list["0.25"])
                        labels.push_back(typeLabelHtml(t,"0.25x: ")+"\n");
                if(hasRes50)
                    for(uint8_t t : effectiveness_list["0.5"])
                        labels.push_back(typeLabelHtml(t,"0.5x: ")+"\n");
                for(size_t i=0;i<labels.size();i++)
                {
                    if(i>0) body << " ";
                    body << labels[i];
                }
                body << "</div></div>\n";
            }
        }

        // ── 15. Immune to ──
        {
            auto immIt=effectiveness_list.find("0");
            if(immIt!=effectiveness_list.end())
            {
                body << "<div class=\"subblock\"><div class=\"valuetitle\">Immune to</div><div class=\"value\">\n";
                for(size_t i=0;i<immIt->second.size();i++)
                {
                    if(i>0) body << " ";
                    body << typeLabelHtml(immIt->second[i]) << "\n";
                }
                body << "</div></div>\n";
            }
        }

        // ── 16. Close header div ──
        body << "</div>\n";

        // ── 17. Drops table ──
        {
            if(CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.has_monsterDrop(id))
            {
                const std::vector<CatchChallenger::MonsterDrops> &dropsList=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_monsterDrop(id);
                if(!dropsList.empty())
                {
                body << "<table class=\"item_list item_list_type_" << resolved_type << "\">\n"
                     << "\t\t<tr class=\"item_list_title item_list_title_type_" << resolved_type << "\">\n"
                     << "\t\t\t<th colspan=\"2\">Item</th>\n"
                     << "\t\t\t<th>Location</th>\n"
                     << "\t\t</tr>\n";
                for(const auto &drop : dropsList)
                {
                    std::string link,iname,image;
                    if(QtDatapackClientLoader::datapackLoader->has_itemExtra(drop.item))
                    {
                        const DatapackClientLoader::ItemExtra &ix=QtDatapackClientLoader::datapackLoader->get_itemExtra(drop.item);
                        iname=ix.name;
                        link=Helper::relUrl(GeneratorItems::relativePathForItem(drop.item));
                        if(!ix.imagePath.empty())
                            image=Helper::relUrl(Helper::publishDatapackFile(Helper::relativeFromDatapack(ix.imagePath)));
                    }
                    std::string quantity_text;
                    if(drop.quantity_min!=drop.quantity_max)
                        quantity_text=Helper::toStringUint(drop.quantity_min)+" to "+Helper::toStringUint(drop.quantity_max)+" ";
                    else if(drop.quantity_min>1)
                        quantity_text=Helper::toStringUint(drop.quantity_min)+" ";

                    body << "<tr class=\"value\">\n\t\t\t\t<td>\n";
                    if(!image.empty())
                    {
                        if(!link.empty())
                            body << "<a href=\"" << Helper::htmlEscape(link) << "\">\n";
                        body << "<img src=\"" << Helper::htmlEscape(image) << "\" width=\"24\" height=\"24\" alt=\"" << Helper::htmlEscape(iname) << "\" title=\"" << Helper::htmlEscape(iname) << "\" />\n";
                        if(!link.empty())
                            body << "</a>\n";
                    }
                    body << "</td>\n\t\t\t\t<td>\n";
                    if(!link.empty())
                        body << "<a href=\"" << Helper::htmlEscape(link) << "\">\n";
                    if(!iname.empty())
                        body << quantity_text << Helper::htmlEscape(iname);
                    else
                        body << quantity_text << "Unknown item";
                    if(!link.empty())
                        body << "</a>\n";
                    body << "</td>\n";
                    body << "<td>Drop luck of " << (unsigned)drop.luck << "%</td>\n";
                    body << "\t\t\t</tr>\n";
                }
                body << "<tr>\n\t\t\t<td colspan=\"3\" class=\"item_list_endline item_list_title_type_" << resolved_type << "\"></td>\n\t\t</tr>\n\t\t</table>\n";
                }
            }
        }

        // ── 18. Attack list by level ──
        if(!m.learn.empty())
        {
            body << "<table class=\"item_list item_list_type_" << resolved_type << "\">\n"
                 << "\t\t<tr class=\"item_list_title item_list_title_type_" << resolved_type << "\">\n"
                 << "\t\t\t<th>Level</th>\n"
                 << "\t\t\t<th>Skill</th>\n"
                 << "\t\t\t<th>Type</th>\n"
                 << "\t\t\t<th>Endurance</th>\n"
                 << "\t\t</tr>\n";
            for(const auto &l : m.learn)
            {
                bool skItFound=CatchChallenger::CommonDatapack::commonDatapack.has_monsterSkill(l.learnSkill);
                bool skxFound=QtDatapackClientLoader::datapackLoader->has_monsterSkillExtra(l.learnSkill);
                std::string sname=(skxFound && !QtDatapackClientLoader::datapackLoader->get_monsterSkillExtra(l.learnSkill).name.empty())
                    ? QtDatapackClientLoader::datapackLoader->get_monsterSkillExtra(l.learnSkill).name : ("Skill #"+Helper::toStringUint(l.learnSkill));

                body << "<tr class=\"value\">\n";
                body << "<td>\n";
                if(l.learnAtLevel==0)
                    body << "Start\n";
                else
                    body << (unsigned)l.learnAtLevel;
                body << "</td>\n";
                body << "<td><a href=\"" << Helper::htmlEscape(Helper::relUrl(GeneratorSkills::relativePathForSkill(l.learnSkill))) << "\">" << Helper::htmlEscape(sname);
                if(l.learnSkillLevel>1)
                    body << " at level " << (unsigned)l.learnSkillLevel;
                body << "</a></td>\n";
                if(skItFound)
                    body << "<td>" << typeLabelHtml(CatchChallenger::CommonDatapack::commonDatapack.get_monsterSkill(l.learnSkill).type) << "</td>\n";
                else
                    body << "<td>&nbsp;</td>\n";
                // Endurance
                if(skItFound && l.learnSkillLevel>0 && (size_t)(l.learnSkillLevel-1)<CatchChallenger::CommonDatapack::commonDatapack.get_monsterSkill(l.learnSkill).level.size())
                    body << "<td>" << (unsigned)CatchChallenger::CommonDatapack::commonDatapack.get_monsterSkill(l.learnSkill).level[l.learnSkillLevel-1].endurance << "</td>\n";
                else
                    body << "<td>&nbsp;</td>\n";
                body << "</tr>\n";
            }
            body << "<tr>\n\t\t\t<td colspan=\"4\" class=\"item_list_endline item_list_title_type_" << resolved_type << "\"></td>\n\t\t</tr>\n\t\t</table>\n";
        }

        // ── 19. Attack list by item (TM-like) with 10-row pagination ──
        if(!m.learnByItem.empty())
        {
            int attack_list_count=0;
            auto writeByItemHeader=[&](){
                body << "<table class=\"skilltm_list item_list item_list_type_" << resolved_type << "\">\n"
                     << "\t\t<tr class=\"item_list_title item_list_title_type_" << resolved_type << "\">\n"
                     << "\t\t\t<th colspan=\"2\">Item</th>\n"
                     << "\t\t\t<th>Skill</th>\n"
                     << "\t\t\t<th>Type</th>\n"
                     << "\t\t\t<th>Endurance</th>\n"
                     << "\t\t</tr>\n";
            };
            writeByItemHeader();

            for(const auto &lp : m.learnByItem)
            {
                uint16_t itemId=lp.first;
                const auto &entry=lp.second;

                bool skIt2Found=CatchChallenger::CommonDatapack::commonDatapack.has_monsterSkill(entry.learnSkill);
                bool skx2Found=QtDatapackClientLoader::datapackLoader->has_monsterSkillExtra(entry.learnSkill);
                if(!skx2Found)
                    continue;
                std::string sname=(skx2Found && !QtDatapackClientLoader::datapackLoader->get_monsterSkillExtra(entry.learnSkill).name.empty())
                    ? QtDatapackClientLoader::datapackLoader->get_monsterSkillExtra(entry.learnSkill).name : ("Skill #"+Helper::toStringUint(entry.learnSkill));

                attack_list_count++;
                if(attack_list_count>10)
                {
                    body << "<tr>\n                            <td colspan=\"5\" class=\"item_list_endline item_list_title_type_" << resolved_type << "\"></td>\n"
                         << "                        </tr>\n"
                         << "                        </table>\n";
                    writeByItemHeader();
                    attack_list_count=1;
                }

                std::string link,iname,iimage;
                if(QtDatapackClientLoader::datapackLoader->has_itemExtra(itemId))
                {
                    const DatapackClientLoader::ItemExtra &ix=QtDatapackClientLoader::datapackLoader->get_itemExtra(itemId);
                    iname=ix.name;
                    link=Helper::relUrl(GeneratorItems::relativePathForItem(itemId));
                    if(!ix.imagePath.empty())
                        iimage=Helper::relUrl(Helper::publishDatapackFile(Helper::relativeFromDatapack(ix.imagePath)));
                }

                body << "<tr class=\"value\">\n";
                body << "<tr class=\"value\">\n\t\t\t\t\t<td>\n";
                if(!iimage.empty())
                {
                    if(!link.empty())
                        body << "<a href=\"" << Helper::htmlEscape(link) << "\">\n";
                    body << "<img src=\"" << Helper::htmlEscape(iimage) << "\" width=\"24\" height=\"24\" alt=\"" << Helper::htmlEscape(iname) << "\" title=\"" << Helper::htmlEscape(iname) << "\" />\n";
                    if(!link.empty())
                        body << "</a>\n";
                }
                body << "</td>\n\t\t\t\t\t<td>\n";
                if(!link.empty())
                    body << "<a href=\"" << Helper::htmlEscape(link) << "\">\n";
                if(!iname.empty())
                    body << Helper::htmlEscape(iname);
                else
                    body << "Unknown item";
                if(!link.empty())
                    body << "</a>\n";
                body << "</td>\n";
                body << "<td><a href=\"" << Helper::htmlEscape(Helper::relUrl(GeneratorSkills::relativePathForSkill(entry.learnSkill))) << "\">" << Helper::htmlEscape(sname);
                if(entry.learnSkillLevel>1)
                    body << " at level " << (unsigned)entry.learnSkillLevel;
                body << "</a></td>\n";
                if(skIt2Found)
                    body << "<td>" << typeLabelHtml(CatchChallenger::CommonDatapack::commonDatapack.get_monsterSkill(entry.learnSkill).type) << "</td>\n";
                else
                    body << "<td>&nbsp;</td>\n";
                if(skIt2Found && entry.learnSkillLevel>0 && (size_t)(entry.learnSkillLevel-1)<CatchChallenger::CommonDatapack::commonDatapack.get_monsterSkill(entry.learnSkill).level.size())
                    body << "<td>" << (unsigned)CatchChallenger::CommonDatapack::commonDatapack.get_monsterSkill(entry.learnSkill).level[entry.learnSkillLevel-1].endurance << "</td>\n";
                else
                    body << "<td>&nbsp;</td>\n";
                body << "</tr>\n";
            }
            body << "<tr>\n\t\t\t<td colspan=\"5\" class=\"item_list_endline item_list_title_type_" << resolved_type << "\"></td>\n\t\t</tr>\n\t\t</table>\n";
            body << "<br style=\"clear:both;\" />\n";
        }

        // ── 20. Evolution table ──
        {
            auto revIt=s_reverseEvolution.find(id);
            bool hasRevEvo=revIt!=s_reverseEvolution.end() && !revIt->second.empty();
            bool hasFwdEvo=!m.evolutions.empty();

            if(hasRevEvo || hasFwdEvo)
            {
                body << "<table class=\"item_list item_list_type_" << resolved_type << "\">\n"
                     << "\t\t<tr class=\"item_list_title item_list_title_type_" << resolved_type << "\">\n"
                     << "\t\t\t<th>\n";
                if(hasRevEvo)
                    body << "Evolve from";
                body << "</th>\n\t\t</tr>\n";

                // Pre-evolution monsters
                if(hasRevEvo)
                {
                    body << "<tr class=\"value\">\n<td>\n";
                    body << "<table class=\"monsterforevolution\">\n";
                    for(const auto &re : revIt->second)
                    {
                        std::string preName=QtDatapackClientLoader::datapackLoader->has_monsterExtra(re.evolveFrom)?QtDatapackClientLoader::datapackLoader->get_monsterExtra(re.evolveFrom).name:("Monster #"+Helper::toStringUint(re.evolveFrom));
                        std::string preImg=frontImageUrlFrom(rel,re.evolveFrom);
                        std::string preLink=Helper::relUrl(relativePathForMonster(re.evolveFrom));

                        if(!preImg.empty())
                            body << "<tr><td><a href=\"" << Helper::htmlEscape(preLink) << "\"><img src=\"" << Helper::htmlEscape(preImg) << "\" width=\"80\" height=\"80\" alt=\"" << Helper::htmlEscape(preName) << "\" title=\"" << Helper::htmlEscape(preName) << "\" /></a></td></tr>\n";
                        body << "<tr><td class=\"evolution_name\"><a href=\"" << Helper::htmlEscape(preLink) << "\">" << Helper::htmlEscape(preName) << "</a></td></tr>\n";
                        if(re.type==CatchChallenger::Monster::EvolutionType_Level)
                            body << "<tr><td class=\"evolution_type\">At level " << (int)re.data.level << "</td></tr>\n";
                        else if(re.type==CatchChallenger::Monster::EvolutionType_Item)
                        {
                            if(QtDatapackClientLoader::datapackLoader->has_itemExtra(re.data.item))
                            {
                                const DatapackClientLoader::ItemExtra &evItemEx=QtDatapackClientLoader::datapackLoader->get_itemExtra(re.data.item);
                                std::string evItemLink=Helper::relUrl(GeneratorItems::relativePathForItem(re.data.item));
                                std::string evItemName=evItemEx.name;
                                body << "<tr><td class=\"evolution_type\">Evolve with<br /><a href=\"" << Helper::htmlEscape(evItemLink) << "\" title=\"" << Helper::htmlEscape(evItemName) << "\">\n";
                                if(!evItemEx.imagePath.empty())
                                    body << "<img src=\"" << Helper::htmlEscape(Helper::relUrl(Helper::publishDatapackFile(Helper::relativeFromDatapack(evItemEx.imagePath)))) << "\" alt=\"" << Helper::htmlEscape(evItemName) << "\" title=\"" << Helper::htmlEscape(evItemName) << "\" style=\"float:left;\" />\n";
                                body << Helper::htmlEscape(evItemName) << "</a></td></tr>\n";
                            }
                            else
                                body << "<tr><td class=\"evolution_type\">With unknown item</td></tr>\n";
                        }
                        else if(re.type==CatchChallenger::Monster::EvolutionType_Trade)
                            body << "<tr><td class=\"evolution_type\">After trade</td></tr>\n";
                        else
                            body << "<tr><td class=\"evolution_type\">&nbsp;</td></tr>\n";
                    }
                    body << "</table>\n</td>\n</tr>\n";
                }

                // Current monster
                body << "<tr class=\"value\">\n<td>\n";
                body << "<table class=\"monsterforevolution\">\n";
                if(!image.empty())
                    body << "<tr><td><img src=\"" << Helper::htmlEscape(image) << "\" width=\"80\" height=\"80\" alt=\"" << Helper::htmlEscape(name) << "\" title=\"" << Helper::htmlEscape(name) << "\" /></td></tr>\n";
                body << "<tr><td class=\"evolution_name\">" << Helper::htmlEscape(name) << "</td></tr>\n";
                body << "</table>\n</td>\n</tr>\n";

                // Forward evolutions
                if(hasFwdEvo)
                {
                    body << "<tr class=\"value\">\n<td>\n";
                    body << "<table class=\"monsterforevolution\">\n";
                    for(const auto &ev : m.evolutions)
                    {
                        std::string evoName=QtDatapackClientLoader::datapackLoader->has_monsterExtra(ev.evolveTo)?QtDatapackClientLoader::datapackLoader->get_monsterExtra(ev.evolveTo).name:"???";
                        std::string evoImg=frontImageUrlFrom(rel,ev.evolveTo);
                        std::string evoLink=Helper::relUrl(relativePathForMonster(ev.evolveTo));

                        if(!evoImg.empty())
                            body << "<tr><td><a href=\"" << Helper::htmlEscape(evoLink) << "\"><img src=\"" << Helper::htmlEscape(evoImg) << "\" width=\"80\" height=\"80\" alt=\"" << Helper::htmlEscape(evoName) << "\" title=\"" << Helper::htmlEscape(evoName) << "\" /></a></td></tr>\n";
                        if(evoName!="???")
                            body << "<tr><td class=\"evolution_name\"><a href=\"" << Helper::htmlEscape(evoLink) << "\">" << Helper::htmlEscape(evoName) << "</a></td></tr>\n";
                        else
                            body << "<tr><td class=\"evolution_name\">" << Helper::htmlEscape(evoName) << "</td></tr>\n";
                        if(ev.type==CatchChallenger::Monster::EvolutionType_Level)
                            body << "<tr><td class=\"evolution_type\">At level " << (int)ev.data.level << "</td></tr>\n";
                        else if(ev.type==CatchChallenger::Monster::EvolutionType_Item)
                        {
                            if(QtDatapackClientLoader::datapackLoader->has_itemExtra(ev.data.item))
                            {
                                const DatapackClientLoader::ItemExtra &evItemEx2=QtDatapackClientLoader::datapackLoader->get_itemExtra(ev.data.item);
                                std::string evItemLink=Helper::relUrl(GeneratorItems::relativePathForItem(ev.data.item));
                                std::string evItemName=evItemEx2.name;
                                body << "<tr><td class=\"evolution_type\">Evolve with<br /><a href=\"" << Helper::htmlEscape(evItemLink) << "\" title=\"" << Helper::htmlEscape(evItemName) << "\">\n";
                                if(!evItemEx2.imagePath.empty())
                                    body << "<img src=\"" << Helper::htmlEscape(Helper::relUrl(Helper::publishDatapackFile(Helper::relativeFromDatapack(evItemEx2.imagePath)))) << "\" alt=\"" << Helper::htmlEscape(evItemName) << "\" title=\"" << Helper::htmlEscape(evItemName) << "\" style=\"float:left;\" />\n";
                                body << Helper::htmlEscape(evItemName) << "</a></td></tr>\n";
                            }
                            else
                                body << "<tr><td class=\"evolution_type\">With unknown item</td></tr>\n";
                        }
                        else if(ev.type==CatchChallenger::Monster::EvolutionType_Trade)
                            body << "<tr><td class=\"evolution_type\">After trade</td></tr>\n";
                        else
                            body << "<tr><td class=\"evolution_type\">&nbsp;</td></tr>\n";
                    }
                    body << "</table>\n</td>\n</tr>\n";
                }

                // End line
                body << "<tr>\n\t\t\t<th class=\"item_list_endline item_list_title item_list_title_type_" << resolved_type << "\">\n";
                if(hasFwdEvo)
                    body << "Evolve to";
                body << "</th>\n\t\t</tr>\n\t\t</table>\n";
            }
        }

        // ── 21. Map locations table ──
        {
            auto wildIt=s_monsterToMapEntries.find(id);
            if(wildIt!=s_monsterToMapEntries.end() && !wildIt->second.empty())
            {
                body << "<table class=\"item_list item_list_type_" << resolved_type << "\">\n"
                     << "\t\t<tr class=\"item_list_title item_list_title_type_" << resolved_type << "\">\n"
                     << "\t\t\t<th colspan=\"2\">Map</th>\n"
                     << "\t\t\t<th>Location</th>\n"
                     << "\t\t\t<th>Levels</th>\n"
                     << "\t\t\t<th colspan=\"3\">Rate</th>\n"
                     << "\t\t</tr>\n";

                // Group by layer type
                std::map<std::string,std::vector<const WildMonsterMapEntry*>> byLayer;
                for(const auto &e : wildIt->second)
                    byLayer[e.layerType].push_back(&e);

                const auto &allSets=MapStore::sets();
                for(const auto &layerPair : byLayer)
                {
                    const std::string &layerName=layerPair.first;
                    body << "<tr class=\"item_list_title_type_" << resolved_type << "\">\n"
                         << "                    <th colspan=\"7\">\n"
                         << layerName << "\n"
                         << "</th>\n                </tr>\n";

                    for(const auto *e : layerPair.second)
                    {
                        body << "<tr class=\"value\">\n";
                        if(e->setIdx<allSets.size() && e->mapIdx<allSets[e->setIdx].mapPaths.size())
                        {
                            std::string mName=mapDisplayName(e->setIdx,e->mapIdx);
                            std::string mLink=Helper::relUrl(GeneratorMaps::relativePathForMapRef(e->setIdx,e->mapIdx));
                            std::string zone=mapZoneName(e->setIdx,e->mapIdx);
                            if(!zone.empty())
                            {
                                body << "<td><a href=\"" << Helper::htmlEscape(mLink) << "\" title=\"" << Helper::htmlEscape(mName) << "\">" << Helper::htmlEscape(mName) << "</a></td>\n";
                                body << "<td>" << Helper::htmlEscape(zone) << "</td>\n";
                            }
                            else
                                body << "<td colspan=\"2\"><a href=\"" << Helper::htmlEscape(mLink) << "\" title=\"" << Helper::htmlEscape(mName) << "\">" << Helper::htmlEscape(mName) << "</a></td>\n";
                        }
                        else
                            body << "<td colspan=\"2\">Unknown map</td>\n";
                        body << "<td>\n"
                             << "<img src=\"/images/datapack-explorer/" << layerName << ".png\" alt=\"\" class=\"locationimg\" width=\"16px\" height=\"16px\">" << layerName
                             << "</td>\n<td>\n";
                        if(e->minLevel==e->maxLevel)
                            body << (unsigned)e->minLevel;
                        else
                            body << (unsigned)e->minLevel << "-" << (unsigned)e->maxLevel;
                        body << "</td>\n";
                        body << "<td colspan=\"3\">" << (unsigned)e->luck << "%</td>\n"
                             << "                </tr>\n";
                    }
                }

                body << "<tr>\n\t\t\t<td colspan=\"7\" class=\"item_list_endline item_list_title_type_" << resolved_type << "\"></td>\n\t\t</tr>\n\t\t</table>\n";
            }
        }

        // ── 22. Bot fights table ──
        {
            auto fightIt=s_monsterToFight.find(id);
            if(fightIt!=s_monsterToFight.end() && !fightIt->second.empty())
            {
                body << "<center><table class=\"item_list item_list_type_" << resolved_type << "\">\n"
                     << "\t\t<tr class=\"item_list_title item_list_title_type_" << resolved_type << "\">\n"
                     << "\t\t\t<th colspan=\"2\">Bot</th>\n"
                     << "\t\t\t<th>Type</th>\n"
                     << "\t\t\t<th>Content</th>\n"
                     << "\t\t</tr>\n";

                for(const auto &bf : fightIt->second)
                {
                    body << "<tr class=\"value\">\n";

                    // Bot skin sprite
                    bool have_skin=false;
                    if(!bf.botSkin.empty())
                    {
                        std::string skinBase=Helper::datapackPath()+"skin/bot/"+bf.botSkin+"/trainer.png";
                        std::string skinBase2=Helper::datapackPath()+"skin/fighter/"+bf.botSkin+"/trainer.png";
                        std::string skinUrl;
                        if(Helper::fileExists(skinBase))
                            skinUrl=Helper::relUrl(Helper::publishDatapackFile("skin/bot/"+bf.botSkin+"/trainer.png"));
                        else if(Helper::fileExists(skinBase2))
                            skinUrl=Helper::relUrl(Helper::publishDatapackFile("skin/fighter/"+bf.botSkin+"/trainer.png"));
                        if(!skinUrl.empty())
                        {
                            body << "<td><div style=\"width:16px;height:24px;background-image:url('" << Helper::htmlEscape(skinUrl) << "');background-repeat:no-repeat;background-position:-16px -48px;\"></div></td>\n";
                            have_skin=true;
                        }
                    }
                    body << "<td";
                    if(!have_skin)
                        body << " colspan=\"2\"";
                    body << ">Bot #" << bf.botLocalId << "</td>\n";
                    body << "<td><center>Fight<div style=\"background-position:-16px -16px;\" class=\"flags flags16\"></div></center></td><td>\n";
                    body << "</td>\n</tr>\n";
                }

                body << "<tr>\n\t\t\t<td colspan=\"4\" class=\"item_list_endline item_list_title_type_" << resolved_type << "\"></td>\n\t\t</tr>\n\t\t</table></center>\n";
            }
        }

        Helper::writeHtml(rel,name,body.str());
    }

    // ── Generate monster index page ──
    {
        const std::string indexPage="monsters/index.html";
        Helper::setCurrentPage(indexPage);

        std::ostringstream indexBody;

        // Legend (matching PHP exactly)
        indexBody << "<img src=\"/images/datapack-explorer/Grass.png\" alt=\"\" class=\"locationimg\" width=\"16px\" height=\"16px\">: Wild monster<br style=\"clear:both\" />"
                  << "<img src=\"/images/datapack-explorer/GrassUp.png\" alt=\"\" class=\"locationimg\" width=\"16px\" height=\"16px\">: Evole from wild monster<br style=\"clear:both\" />"
                  << "<div style=\"background-position:-16px -16px;\" class=\"flags flags16\" class=\"locationimg\"></div>: Used in bot fight<br style=\"clear:both\" />"
                  << "<img src=\"/official-server/images/top-1.png\" alt=\"\" class=\"locationimg\" width=\"16px\" height=\"16px\">: Unique to a version<br style=\"clear:both\" />"
                  << "<div style=\"width:16px;height:16px;float:left;background-color:#e5eaff;\"></div>: Very common<br style=\"clear:both\" />"
                  << "<div style=\"width:16px;height:16px;float:left;background-color:#e0ffdd;\"></div>: Common<br style=\"clear:both\" />"
                  << "<div style=\"width:16px;height:16px;float:left;background-color:#fbfdd3;\"></div>: Less common<br style=\"clear:both\" />"
                  << "<div style=\"width:16px;height:16px;float:left;background-color:#ffefdb;\"></div>: Rare<br style=\"clear:both\" />"
                  << "<div style=\"width:16px;height:16px;float:left;background-color:#ffe5e5;\"></div>: Very rare<br style=\"clear:both\" />"
                  << "<br style=\"clear:both\" />";

        // Table
        indexBody << "<table class=\"item_list item_list_type_normal monster_list\">\n"
                  << "<tr class=\"item_list_title item_list_title_type_normal\">\n"
                  << "\t<th colspan=\"4\">Monster</th>\n"
                  << "</tr>\n";

        std::vector<uint16_t> sortedMonsterIds;
        for(CATCHCHALLENGER_TYPE_MONSTER sid=1;sid<=CatchChallenger::CommonDatapack::commonDatapack.get_monstersMaxId();sid++) {
            if(!CatchChallenger::CommonDatapack::commonDatapack.has_monster(sid))
                continue;
            sortedMonsterIds.push_back(sid);
        }
        std::sort(sortedMonsterIds.begin(),sortedMonsterIds.end());

        int monsterCount=0;
        for(uint16_t id : sortedMonsterIds)
        {
            const CatchChallenger::Monster &m=CatchChallenger::CommonDatapack::commonDatapack.get_monster(id);
            std::string name;
            if(QtDatapackClientLoader::datapackLoader->has_monsterExtra(id))
                name=QtDatapackClientLoader::datapackLoader->get_monsterExtra(id).name;
            if(name.empty())
                name="Monster #"+Helper::toStringUint(id);

            std::string monsterUrl=Helper::relUrlFrom(indexPage,relativePathForMonster(id));

            monsterCount++;
            if(monsterCount>20)
            {
                indexBody << "<tr>\n"
                          << "            <td colspan=\"4\" class=\"item_list_endline item_list_title_type_normal\"></td>\n"
                          << "        </tr>\n"
                          << "        </table>\n";
                indexBody << "<table class=\"item_list item_list_type_normal monster_list\">\n"
                          << "        <tr class=\"item_list_title item_list_title_type_normal\">\n"
                          << "            <th colspan=\"4\">Monster</th>\n"
                          << "        </tr>\n";
                monsterCount=1;
            }

            // Background color from rarity
            std::string bgColor;
            bool isWild=s_monsterToMaps.find(id)!=s_monsterToMaps.end();
            if(isWild)
            {
                auto rit=s_monsterRarityPosition.find(id);
                if(rit==s_monsterRarityPosition.end())
                    bgColor="#ffe5e5";
                else if(s_monsterRarityCount>0)
                {
                    double percent=100.0*rit->second/s_monsterRarityCount;
                    if(percent>90) bgColor="#e5eaff";
                    else if(percent>70) bgColor="#e0ffdd";
                    else if(percent>40) bgColor="#fbfdd3";
                    else if(percent>10) bgColor="#ffefdb";
                    else bgColor="#ffe5e5";
                }
            }

            // Build flags
            std::vector<std::string> flags;
            if(isWild)
                flags.push_back("<img src=\"/images/datapack-explorer/Grass.png\" alt=\"\" class=\"locationimg\" width=\"16px\" height=\"16px\">");
            if(isEvolvedFromWild(id))
                flags.push_back("<img src=\"/images/datapack-explorer/GrassUp.png\" alt=\"\" class=\"locationimg\" width=\"16px\" height=\"16px\">");
            if(s_monsterInTrainerOnMaps.find(id)!=s_monsterInTrainerOnMaps.end())
                flags.push_back("<div style=\"background-position:-16px -16px;\" class=\"flags flags16\" class=\"locationimg\"></div>");

            // Row
            indexBody << "<tr class=\"value\"";
            if(!bgColor.empty())
                indexBody << " style=\"background-color:" << bgColor << ";\"";
            indexBody << ">\n";

            // Column 1: monster icon
            std::string smallImg=smallImageUrlFrom(indexPage,id);
            indexBody << "<td>\n";
            if(!smallImg.empty())
                indexBody << "<div class=\"monstericon\"><a href=\"" << Helper::htmlEscape(monsterUrl)
                          << "\"><img src=\"" << Helper::htmlEscape(smallImg)
                          << "\" width=\"32\" height=\"32\" alt=\"" << Helper::htmlEscape(name)
                          << "\" title=\"" << Helper::htmlEscape(name) << "\" /></a></div>\n";
            indexBody << "</td>\n";

            // Column 2: flags
            if(!flags.empty())
            {
                indexBody << "<td class=\"monsterlistflag\">\n";
                for(const auto &f : flags)
                    indexBody << f;
                indexBody << "</td>\n";
            }

            // Column 3: name
            indexBody << "<td";
            if(flags.empty())
                indexBody << " colspan=\"2\"";
            indexBody << "><a href=\"" << Helper::htmlEscape(monsterUrl) << "\">"
                      << Helper::htmlEscape(name) << "</a>";
            indexBody << "</td>\n";

            // Column 4: types
            indexBody << "<td>\n";
            {
                std::vector<std::string> type_labels;
                for(const uint8_t tId : m.type)
                    type_labels.push_back(typeLabelHtml(tId)+"\n");
                std::string joined;
                for(size_t i=0;i<type_labels.size();i++)
                {
                    if(i>0) joined+=" ";
                    joined+=type_labels[i];
                }
                indexBody << "<div class=\"type_label_list\">" << joined << "</div>\n";
            }
            indexBody << "</td>\n";
            indexBody << "</tr>\n";
        }

        // Close last table
        indexBody << "<tr>\n\t<td colspan=\"4\" class=\"item_list_endline item_list_title_type_normal\"></td>\n</tr>\n</table>\n";

        Helper::writeHtml(indexPage,"Monsters list",indexBody.str());
    }
}

} // namespace GeneratorMonsters
