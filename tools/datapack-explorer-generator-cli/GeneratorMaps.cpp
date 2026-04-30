#include "GeneratorMaps.hpp"
#include "GeneratorItems.hpp"
#include "GeneratorMonsters.hpp"
#include "Helper.hpp"
#include "MapStore.hpp"

#include "../../client/libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include "../../general/base/CommonDatapackServerSpec.hpp"
#include "../../general/base/CommonMap/CommonMap.hpp"

#include <sstream>
#include <fstream>
#include <set>
#include <map>
#include <unordered_set>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <iostream>

namespace GeneratorMaps {

// ── Path cache ────────────────────────────────────────────────────────

static std::map<std::pair<size_t,size_t>,std::string> s_mapRefPath;

static std::string stripTmx(std::string p)
{
    if(p.size()>=4 && p.compare(p.size()-4,4,".tmx")==0)
        p=p.substr(0,p.size()-4);
    return p;
}

static std::string htmlPathFor(const std::string &mainCode, const std::string &stem)
{
    std::string s=stem;
    if(s.empty())
        s="unknown";
    return "map/"+mainCode+"/"+s+".html";
}

std::string relativePathForMapRef(size_t setIdx, size_t mapId)
{
    std::map<std::pair<size_t,size_t>, std::string>::const_iterator it=s_mapRefPath.find({setIdx,mapId});
    if(it!=s_mapRefPath.end())
        return it->second;
    const std::vector<MapStore::MainCodeSet> &sets=MapStore::sets();
    if(setIdx>=sets.size())
        return std::string();
    const MapStore::MainCodeSet &set=sets[setIdx];
    if(mapId>=set.mapPaths.size())
        return std::string();
    return htmlPathFor(set.mainCode,stripTmx(set.mapPaths[mapId]));
}

// ── Display-name helpers ──────────────────────────────────────────────

static std::string itemDisplayName(uint16_t id)
{
    if(QtDatapackClientLoader::datapackLoader->has_itemExtra(id) && !QtDatapackClientLoader::datapackLoader->get_itemExtra(id).name.empty())
        return QtDatapackClientLoader::datapackLoader->get_itemExtra(id).name;
    return "Item #"+Helper::toStringUint(id);
}

// ── Path / image helpers ──────────────────────────────────────────────

static void buildPaths()
{
    s_mapRefPath.clear();
    const std::vector<MapStore::MainCodeSet> &sets=MapStore::sets();
    for(size_t si=0;si<sets.size();++si)
    {
        const MapStore::MainCodeSet &set=sets[si];
        for(size_t mi=0;mi<set.mapPaths.size();++mi)
            s_mapRefPath[{si,mi}]=htmlPathFor(set.mainCode,stripTmx(set.mapPaths[mi]));
    }
}

static bool runMap2Png(const std::string &tmxAbs, const std::string &pngAbs)
{
    const std::string &bin=Helper::map2pngPath();
    if(bin.empty())
        return false;
    size_t pos=pngAbs.find_last_of('/');
    if(pos!=std::string::npos)
        Helper::mkpath(pngAbs.substr(0,pos));
    std::string cmd=bin+" -platform offscreen \"";
    cmd+=tmxAbs;
    cmd+="\" \"";
    cmd+=pngAbs;
    cmd+="\" >/dev/null 2>&1";
    int rc=std::system(cmd.c_str());
    if(rc!=0)
        return false;
    return Helper::fileExists(pngAbs);
}

// Parse start.xml for start map file names (same logic as GeneratorTree)
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

// Generate overview/preview PNGs and batch single-map renders (map_preview.php logic)
// Returns the number of overview/preview pairs generated.
static int generateMapPreviews()
{
    const std::string &bin=Helper::map2pngPath();
    if(bin.empty())
        return 0;

    const std::string mapsDir=Helper::localPath()+"maps/";
    Helper::mkpath(mapsDir);

    // Create subdirectories for all maps
    const std::vector<MapStore::MainCodeSet> &sets=MapStore::sets();
    for(size_t si=0;si<sets.size();++si)
    {
        for(size_t mi=0;mi<sets[si].mapPaths.size();++mi)
        {
            std::string mapImage=sets[si].mapPaths[mi];
            // Replace .tmx with .png
            if(mapImage.size()>=4 && mapImage.compare(mapImage.size()-4,4,".tmx")==0)
                mapImage=mapImage.substr(0,mapImage.size()-4)+".png";
            std::string fullPath=mapsDir+sets[si].mainCode+"/"+mapImage;
            size_t pos=fullPath.find_last_of('/');
            if(pos!=std::string::npos)
                Helper::mkpath(fullPath.substr(0,pos));
        }
    }

    int overviewCount=0;

    // Overview rendering for start maps (--renderAll)
    for(size_t si=0;si<sets.size();++si)
    {
        const std::string &mc=sets[si].mainCode;
        std::string startPath=Helper::datapackPath()+"map/main/"+mc+"/start.xml";
        std::vector<std::string> startMaps=parseStartMaps(startPath);

        for(const std::string &map : startMaps)
        {
            // Check map exists in the loaded maps
            bool found=false;
            for(size_t mi=0;mi<sets[si].mapPaths.size();++mi)
            {
                if(sets[si].mapPaths[mi]==map)
                { found=true; break; }
            }
            if(!found)
            {
                std::cout << "map for starter " << map << " missing" << std::endl;
                continue;
            }

            overviewCount++;
            std::string overviewPng=mapsDir+"overview-"+std::to_string(overviewCount)+".png";
            std::string previewPng=mapsDir+"preview-"+std::to_string(overviewCount)+".png";

            // Remove old files
            std::remove(overviewPng.c_str());
            std::remove(previewPng.c_str());

            std::string tmxAbs=Helper::datapackPath()+"map/main/"+mc+"/"+map;
            std::string cmd=bin+" -platform offscreen \""+tmxAbs+"\" \""+overviewPng+"\" --renderAll >/dev/null 2>&1";
            std::cout << "Generating overview " << overviewCount << "..." << std::flush;
            int rc=std::system(cmd.c_str());
            if(rc!=0)
            {
                std::cout << " failed (exit " << rc << ")" << std::endl;
                std::cout << "Bug: cd " << mapsDir << " && " << bin
                          << " -platform offscreen \"" << tmxAbs << "\" \""
                          << overviewPng << "\" --renderAll" << std::endl;
                overviewCount--;
                continue;
            }
            std::cout << " done" << std::endl;

            // Resize overview to preview (256x256) using ImageMagick convert
            if(Helper::fileExists(overviewPng))
            {
                if(Helper::fileExists("/usr/bin/convert"))
                {
                    std::string resizeCmd="/usr/bin/ionice -c 3 /usr/bin/nice -n 19 /usr/bin/convert"
                        " -limit memory 2GiB -limit map 2GiB -limit disk 4GiB \""
                        +overviewPng+"\" -resize 256x256 \""+previewPng+"\" >/dev/null 2>&1";
                    if(std::system(resizeCmd.c_str())!=0)
                        std::cout << "convert resize failed" << std::endl;
                }
                else
                    std::cout << "no /usr/bin/convert found, install imagemagick" << std::endl;
            }
            else
            {
                std::cout << "overview.png not found" << std::endl;
                overviewCount--;
            }
        }
    }

    // Batch single map render for all maps under map/main/
    {
        std::string cmd="cd \""+mapsDir+"\" && "+bin+" -platform offscreen \""
            +Helper::datapackPath()+"map/main/\" >/dev/null 2>&1";
        std::cout << "Generating single map previews..." << std::flush;
        int rc=std::system(cmd.c_str());
        if(rc!=0)
            std::cout << " failed (exit " << rc << ")" << std::endl;
        else
            std::cout << " done" << std::endl;
    }

    // Trim all PNGs with mogrify
    if(Helper::fileExists("/usr/bin/mogrify"))
    {
        std::string cmd="/usr/bin/find \""+mapsDir+"\" -name '*.png' -exec"
            " /usr/bin/ionice -c 3 /usr/bin/nice -n 19 /usr/bin/mogrify -trim +repage {} \\; >/dev/null 2>&1";
        std::cout << "Trimming PNGs..." << std::flush;
        if(std::system(cmd.c_str())!=0)
            std::cout << "mogrify trim failed" << std::endl;
        std::cout << " done" << std::endl;
    }
    else
        std::cout << "no /usr/bin/mogrify found, install imagemagick" << std::endl;

    return overviewCount;
}

// ── Map metadata (from sibling XML + computed from CommonMap) ─────────

struct FightMonsterEntry { uint16_t id; uint8_t level; };
struct FightItemEntry { uint16_t id; uint32_t quantity; };

struct BotStepData {
    std::string type; // shop, fight, heal, learn, warehouse, market, quests, clan, sell, zonecapture, industry, text
    std::string shopId;
    uint8_t fightLocalId;
    bool isLeader;
    std::string industryId;
    std::unordered_map<uint16_t,uint32_t> shopProducts; // item→price, parsed from XML
    // Fight data parsed directly from XML (engine's Map_loader has case-sensitivity bug)
    std::vector<FightMonsterEntry> fightMonsters;
    std::vector<FightItemEntry> fightItems;
    uint32_t fightCash;
};

struct BotData {
    uint8_t localId;
    std::string name;
    std::string skinName; // from TMX via unknownBotStepBuffer or XML
    bool onlyText;
    std::vector<BotStepData> steps;
};

struct WildMonsterXmlEntry {
    uint16_t id;
    uint8_t minLevel, maxLevel;
    uint8_t luck;
};

struct WildMonsterLayerData {
    std::string layerName; // Grass, Water, Cave, etc.
    std::vector<WildMonsterXmlEntry> monsters;
};

struct MapMeta {
    std::string name;
    std::string zoneCode;
    std::string type;
    std::string shortDesc;
    std::string description;
    double averageWildLevel;
    int maxFightLevel;
    std::set<std::string> flags;
    std::vector<BotData> bots;
    std::vector<WildMonsterLayerData> wildLayers;
    // TMX "door" positions (source_x,source_y) — used to label "Door" vs "Teleporter"
    std::set<std::pair<uint8_t,uint8_t>> doorPositions;
};

static std::map<std::pair<size_t,size_t>,MapMeta> s_mapMeta;
static std::map<std::pair<std::string,std::string>,std::string> s_zoneNames;

// Parse English (or first available) text from a named child element
// that may carry an optional lang="..." attribute.
static std::string parseLocalizedText(const tinyxml2::XMLElement *parent, const char *elementName)
{
    if(!parent) return std::string();
    std::string firstText;
    const tinyxml2::XMLElement *el=parent->FirstChildElement(elementName);
    while(el)
    {
        const char *lang=el->Attribute("lang");
        const char *text=el->GetText();
        if(text)
        {
            if(!lang || std::string(lang)=="en")
                return text;
            if(firstText.empty())
                firstText=text;
        }
        el=el->NextSiblingElement(elementName);
    }
    return firstText;
}

static std::string getZoneName(const std::string &mainCode, const std::string &zoneCode)
{
    if(zoneCode.empty()) return "Unknown zone";
    std::pair<std::string, std::string> key=std::make_pair(mainCode,zoneCode);
    std::map<std::pair<std::string,std::string>, std::string>::const_iterator it=s_zoneNames.find(key);
    if(it!=s_zoneNames.end()) return it->second;

    std::string result=zoneCode;
    std::string xmlPath=Helper::datapackPath()+"map/main/"+mainCode+"/zone/"+zoneCode+".xml";
    tinyxml2::XMLDocument doc;
    if(doc.LoadFile(xmlPath.c_str())==tinyxml2::XML_SUCCESS)
    {
        std::string n=parseLocalizedText(doc.RootElement(),"name");
        if(!n.empty()) result=n;
    }
    s_zoneNames[key]=result;
    return result;
}

static MapMeta computeMapMeta(const CatchChallenger::CommonMap &m,
                               const std::string &mainCode,
                               const std::string &mapStem)
{
    MapMeta meta;
    meta.averageWildLevel=0;
    meta.maxFightLevel=0;

    // Parse sibling XML for name / zone / type / descriptions
    std::string xmlPath=Helper::datapackPath()+"map/main/"+mainCode+"/"+mapStem+".xml";
    tinyxml2::XMLDocument doc;
    if(doc.LoadFile(xmlPath.c_str())==tinyxml2::XML_SUCCESS)
    {
        const tinyxml2::XMLElement *root=doc.RootElement();
        if(root)
        {
            const char *zone=root->Attribute("zone");
            if(zone) meta.zoneCode=zone;
            const char *type=root->Attribute("type");
            if(type) meta.type=type;
            meta.name=parseLocalizedText(root,"name");
            meta.shortDesc=parseLocalizedText(root,"shortdescription");
            meta.description=parseLocalizedText(root,"description");
        }
    }

    // Fallback name: last path component of mapStem
    if(meta.name.empty())
    {
        size_t pos=mapStem.find_last_of('/');
        meta.name=(pos!=std::string::npos)?mapStem.substr(pos+1):mapStem;
    }

    // Parse wild monster layers from XML (CATCHCHALLENGER_ONLYMAPRENDER leaves zones empty)
    {
        static const char *s_wildTypes[]={"grass","water","cave","waterRod","waterSuperRod","lava",nullptr};
        static const char *s_wildNames[]={"Grass","Water","Cave","Water (Rod)","Water (Super Rod)","Lava",nullptr};
        double totalLevel=0;
        int totalCount=0;
        if(doc.RootElement())
        {
            for(int t=0;s_wildTypes[t];t++)
            {
                const tinyxml2::XMLElement *layer=doc.RootElement()->FirstChildElement(s_wildTypes[t]);
                if(!layer) continue;
                WildMonsterLayerData layerData;
                layerData.layerName=s_wildNames[t];
                const tinyxml2::XMLElement *mon=layer->FirstChildElement("monster");
                while(mon)
                {
                    WildMonsterXmlEntry e;
                    e.id=0; e.minLevel=0; e.maxLevel=0; e.luck=0;
                    if(mon->Attribute("id"))
                        e.id=(uint16_t)std::atoi(mon->Attribute("id"));
                    if(mon->Attribute("minLevel") && mon->Attribute("maxLevel"))
                    {
                        e.minLevel=(uint8_t)std::atoi(mon->Attribute("minLevel"));
                        e.maxLevel=(uint8_t)std::atoi(mon->Attribute("maxLevel"));
                    }
                    else if(mon->Attribute("level"))
                    {
                        e.minLevel=e.maxLevel=(uint8_t)std::atoi(mon->Attribute("level"));
                    }
                    if(mon->Attribute("luck"))
                        e.luck=(uint8_t)std::atoi(mon->Attribute("luck"));
                    if(e.id>0 && e.minLevel>0 && e.maxLevel>0)
                    {
                        layerData.monsters.push_back(e);
                        totalLevel+=(e.minLevel+e.maxLevel)/2.0;
                        totalCount++;
                    }
                    mon=mon->NextSiblingElement("monster");
                }
                if(!layerData.monsters.empty())
                    meta.wildLayers.push_back(std::move(layerData));
            }
        }
        if(totalCount>0)
            meta.averageWildLevel=totalLevel/totalCount;
    }

    // Compute maxFightLevel from XML bot fight monster levels
    if(doc.RootElement())
    {
        const tinyxml2::XMLElement *bot=doc.RootElement()->FirstChildElement("bot");
        while(bot)
        {
            const tinyxml2::XMLElement *step=bot->FirstChildElement("step");
            while(step)
            {
                if(step->Attribute("type") && strcmp(step->Attribute("type"),"fight")==0)
                {
                    const tinyxml2::XMLElement *mon=step->FirstChildElement("monster");
                    while(mon)
                    {
                        if(mon->Attribute("level"))
                        {
                            int lvl=std::atoi(mon->Attribute("level"));
                            if(lvl>meta.maxFightLevel)
                                meta.maxFightLevel=lvl;
                        }
                        mon=mon->NextSiblingElement("monster");
                    }
                }
                step=step->NextSiblingElement("step");
            }
            bot=bot->NextSiblingElement("bot");
        }
    }

    // Parse TMX for bot skins and door positions
    std::unordered_map<uint8_t,std::string> botIdToSkin;
    {
        std::string tmxPath=Helper::datapackPath()+"map/main/"+mainCode+"/"+mapStem+".tmx";
        tinyxml2::XMLDocument tmxDoc;
        if(tmxDoc.LoadFile(tmxPath.c_str())==tinyxml2::XML_SUCCESS && tmxDoc.RootElement())
        {
            int tileWidth=16, tileHeight=16;
            if(tmxDoc.RootElement()->Attribute("tilewidth"))
                tileWidth=std::atoi(tmxDoc.RootElement()->Attribute("tilewidth"));
            if(tmxDoc.RootElement()->Attribute("tileheight"))
                tileHeight=std::atoi(tmxDoc.RootElement()->Attribute("tileheight"));

            const tinyxml2::XMLElement *og=tmxDoc.RootElement()->FirstChildElement("objectgroup");
            while(og)
            {
                const tinyxml2::XMLElement *obj=og->FirstChildElement("object");
                while(obj)
                {
                    const char *otype=obj->Attribute("type");
                    if(otype)
                    {
                        std::string typeStr(otype);
                        if(typeStr=="bot")
                        {
                            const tinyxml2::XMLElement *props=obj->FirstChildElement("properties");
                            if(props)
                            {
                                std::string skinVal;
                                uint8_t botId=0;
                                bool hasId=false;
                                const tinyxml2::XMLElement *p=props->FirstChildElement("property");
                                while(p)
                                {
                                    const char *pn=p->Attribute("name");
                                    const char *pv=p->Attribute("value");
                                    if(pn && pv)
                                    {
                                        if(std::string(pn)=="id") { botId=(uint8_t)std::atoi(pv); hasId=true; }
                                        else if(std::string(pn)=="skin") skinVal=pv;
                                    }
                                    p=p->NextSiblingElement("property");
                                }
                                if(hasId && !skinVal.empty())
                                    botIdToSkin[botId]=skinVal;
                            }
                        }
                        else if(typeStr=="door")
                        {
                            // "door" type objects → Door label (vs Teleporter)
                            int px=0, py=0;
                            if(obj->Attribute("x")) px=std::atoi(obj->Attribute("x"));
                            if(obj->Attribute("y")) py=std::atoi(obj->Attribute("y"));
                            // TMX objects with gid have y at bottom; adjust
                            if(obj->Attribute("gid"))
                                py-=tileHeight;
                            uint8_t tx=(uint8_t)(px/tileWidth);
                            uint8_t ty=(uint8_t)(py/tileHeight);
                            meta.doorPositions.insert({tx,ty});
                        }
                    }
                    obj=obj->NextSiblingElement("object");
                }
                og=og->NextSiblingElement("objectgroup");
            }
        }
    }

    // Parse bot data from XML
    if(doc.RootElement())
    {
        const tinyxml2::XMLElement *botEl=doc.RootElement()->FirstChildElement("bot");
        while(botEl)
        {
            BotData bd;
            bd.localId=0;
            bd.onlyText=true;
            if(botEl->Attribute("id"))
                bd.localId=(uint8_t)std::atoi(botEl->Attribute("id"));
            bd.name=parseLocalizedText(botEl,"name");
            if(bd.name.empty())
                bd.name="Bot #"+Helper::toStringUint(bd.localId);
            // Parse skin: first try XML attribute, then TMX
            if(botEl->Attribute("skin"))
                bd.skinName=botEl->Attribute("skin");
            else if(botIdToSkin.find(bd.localId)!=botIdToSkin.end())
                bd.skinName=botIdToSkin[bd.localId];

            const tinyxml2::XMLElement *stepEl=botEl->FirstChildElement("step");
            while(stepEl)
            {
                BotStepData sd;
                sd.fightLocalId=0;
                sd.isLeader=false;
                sd.fightCash=0;
                const char *t=stepEl->Attribute("type");
                if(t) sd.type=t;
                if(stepEl->Attribute("shop"))
                    sd.shopId=stepEl->Attribute("shop");
                if(stepEl->Attribute("fightid"))
                    sd.fightLocalId=(uint8_t)std::atoi(stepEl->Attribute("fightid"));
                if(stepEl->Attribute("leader"))
                    sd.isLeader=true;
                if(stepEl->Attribute("industry"))
                    sd.industryId=stepEl->Attribute("industry");

                // Helper: resolve item name or numeric id
                std::function<uint16_t(const char *)> resolveItemId=[](const char *attr) -> uint16_t {
                    if(!attr) return 0;
                    // Try numeric first
                    char *end=nullptr;
                    long v=std::strtol(attr,&end,10);
                    if(end!=attr && *end=='\0' && v>0)
                    {
                        uint16_t id=(uint16_t)v;
                        if(CatchChallenger::CommonDatapack::commonDatapack.has_item(id))
                            return id;
                    }
                    // Try name lookup (lowercase, like engine)
                    std::string lower=attr;
                    for(size_t ci=0;ci<lower.size();ci++) lower[ci]=(char)std::tolower((unsigned char)lower[ci]);
                    if(CatchChallenger::CommonDatapack::commonDatapack.has_tempNameToItemId(lower))
                        return CatchChallenger::CommonDatapack::commonDatapack.get_tempNameToItemId(lower);
                    return 0;
                };
                // Helper: resolve monster name or numeric id
                std::function<uint16_t(const char *)> resolveMonsterId=[](const char *attr) -> uint16_t {
                    if(!attr) return 0;
                    char *end=nullptr;
                    long v=std::strtol(attr,&end,10);
                    if(end!=attr && *end=='\0' && v>0)
                    {
                        uint16_t id=(uint16_t)v;
                        if(CatchChallenger::CommonDatapack::commonDatapack.has_monster(id))
                            return id;
                    }
                    std::string lower=attr;
                    for(size_t ci=0;ci<lower.size();ci++) lower[ci]=(char)std::tolower((unsigned char)lower[ci]);
                    if(CatchChallenger::CommonDatapack::commonDatapack.has_tempNameToMonsterId(lower))
                        return CatchChallenger::CommonDatapack::commonDatapack.get_tempNameToMonsterId(lower);
                    return 0;
                };

                // Parse shop products inline from XML
                if(sd.type=="shop")
                {
                    const tinyxml2::XMLElement *product=stepEl->FirstChildElement("product");
                    while(product)
                    {
                        uint16_t itemId=resolveItemId(product->Attribute("item"));
                        if(itemId>0)
                        {
                            uint32_t price=CatchChallenger::CommonDatapack::commonDatapack.get_item(itemId).price;
                            if(product->Attribute("overridePrice"))
                                price=(uint32_t)std::atoi(product->Attribute("overridePrice"));
                            sd.shopProducts[itemId]=price;
                        }
                        product=product->NextSiblingElement("product");
                    }
                }
                // Parse fight data from XML (monsters, cash, items)
                if(sd.type=="fight")
                {
                    // Monsters
                    const tinyxml2::XMLElement *monEl=stepEl->FirstChildElement("monster");
                    while(monEl)
                    {
                        uint16_t mid=resolveMonsterId(monEl->Attribute("id"));
                        if(mid>0)
                        {
                            FightMonsterEntry fme;
                            fme.id=mid;
                            fme.level=1;
                            if(monEl->Attribute("level"))
                                fme.level=(uint8_t)std::atoi(monEl->Attribute("level"));
                            sd.fightMonsters.push_back(fme);
                        }
                        monEl=monEl->NextSiblingElement("monster");
                    }
                    // Cash and items from <gain>
                    const tinyxml2::XMLElement *gainEl=stepEl->FirstChildElement("gain");
                    while(gainEl)
                    {
                        if(gainEl->Attribute("cash"))
                            sd.fightCash=(uint32_t)std::atoi(gainEl->Attribute("cash"));
                        uint16_t gItemId=resolveItemId(gainEl->Attribute("item"));
                        if(gItemId>0)
                        {
                            FightItemEntry fie;
                            fie.id=gItemId;
                            fie.quantity=1;
                            if(gainEl->Attribute("quantity"))
                                fie.quantity=(uint32_t)std::atoi(gainEl->Attribute("quantity"));
                            sd.fightItems.push_back(fie);
                        }
                        gainEl=gainEl->NextSiblingElement("gain");
                    }
                }
                if(sd.type!="text")
                    bd.onlyText=false;
                bd.steps.push_back(std::move(sd));
                stepEl=stepEl->NextSiblingElement("step");
            }
            meta.bots.push_back(std::move(bd));
            botEl=botEl->NextSiblingElement("bot");
        }
    }

    // Detect flags from CommonMap data + XML
    if(!m.shops.empty()) meta.flags.insert("shop");
    if(!m.botFights.empty()) meta.flags.insert("fight");
    if(!m.industries.empty()) meta.flags.insert("industry");

    // Detect additional flags from XML bot step types
    if(doc.RootElement())
    {
        const tinyxml2::XMLElement *bot=doc.RootElement()->FirstChildElement("bot");
        while(bot)
        {
            const tinyxml2::XMLElement *step=bot->FirstChildElement("step");
            while(step)
            {
                const char *t=step->Attribute("type");
                if(t)
                {
                    if(strcmp(t,"shop")==0) meta.flags.insert("shop");
                    else if(strcmp(t,"heal")==0) meta.flags.insert("heal");
                    else if(strcmp(t,"learn")==0) meta.flags.insert("learn");
                    else if(strcmp(t,"warehouse")==0) meta.flags.insert("warehouse");
                    else if(strcmp(t,"clan")==0) meta.flags.insert("clan");
                    else if(strcmp(t,"sell")==0) meta.flags.insert("sell");
                    else if(strcmp(t,"market")==0) meta.flags.insert("market");
                    else if(strcmp(t,"quests")==0) meta.flags.insert("quests");
                    else if(strcmp(t,"zonecapture")==0) meta.flags.insert("zonecapture");
                }
                step=step->NextSiblingElement("step");
            }
            bot=bot->NextSiblingElement("bot");
        }
    }

    return meta;
}

// ── Level colour ranges (replicates PHP bucket algorithm exactly) ─────

struct LevelRange { int lo; int hi; };

static std::vector<LevelRange> computeLevelRanges(int minLvl, int maxLvl)
{
    std::vector<LevelRange> r;
    if(minLvl<=0 || (maxLvl-minLvl)<4)
        return r;
    int full=maxLvl-minLvl;
    int n=(int)std::floor((double)full/2.0);
    if(n>4) n=4;
    int p=full/n;
    r.push_back({minLvl, minLvl+p});
    r.push_back({minLvl+p+1, minLvl+p*2});
    if(n>=3)
    {
        r.push_back({minLvl+p*2+1, minLvl+p*3});
        // PHP replicates the >=3 check for range[3] (always created when >=3)
        r.push_back({minLvl+p*3+1, minLvl+p*4});
    }
    return r;
}

static const char *s_levelColors[]={"#e5eaff","#e0ffdd","#fbfdd3","#ffe5e5"};

static std::string colorForLevel(int level, const std::vector<LevelRange> &ranges)
{
    if(level<=0 || ranges.empty() || level<ranges[0].lo)
        return "white";
    for(size_t i=0;i<ranges.size();i++)
        if(level>=ranges[i].lo && level<ranges[i].hi)
            return s_levelColors[i];
    return "#ffe5e5";
}

// ── Flag icon rendering ──────────────────────────────────────────────

struct FlagDef { const char *key; const char *bgPos; const char *title; };
static const FlagDef s_flagDefs[]={
    {"shop",        "-32px 0px",   "Shop"},
    {"fight",       "-16px -16px", "Fight"},
    {"heal",        "0px 0px",     "Heal"},
    {"learn",       "-48px 0px",   "Learn"},
    {"warehouse",   "0px -16px",   "Warehouse"},
    {"market",      "0px -16px",   "Market"},
    {"clan",        "-48px -16px", "Clan"},
    {"sell",        "-32px 0px",   "Sell"},
    {"zonecapture", "-32px -16px", "Zone capture"},
    {"industry",    "0px -32px",   "Industry"},
    {"quests",      "-16px 0px",   "Quests"},
};

static void writeFlagDivs(std::ostringstream &out, const std::set<std::string> &flags)
{
    for(const GeneratorMaps::FlagDef &fd : s_flagDefs)
        if(flags.count(fd.key))
            out << "<div style=\"float:left;background-position:" << fd.bgPos
                << ";\" class=\"flags flags16\" title=\"" << fd.title << "\"></div>\n";
}

// ── Item image helper ─────────────────────────────────────────────────

static std::string itemImageUrl(uint16_t itemId)
{
    if(!QtDatapackClientLoader::datapackLoader->has_itemExtra(itemId))
        return std::string();
    const DatapackClientLoader::ItemExtra &itemEx=QtDatapackClientLoader::datapackLoader->get_itemExtra(itemId);
    if(itemEx.imagePath.empty())
        return std::string();
    std::string rel=Helper::relativeFromDatapack(itemEx.imagePath);
    if(rel.empty() || !Helper::fileExists(Helper::datapackPath()+rel))
        return std::string();
    return Helper::relUrl(Helper::publishDatapackFile(rel));
}

// Write standard item icon + name table cells (used in items/drops/shops)
static void writeItemIconAndName(std::ostringstream &body, uint16_t itemId, const std::string &prefix=std::string())
{
    std::string name;
    if(QtDatapackClientLoader::datapackLoader->has_itemExtra(itemId))
        name=QtDatapackClientLoader::datapackLoader->get_itemExtra(itemId).name;
    else
        name="Unknown item";
    std::string link=Helper::relUrl(GeneratorItems::relativePathForItem(itemId));
    std::string image=itemImageUrl(itemId);

    body << "<td>\n";
    if(!image.empty())
    {
        if(!link.empty())
            body << "<a href=\"" << Helper::htmlEscape(link) << "\">\n";
        body << "<img src=\"" << image << "\" width=\"24\" height=\"24\" alt=\""
             << Helper::htmlEscape(name) << "\" title=\"" << Helper::htmlEscape(name) << "\" />\n";
        if(!link.empty())
            body << "</a>\n";
    }
    body << "</td>\n<td>\n";
    if(!link.empty())
        body << "<a href=\"" << Helper::htmlEscape(link) << "\">\n";
    body << prefix << Helper::htmlEscape(name);
    if(!link.empty())
        body << "</a>\n";
    body << "</td>\n";
}

// ── Monster card display (matches PHP monsterAndLevelToDisplay) ──────

static void monsterAndLevelToDisplay(std::ostringstream &body, uint16_t monsterId, uint8_t level, bool full)
{
    if(!CatchChallenger::CommonDatapack::commonDatapack.has_monster(monsterId))
        return;
    const CatchChallenger::Monster &monsterDef=CatchChallenger::CommonDatapack::commonDatapack.get_monster(monsterId);
    std::string mname=QtDatapackClientLoader::datapackLoader->has_monsterExtra(monsterId)?QtDatapackClientLoader::datapackLoader->get_monsterExtra(monsterId).name:("Monster #"+Helper::toStringUint(monsterId));
    std::string monLink=Helper::relUrl(GeneratorMonsters::relativePathForMonster(monsterId));

    // Determine the first type for CSS class
    std::string firstType="normal";
    if(!monsterDef.type.empty())
    {
        if(QtDatapackClientLoader::datapackLoader->has_typeExtra(monsterDef.type[0]) && !QtDatapackClientLoader::datapackLoader->get_typeExtra(monsterDef.type[0]).name.empty())
            firstType=QtDatapackClientLoader::datapackLoader->get_typeExtra(monsterDef.type[0]).name;
        else
            firstType=Helper::toStringUint(monsterDef.type[0]);
    }
    // Use type number for CSS class to match PHP: type_label_TYPE
    std::string firstTypeCss=monsterDef.type.empty()?"0":Helper::toStringUint(monsterDef.type[0]);

    body << "<table class=\"item_list item_list_type_" << Helper::htmlEscape(firstTypeCss) << " map_list\">\n"
         << "<tr class=\"item_list_title item_list_title_type_" << Helper::htmlEscape(firstTypeCss) << "\">\n"
         << "    <th";
    if(!full) body << " colspan=\"3\"";
    body << "></th>\n</tr>";

    body << "<tr class=\"value\">";
    if(full)
    {
        body << "<td>";
        body << "<table class=\"monsterforevolution\">";

        // Front image
        std::string frontPng="monsters/"+Helper::toStringUint(monsterId)+"/front.png";
        std::string frontGif="monsters/"+Helper::toStringUint(monsterId)+"/front.gif";
        if(Helper::fileExists(Helper::datapackPath()+frontPng))
        {
            std::string imgUrl=Helper::relUrl(Helper::publishDatapackFile(frontPng));
            body << "<tr><td><center><a href=\"" << Helper::htmlEscape(monLink)
                 << "\"><img src=\"" << imgUrl << "\" width=\"80\" height=\"80\" alt=\""
                 << Helper::htmlEscape(mname) << "\" title=\"" << Helper::htmlEscape(mname)
                 << "\" /></a></center></td></tr>";
        }
        else if(Helper::fileExists(Helper::datapackPath()+frontGif))
        {
            std::string imgUrl=Helper::relUrl(Helper::publishDatapackFile(frontGif));
            body << "<tr><td><center><a href=\"" << Helper::htmlEscape(monLink)
                 << "\"><img src=\"" << imgUrl << "\" width=\"80\" height=\"80\" alt=\""
                 << Helper::htmlEscape(mname) << "\" title=\"" << Helper::htmlEscape(mname)
                 << "\" /></a></center></td></tr>";
        }

        // Monster name
        body << "<tr><td class=\"evolution_name\"><a href=\"" << Helper::htmlEscape(monLink)
             << "\">" << Helper::htmlEscape(mname) << "</a></td></tr>";

        // Type badges
        body << "<tr><td>";
        body << "<div class=\"type_label_list\">";
        bool firstTypeLabel=true;
        for(const uint8_t &t : monsterDef.type)
        {
            if(QtDatapackClientLoader::datapackLoader->has_typeExtra(t))
            {
                if(!firstTypeLabel) body << " ";
                firstTypeLabel=false;
                std::string tname=QtDatapackClientLoader::datapackLoader->get_typeExtra(t).name;
                // Capitalize first letter
                if(!tname.empty()) tname[0]=(char)std::toupper((unsigned char)tname[0]);
                body << "<span class=\"type_label type_label_" << Helper::toStringUint(t)
                     << "\"><a href=\"" << Helper::htmlEscape(Helper::relUrl("monsters/type-"+Helper::toStringUint(t)+".html"))
                     << "\">" << Helper::htmlEscape(tname) << "</a></span>";
            }
        }
        body << "</div></td></tr>";

        // Level
        body << "<tr><td>Level " << (unsigned)level << "</td></tr>";
        body << "</table>";
        body << "</td>";
    }
    else
    {
        // Small mode: small icon + name + level
        std::string smallPng="monsters/"+Helper::toStringUint(monsterId)+"/small.png";
        std::string smallGif="monsters/"+Helper::toStringUint(monsterId)+"/small.gif";
        if(Helper::fileExists(Helper::datapackPath()+smallPng))
        {
            std::string imgUrl=Helper::relUrl(Helper::publishDatapackFile(smallPng));
            body << "<td><center><a href=\"" << Helper::htmlEscape(monLink)
                 << "\"><img src=\"" << imgUrl << "\" width=\"32\" height=\"32\" alt=\""
                 << Helper::htmlEscape(mname) << "\" title=\"" << Helper::htmlEscape(mname)
                 << "\" /></a></center></td>";
        }
        body << "<td class=\"evolution_name\"><a href=\"" << Helper::htmlEscape(monLink)
             << "\">" << Helper::htmlEscape(mname) << "</a></td>";
        body << "<td>Level " << (unsigned)level << "</td>";
    }
    body << "</tr>";

    // Endline
    body << "<tr>\n    <th class=\"item_list_endline item_list_title item_list_title_type_"
         << Helper::htmlEscape(firstTypeCss) << "\"";
    if(!full) body << " colspan=\"3\"";
    body << ">";
    body << "</th>\n</tr>\n</table>";
}

// ── Bot skin sprite helper ──────────────────────────────────────────

static std::string findBotSkin(const std::string &skinName)
{
    if(skinName.empty())
        return std::string();
    // Check skin/bot/NAME/trainer.png, .gif, then skin/fighter/NAME/trainer.png, .gif
    static const char *dirs[]={"skin/bot/","skin/fighter/",nullptr};
    static const char *exts[]={"/trainer.png","/trainer.gif",nullptr};
    for(int d=0;dirs[d];d++)
        for(int e=0;exts[e];e++)
        {
            std::string rel=std::string(dirs[d])+skinName+exts[e];
            if(Helper::fileExists(Helper::datapackPath()+rel))
                return Helper::relUrl(Helper::publishDatapackFile(rel));
        }
    return std::string();
}

static void writeBotSkinTd(std::ostringstream &body, const std::string &skinUrl, bool center=false)
{
    body << "<td>";
    if(center) body << "<center>";
    body << "<div style=\"width:16px;height:24px;background-image:url('" << skinUrl
         << "');background-repeat:no-repeat;background-position:-16px -48px;\"></div>";
    if(center) body << "</center>";
    body << "</td>\n";
}

// ── Location image helper ────────────────────────────────────────────

static std::string locationImageName(const std::string &layerName)
{
    // Map layer names to image filenames (matching PHP $full_monsterType_name_top)
    if(layerName=="Grass") return "Grass";
    if(layerName=="Water") return "Water";
    if(layerName=="Cave") return "Cave";
    if(layerName=="Lava") return "Lava";
    if(layerName=="Water (Rod)") return "Water";
    if(layerName=="Water (Super Rod)") return "Water";
    return layerName;
}

static void ensureLocationImages()
{
    // Copy location images from generator source to output if not present
    static const char *imgs[]={"Grass","Water","Cave","Lava","GrassUp",nullptr};
    std::string srcDir=std::string(SRC_DIR)+"/images/datapack-explorer/";
    for(int i=0;imgs[i];i++)
    {
        std::string name=std::string(imgs[i])+".png";
        std::string dst=Helper::localPath()+"images/datapack-explorer/"+name;
        if(!Helper::fileExists(dst))
        {
            std::string src=srcDir+name;
            if(Helper::fileExists(src))
            {
                Helper::mkpath(Helper::localPath()+"images/datapack-explorer");
                Helper::copyFile(src,dst);
            }
        }
    }
}

// ── Bot pages ─────────────────────────────────────────────────────────

struct BotPage {
    uint8_t botId;
    std::string rel;
    std::string label;
};

static std::vector<BotPage> generateBotPages(
        const std::vector<BotData> &bots,
        const CatchChallenger::CommonMap &m,
        const std::string &mainCode,
        const std::string &mapStem,
        const std::string &mapDisplayName,
        const std::string &mapZoneCode)
{
    std::vector<BotPage> out;
    const std::string botsFolder="map/"+mainCode+"/"+mapStem+"/bots/";
    std::set<std::string> used;
    for(const BotData &bd : bots)
    {
        // Generate pages for bots with non-text steps or with a skin
        bool hasNonText=false;
        for(const BotStepData &step : bd.steps)
            if(step.type!="text") { hasNonText=true; break; }
        std::string skinUrl=findBotSkin(bd.skinName);
        if(!hasNonText && skinUrl.empty())
            continue;

        std::string stem="bot-"+Helper::toStringUint(bd.localId);
        std::string rel=botsFolder+stem+".html";
        int n=2;
        while(used.count(rel)>0)
        {
            rel=botsFolder+stem+"-"+Helper::toStringInt(n)+".html";
            n++;
        }
        used.insert(rel);
        std::string label=bd.name;
        if(label.empty())
            label="Bot #"+Helper::toStringUint(bd.localId);
        Helper::setCurrentPage(rel);

        std::ostringstream body;
        body << "<div class=\"map item_details\">\n";

        // Name heading + bot ID sub-heading
        body << "<div class=\"subblock\"><h1>" << Helper::htmlEscape(label) << "</h1>\n";
        if(!bd.name.empty())
            body << "<h2>Bot #" << (unsigned)bd.localId << "</h2>\n";

        // Trainer sprite
        if(!skinUrl.empty())
            body << "<center><h2><div style=\"width:16px;height:24px;background-image:url('"
                 << skinUrl << "');background-repeat:no-repeat;background-position:-16px -48px;\" title=\"Skin: "
                 << Helper::htmlEscape(bd.skinName) << "\"></div></h2></center>\n";
        body << "</div>\n";

        // Front sprite
        if(!bd.skinName.empty())
        {
            static const char *dirs[]={"skin/bot/","skin/fighter/",nullptr};
            static const char *exts[]={"/front.png","/front.gif",nullptr};
            for(int d=0;dirs[d];d++)
                for(int e=0;exts[e];e++)
                {
                    std::string frel=std::string(dirs[d])+bd.skinName+exts[e];
                    if(Helper::fileExists(Helper::datapackPath()+frel))
                    {
                        std::string furl=Helper::relUrl(Helper::publishDatapackFile(frel));
                        body << "<div class=\"value datapackscreenshot\"><center>\n"
                             << "<img src=\"" << furl << "\" width=\"80\" height=\"80\" alt=\"\" />\n"
                             << "</center></div>\n";
                        goto frontDone;
                    }
                }
        }
        frontDone:;

        // Map link + zone
        {
            std::string mapPageRel="map/"+mainCode+"/"+mapStem+".html";
            body << "<div class=\"subblock\"><div class=\"valuetitle\">Map</div><div class=\"value\">\n";
            body << "<a href=\"" << Helper::htmlEscape(Helper::relUrl(mapPageRel))
                 << "\" title=\"" << Helper::htmlEscape(mapDisplayName) << "\">"
                 << Helper::htmlEscape(mapDisplayName) << "</a>&nbsp;\n";
            if(!mapZoneCode.empty())
            {
                std::map<std::pair<std::string,std::string>, std::string>::const_iterator zit=s_zoneNames.find({mainCode,mapZoneCode});
                if(zit!=s_zoneNames.end())
                {
                    body << "(Zone: <a href=\"" << Helper::htmlEscape(Helper::relUrl("zones/"+mainCode+"/"+Helper::textForUrl(zit->second)+".html"))
                         << "\" title=\"" << Helper::htmlEscape(zit->second) << "\">"
                         << Helper::htmlEscape(zit->second) << "</a>)\n";
                }
            }
            body << "</div></div>\n";
        }

        // Step details
        int stepIdx=0;
        for(const BotStepData &step : bd.steps)
        {
            stepIdx++;
            if(step.type=="text")
            {
                body << "<div class=\"subblock\"><div class=\"valuetitle\" id=\"step" << stepIdx << "\">Text</div><div class=\"value\">\n";
                body << "</div></div>\n";
            }
            else if(step.type=="shop")
            {
                body << "<div class=\"subblock\"><div class=\"valuetitle\" id=\"step" << stepIdx << "\">Shop</div>\n"
                     << "<center><div style=\"background-position:-32px 0px;\" class=\"flags flags16\"></div></center>\n"
                     << "<div class=\"value\">\n";
                if(!step.shopProducts.empty())
                {
                    body << "<center><table class=\"item_list item_list_type_normal\">\n"
                         << "<tr class=\"item_list_title item_list_title_type_normal\">\n"
                         << "\t<th colspan=\"2\">Item</th>\n"
                         << "\t<th>Price</th>\n"
                         << "</tr>\n";
                    for(const std::pair<const uint16_t, uint32_t> &sp : step.shopProducts)
                    {
                        body << "<tr class=\"value\">\n";
                        writeItemIconAndName(body,sp.first);
                        body << "<td>" << sp.second << "$</td>\n";
                        body << "</tr>\n";
                    }
                    body << "<tr>\n\t<td colspan=\"3\" class=\"item_list_endline item_list_title_type_normal\"></td>\n</tr>\n"
                         << "</table>\n</center>\n";
                }
                body << "</div></div>\n";
            }
            else if(step.type=="fight")
            {
                body << "<div class=\"subblock\"><div class=\"valuetitle\" id=\"step" << stepIdx << "\">Fight</div>\n"
                     << "<center><div style=\"background-position:-16px -16px;\" class=\"flags flags16\"></div></center>\n"
                     << "<div class=\"value\">\n";
                if(step.fightCash>0)
                    body << "Rewards: <b>" << step.fightCash << "$</b><br />\n";

                // Fight item rewards
                if(!step.fightItems.empty())
                {
                    body << "<center><table class=\"item_list item_list_type_normal\">\n"
                         << "<tr class=\"item_list_title item_list_title_type_normal\">\n"
                         << "\t<th colspan=\"2\">Item</th>\n"
                         << "</tr>\n";
                    for(const FightItemEntry &ri : step.fightItems)
                    {
                        std::string qText;
                        if(ri.quantity>1)
                            qText=Helper::toStringUint(ri.quantity)+" ";
                        body << "<tr class=\"value\">\n";
                        writeItemIconAndName(body,ri.id,qText);
                        body << "</tr>\n";
                    }
                    body << "<tr>\n\t<td colspan=\"2\" class=\"item_list_endline item_list_title_type_normal\"></td>\n</tr>\n"
                         << "</table></center>\n";
                }

                // Monster cards
                for(const FightMonsterEntry &bm : step.fightMonsters)
                    monsterAndLevelToDisplay(body,bm.id,bm.level,true);
                body << "<br style=\"clear:both;\" />\n";

                body << "</div>\n";
            }
            else if(step.type=="heal")
                body << "<div class=\"subblock\"><div class=\"valuetitle\" id=\"step" << stepIdx << "\">Heal</div>\n"
                     << "<div class=\"value\">\n\t<center><div style=\"background-position:0px 0px;\" class=\"flags flags128\"></div></center>\n</div>\n";
            else if(step.type=="learn")
                body << "<div class=\"subblock\"><div class=\"valuetitle\" id=\"step" << stepIdx << "\">Learn</div>\n"
                     << "<div class=\"value\">\n\t<center><div style=\"background-position:-384px 0px;\" class=\"flags flags128\"></div></center>\n</div>\n";
            else if(step.type=="warehouse")
                body << "<div class=\"subblock\"><div class=\"valuetitle\" id=\"step" << stepIdx << "\">Warehouse</div>\n"
                     << "<div class=\"value\">\n\t<center><div style=\"background-position:0px -128px;\" class=\"flags flags128\"></div></center>\n</div>\n";
            else if(step.type=="market")
                body << "<div class=\"subblock\"><div class=\"valuetitle\" id=\"step" << stepIdx << "\">Market</div>\n"
                     << "<div class=\"value\">\n\t<center><div style=\"background-position:0px -128px;\" class=\"flags flags128\"></div></center>\n</div>\n";
            else if(step.type=="clan")
                body << "<div class=\"subblock\"><div class=\"valuetitle\" id=\"step" << stepIdx << "\">Clan</div>\n"
                     << "<div class=\"value\">\n\t<center><div style=\"background-position:-384px -128px;\" class=\"flags flags128\"></div></center>\n</div>\n";
            else if(step.type=="sell")
                body << "<div class=\"subblock\"><div class=\"valuetitle\" id=\"step" << stepIdx << "\">Sell</div>\n"
                     << "<div class=\"value\">\n\t<center><div style=\"background-position:-256px 0px;\" class=\"flags flags128\"></div></center>\n</div>\n";
            else if(step.type=="zonecapture")
            {
                body << "<div class=\"subblock\"><div class=\"valuetitle\" id=\"step" << stepIdx << "\">Zone capture</div>\n"
                     << "<div class=\"value\">\nZone: \n";
                body << "<center><div style=\"background-position:-256px -128px;\" class=\"flags flags128\"></div></center>\n"
                     << "</div>\n";
            }
            else if(step.type=="industry")
            {
                body << "<div class=\"subblock\"><div class=\"valuetitle\" id=\"step" << stepIdx << "\">Industry</div>\n"
                     << "<center><div style=\"background-position:0px -32px;\" class=\"flags flags16\"></div></center>\n"
                     << "<div class=\"value\">\n";
                if(!step.industryId.empty())
                {
                    int idx=std::atoi(step.industryId.c_str());
                    if(idx>=0 && (size_t)idx<m.industries.size())
                    {
                        const CatchChallenger::Industry &ind=m.industries[idx];
                        body << "<center><table class=\"item_list item_list_type_normal\">\n"
                             << "<tr class=\"item_list_title item_list_title_type_normal\">\n"
                             << "\t<th>Industry</th>\n\t<th>Resources</th>\n\t<th>Products</th>\n"
                             << "</tr>\n<tr class=\"value\">\n<td>\n"
                             << "<a href=\"" << Helper::htmlEscape(Helper::relUrl("industries/"+step.industryId+".html"))
                             << "\">#" << step.industryId << "</a>\n</td>\n<td>\n";
                        for(const CatchChallenger::Industry::Resource &res : ind.resources)
                        {
                            body << "<div style=\"float:left;text-align:center;\">\n";
                            writeItemIconAndName(body,res.item);
                            body << "</div>\n";
                        }
                        body << "</td>\n<td>\n";
                        for(const CatchChallenger::Industry::Product &prod : ind.products)
                        {
                            body << "<div style=\"float:left;text-align:middle;\">\n";
                            writeItemIconAndName(body,prod.item);
                            body << "</div>\n";
                        }
                        body << "</td>\n</tr>\n<tr>\n\t<td colspan=\"3\" class=\"item_list_endline item_list_title_type_normal\"></td>\n</tr>\n"
                             << "</table></center>\n";
                    }
                }
                body << "\n</div>\n";
            }
        }
        body << "</div>\n";
        Helper::writeHtml(rel,label+" - "+mapDisplayName,body.str());
        out.push_back({bd.localId,rel,label});
    }
    return out;
}

// ── Main generation ───────────────────────────────────────────────────

void generate()
{
    buildPaths();
    s_mapMeta.clear();
    s_zoneNames.clear();
    ensureLocationImages();

    // Generate map preview PNGs (map_preview.php logic)
    int overviewCount=generateMapPreviews();

    const std::vector<MapStore::MainCodeSet> &sets=MapStore::sets();

    // ── Pass 1: compute metadata for every map, track global level extremes ──
    int globalMinWild=0, globalMaxWild=0;
    int globalMinFight=0, globalMaxFight=0;

    for(size_t si=0;si<sets.size();++si)
    {
        const MapStore::MainCodeSet &set=sets[si];
        for(size_t mi=0;mi<set.mapPaths.size();++mi)
        {
            const std::string mapStem=stripTmx(set.mapPaths[mi]);
            MapMeta meta=computeMapMeta(set.mapList[mi],set.mainCode,mapStem);

            if(meta.averageWildLevel>0)
            {
                int avg=(int)meta.averageWildLevel;
                if(globalMinWild==0 || avg<globalMinWild) globalMinWild=avg;
                if(globalMaxWild==0 || avg>globalMaxWild) globalMaxWild=avg;
            }
            if(meta.maxFightLevel>0)
            {
                if(globalMinFight==0 || meta.maxFightLevel<globalMinFight) globalMinFight=meta.maxFightLevel;
                if(globalMaxFight==0 || meta.maxFightLevel>globalMaxFight) globalMaxFight=meta.maxFightLevel;
            }
            s_mapMeta[{si,mi}]=std::move(meta);
        }
    }

    // ── Pass 2: generate individual map + bot pages, collect index data ──
    struct IndexEntry {
        std::string mapPath;
        std::string mapStem;
        std::string rel;
        size_t setIdx;
        size_t mapIdx;
    };
    // zoneToMap[mainCode][zoneCode] → entries
    std::map<std::string, std::map<std::string, std::vector<IndexEntry>>> zoneToMap;
    // zoneFlags[mainCode][zoneCode] → union of all map flags in zone
    std::map<std::string, std::map<std::string, std::set<std::string>>> zoneFlags;

    for(size_t si=0;si<sets.size();++si)
    {
        const MapStore::MainCodeSet &set=sets[si];
        const std::string &mainCode=set.mainCode;

        for(size_t mi=0;mi<set.mapPaths.size();++mi)
        {
            const std::string &mapPath=set.mapPaths[mi];
            const CatchChallenger::CommonMap &m=set.mapList[mi];
            const std::string mapStem=stripTmx(mapPath);
            const std::string rel=s_mapRefPath[{si,mi}];
            const MapMeta &meta=s_mapMeta[{si,mi}];
            Helper::setCurrentPage(rel);

            std::vector<BotPage> botPages=generateBotPages(meta.bots,m,mainCode,mapStem,meta.name,meta.zoneCode);
            Helper::setCurrentPage(rel);

            // Best-effort map preview
            std::string previewRel;
            {
                const std::string tmxAbs=Helper::datapackPath()+"map/main/"+mainCode+"/"+mapPath;
                const std::string pngRel="map/"+mainCode+"/"+mapStem+".png";
                const std::string pngAbs=Helper::localPath()+pngRel;
                if(!Helper::fileExists(pngAbs) && runMap2Png(tmxAbs,pngAbs))
                    previewRel=pngRel;
                else if(Helper::fileExists(pngAbs))
                    previewRel=pngRel;
            }

            // ── Build map page body (matching PHP format) ──
            std::ostringstream body;
            body << "<div class=\"map";
            if(!meta.type.empty())
                body << " map_type_" << Helper::htmlEscape(meta.type);
            body << "\">\n";
            body << "<div class=\"subblock\"><h1>" << Helper::htmlEscape(meta.name) << "</h1>\n";
            if(!meta.type.empty())
                body << "<h3>(" << Helper::htmlEscape(meta.type) << ")</h3>\n";
            if(!meta.shortDesc.empty())
                body << "<h2>" << Helper::htmlEscape(meta.shortDesc) << "</h2>\n";
            body << "</div>\n";

            if(!previewRel.empty())
            {
                body << "<div class=\"value mapscreenshot datapackscreenshot\"><a href=\""
                     << Helper::htmlEscape(Helper::relUrl(previewRel))
                     << "\"><center><img src=\"" << Helper::htmlEscape(Helper::relUrl(previewRel))
                     << "\" alt=\"Screenshot of " << Helper::htmlEscape(meta.name)
                     << "\" title=\"Screenshot of " << Helper::htmlEscape(meta.name)
                     << "\" /></center></a></div>\n";
            }

            if(!meta.description.empty())
                body << "<div class=\"subblock\"><div class=\"valuetitle\">Map description</div><div class=\"value\">"
                     << Helper::htmlEscape(meta.description) << "</div></div>\n";

            if(meta.averageWildLevel>0)
                body << "<div class=\"subblock\"><div class=\"valuetitle\">Average wild level</div><div class=\"value\">"
                     << (int)meta.averageWildLevel << "</div></div>\n";
            if(meta.maxFightLevel>0)
                body << "<div class=\"subblock\"><div class=\"valuetitle\">Max fight level</div><div class=\"value\">"
                     << meta.maxFightLevel << "</div></div>\n";

            // Zone link
            if(!meta.zoneCode.empty())
            {
                std::string zoneName=getZoneName(mainCode,meta.zoneCode);
                std::string zoneRel="zones/"+mainCode+"/"+Helper::textForUrl(zoneName)+".html";
                body << "<div class=\"subblock\"><div class=\"valuetitle\">Zone</div><div class=\"value\">"
                     << "<a href=\"" << Helper::htmlEscape(Helper::relUrl(zoneRel))
                     << "\" title=\"" << Helper::htmlEscape(zoneName) << "\">\n"
                     << Helper::htmlEscape(zoneName) << "</a></div></div>\n";
            }

            // Linked locations (borders + teleporters, deduplicated)
            {
                std::function<bool(uint16_t)> hasBorder=[&](uint16_t idx){ return idx!=65535 && idx<set.mapPaths.size(); };
                bool hasLinked=hasBorder(m.border.top.mapIndex) || hasBorder(m.border.bottom.mapIndex) ||
                               hasBorder(m.border.left.mapIndex) || hasBorder(m.border.right.mapIndex) ||
                               !m.teleporters.empty();
                if(hasLinked)
                {
                    body << "<div class=\"subblock\"><div class=\"valuetitle\">Linked locations</div><div class=\"value\"><ul>\n";
                    std::set<uint16_t> seen;
                    std::function<void(const char *, uint16_t)> renderBorder=[&](const char *dir, uint16_t nid){
                        if(nid==65535 || nid>=set.mapPaths.size()) return;
                        if(!seen.insert(nid).second) return;
                        std::map<std::pair<size_t,size_t>, MapMeta>::const_iterator nit=s_mapMeta.find({si,(size_t)nid});
                        std::string nn=(nit!=s_mapMeta.end())?nit->second.name:set.mapPaths[nid];
                        body << "<li>Border " << dir << ": <a href=\""
                             << Helper::htmlEscape(Helper::relUrl(relativePathForMapRef(si,nid)))
                             << "\">" << Helper::htmlEscape(nn) << "</a></li>\n";
                    };
                    renderBorder("top",    m.border.top.mapIndex);
                    renderBorder("bottom", m.border.bottom.mapIndex);
                    renderBorder("left",   m.border.left.mapIndex);
                    renderBorder("right",  m.border.right.mapIndex);

                    for(const CatchChallenger::Teleporter &tp : m.teleporters)
                    {
                        if(tp.mapIndex>=set.mapPaths.size()) continue;
                        if(!seen.insert(tp.mapIndex).second) continue;
                        std::map<std::pair<size_t,size_t>, MapMeta>::const_iterator nit=s_mapMeta.find({si,(size_t)tp.mapIndex});
                        std::string dn=(nit!=s_mapMeta.end())?nit->second.name:set.mapPaths[tp.mapIndex];
                        // Distinguish Door (from TMX type="door") vs Teleporter
                        bool isDoor=meta.doorPositions.count({tp.source_x,tp.source_y})>0;
                        body << "<li>" << (isDoor?"Door":"Teleporter") << ": <a href=\""
                             << Helper::htmlEscape(Helper::relUrl(relativePathForMapRef(si,tp.mapIndex)))
                             << "\">" << Helper::htmlEscape(dn) << "</a></li>\n";
                    }
                    body << "</ul></div></div>\n";
                }
            }

            // ── Close the <div class="map"> BEFORE items/monsters/bots (matches PHP) ──
            body << "</div>\n";

            // ── Combined Items + Drops table (PHP merges both into one table) ──
            {
                // Collect drops from wild monsters on this map using per-monster drop data
                // (PHP: iterate monsters_list, look up monster_meta[m]['drops'], group by item)
                // Using per-key accessors for monster drops and extras

                // Gather all wild monster IDs on this map
                std::set<uint16_t> wildMonsterIds;
                for(const WildMonsterLayerData &layer : meta.wildLayers)
                    for(const WildMonsterXmlEntry &wm : layer.monsters)
                        wildMonsterIds.insert(wm.id);

                // Build droplist: item → { monster → drop }
                struct DropInfo { uint16_t item; uint32_t quantity_min; uint32_t quantity_max; uint8_t luck; };
                std::map<uint16_t/*item*/, std::map<uint16_t/*monster*/, DropInfo>> droplist;
                for(const uint16_t mId : wildMonsterIds)
                {
                    if(!CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.has_monsterDrop(mId)) continue;
                    const std::vector<CatchChallenger::MonsterDrops> &mDrops=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_monsterDrop(mId);
                    for(const CatchChallenger::MonsterDrops &drop : mDrops)
                    {
                        DropInfo &entry=droplist[drop.item][mId];
                        entry.item=drop.item;
                        entry.quantity_min=drop.quantity_min;
                        entry.quantity_max=drop.quantity_max;
                        entry.luck=drop.luck;
                    }
                }

                bool hasDrops=!droplist.empty();
                bool hasMapItems=!m.items.empty();
                std::string tc=meta.type.empty()?"normal":meta.type;

                if(hasDrops || hasMapItems)
                {
                    body << "<table class=\"item_list item_list_type_" << Helper::htmlEscape(tc) << "\">\n"
                         << "<tr class=\"item_list_title item_list_title_type_" << Helper::htmlEscape(tc) << "\">\n"
                         << "\t<th colspan=\"2\">Item</th>\n"
                         << "\t<th>Location</th>\n"
                         << "</tr>\n";

                    // Drops rows first (grouped by item, then by luck desc)
                    for(const std::pair<const uint16_t, std::map<uint16_t, DropInfo>> &dlp : droplist)
                    {
                        uint16_t itemId=dlp.first;
                        const std::map<uint16_t, DropInfo> &monsterMap=dlp.second;

                        // Use the first monster's drop for quantity text
                        std::string quantityText;
                        const DropInfo &firstDrop=monsterMap.begin()->second;
                        if(firstDrop.quantity_min!=firstDrop.quantity_max)
                            quantityText=Helper::toStringUint(firstDrop.quantity_min)+" to "+Helper::toStringUint(firstDrop.quantity_max)+" ";
                        else if(firstDrop.quantity_min>1)
                            quantityText=Helper::toStringUint(firstDrop.quantity_min)+" ";

                        body << "<tr class=\"value\">\n";
                        writeItemIconAndName(body,itemId,quantityText);

                        // Location: "Drop on [Monster links] with luck of X%"
                        body << "<td>Drop on ";
                        // Group monsters by luck (desc)
                        std::map<uint8_t, std::vector<uint16_t>, std::greater<uint8_t>> luckToMonster;
                        for(const std::pair<const uint16_t, DropInfo> &mp : monsterMap)
                            luckToMonster[mp.second.luck].push_back(mp.first);

                        bool firstLuckGroup=true;
                        for(const std::pair<const uint8_t, std::vector<uint16_t>> &lg : luckToMonster)
                        {
                            if(!firstLuckGroup) body << ", ";
                            firstLuckGroup=false;
                            bool firstMon=true;
                            for(const uint16_t mid : lg.second)
                            {
                                if(!firstMon) body << ", ";
                                firstMon=false;
                                std::string mname=QtDatapackClientLoader::datapackLoader->has_monsterExtra(mid)?QtDatapackClientLoader::datapackLoader->get_monsterExtra(mid).name:("Monster #"+Helper::toStringUint(mid));
                                std::string monLink=Helper::relUrl(GeneratorMonsters::relativePathForMonster(mid));
                                body << "<a href=\"" << Helper::htmlEscape(monLink)
                                     << "\" title=\"" << Helper::htmlEscape(mname) << "\">"
                                     << Helper::htmlEscape(mname) << "</a>\n";
                            }
                            body << " with luck of " << (unsigned)lg.first << "%";
                        }
                        body << "</td>\n</tr>\n";
                    }

                    // Map items rows
                    for(const std::pair<const std::pair<uint8_t,uint8_t>, CatchChallenger::ItemOnMap> &ip : m.items)
                    {
                        const CatchChallenger::ItemOnMap &io=ip.second;
                        body << "<tr class=\"value\">\n";
                        writeItemIconAndName(body,io.item);
                        if(io.infinite)
                            body << "<td>On the map</td>\n";
                        else
                            body << "<td>Hidden on the map</td>\n";
                        body << "</tr>\n";
                    }

                    body << "<tr>\n\t<td colspan=\"3\" class=\"item_list_endline item_list_title_type_"
                         << Helper::htmlEscape(tc) << "\"></td>\n</tr>\n</table>\n";
                }
            }

            // ── Wild monsters table with layer type headers (matches PHP) ──
            if(!meta.wildLayers.empty())
            {
                std::string tc=meta.type.empty()?"normal":meta.type;
                body << "<table class=\"item_list item_list_type_" << Helper::htmlEscape(tc) << "\">\n"
                     << "<tr class=\"item_list_title item_list_title_type_" << Helper::htmlEscape(tc) << "\">\n"
                     << "\t<th colspan=\"2\">Monster</th>\n"
                     << "\t<th>Location</th>\n"
                     << "\t<th>Levels</th>\n"
                     << "\t<th colspan=\"3\">Rate</th>\n"
                     << "</tr>\n";

                for(const WildMonsterLayerData &layer : meta.wildLayers)
                {
                    // Layer type sub-header
                    body << "<tr class=\"item_list_title_type_" << Helper::htmlEscape(tc) << "\">\n"
                         << "\t<th colspan=\"7\">\n"
                         << Helper::htmlEscape(layer.layerName) << "\n"
                         << "</th>\n</tr>\n";

                    // Sort monsters by luck descending
                    std::vector<WildMonsterXmlEntry> sorted=layer.monsters;
                    std::sort(sorted.begin(),sorted.end(),[](const WildMonsterXmlEntry &a, const WildMonsterXmlEntry &b){
                        return a.luck>b.luck;
                    });

                    for(const WildMonsterXmlEntry &wm : sorted)
                    {
                        std::string mname=QtDatapackClientLoader::datapackLoader->has_monsterExtra(wm.id)?QtDatapackClientLoader::datapackLoader->get_monsterExtra(wm.id).name:("Monster #"+Helper::toStringUint(wm.id));
                        std::string link=Helper::relUrl(GeneratorMonsters::relativePathForMonster(wm.id));

                        body << "<tr class=\"value\">\n\t<td>\n";

                        // Monster icon (small.png or small.gif)
                        std::string smallPng="monsters/"+Helper::toStringUint(wm.id)+"/small.png";
                        std::string smallGif="monsters/"+Helper::toStringUint(wm.id)+"/small.gif";
                        if(Helper::fileExists(Helper::datapackPath()+smallPng))
                        {
                            std::string imgUrl=Helper::relUrl(Helper::publishDatapackFile(smallPng));
                            body << "<div class=\"monstericon\"><a href=\"" << Helper::htmlEscape(link) << "\"><img src=\"" << imgUrl
                                 << "\" width=\"32\" height=\"32\" alt=\"" << Helper::htmlEscape(mname)
                                 << "\" title=\"" << Helper::htmlEscape(mname) << "\" /></a></div>\n";
                        }
                        else if(Helper::fileExists(Helper::datapackPath()+smallGif))
                        {
                            std::string imgUrl=Helper::relUrl(Helper::publishDatapackFile(smallGif));
                            body << "<div class=\"monstericon\"><a href=\"" << Helper::htmlEscape(link) << "\"><img src=\"" << imgUrl
                                 << "\" width=\"32\" height=\"32\" alt=\"" << Helper::htmlEscape(mname)
                                 << "\" title=\"" << Helper::htmlEscape(mname) << "\" /></a></div>\n";
                        }

                        body << "</td>\n\t<td><a href=\"" << Helper::htmlEscape(link) << "\">" << Helper::htmlEscape(mname) << "</a></td>\n";

                        // Location column with image
                        body << "<td>\n";
                        {
                            std::string locImg=locationImageName(layer.layerName);
                            std::string locImgPath="images/datapack-explorer/"+locImg+".png";
                            if(Helper::fileExists(Helper::localPath()+locImgPath))
                                body << "<img src=\"" << Helper::htmlEscape(Helper::relUrl(locImgPath))
                                     << "\" alt=\"\" class=\"locationimg\">";
                        }
                        body << Helper::htmlEscape(layer.layerName) << "</td>\n";

                        // Levels column
                        body << "<td>\n";
                        if(wm.minLevel==wm.maxLevel)
                            body << (unsigned)wm.minLevel;
                        else
                            body << (unsigned)wm.minLevel << "-" << (unsigned)wm.maxLevel;
                        body << "</td>\n";

                        // Rate column
                        body << "<td colspan=\"3\">" << (unsigned)wm.luck << "%</td>\n";
                        body << "</tr>\n";
                    }
                }

                body << "<tr>\n\t<td colspan=\"7\" class=\"item_list_endline item_list_title_type_"
                     << Helper::htmlEscape(tc) << "\"></td>\n</tr>\n</table>\n";
            }

            // ── Bots inline table (matches PHP structure) ──
            if(!meta.bots.empty())
            {
                std::string tc=meta.type.empty()?"normal":meta.type;
                bool hasBotSteps=false;
                for(const BotData &bd : meta.bots)
                    if(!bd.onlyText || !bd.steps.empty())
                        hasBotSteps=true;

                if(hasBotSteps)
                {
                    body << "<center><table class=\"item_list item_list_type_" << Helper::htmlEscape(tc) << "\">\n"
                         << "<tr class=\"item_list_title item_list_title_type_" << Helper::htmlEscape(tc) << "\">\n"
                         << "\t<th colspan=\"2\">Bot</th>\n"
                         << "\t<th>Type</th>\n"
                         << "\t<th>Content</th>\n"
                         << "</tr>\n";

                    for(const BotData &bd : meta.bots)
                    {
                        // Bot name link (links to separate bot page if it exists)
                        std::string botLink;
                        for(const BotPage &bp : botPages)
                            if(bp.botId==bd.localId) { botLink=Helper::relUrl(bp.rel); break; }

                        // Bot skin sprite
                        std::string skinUrl=findBotSkin(bd.skinName);

                        if(bd.onlyText)
                        {
                            // Text-only bot: single row
                            body << "<tr class=\"value\">\n";
                            if(!skinUrl.empty())
                                writeBotSkinTd(body,skinUrl,true);
                            body << "<td";
                            if(skinUrl.empty())
                                body << " colspan=\"3\"";
                            else
                                body << " colspan=\"2\"";
                            body << ">";
                            if(!botLink.empty())
                                body << "<a href=\"" << Helper::htmlEscape(botLink) << "\" title=\"" << Helper::htmlEscape(bd.name) << "\">";
                            body << Helper::htmlEscape(bd.name);
                            if(!botLink.empty())
                                body << "</a>";
                            body << "</td>\n";
                            body << "<td>Text only</td>\n";
                            body << "</tr>\n";
                        }
                        else
                        {
                            for(const BotStepData &step : bd.steps)
                            {
                                if(step.type=="text")
                                    continue;

                                body << "<tr class=\"value\">\n";

                                // Bot skin sprite + name
                                if(!skinUrl.empty())
                                    writeBotSkinTd(body,skinUrl);
                                body << "<td";
                                if(skinUrl.empty())
                                    body << " colspan=\"2\"";
                                body << ">";
                                if(!botLink.empty())
                                    body << "<a href=\"" << Helper::htmlEscape(botLink) << "\" title=\"" << Helper::htmlEscape(bd.name) << "\">";
                                body << Helper::htmlEscape(bd.name);
                                if(!botLink.empty())
                                    body << "</a>";
                                body << "</td>\n";

                                // Type column with flag icon + Content column
                                if(step.type=="shop")
                                {
                                    body << "<td><center>Shop<div style=\"background-position:-32px 0px;\" class=\"flags flags16\"></div></center></td><td>\n";
                                    // Inline shop table from parsed XML products
                                    if(!step.shopProducts.empty())
                                    {
                                        body << "<center><table class=\"item_list item_list_type_" << Helper::htmlEscape(tc) << "\">\n"
                                             << "<tr class=\"item_list_title item_list_title_type_" << Helper::htmlEscape(tc) << "\">\n"
                                             << "\t<th colspan=\"2\">Item</th>\n"
                                             << "\t<th>Price</th>\n"
                                             << "</tr>\n";
                                        for(const std::pair<const uint16_t, uint32_t> &sp : step.shopProducts)
                                        {
                                            body << "<tr class=\"value\">\n";
                                            writeItemIconAndName(body,sp.first);
                                            body << "<td>" << sp.second << "$</td>\n";
                                            body << "</tr>\n";
                                        }
                                        body << "<tr>\n\t<td colspan=\"3\" class=\"item_list_endline item_list_title_type_"
                                             << Helper::htmlEscape(tc) << "\"></td>\n</tr>\n</table>\n";
                                        body << "</center>";
                                    }
                                    body << "</td>\n";
                                }
                                else if(step.type=="fight")
                                {
                                    body << "<td><center>Fight<div style=\"background-position:-16px -16px;\" class=\"flags flags16\"></div></center></td><td>\n";
                                    if(step.isLeader)
                                    {
                                        body << "<b>Leader</b><br />\n";
                                        // Leader front skin
                                        if(!bd.skinName.empty())
                                        {
                                            static const char *dirs[]={"skin/bot/","skin/fighter/",nullptr};
                                            static const char *exts[]={"/front.png","/front.gif",nullptr};
                                            for(int d=0;dirs[d];d++)
                                                for(int e=0;exts[e];e++)
                                                {
                                                    std::string rel2=std::string(dirs[d])+bd.skinName+exts[e];
                                                    if(Helper::fileExists(Helper::datapackPath()+rel2))
                                                    {
                                                        std::string furl=Helper::relUrl(Helper::publishDatapackFile(rel2));
                                                        body << "<center><img src=\"" << furl << "\" width=\"80\" height=\"80\" alt=\"\" /></center>\n";
                                                        goto leaderFrontDone;
                                                    }
                                                }
                                        }
                                        leaderFrontDone:;
                                    }
                                    if(step.fightCash>0)
                                        body << "Rewards: <b>" << step.fightCash << "$</b><br />\n";

                                    // Fight item rewards inline table
                                    if(!step.fightItems.empty())
                                    {
                                        body << "<center><table class=\"item_list item_list_type_" << Helper::htmlEscape(tc) << "\">\n"
                                             << "<tr class=\"item_list_title item_list_title_type_" << Helper::htmlEscape(tc) << "\">\n"
                                             << "\t<th colspan=\"2\">Item</th>\n"
                                             << "</tr>\n";
                                        for(const FightItemEntry &ri : step.fightItems)
                                        {
                                            std::string qText;
                                            if(ri.quantity>1)
                                                qText=Helper::toStringUint(ri.quantity)+" ";
                                            body << "<tr class=\"value\">\n";
                                            writeItemIconAndName(body,ri.id,qText);
                                            body << "</tr>\n";
                                        }
                                        body << "<tr>\n\t<td colspan=\"2\" class=\"item_list_endline item_list_title_type_"
                                             << Helper::htmlEscape(tc) << "\"></td>\n</tr>\n</table></center>\n";
                                    }

                                    // Monster cards (monsterAndLevelToDisplay)
                                    for(const FightMonsterEntry &bm : step.fightMonsters)
                                        monsterAndLevelToDisplay(body,bm.id,bm.level,step.isLeader);
                                    body << "<br style=\"clear:both;\" />\n";

                                    body << "</td>\n";
                                }
                                else if(step.type=="heal")
                                    body << "<td><center>Heal</center></td>\n<td><center><div style=\"background-position:0px 0px;\" class=\"flags flags64\"></div></center></td>\n";
                                else if(step.type=="learn")
                                    body << "<td><center>Learn</center></td>\n<td><center><div style=\"background-position:-192px 0px;\" class=\"flags flags64\"></div></center></td>\n";
                                else if(step.type=="warehouse")
                                    body << "<td><center>Warehouse</center></td>\n<td><center><div style=\"background-position:0px -64px;\" class=\"flags flags64\"></div></center></td>\n";
                                else if(step.type=="market")
                                    body << "<td><center>Market</center></td>\n<td><center><div style=\"background-position:0px -64px;\" class=\"flags flags64\"></div></center></td>\n";
                                else if(step.type=="quests")
                                    body << "<td><center>Quests</center></td>\n<td><center><div style=\"background-position:-32px 0px;\" class=\"flags flags32\"></div></center></td>\n";
                                else if(step.type=="clan")
                                    body << "<td><center>Clan</center></td>\n<td><center><div style=\"background-position:-192px -64px;\" class=\"flags flags64\"></div></center></td>\n";
                                else if(step.type=="sell")
                                    body << "<td><center>Sell</center></td>\n<td><center><div style=\"background-position:-128px 0px;\" class=\"flags flags64\"></div></center></td>\n";
                                else if(step.type=="zonecapture")
                                    body << "<td><center>Zone capture</center></td>\n<td><center><div style=\"background-position:-128px -64px;\" class=\"flags flags64\"></div></center></td>\n";
                                else if(step.type=="industry")
                                {
                                    body << "<td><center>Industry<div style=\"background-position:0px -32px;\" class=\"flags flags16\"></div></center></td><td>\n";
                                    // Try to find industry data from m.industries by industryId
                                    bool industryFound=false;
                                    if(!step.industryId.empty())
                                    {
                                        // Industries are indexed by position in the vector; try to use industryId as index
                                        int idx=std::atoi(step.industryId.c_str());
                                        if(idx>=0 && (size_t)idx<m.industries.size())
                                        {
                                            const CatchChallenger::Industry &ind=m.industries[idx];
                                            body << "<center><table class=\"item_list item_list_type_" << Helper::htmlEscape(tc) << "\">\n"
                                                 << "<tr class=\"item_list_title item_list_title_type_" << Helper::htmlEscape(tc) << "\">\n"
                                                 << "\t<th>Industry</th>\n"
                                                 << "\t<th>Resources</th>\n"
                                                 << "\t<th>Products</th>\n"
                                                 << "</tr>\n";
                                            body << "<tr class=\"value\">\n";
                                            body << "<td>\n#" << step.industryId << "\n</td>\n";
                                            // Resources
                                            body << "<td>\n";
                                            for(const CatchChallenger::Industry::Resource &res : ind.resources)
                                            {
                                                body << "<div style=\"float:left;text-align:center;\">\n";
                                                std::string rimg=itemImageUrl(res.item);
                                                std::string rlink=Helper::relUrl(GeneratorItems::relativePathForItem(res.item));
                                                std::string rname=itemDisplayName(res.item);
                                                if(!rimg.empty())
                                                {
                                                    if(!rlink.empty()) body << "<a href=\"" << Helper::htmlEscape(rlink) << "\">\n";
                                                    body << "<img src=\"" << rimg << "\" width=\"24\" height=\"24\" alt=\""
                                                         << Helper::htmlEscape(rname) << "\" title=\"" << Helper::htmlEscape(rname) << "\" />\n";
                                                    if(!rlink.empty()) body << "</a>\n";
                                                }
                                                if(!rlink.empty()) body << "<a href=\"" << Helper::htmlEscape(rlink) << "\">\n";
                                                body << Helper::htmlEscape(rname);
                                                if(!rlink.empty()) body << "</a>\n";
                                                body << "</div>\n";
                                            }
                                            body << "</td>\n";
                                            // Products
                                            body << "<td>\n";
                                            for(const CatchChallenger::Industry::Product &prod : ind.products)
                                            {
                                                body << "<div style=\"float:left;text-align:middle;\">\n";
                                                std::string pimg=itemImageUrl(prod.item);
                                                std::string plink=Helper::relUrl(GeneratorItems::relativePathForItem(prod.item));
                                                std::string pname=itemDisplayName(prod.item);
                                                if(!pimg.empty())
                                                {
                                                    if(!plink.empty()) body << "<a href=\"" << Helper::htmlEscape(plink) << "\">\n";
                                                    body << "<img src=\"" << pimg << "\" width=\"24\" height=\"24\" alt=\""
                                                         << Helper::htmlEscape(pname) << "\" title=\"" << Helper::htmlEscape(pname) << "\" />\n";
                                                    if(!plink.empty()) body << "</a>\n";
                                                }
                                                if(!plink.empty()) body << "<a href=\"" << Helper::htmlEscape(plink) << "\">\n";
                                                body << Helper::htmlEscape(pname);
                                                if(!plink.empty()) body << "</a>\n";
                                                body << "</div>\n";
                                            }
                                            body << "</td>\n";
                                            body << "</tr>\n";
                                            body << "<tr>\n\t<td colspan=\"3\" class=\"item_list_endline item_list_title_type_"
                                                 << Helper::htmlEscape(tc) << "\"></td>\n</tr>\n</table></center>\n";
                                            industryFound=true;
                                        }
                                    }
                                    if(!industryFound)
                                        body << "Industry " << Helper::htmlEscape(step.industryId) << " not found";
                                    body << "</td>\n";
                                }
                                else
                                    body << "<td>" << Helper::htmlEscape(step.type) << "</td><td>Unknown type (" << Helper::htmlEscape(step.type) << ")</td>\n";

                                body << "</tr>\n";
                            }
                        }
                    }

                    body << "<tr>\n\t<td colspan=\"4\" class=\"item_list_endline item_list_title_type_"
                         << Helper::htmlEscape(tc) << "\"></td>\n</tr>\n</table></center>\n";
                }
            }
            Helper::writeHtml(rel,meta.name,body.str());

            // Collect for the zone-grouped index
            IndexEntry ie;
            ie.mapPath=mapPath;
            ie.mapStem=mapStem;
            ie.rel=rel;
            ie.setIdx=si;
            ie.mapIdx=mi;
            zoneToMap[mainCode][meta.zoneCode].push_back(ie);

            for(const std::string &f : meta.flags)
                zoneFlags[mainCode][meta.zoneCode].insert(f);
        }
    }

    // ── Pass 3: generate zone-grouped index page ──
    Helper::setCurrentPage("map/index.html");
    std::ostringstream indexBody;

    std::vector<LevelRange> wildRanges=computeLevelRanges(globalMinWild,globalMaxWild);
    std::vector<LevelRange> fightRanges=computeLevelRanges(globalMinFight,globalMaxFight);

    // Wild legend (float:left, low→high)
    for(size_t i=0;i<wildRanges.size();i++)
        indexBody << "<div style=\"float:left\"><div style=\"width:16px;height:16px;float:left;background-color:"
                  << s_levelColors[i] << ";\"></div>: Wild "
                  << wildRanges[i].lo << "-" << wildRanges[i].hi << "</div> ";

    // Fight legend (float:right, high→low)
    for(int i=(int)fightRanges.size()-1;i>=0;i--)
        indexBody << "<div style=\"float:right\"><div style=\"width:16px;height:16px;float:left;background-color:"
                  << s_levelColors[i] << ";\"></div>: Fight "
                  << fightRanges[i].lo << "-" << fightRanges[i].hi << "</div> ";

    indexBody << "<br style=\"clear:both\" />";

    // Overview/preview images (from map_preview.php)
    {
        int idx=1;
        while(idx<=overviewCount)
        {
            std::string overviewPng=Helper::localPath()+"maps/overview-"+std::to_string(idx)+".png";
            std::string previewPng=Helper::localPath()+"maps/preview-"+std::to_string(idx)+".png";
            if(!Helper::fileExists(overviewPng) || !Helper::fileExists(previewPng))
                break;

            // Get preview image size for ratio calculation
            // Simple approach: use fixed 256x256 (the resize target)
            // PHP reads actual size and applies ratio if >1600/800 pixels
            int pw=256, ph=256;
            int ratio=1;
            if(pw>1600 || ph>800)
                ratio=4;
            else if(pw>800 || ph>400)
                ratio=2;

            indexBody << "<div class=\"value datapackscreenshot\"><a href=\""
                      << Helper::htmlEscape(Helper::relUrl("maps/overview-"+std::to_string(idx)+".png"))
                      << "\"><center><img src=\""
                      << Helper::htmlEscape(Helper::relUrl("maps/preview-"+std::to_string(idx)+".png"))
                      << "\" alt=\"Map overview\" title=\"Map overview\" width=\""
                      << (pw/ratio) << "\" height=\"" << (ph/ratio) << "\" />\n";

            // File size display
            std::ifstream sizeCheck(overviewPng,std::ios::ate|std::ios::binary);
            if(sizeCheck.is_open())
            {
                long long fileSize=(long long)sizeCheck.tellg();
                if(fileSize>=1000)
                {
                    indexBody << "<b>Size: ";
                    if(fileSize<1000000)
                        indexBody << (fileSize/1000) << "KB";
                    else if(fileSize<10000000)
                    {
                        char buf[32];
                        snprintf(buf,sizeof(buf),"%.2f",(double)fileSize/1000000.0);
                        indexBody << buf << "MB";
                    }
                    else
                    {
                        char buf[32];
                        snprintf(buf,sizeof(buf),"%.1f",(double)fileSize/1000000.0);
                        indexBody << buf << "MB";
                    }
                    indexBody << "</b>\n";
                }
            }

            indexBody << "</center></a></div>\n";
            idx++;
        }
    }

    // Zone-grouped tables
    for(std::pair<const std::string, std::map<std::string, std::vector<IndexEntry>>> &mcPair : zoneToMap)
    {
        const std::string &mainCode=mcPair.first;
        for(std::pair<const std::string, std::vector<IndexEntry>> &zonePair : mcPair.second)
        {
            const std::string &zoneCode=zonePair.first;
            std::vector<IndexEntry> &entries=zonePair.second;

            std::string zoneName=getZoneName(mainCode,zoneCode);
            std::string zoneLink="zones/"+mainCode+"/"+Helper::textForUrl(zoneName)+".html";

            // Sort entries alphabetically by display name
            std::sort(entries.begin(),entries.end(),[](const IndexEntry &a, const IndexEntry &b){
                const GeneratorMaps::MapMeta &ma=s_mapMeta[{a.setIdx,a.mapIdx}];
                const GeneratorMaps::MapMeta &mb=s_mapMeta[{b.setIdx,b.mapIdx}];
                return ma.name<mb.name;
            });

            // Determine if zone has any flags
            const std::set<std::string> &zf=zoneFlags[mainCode][zoneCode];
            bool hasFlags=!zf.empty();

            // Lambda to emit a table header (reused on 21-row split)
            std::function<void()> writeTableHeader=[&](){
                indexBody << "<div class=\"divfixedwithlist\"><table class=\"item_list item_list_type_outdoor map_list\">"
                          << "<tr class=\"item_list_title item_list_title_type_outdoor\">\n";
                indexBody << "<th><a href=\"" << Helper::htmlEscape(Helper::relUrl(zoneLink))
                          << "\" title=\"" << Helper::htmlEscape(zoneName) << "\">\n"
                          << Helper::htmlEscape(zoneName) << "</a></th>\n";
                if(hasFlags)
                {
                    indexBody << "<th>\n";
                    writeFlagDivs(indexBody,zf);
                    indexBody << "</th>\n";
                }
                indexBody << "</tr>\n";
            };

            writeTableHeader();
            int mapCount=0;

            for(const IndexEntry &ie : entries)
            {
                mapCount++;
                if(mapCount>21)
                {
                    // Close current table, start a new one
                    indexBody << "<tr>\n<td";
                    if(hasFlags) indexBody << " colspan=\"2\"";
                    indexBody << " class=\"item_list_endline item_list_title_type_outdoor\"></td>\n"
                              << "</tr></table></div>\n";
                    writeTableHeader();
                    mapCount=1;
                }

                const MapMeta &meta=s_mapMeta[{ie.setIdx,ie.mapIdx}];
                std::string fightColor=colorForLevel(meta.maxFightLevel,fightRanges);
                std::string wildColor=colorForLevel((int)meta.averageWildLevel,wildRanges);

                indexBody << "<tr class=\"value\" style=\"background: linear-gradient(-45deg, "
                          << fightColor << ", " << wildColor << " 90%)\">"
                          << "<td><a href=\"" << Helper::htmlEscape(Helper::relUrl(ie.rel))
                          << "\" title=\"" << Helper::htmlEscape(meta.name) << "\">"
                          << Helper::htmlEscape(meta.name) << "</a></td>\n";

                if(hasFlags)
                {
                    indexBody << "<td><a href=\"" << Helper::htmlEscape(Helper::relUrl(ie.rel))
                              << "\" title=\"" << Helper::htmlEscape(meta.name) << "\">\n";
                    writeFlagDivs(indexBody,meta.flags);
                    indexBody << "</a></td>\n";
                }
                indexBody << "</tr>\n";
            }

            // Close last table for this zone
            indexBody << "<tr>\n<td";
            if(hasFlags) indexBody << " colspan=\"2\"";
            indexBody << " class=\"item_list_endline item_list_title_type_outdoor\"></td>\n"
                      << "</tr></table></div>\n";
        }
    }

    Helper::writeHtml("map/index.html","Maps list",indexBody.str());

    // ── Bots index page (bots.html) ──────────────────────────────────────
    {
        // Group bots by zone name. Filter out text-only bots without skins (matching PHP).
        struct BotEntry {
            std::string mainCode;
            std::string mapStem;
            std::string mapName;
            std::string botName;
            uint8_t botLocalId;
            std::string skinName;
            bool hasSkin;
        };
        // zoneName → list of bots
        std::map<std::string,std::vector<BotEntry>> botsByZone;

        for(const std::pair<const std::pair<size_t,size_t>, GeneratorMaps::MapMeta> &kv : s_mapMeta)
        {
            const GeneratorMaps::MapMeta &meta=kv.second;
            size_t si=kv.first.first;
            size_t mi=kv.first.second;
            const MapStore::MainCodeSet &set=sets[si];
            const std::string mc=set.mainCode;
            const std::string stem=stripTmx(set.mapPaths[mi]);

            // Resolve zone display name
            std::string zoneName;
            if(!meta.zoneCode.empty())
            {
                std::string zoneXml=Helper::datapackPath()+"map/main/"+mc+"/zone/"+meta.zoneCode+".xml";
                tinyxml2::XMLDocument zdoc;
                if(zdoc.LoadFile(zoneXml.c_str())==tinyxml2::XML_SUCCESS && zdoc.RootElement())
                {
                    const tinyxml2::XMLElement *n=zdoc.RootElement()->FirstChildElement("name");
                    if(n && n->GetText()) zoneName=n->GetText();
                }
            }
            if(zoneName.empty()) zoneName=meta.name; // fallback to map name

            for(const BotData &bd : meta.bots)
            {
                // Check skin existence
                bool hasSkin=false;
                if(!bd.skinName.empty())
                {
                    static const char *dirs[]={"skin/bot/","skin/fighter/",nullptr};
                    static const char *exts[]={"trainer.png","trainer.gif",nullptr};
                    for(int d=0; dirs[d] && !hasSkin; d++)
                        for(int e=0; exts[e] && !hasSkin; e++)
                            if(Helper::fileExists(Helper::datapackPath()+dirs[d]+bd.skinName+"/"+exts[e]))
                                hasSkin=true;
                }

                // PHP filter: include bot if it has a skin OR is not text-only
                if(hasSkin || !bd.onlyText)
                {
                    BotEntry be;
                    be.mainCode=mc;
                    be.mapStem=stem;
                    be.mapName=meta.name;
                    be.botName=bd.name;
                    be.botLocalId=bd.localId;
                    be.skinName=bd.skinName;
                    be.hasSkin=hasSkin;
                    botsByZone[zoneName].push_back(std::move(be));
                }
            }
        }

        Helper::setCurrentPage("bots.html");
        std::ostringstream botsBody;
        for(const std::pair<const std::string, std::vector<BotEntry>> &zp : botsByZone)
        {
            const std::string &zone=zp.first;
            const std::vector<BotEntry> &botList=zp.second;
            if(botList.empty()) continue;

            // Count duplicate bot names within this zone (for disambiguation)
            std::map<std::string,std::map<std::string,bool>> nameCountForZone;
            for(const BotEntry &be : botList)
            {
                if(!be.botName.empty())
                    nameCountForZone[be.botName][be.mapName]=true;
            }

            int botCount=0;
            std::function<void()> writeZoneHeader=[&](){
                botsBody << "<div class=\"divfixedwithlist\"><table class=\"item_list item_list_type_normal map_list\">\n"
                         << "<tr class=\"item_list_title item_list_title_type_normal\">\n"
                         << "\t<th colspan=\"2\">\n"
                         << Helper::htmlEscape(zone)
                         << "\n</th>\n</tr>\n";
            };
            writeZoneHeader();

            for(const BotEntry &be : botList)
            {
                botCount++;
                if(botCount>15)
                {
                    botsBody << "<tr>\n\t<td colspan=\"2\" class=\"item_list_endline item_list_title_type_normal\"></td>\n</tr>\n"
                             << "</table></div>\n";
                    writeZoneHeader();
                    botCount=1;
                }

                // Display name: if name is empty show "Bot #id", if duplicate in zone append (mapName)
                std::string finalName;
                if(be.botName.empty())
                    finalName="Bot #"+std::to_string(be.botLocalId);
                else if(nameCountForZone[be.botName].size()==1)
                    finalName=be.botName;
                else
                    finalName=be.botName+" ("+be.mapName+")";

                botsBody << "<tr class=\"value\">\n";

                // Skin sprite column
                if(be.hasSkin)
                {
                    static const char *dirs[]={"skin/bot/","skin/fighter/",nullptr};
                    static const char *exts[]={"trainer.png","trainer.gif",nullptr};
                    for(int d=0; dirs[d]; d++)
                    {
                        bool found=false;
                        for(int e=0; exts[e]; e++)
                        {
                            std::string rel=std::string(dirs[d])+be.skinName+"/"+exts[e];
                            if(Helper::fileExists(Helper::datapackPath()+rel))
                            {
                                std::string pub=Helper::publishDatapackFile(rel);
                                botsBody << "<td><center><div style=\"width:16px;height:24px;background-image:url('"
                                         << Helper::relUrl(pub)
                                         << "');background-repeat:no-repeat;background-position:-16px -48px;\"></div></center></td>\n";
                                found=true;
                                break;
                            }
                        }
                        if(found) break;
                    }
                }

                // Name + link column
                std::string botPageRel="map/"+be.mainCode+"/"+be.mapStem+"/bots/bot-"+std::to_string(be.botLocalId)+".html";
                botsBody << "<td";
                if(!be.hasSkin) botsBody << " colspan=\"2\"";
                botsBody << "><a href=\"" << Helper::htmlEscape(Helper::relUrl(botPageRel))
                         << "\" title=\"" << Helper::htmlEscape(finalName) << "\">"
                         << Helper::htmlEscape(finalName) << "</a></td>\n";
                botsBody << "</tr>\n";
            }
            botsBody << "<tr>\n\t<td colspan=\"2\" class=\"item_list_endline item_list_title_type_normal\"></td>\n</tr>\n"
                     << "</table></div>\n";
        }

        Helper::writeHtml("bots.html","Bots list",botsBody.str());
    }

    // ── Zone pages ─���──────────────────────────────────────────────────────
    // Group maps by (mainCode, zoneCode)
    struct ZoneMapEntry {
        std::string mapStem;
        std::string mapName;
        std::string mapType;
        size_t si;
        size_t mi;
        std::map<std::string,int> funcCounts; // step type → count
    };
    // zone key: (mainCode, zoneCode) → entries
    std::map<std::pair<std::string,std::string>, std::vector<ZoneMapEntry>> zoneToMaps;

    for(const std::pair<const std::pair<size_t,size_t>, GeneratorMaps::MapMeta> &kv : s_mapMeta)
    {
        const GeneratorMaps::MapMeta &meta=kv.second;
        size_t si=kv.first.first;
        size_t mi=kv.first.second;
        const MapStore::MainCodeSet &set=sets[si];
        const std::string mainCode=set.mainCode;
        const std::string mapStem=stripTmx(set.mapPaths[mi]);

        ZoneMapEntry ze;
        ze.mapStem=mapStem;
        ze.mapName=meta.name;
        ze.mapType=meta.type;
        ze.si=si;
        ze.mi=mi;

        // Count bot functions on this map
        for(const BotData &bd : meta.bots)
        {
            if(bd.onlyText)
                continue;
            for(const BotStepData &step : bd.steps)
            {
                if(step.type!="text")
                    ze.funcCounts[step.type]++;
            }
        }

        std::string zc=meta.zoneCode;
        zoneToMaps[{mainCode,zc}].push_back(ze);
    }

    // Generate a page for each zone
    for(const std::pair<const std::pair<std::string,std::string>, std::vector<ZoneMapEntry>> &zkv : zoneToMaps)
    {
        const std::string &mainCode=zkv.first.first;
        const std::string &zoneCode=zkv.first.second;
        const std::vector<ZoneMapEntry> &maps=zkv.second;

        // Zone name
        std::string zoneName;
        std::map<std::pair<std::string,std::string>, std::string>::const_iterator zit=s_zoneNames.find({mainCode,zoneCode});
        if(zit!=s_zoneNames.end())
            zoneName=zit->second;
        else if(zoneCode.empty())
            zoneName="Unknown zone";
        else
            zoneName=zoneCode;

        // First map type for the wrapper div class
        std::string firstMapType="outdoor";
        for(const ZoneMapEntry &ze : maps)
            if(!ze.mapType.empty()) { firstMapType=ze.mapType; break; }

        std::string rel="zones/"+mainCode+"/"+Helper::textForUrl(zoneName)+".html";
        Helper::setCurrentPage(rel);

        std::ostringstream body;
        body << "<div class=\"map map_type_" << Helper::htmlEscape(firstMapType) << "\">\n";
        body << "<div class=\"subblock\"><h1>" << Helper::htmlEscape(zoneName) << "</h1></div>\n";

        // Bot population count
        int totalBots=0;
        std::map<std::string,int> zoneFuncCounts;
        for(const ZoneMapEntry &ze : maps)
            for(const std::pair<const std::string, int> &fc : ze.funcCounts)
            {
                zoneFuncCounts[fc.first]+=fc.second;
                totalBots+=fc.second;
            }

        // Count text-only bots too
        for(const std::pair<const std::pair<size_t,size_t>, GeneratorMaps::MapMeta> &kv2 : s_mapMeta)
        {
            size_t si2=kv2.first.first;
            const GeneratorMaps::MapMeta &meta2=kv2.second;
            const MapStore::MainCodeSet &set2=sets[si2];
            if(set2.mainCode!=mainCode || meta2.zoneCode!=zoneCode) continue;
            for(const BotData &bd : meta2.bots)
                if(bd.onlyText) totalBots++;
        }

        if(totalBots==0)
            body << "<div class=\"subblock\"><div class=\"valuetitle\">Population</div><div class=\"value\">No bots in this zone!</div></div>\n";
        else if(totalBots==1)
            body << "<div class=\"subblock\"><div class=\"valuetitle\">Population</div><div class=\"value\">1 bot</div></div>\n";
        else
        {
            body << "<div class=\"subblock\"><div class=\"valuetitle\">Population</div><div class=\"value\">"
                 << totalBots << " bots<br />\n";

            body << "<center><table class=\"item_list item_list_type_outdoor\"><tr class=\"item_list_title item_list_title_type_outdoor\"><th>Bots list</th></tr>\n";
            std::function<void(const char *, const char *, const char *)> writeFuncRow=[&](const char *type, const char *pos, const char *label){
                std::map<std::string, int>::const_iterator it=zoneFuncCounts.find(type);
                if(it!=zoneFuncCounts.end())
                    body << "<tr class=\"value\"><td><div style=\"float:left;background-position:" << pos
                         << ";\" class=\"flags flags16\" title=\"" << type << "\"></div>"
                         << it->second << " " << label << "</td></tr>\n";
            };
            writeFuncRow("shop","-32px 0px","shop(s)");
            writeFuncRow("fight","-16px -16px","bot(s) of fight");
            writeFuncRow("heal","0px 0px","bot(s) of heal");
            writeFuncRow("learn","-48px 0px","bot(s) of learn");
            writeFuncRow("warehouse","0px -16px","warehouse(s)");
            writeFuncRow("market","0px -16px","market(s)");
            writeFuncRow("clan","-48px -16px","bot(s) to create clan");
            writeFuncRow("sell","-32px 0px","bot(s) to sell your objects");
            writeFuncRow("zonecapture","-32px -16px","bot(s) to capture the zone");
            writeFuncRow("industry","-32px -16px","industries");
            writeFuncRow("quests","-16px 0px","quests to start");

            body << "<tr>\n<td class=\"item_list_endline item_list_title_type_outdoor\"></td>\n</tr></table></center>"
                 << "<br style=\"clear:both;\" />\n</div></div>\n";
        }
        body << "</div>\n";

        // Map list table with function flag icons per map
        body << "<center><table class=\"item_list item_list_type_outdoor\"><tr class=\"item_list_title item_list_title_type_outdoor\">\n<th>\n"
             << Helper::htmlEscape(zoneName)
             << "</th><th>\n";
        // Zone-level flag icons in header
        std::function<void(const char *, const char *)> writeHeaderFlag=[&](const char *type, const char *pos){
            if(zoneFuncCounts.count(type))
                body << "<div style=\"float:left;background-position:" << pos << ";\" class=\"flags flags16\" title=\"" << type << "\"></div>\n";
        };
        writeHeaderFlag("shop","-32px 0px");
        writeHeaderFlag("fight","-16px -16px");
        writeHeaderFlag("heal","0px 0px");
        writeHeaderFlag("learn","-48px 0px");
        writeHeaderFlag("warehouse","0px -16px");
        writeHeaderFlag("market","0px -16px");
        writeHeaderFlag("clan","-48px -16px");
        writeHeaderFlag("sell","-32px 0px");
        writeHeaderFlag("zonecapture","-32px -16px");
        writeHeaderFlag("industry","-32px -16px");
        writeHeaderFlag("quests","-16px 0px");
        body << "</th></tr>\n";

        // Map rows (sorted by name)
        std::vector<const ZoneMapEntry*> sorted;
        for(const ZoneMapEntry &ze : maps) sorted.push_back(&ze);
        std::sort(sorted.begin(),sorted.end(),[](const ZoneMapEntry *a, const ZoneMapEntry *b){ return a->mapName<b->mapName; });

        for(const ZoneMapEntry *ze : sorted)
        {
            std::string mapRel=relativePathForMapRef(ze->si,ze->mi);
            body << "<tr class=\"value\"><td><a href=\"" << Helper::htmlEscape(Helper::relUrl(mapRel))
                 << "\" title=\"" << Helper::htmlEscape(ze->mapName) << "\">"
                 << Helper::htmlEscape(ze->mapName) << "</a></td>\n";
            body << "<td><a href=\"" << Helper::htmlEscape(Helper::relUrl(mapRel))
                 << "\" title=\"" << Helper::htmlEscape(ze->mapName) << "\">\n";

            // Per-map function flags (repeat icon for each occurrence)
            std::function<void(const char *, const char *)> writeMapFlags=[&](const char *type, const char *pos){
                std::map<std::string, int>::const_iterator it=ze->funcCounts.find(type);
                if(it!=ze->funcCounts.end())
                    for(int i=0;i<it->second;i++)
                        body << "<div style=\"float:left;background-position:" << pos
                             << ";\" class=\"flags flags16\" title=\"" << type << "\"></div>\n";
            };
            writeMapFlags("shop","-32px 0px");
            writeMapFlags("fight","-16px -16px");
            writeMapFlags("heal","0px 0px");
            writeMapFlags("learn","-48px 0px");
            writeMapFlags("warehouse","0px -16px");
            writeMapFlags("market","0px -16px");
            writeMapFlags("clan","-48px -16px");
            writeMapFlags("sell","-32px 0px");
            writeMapFlags("zonecapture","-32px -16px");
            writeMapFlags("industry","-32px -16px");
            writeMapFlags("quests","-16px 0px");

            body << "</a></td></tr>\n";
        }
        body << "<tr>\n<td colspan=\"2\" class=\"item_list_endline item_list_title_type_outdoor\"></td>\n</tr></table></center>\n";

        Helper::writeHtml(rel,zoneName,body.str());
    }
}

} // namespace GeneratorMaps
