#include "Map_loader.hpp"
#include "../tinyXML2/customtinyxml2.hpp"
#include "CommonDatapack.hpp"
#include "GeneralVariable.hpp"
#include "CommonDatapackServerSpec.hpp"
#include "cpp11addition.hpp"

#ifdef TILED_ZLIB
    #ifndef __EMSCRIPTEN__
        #include <zlib.h>
    #else
        #include <QtZlib/zlib.h>
    #endif
#endif
#include <iostream>

using namespace CatchChallenger;

std::unordered_map<std::string/*file*/, std::unordered_map<uint16_t/*id*/,tinyxml2::XMLElement *> > Map_loader::teleportConditionsUnparsed;
//std::regex e("[^A-Za-z0-9+/=]+",std::regex::ECMAScript|std::regex::optimize);->very slow, 1000x more slow than dropPrefixAndSuffixLowerThen33(

/// \todo put at walkable the tp on push

#ifndef BOTTESTCONNECT
#if defined(__has_feature)
#  if __has_feature(address_sanitizer)
extern "C" {
const char* __asan_default_options() { return "alloc_dealloc_mismatch=0:detect_container_overflow=0"; }
}  // extern "C"
#  endif
#endif
#endif

Map_loader::Map_loader()
{
    /// \warning more init at try Load
    map_to_send.border.bottom.x_offset=0;
    map_to_send.border.top.x_offset=0;
    map_to_send.border.left.y_offset=0;
    map_to_send.border.right.y_offset=0;
    map_to_send.xmlRoot=NULL;
    map_to_send.monstersCollisionMap=NULL;
    map_to_send.width=0;
    map_to_send.height=0;
}

Map_loader::~Map_loader()
{
}

#ifdef TILED_ZLIB
static void logZlibError(int error)
{
    switch (error)
    {
    case Z_MEM_ERROR:
        std::cerr << "Out of memory while (de)compressing data!" << std::endl;
        break;
    case Z_VERSION_ERROR:
        std::cerr << "Incompatible zlib version!" << std::endl;
        break;
    case Z_NEED_DICT:
    case Z_DATA_ERROR:
        std::cerr << "Incorrect zlib compressed data!" << std::endl;
        break;
    default:
    {
        std::cerr << "Unknown error while (de)compressing data!" << std::endl;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        abort();
        #endif
    }
    }
}

int32_t Map_loader::decompressZlib(const char * const input, const uint32_t &intputSize, char * const output, const uint32_t &maxOutputSize)
{
    z_stream strm;

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.next_in = (Bytef *) input;
    strm.avail_in = intputSize;
    strm.next_out = (Bytef *) output;
    strm.avail_out = maxOutputSize;

    int ret = inflateInit2(&strm, 15 + 32);

    if (ret != Z_OK) {
    logZlibError(ret);
    return -1;
    }

    do {
        ret=inflate(&strm, Z_SYNC_FLUSH);

        switch(ret)
        {
            case Z_NEED_DICT:
            case Z_STREAM_ERROR:
            ret = Z_DATA_ERROR;
            inflateEnd(&strm);
            logZlibError(ret);
            return -1;
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
            inflateEnd(&strm);
            logZlibError(ret);
            return -1;
            default:
            break;
        }

        if (ret != Z_OK && ret != Z_STREAM_END)
        {
            const unsigned char * const val=reinterpret_cast<unsigned char *>(output);
            if(val>strm.next_out)
            {
                logZlibError(Z_STREAM_ERROR);
                return -1;
            }
            if(static_cast<uint32_t>(strm.next_out-val)>static_cast<uint32_t>(maxOutputSize))
            {
                logZlibError(Z_STREAM_ERROR);
                return -1;
            }
            logZlibError(ret);
            return -1;
        }
    }
    while(ret!=Z_STREAM_END);

    if(strm.avail_in!=0)
    {
        logZlibError(Z_DATA_ERROR);
        return -1;
    }
    inflateEnd(&strm);

    return maxOutputSize-strm.avail_out;
}
#endif

bool Map_loader::loadMonsterOnMapAndExtra(const std::string &file, std::vector<std::string> detectedMonsterCollisionMonsterType, std::vector<std::string> detectedMonsterCollisionLayer,std::string &zoneName)
{
    tinyxml2::XMLDocument *domDocument=NULL;

    //open and quick check the file
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CommonDatapack::commonDatapack.get_xmlLoadedFile_rw().find(file)!=CommonDatapack::commonDatapack.get_xmlLoadedFile_rw().cend())
        domDocument=&CommonDatapack::commonDatapack.get_xmlLoadedFile_rw()[file];
    else
    {
        domDocument=&CommonDatapack::commonDatapack.get_xmlLoadedFile_rw()[file];
        #else
        domDocument=new tinyxml2::XMLDocument();
        #endif
        const auto loadOkay = domDocument->LoadFile(file.c_str());
        if(loadOkay!=0)
        {
            std::cerr << file << ", " << tinyxml2errordoc(domDocument) << std::endl;
            return false;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
    }
    #endif
    this->map_to_send.xmlRoot = domDocument->RootElement();
    if(map_to_send.xmlRoot==NULL)
    {
        std::cerr << file+", loadMonsterMap(): no root balise found for the xml file" << std::endl;
        return false;
    }
    if(this->map_to_send.xmlRoot->Name()==NULL)
    {
        std::cerr << file+", loadMonsterMap(): \"map\" root balise not found 2 for the xml file" << std::endl;
        return false;
    }
    if(strcmp(this->map_to_send.xmlRoot->Name(),"map")!=0)
    {
        std::cerr << file+", loadMonsterMap(): \"map\" root balise not found for the xml file" << std::endl;
        return false;
    }
    if(this->map_to_send.xmlRoot->Attribute("zone")!=NULL)
        zoneName=this->map_to_send.xmlRoot->Attribute("zone");
    //std::cerr << file+", loadMonsterMap(): this->map_to_send.xmlRoot->Name()" << this->map_to_send.xmlRoot->Name() << std::endl;

    //found the cave name
    std::vector<std::string> caveName;
    {
        unsigned int index=0;
        while(index<CommonDatapack::commonDatapack.get_monstersCollision().size())
        {
            if(CommonDatapack::commonDatapack.get_monstersCollisionTemp().at(index).layer.empty())
            {
                caveName=CommonDatapack::commonDatapack.get_monstersCollisionTemp().at(index).monsterTypeList;
                break;
            }
            index++;
        }
        if(caveName.empty())
            caveName.push_back("cave");
    }
    {
        unsigned int index=0;
        while(index<caveName.size())
        {
            const std::string &entryName=caveName.at(index);
            if(!vectorcontainsAtLeastOne(detectedMonsterCollisionMonsterType,entryName))
                detectedMonsterCollisionMonsterType.push_back(entryName);
            index++;
        }
    }

    //load the found monster type
    std::unordered_map<std::string/*monsterType*/,std::vector<MapMonster> > monsterTypeList;
    {
        if(detectedMonsterCollisionMonsterType.empty())
        {
            std::cerr << "detectedMonsterCollisionMonsterType.empty() at " << __FILE__ << ":" << __LINE__ << ", internal error, abort()" << std::endl;
            abort();
        }
        unsigned int index=0;
        while(index<detectedMonsterCollisionMonsterType.size())
        {
            if(!detectedMonsterCollisionMonsterType.at(index).empty())
                if(monsterTypeList.find(detectedMonsterCollisionMonsterType.at(index))==monsterTypeList.cend())
                    monsterTypeList[detectedMonsterCollisionMonsterType.at(index)]=loadSpecificMonster(file,detectedMonsterCollisionMonsterType.at(index));
            index++;
        }
    }

    this->map_to_send.parsed_layer.monstersCollisionList.clear();
    this->map_to_send.parsed_layer.monstersCollisionList.push_back(MonstersCollisionValue());//cave
    //found the zone number
    zoneNumber.clear();
    uint8_t zoneNumberIndex=1;
    {
        unsigned int index=0;
        while(index<CommonDatapack::commonDatapack.get_monstersCollision().size())
        {
            //const MonstersCollision &monstersCollision=CommonDatapack::commonDatapack.monstersCollision.at(index);
            const MonstersCollisionTemp &monstersCollisionTemp=CommonDatapack::commonDatapack.get_monstersCollisionTemp().at(index);
            const std::vector<std::string> &searchList=monstersCollisionTemp.defautMonsterTypeList;
            unsigned int index_search=0;
            while(index_search<searchList.size())
            {
                if(monsterTypeList.find(searchList.at(index_search))!=monsterTypeList.cend())
                    break;
                index_search++;
            }
            if(index_search<searchList.size())
            {
                uint8_t tempZoneNumberIndex=0;
                //cave
                if(monstersCollisionTemp.layer.empty())
                {}
                //not cave
                else if(vectorcontainsAtLeastOne(detectedMonsterCollisionLayer,monstersCollisionTemp.layer))
                {
                    if(zoneNumber.find(monstersCollisionTemp.layer)==zoneNumber.cend())
                    {
                        zoneNumber[monstersCollisionTemp.layer]=zoneNumberIndex;
                        this->map_to_send.parsed_layer.monstersCollisionList.push_back(MonstersCollisionValue());//create
                        tempZoneNumberIndex=zoneNumberIndex;
                        zoneNumberIndex++;
                    }
                    else
                        tempZoneNumberIndex=zoneNumber.at(monstersCollisionTemp.layer);
                }
                {
                    MonstersCollisionValue *monstersCollisionValue=&this->map_to_send.parsed_layer.monstersCollisionList[tempZoneNumberIndex];
                    if(CommonDatapack::commonDatapack.get_monstersCollision().at(index).type==MonstersCollisionType_ActionOn)
                    {
                        monstersCollisionValue->actionOn.push_back(index);
                        monstersCollisionValue->actionOnMonsters.push_back(monsterTypeList.at(searchList.at(index_search)));
                    }
                    else
                    {
                        monstersCollisionValue->walkOn.push_back(index);
                        MonstersCollisionValue::MonstersCollisionContent monstersCollisionContent;
                        monstersCollisionContent.defaultMonsters=monsterTypeList.at(searchList.at(index_search));
                        unsigned int event_index=0;
                        while(event_index<monstersCollisionTemp.events.size())
                        {
                            const MonstersCollisionTemp::MonstersCollisionEvent &monstersCollisionEvent=monstersCollisionTemp.events.at(event_index);
                            const std::vector<std::string> &searchList=monstersCollisionEvent.monsterTypeList;
                            unsigned int index_search=0;
                            while(index_search<searchList.size())
                            {
                                if(monsterTypeList.find(searchList.at(index_search))!=monsterTypeList.cend())
                                    break;
                                index_search++;
                            }
                            if(index_search<searchList.size())
                            {
                                MonstersCollisionValue::MonstersCollisionValueOnCondition monstersCollisionValueOnCondition;
                                monstersCollisionValueOnCondition.event=monstersCollisionEvent.event;
                                monstersCollisionValueOnCondition.event_value=monstersCollisionEvent.event_value;
                                monstersCollisionValueOnCondition.monsters=monsterTypeList.at(searchList.at(index_search));
                                monstersCollisionContent.conditions.push_back(monstersCollisionValueOnCondition);
                            }
                            event_index++;
                        }
                        monstersCollisionValue->walkOnMonsters.push_back(monstersCollisionContent);
                    }
                }
            }
            index++;
        }
    }
    #ifdef EPOLLCATCHCHALLENGERSERVER
    delete domDocument;
    #endif
    return true;
}

std::vector<MapMonster> Map_loader::loadSpecificMonster(const std::string &fileName,const std::string &monsterType)
{
    #ifdef ONLYMAPRENDER
    return std::vector<MapMonster>();
    #endif
    std::vector<MapMonster> monsterTypeList;
    bool ok;
    uint32_t tempLuckTotal=0;
    const tinyxml2::XMLElement *layer = map_to_send.xmlRoot->FirstChildElement(monsterType.c_str());
    if(layer!=NULL)
    {
        const tinyxml2::XMLElement *monsters=layer->FirstChildElement("monster");
        while(monsters!=NULL)
        {
            if(monsters->Attribute("id")!=NULL && ((monsters->Attribute("minLevel")!=NULL &&
               monsters->Attribute("maxLevel")!=NULL) || monsters->Attribute("level")!=NULL) && monsters->Attribute("luck")!=NULL)
            {
                MapMonster mapMonster;
                mapMonster.id=stringtouint16(monsters->Attribute("id"),&ok);
                if(!ok)
                    std::cerr << "id is not a number: child->Name(): " << monsters->Name() << ", file: " << fileName << std::endl;
                if(ok)
                    if(CommonDatapack::commonDatapack.get_monsters().find(mapMonster.id)==CommonDatapack::commonDatapack.get_monsters().cend())
                    {
                        std::cerr << "monster " << mapMonster.id << " not found into the monster list: " << monsters->Name() << ", file: " << fileName << std::endl;
                        ok=false;
                    }
                if(monsters->Attribute("minLevel")!=NULL && monsters->Attribute("maxLevel")!=NULL)
                {
                    if(ok)
                    {
                        mapMonster.minLevel=stringtouint8(monsters->Attribute("minLevel"),&ok);
                        if(!ok)
                            std::cerr << "minLevel is not a number: child->Name(): " << monsters->Name() << ", file: " << fileName << std::endl;
                    }
                    if(ok)
                    {
                        mapMonster.maxLevel=stringtouint8(monsters->Attribute("maxLevel"),&ok);
                        if(!ok)
                            std::cerr << "maxLevel is not a number: child->Name(): " << monsters->Name() << ", file: " << fileName << std::endl;
                    }
                }
                else
                {
                    if(ok)
                    {
                        mapMonster.maxLevel=stringtouint8(monsters->Attribute("level"),&ok);
                        mapMonster.minLevel=mapMonster.maxLevel;
                        if(!ok)
                            std::cerr << "level is not a number: child->Name(): " << monsters->Name() << ", file: " << fileName << std::endl;
                    }
                }
                if(ok)
                {
                    std::string textLuck=std::string(monsters->Attribute("luck"));
                    stringreplaceAll(textLuck,"percent","");
                    mapMonster.luck=stringtouint8(textLuck,&ok);
                    if(!ok)
                        std::cerr << "luck is not a number: child->Name(): " << monsters->Name() << ", file: " << fileName << std::endl;
                }
                if(ok)
                    if(mapMonster.minLevel>mapMonster.maxLevel)
                    {
                        std::cerr << "min > max for the level: child->Name(): " << monsters->Name() << ", file: " << fileName << std::endl;
                        ok=false;
                    }
                if(ok)
                    if(mapMonster.luck<=0)
                    {
                        std::cerr << "luck is too low: child->Name(): " << monsters->Name() << ", file: " << fileName << std::endl;
                        ok=false;
                    }
                if(ok)
                    if(mapMonster.minLevel<=0)
                    {
                        std::cerr << "min level is too low: child->Name(): " << monsters->Name() << ", file: " << fileName << std::endl;
                        ok=false;
                    }
                if(ok)
                    if(mapMonster.maxLevel<=0)
                    {
                        std::cerr << "max level is too low: child->Name(): " << monsters->Name() << ", file: " << fileName << std::endl;
                        ok=false;
                    }
                if(ok)
                    if(mapMonster.luck>100)
                    {
                        std::cerr << "luck is greater than 100: child->Name(): " << monsters->Name() << ", file: " << fileName << std::endl;
                        ok=false;
                    }
                if(ok)
                    if(mapMonster.minLevel>CATCHCHALLENGER_MONSTER_LEVEL_MAX)
                    {
                        std::cerr << "min level is greater than " << CATCHCHALLENGER_MONSTER_LEVEL_MAX << ": child->Name(): " << monsters->Name() << ", file: " << fileName << std::endl;
                        ok=false;
                    }
                if(ok)
                    if(mapMonster.maxLevel>CATCHCHALLENGER_MONSTER_LEVEL_MAX)
                    {
                        std::cerr << "max level is greater than " << CATCHCHALLENGER_MONSTER_LEVEL_MAX << ": child->Name(): " << monsters->Name() << ", file: " << fileName << std::endl;
                        ok=false;
                    }
                if(ok)
                {
                    tempLuckTotal+=mapMonster.luck;
                    monsterTypeList.push_back(mapMonster);
                }
            }
            else
                std::cerr << "Missing attribute: child->Name(): " << monsters->Name() << ", file: " << fileName << std::endl;
            monsters = monsters->NextSiblingElement("monster");
        }
        if(monsterTypeList.empty())
            std::cerr << "map have empty monster layer:" << fileName << "type:" << monsterType;
        if(tempLuckTotal!=100)
        {
            std::cerr << "total luck is not egal to 100 (" << tempLuckTotal << ") for grass, monsters dropped: child->Name(): " << layer->Name()
                      << ", file: " << fileName << std::endl;
            monsterTypeList.clear();
        }
        else
            return monsterTypeList;
    }
    /*else//do ignore for cave
    qDebug() << std::stringLiteral("A layer on map is found, but no matching monster list into the meta (%3), monsters dropped: child->Name(): %1 (at line: %2)").arg(layer->Name()).arg(CATCHCHALLENGER_XMLELENTATLINE(layer)).arg(fileName);*/
    return monsterTypeList;
}

std::string Map_loader::errorString()
{
    return error;
}

std::string Map_loader::resolvRelativeMap(const std::string &file,const std::string &link,const std::string &datapackPath)
{
    if(link.empty())
        return link;
    std::string newPath=FSabsoluteFilePath(FSabsolutePath(file)+'/'+link);
    if(datapackPath.empty() || stringStartWith(newPath,datapackPath))
    {
        newPath.erase(0,datapackPath.size());
        return newPath;
    }
    std::cerr << "map link not resolved: " << link
              << ", file: " << file << ", link: " << link
              << ", newPath: " << newPath
              << ", datapackPath: " << datapackPath << std::endl;
    return link;
}
