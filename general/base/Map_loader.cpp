#include "Map_loader.h"
#include "GeneralVariable.h"
#include "CommonDatapackServerSpec.h"
#include "ProtocolParsing.h"
#include "FacilityLibGeneral.h"
#include "cpp11addition.h"
#include "tinyXML2/customtinyxml2.h"

#include "CommonDatapack.h"
#include "CachedString.h"

#include <iostream>
#include <unordered_map>
#include <map>
#ifdef TILED_ZLIB
#include <zlib.h>
#endif
#include <zstd.h>      // presumes zstd library is installed

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
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
            inflateEnd(&strm);
            logZlibError(ret);
            return -1;
        }

        if (ret != Z_OK && ret != Z_STREAM_END)
        {
            unsigned char * const val=reinterpret_cast<unsigned char * const>(output);
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

void Map_loader::removeMapLayer(const ParsedLayer &parsed_layer)
{
    if(parsed_layer.dirt!=NULL)
        delete parsed_layer.dirt;
    if(parsed_layer.monstersCollisionMap!=NULL)
        delete parsed_layer.monstersCollisionMap;
    if(parsed_layer.ledges!=NULL)
        delete parsed_layer.ledges;
    if(parsed_layer.walkable!=NULL)
        delete parsed_layer.walkable;
}

bool Map_loader::tryLoadMap(const std::string &file,const bool &botIsNotWalkable)
{
    error.clear();
    Map_to_send map_to_send_temp;
    map_to_send_temp.border.bottom.x_offset=0;
    map_to_send_temp.border.top.x_offset=0;
    map_to_send_temp.border.left.y_offset=0;
    map_to_send_temp.border.right.y_offset=0;

    std::vector<std::string> detectedMonsterCollisionMonsterType,detectedMonsterCollisionLayer;
    std::vector<char> Walkable,Collisions,Dirt,LedgesRight,LedgesLeft,LedgesBottom,LedgesTop;
    std::vector<std::vector<char> > monsterList;
    std::map<std::string/*layer*/,std::vector<char> > mapLayerContentForMonsterCollision;
    bool ok;
    tinyxml2::XMLDocument *domDocument;

    //open and quick check the file
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=CommonDatapack::commonDatapack.xmlLoadedFile.cend())
        domDocument=&CommonDatapack::commonDatapack.xmlLoadedFile[file];
    else
    {
        domDocument=&CommonDatapack::commonDatapack.xmlLoadedFile[file];
        #else
        domDocument=new tinyxml2::XMLDocument();
        #endif
        const auto loadOkay = domDocument->LoadFile(file.c_str());
        if(loadOkay!=0)
        {
            error=file+", "+tinyxml2errordoc(domDocument);
            return false;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
    }
    #endif
    const tinyxml2::XMLElement * root = domDocument->RootElement();
    if(root==NULL)
    {
        error=file+", tryLoadMap(): no root balise found for the xml file";
        return false;
    }
    if(strcmp(root->Name(),"map")!=0)
    {
        error=file+", tryLoadMap(): \"map\" root balise not found for the xml file";
        return false;
    }

    //get the width
    if(root->Attribute(XMLCACHEDSTRING_width)==NULL)
    {
        error="the root node has not the attribute \"width\"";
        return false;
    }
    map_to_send_temp.width=stringtouint32(root->Attribute("width"),&ok);
    if(!ok)
    {
        error="the root node has wrong attribute \"width\"";
        return false;
    }

    //get the height
    if(root->Attribute("height")==NULL)
    {
        error="the root node has not the attribute \"height\"";
        return false;
    }
    map_to_send_temp.height=stringtouint32(root->Attribute("height"),&ok);
    if(!ok)
    {
        error="the root node has wrong attribute \"height\"";
        return false;
    }

    //check the size
    if(map_to_send_temp.width<1 || map_to_send_temp.width>255)
    {
        error="the width should be greater or equal than 1, and lower or equal than 32000";
        return false;
    }
    if(map_to_send_temp.height<1 || map_to_send_temp.height>255)
    {
        error="the height should be greater or equal than 1, and lower or equal than 32000";
        return false;
    }

    //properties
    const tinyxml2::XMLElement * child = root->FirstChildElement(XMLCACHEDSTRING_properties);
    if(child!=NULL)
    {
        const tinyxml2::XMLElement * SubChild=child->FirstChildElement(XMLCACHEDSTRING_property);
        while(SubChild!=NULL)
        {
            if(SubChild->Attribute(XMLCACHEDSTRING_name)!=NULL && SubChild->Attribute(XMLCACHEDSTRING_value)!=NULL)
                map_to_send_temp.property[std::string(SubChild->Attribute(XMLCACHEDSTRING_name))]=
                        std::string(SubChild->Attribute(XMLCACHEDSTRING_value));
            else
            {
                error=std::string("Missing attribute name or value: child->Name(): ")+std::string(SubChild->Name())+")";
                return false;
            }
            SubChild = SubChild->NextSiblingElement(XMLCACHEDSTRING_property);
        }
    }

    int8_t tilewidth=16;
    int8_t tileheight=16;
    if(root->Attribute(XMLCACHEDSTRING_tilewidth)!=NULL)
    {
        tilewidth=stringtouint8(root->Attribute(XMLCACHEDSTRING_tilewidth),&ok);
        if(!ok)
        {
            std::cerr << "Unable to open the file: " << file << ", tilewidth is not a number" << std::endl;
            tilewidth=16;
        }
    }
    if(root->Attribute(XMLCACHEDSTRING_tileheight)!=NULL)
    {
        tileheight=stringtouint8(root->Attribute(XMLCACHEDSTRING_tileheight),&ok);
        if(!ok)
        {
            std::cerr << "Unable to open the file: " << file << ", tilewidth is not a number" << std::endl;
            tileheight=16;
        }
    }

    // objectgroup
    child = root->FirstChildElement(XMLCACHEDSTRING_objectgroup);
    while(child!=NULL)
    {
        if(child->Attribute(XMLCACHEDSTRING_name)==NULL)
            std::cerr << "Has not attribute \"name\": child->Name(): " << child->Name() << ")";
        else
        {
            if(strcmp(child->Attribute(XMLCACHEDSTRING_name),"Moving")==0)
            {
                const tinyxml2::XMLElement *SubChild=child->FirstChildElement(XMLCACHEDSTRING_object);
                while(SubChild!=NULL)
                {
                    if(SubChild->Attribute(XMLCACHEDSTRING_x)!=NULL && SubChild->Attribute(XMLCACHEDSTRING_y)!=NULL)
                    {
                        const uint32_t &object_x=stringtouint32(SubChild->Attribute(XMLCACHEDSTRING_x),&ok)/tilewidth;
                        if(!ok)
                            std::cerr << "Wrong conversion with x: child->Name(): " << SubChild->Name()
                                        << ", file: " << file << std::endl;
                        else
                        {
                            /** the -1 is important to fix object layer bug into tiled!!!
                             * Don't remove! */
                            const uint32_t &object_y=(stringtouint32(SubChild->Attribute(XMLCACHEDSTRING_y),&ok)/tileheight)-1;

                            if(!ok)
                                std::cerr << "Wrong conversion with y: child->Name(): " << SubChild->Name()
                                            << ", file: " << file << std::endl;
                            else if(object_x>map_to_send_temp.width || object_y>map_to_send_temp.height)
                                std::cerr << "Object out of the map: child->Name(): " << SubChild->Name()
                                            << ", file: " << file << std::endl;
                            else if(SubChild->Attribute(XMLCACHEDSTRING_type)==NULL)
                                std::cerr << "Missing attribute type missing: SubChild->Name(): " << SubChild->Name() << ", file: " << file << std::endl;
                            else
                            {
                                const std::string type(SubChild->Attribute(XMLCACHEDSTRING_type));

                                std::unordered_map<std::string,std::string> property_text;
                                const tinyxml2::XMLElement *prop=SubChild->FirstChildElement(XMLCACHEDSTRING_properties);
                                if(prop!=NULL)
                                {
                                    #ifdef DEBUG_MESSAGE_MAP
                                    std::cerr << "Wrong conversion with y: child->Name(): " << prop->Name()
                                                << " (at line: " << std::to_string(prop->Row())
                                                << "), file: " << fileName << std::endl;
                                    #endif
                                    const tinyxml2::XMLElement *property=prop->FirstChildElement(XMLCACHEDSTRING_property);
                                    while(property!=NULL)
                                    {
                                        if(property->Attribute(XMLCACHEDSTRING_name)!=NULL && property->Attribute(XMLCACHEDSTRING_value)!=NULL)
                                            property_text[std::string(property->Attribute(XMLCACHEDSTRING_name))]=
                                                    std::string(property->Attribute(XMLCACHEDSTRING_value));
                                        property = property->NextSiblingElement(XMLCACHEDSTRING_property);
                                    }
                                }
                                if(type==CACHEDSTRING_borderleft || type==CACHEDSTRING_borderright || type==CACHEDSTRING_bordertop || type==CACHEDSTRING_borderbottom)
                                {
                                    #ifdef DEBUG_MESSAGE_MAP
                                    DebugClass::debugConsole(std::stringLiteral("type: %1, object_x: %2, object_y: %3, border")
                                         .arg(type)
                                         .arg(object_x)
                                         .arg(object_y)
                                         );
                                    #endif
                                    if(property_text.find(CACHEDSTRING_map)!=property_text.cend())
                                    {
                                        if(type=="border-left")//border left
                                        {
                                            const std::string &borderMap=property_text.at(CACHEDSTRING_map);
                                            if(!borderMap.empty())
                                            {
                                                if(map_to_send_temp.border.left.fileName.empty())
                                                {
                                                    map_to_send_temp.border.left.fileName=borderMap;
                                                    if(!stringEndsWith(map_to_send_temp.border.left.fileName,".tmx") && !map_to_send_temp.border.left.fileName.empty())
                                                        map_to_send_temp.border.left.fileName+=".tmx";
                                                    map_to_send_temp.border.left.y_offset=static_cast<uint16_t>(object_y);
                                                }
                                                else
                                                    std::cerr << "The border " << SubChild->Name() << " " << type << " is already set, file: " << file << std::endl;
                                            }
                                            else
                                                std::cerr << "The border " << SubChild->Name() << " " << type << " can't be empty, file: " << file << std::endl;
                                        }
                                        else if(type=="border-right")//border right
                                        {
                                            const std::string &borderMap=property_text.at(CACHEDSTRING_map);
                                            if(!borderMap.empty())
                                            {
                                                if(map_to_send_temp.border.right.fileName.empty())
                                                {
                                                    map_to_send_temp.border.right.fileName=borderMap;
                                                    if(!stringEndsWith(map_to_send_temp.border.right.fileName,".tmx") && !map_to_send_temp.border.right.fileName.empty())
                                                        map_to_send_temp.border.right.fileName+=".tmx";
                                                    map_to_send_temp.border.right.y_offset=static_cast<uint16_t>(object_y);
                                                }
                                                else
                                                    std::cerr << "The border " << SubChild->Name() << " " << type << " is already set, file: " << file << std::endl;
                                            }
                                            else
                                                std::cerr << "The border " << SubChild->Name() << " " << type << " can't be empty, file: " << file << std::endl;
                                        }
                                        else if(type=="border-top")//border top
                                        {
                                            const std::string &borderMap=property_text.at(CACHEDSTRING_map);
                                            if(!borderMap.empty())
                                            {
                                                if(map_to_send_temp.border.top.fileName.empty())
                                                {
                                                    map_to_send_temp.border.top.fileName=borderMap;
                                                    if(!stringEndsWith(map_to_send_temp.border.top.fileName,".tmx") && !map_to_send_temp.border.top.fileName.empty())
                                                        map_to_send_temp.border.top.fileName+=".tmx";
                                                    map_to_send_temp.border.top.x_offset=static_cast<uint16_t>(object_x);
                                                }
                                                else
                                                    std::cerr << "The border " << SubChild->Name() << " " << type << " is already set, file: " << file << std::endl;
                                            }
                                            else
                                                std::cerr << "The border " << SubChild->Name() << " " << type << " can't be empty, file: " << file << std::endl;
                                        }
                                        else if(type=="border-bottom")//border bottom
                                        {
                                            const std::string &borderMap=property_text.at(CACHEDSTRING_map);
                                            if(!borderMap.empty())
                                            {
                                                if(map_to_send_temp.border.bottom.fileName.empty())
                                                {
                                                    map_to_send_temp.border.bottom.fileName=borderMap;
                                                    if(!stringEndsWith(map_to_send_temp.border.bottom.fileName,".tmx") && !map_to_send_temp.border.bottom.fileName.empty())
                                                        map_to_send_temp.border.bottom.fileName+=".tmx";
                                                    map_to_send_temp.border.bottom.x_offset=static_cast<uint16_t>(object_x);
                                                }
                                                else
                                                    std::cerr << "The border " << SubChild->Name() << " " << type << " is already set, file: " << file << std::endl;
                                            }
                                            else
                                                std::cerr << "The border " << SubChild->Name() << " " << type << " can't be empty, file: " << file << std::endl;
                                        }
                                        else
                                            std::cerr << "Not at middle of border: child->Name(): " << SubChild->Name() << ", object_x: " << object_x << ", object_y: " << object_y << ", file: " << file << std::endl;
                                    }
                                    else
                                        std::cerr << "Missing \"map\" properties for the border: " << SubChild->Name() << "" << std::endl;
                                }
                                else if(type=="teleport on push" || type=="teleport on it" || type=="door")
                                {
                                    if(property_text.find(CACHEDSTRING_map)!=property_text.cend() && property_text.find(CACHEDSTRING_x)!=property_text.cend() && property_text.find(CACHEDSTRING_y)!=property_text.cend())
                                    {
                                        Map_semi_teleport new_tp;
                                        new_tp.source_x=static_cast<uint8_t>(object_x);
                                        new_tp.source_y=static_cast<uint8_t>(object_y);
                                        new_tp.condition.type=MapConditionType_None;
                                        new_tp.condition.data.fightBot=0;
                                        new_tp.condition.data.item=0;
                                        new_tp.condition.data.quest=0;
                                        new_tp.conditionUnparsed=NULL;
                                        new_tp.destination_x = stringtouint8(property_text.at(CACHEDSTRING_x),&ok);
                                        if(ok)
                                        {
                                            new_tp.destination_y = stringtouint8(property_text.at(CACHEDSTRING_y),&ok);
                                            if(ok)
                                            {
                                                //std::cerr << "CACHEDSTRING_condition_file: " << CACHEDSTRING_condition_file << std::endl;
                                                //std::cerr << "CACHEDSTRING_condition_id: " << CACHEDSTRING_condition_id << std::endl;
                                                if(property_text.find(CACHEDSTRING_condition_file)!=property_text.cend() && property_text.find(CACHEDSTRING_condition_id)!=property_text.cend())
                                                {
                                                    uint32_t conditionId=stringtouint32(property_text.at(CACHEDSTRING_condition_id),&ok);
                                                    if(!ok)
                                                        std::cerr << "condition id is not a number, id: " << property_text.at(CACHEDSTRING_condition_id) << " (" << file << ")" << std::endl;
                                                    else
                                                    {
                                                        std::string conditionFile=FSabsoluteFilePath(
                                                                    FSabsolutePath(file)+
                                                                    "/"+
                                                                    property_text.at(CACHEDSTRING_condition_file)
                                                                    );
                                                        if(!stringEndsWith(conditionFile,".xml"))
                                                            conditionFile+=".xml";
                                                        new_tp.conditionUnparsed=getXmlCondition(file,conditionFile,conditionId);
                                                        new_tp.condition=xmlConditionToMapCondition(conditionFile,new_tp.conditionUnparsed);
                                                    }
                                                }
                                                new_tp.map=property_text.at(CACHEDSTRING_map);
                                                if(!stringEndsWith(new_tp.map,".tmx") && !new_tp.map.empty())
                                                    new_tp.map+=".tmx";
                                                map_to_send_temp.teleport.push_back(new_tp);
                                            }
                                            else
                                                std::cerr << "Bad convertion to int for y, type: " << type << ", value: " << property_text.at(CACHEDSTRING_y) << " (" << file << ")" << std::endl;
                                        }
                                        else
                                            std::cerr << "Bad convertion to int for x, type: " << type << ", value: " << property_text.at(CACHEDSTRING_x) << " (" << file << ")" << std::endl;
                                    }
                                    else
                                        std::cerr << "Missing map,x or y, type: " << type << ", file: " << file << std::endl;
                                }
                                else if(type=="rescue")
                                {
                                    Map_to_send::Map_Point tempPoint;
                                    tempPoint.x=static_cast<uint8_t>(object_x);
                                    tempPoint.y=static_cast<uint8_t>(object_y);
                                    map_to_send_temp.rescue_points.push_back(tempPoint);
                                    //map_to_send_temp.bot_spawn_points << tempPoint;
                                }
                                else
                                {
                                    std::cerr << "Unknown type: " << type
                                              << ", object_x: " << object_x
                                              << ", object_y: " << object_y
                                              << " (moving), " << SubChild->Name()
                                              << "file: " << file << std::endl;
                                }
                            }
                        }
                    }
                    else
                        std::cerr << "Is not Element: SubChild->Name(): " << SubChild->Name() << ", file: " << file << std::endl;
                    SubChild = SubChild->NextSiblingElement(XMLCACHEDSTRING_object);
                }
            }
            if(strcmp(child->Attribute(XMLCACHEDSTRING_name),"Object")==0)
            {
                const tinyxml2::XMLElement * SubChild=child->FirstChildElement(XMLCACHEDSTRING_object);
                while(SubChild!=NULL)
                {
                    if(SubChild->Attribute(XMLCACHEDSTRING_x)!=NULL && SubChild->Attribute(XMLCACHEDSTRING_y)!=NULL)
                    {
                        const uint32_t &object_x=stringtouint32(SubChild->Attribute(XMLCACHEDSTRING_x),&ok)/tilewidth;
                        if(!ok)
                            std::cerr << "Wrong conversion with x: child->Name(): " << SubChild->Name()
                                        << ", file: " << file << std::endl;
                        else
                        {
                            /** the -1 is important to fix object layer bug into tiled!!!
                             * Don't remove! */
                            const uint32_t &object_y=(stringtouint32(SubChild->Attribute(XMLCACHEDSTRING_y),&ok)/tileheight)-1;

                            if(!ok)
                                std::cerr << "Wrong conversion with y: child->Name(): " << SubChild->Name()
                                            << ", file: " << file << std::endl;
                            else if(object_x>map_to_send_temp.width || object_y>map_to_send_temp.height)
                                std::cerr << "Object out of the map: child->Name(): " << SubChild->Name()
                                            << ", file: " << file << std::endl;
                            else if(SubChild->Attribute(XMLCACHEDSTRING_type)!=NULL)
                            {
                                const std::string &type=SubChild->Attribute(XMLCACHEDSTRING_type);

                                std::unordered_map<std::string,std::string> property_text;
                                const tinyxml2::XMLElement * prop=SubChild->FirstChildElement(XMLCACHEDSTRING_properties);
                                if(prop!=NULL)
                                {
                                    const tinyxml2::XMLElement * property=prop->FirstChildElement(XMLCACHEDSTRING_property);
                                    while(property!=NULL)
                                    {
                                        if(property->Attribute(XMLCACHEDSTRING_name)!=NULL && property->Attribute(XMLCACHEDSTRING_value)!=NULL)
                                            property_text[std::string(property->Attribute(XMLCACHEDSTRING_name))]=
                                                    std::string(property->Attribute(XMLCACHEDSTRING_value));
                                        property = property->NextSiblingElement(XMLCACHEDSTRING_property);
                                    }
                                }
                                if(type==CACHEDSTRING_bot)
                                {
                                    if(property_text.find(CACHEDSTRING_skin)!=property_text.cend() && !property_text.at(CACHEDSTRING_skin).empty() && property_text.find(CACHEDSTRING_lookAt)==property_text.cend())
                                    {
                                        property_text[CACHEDSTRING_lookAt]="bottom";
                                        std::cerr << "skin but not lookAt, fixed by bottom: " << SubChild->Name() << " (" << file << ")" << std::endl;
                                    }
                                    if(property_text.find(CACHEDSTRING_file)!=property_text.cend() && property_text.find(CACHEDSTRING_id)!=property_text.cend())
                                    {
                                        if(!stringEndsWith(property_text["file"],".xml"))
                                            property_text["file"]+=".xml";
                                        Map_to_send::Bot_Semi bot_semi;
                                        bot_semi.file=FSabsoluteFilePath(
                                                            FSabsolutePath(file)+
                                                            "/"+
                                                            property_text.at(CACHEDSTRING_file)
                                                    );
                                        bot_semi.id=stringtouint16(property_text.at(CACHEDSTRING_id),&ok);
                                        bot_semi.property_text=property_text;
                                        if(ok)
                                        {
                                            bot_semi.point.x=static_cast<uint8_t>(object_x);
                                            bot_semi.point.y=static_cast<uint8_t>(object_y);
                                            map_to_send_temp.bots.push_back(bot_semi);
                                        }
                                    }
                                    else
                                        std::cerr << "Missing \"bot\" properties for the bot: " << SubChild->Name()
                                                  << ", file: " << file << std::endl;
                                }
                                else if(type=="object")
                                {
                                    if(property_text.find(CACHEDSTRING_item)!=property_text.cend())
                                    {
                                        Map_to_send::ItemOnMap_Semi item_semi;
                                        item_semi.infinite=false;
                                        if(property_text.find(CACHEDSTRING_infinite)!=property_text.cend() && property_text.at(CACHEDSTRING_infinite)==CACHEDSTRING_true)
                                            item_semi.infinite=true;
                                        item_semi.visible=true;
                                        if(property_text.find(CACHEDSTRING_visible)!=property_text.cend() && property_text.at(CACHEDSTRING_visible)==CACHEDSTRING_false)
                                            item_semi.visible=false;
                                        item_semi.item=stringtouint16(property_text.at(CACHEDSTRING_item),&ok);
                                        if(ok)
                                        {
                                            item_semi.point.x=static_cast<uint8_t>(object_x);
                                            item_semi.point.y=static_cast<uint8_t>(object_y);
                                            map_to_send_temp.items.push_back(item_semi);
                                        }
                                    }
                                    else
                                        std::cerr << "Missing \"bot\" properties for the bot: " << SubChild->Name()
                                                  << ", file: " << file
                                                  << std::endl;
                                }
                                else if(type=="StrengthBoulder")
                                {}
                                else
                                {
                                    std::cerr << "Unknown type: " << type
                                              << ", object_x: " << object_x
                                              << ", object_y: " << object_y
                                              << " (object), " << SubChild->Name()
                                              << ", file: " << file
                                              << std::endl;
                                }
                            }
                            else
                                std::cerr << "Missing attribute type missing: SubChild->Name(): " << SubChild->Name() << ", file: " << file << std::endl;
                        }
                    }
                    else
                        std::cerr << "Is not Element: SubChild->Name(): " << SubChild->Name() << ", file: " << file << std::endl;
                    SubChild = SubChild->NextSiblingElement(XMLCACHEDSTRING_object);
                }
            }
        }
        child = child->NextSiblingElement(XMLCACHEDSTRING_objectgroup);
    }

    const uint32_t &rawSize=map_to_send_temp.width*map_to_send_temp.height*4;

    // layer
    child = root->FirstChildElement(XMLCACHEDSTRING_layer);
    while(child!=NULL)
    {
        if(child->Attribute(XMLCACHEDSTRING_name)==NULL)
        {
            error=std::string("Has not attribute \"name\": child->Name(): ")+child->Name()+", file: "+file;
            return false;
        }
        else
        {
            const tinyxml2::XMLElement *data=child->FirstChildElement(XMLCACHEDSTRING_data);
            const std::string name=child->Attribute(XMLCACHEDSTRING_name);
            if(data==NULL)
            {
                error=std::string("Is Element for layer is null: ")+data->Name()+" and name: "+name+", file: "+file;
                return false;
            }
            else if(data->Attribute(XMLCACHEDSTRING_encoding)==NULL)
            {
                error=std::string("Has not attribute \"base64\": child->Name(): ")+data->Name()+", file: "+file;
                return false;
            }
            else if(data->Attribute(XMLCACHEDSTRING_compression)==NULL)
            {
                error=std::string("Has not attribute \"zlib\": child->Name(): ")+data->Name()+", file: "+file;
                return false;
            }
            else if(strcmp(data->Attribute(XMLCACHEDSTRING_encoding),XMLCACHEDSTRING_base64)!=0)
            {
                error=std::string("only encoding base64 is supported, file: ")+file;
                return false;
            }
            else if(
                    #ifdef TILED_ZLIB
                    strcmp(data->Attribute(XMLCACHEDSTRING_compression),XMLCACHEDSTRING_zlib)!=0
                    &&
                    #endif
                    strcmp(data->Attribute(XMLCACHEDSTRING_compression),"zstd")!=0
                    )
            {
                #ifdef TILED_ZLIB
                error=std::string("Only compression zlib or zstd is supported, file: ")+file;
                #else
                error=std::string("Only compression zstd is supported, file: ")+file;
                #endif
                return false;
            }
            else
            {
                std::string base64text;
                if(data->GetText()!=NULL)
                base64text=FacilityLibGeneral::dropPrefixAndSuffixLowerThen33(data->GetText());
                const std::vector<char> &compressedData=base64toBinary(base64text);
                if(!compressedData.empty())
                {
                    std::vector<char> dataRaw;
                    dataRaw.resize(map_to_send_temp.height*map_to_send_temp.width*4);
                    int32_t decompressedSize=0;
                    #ifdef TILED_ZLIB
                    if(strcmp(data->Attribute(XMLCACHEDSTRING_compression),XMLCACHEDSTRING_zlib)==0)
                        decompressedSize=decompressZlib(
                                compressedData.data(),static_cast<uint32_t>(compressedData.size()),
                                dataRaw.data(),static_cast<uint32_t>(dataRaw.size())
                                    );
                    else
                    #endif
                        {
                            decompressedSize=ZSTD_decompress(dataRaw.data(),static_cast<uint32_t>(dataRaw.size()), compressedData.data(),static_cast<uint32_t>(compressedData.size()));
                            if (ZSTD_isError(decompressedSize)) {
                                std::cerr << "error decoding: " << ZSTD_getErrorName(decompressedSize) << std::endl;
                                decompressedSize=0;
                            }
                        }

                    if((uint32_t)decompressedSize!=map_to_send_temp.height*map_to_send_temp.width*4)
                    {
                        error=std::string("map binary size (")+std::to_string(dataRaw.size())+") != "+std::to_string(map_to_send_temp.height)+"x"+std::to_string(map_to_send_temp.width)+"x4";
                        if(data->GetText()!=NULL)
                            std::cerr << "base64 dump: \"" << data->GetText() << "\"" << std::endl;
                        return false;
                    }
                    if(name=="Walkable")
                    {
                        if(Walkable.empty())
                            Walkable=dataRaw;
                        else
                        {
                            unsigned int index=0;
                            while(index<Walkable.size())
                            {
                                Walkable[index]=Walkable.at(index) || dataRaw.at(index);
                                index++;
                            }
                        }
                    }
                    else if(name=="Collisions")
                    {
                        if(Collisions.empty())
                            Collisions=dataRaw;
                        else
                        {
                            unsigned int index=0;
                            while(index<Collisions.size())
                            {
                                Collisions[index]=Collisions.at(index) || dataRaw.at(index);
                                index++;
                            }
                        }
                    }
                    else if(name=="Dirt")
                    {
                        if(Dirt.empty())
                            Dirt=dataRaw;
                        else
                        {
                            unsigned int index=0;
                            while(index<Dirt.size())
                            {
                                Dirt[index]=Dirt.at(index) || dataRaw.at(index);
                                index++;
                            }
                        }
                    }
                    else if(name=="LedgesRight")
                    {
                        if(LedgesRight.empty())
                            LedgesRight=dataRaw;
                        else
                        {
                            unsigned int index=0;
                            while(index<LedgesRight.size())
                            {
                                LedgesRight[index]=LedgesRight.at(index) || dataRaw.at(index);
                                index++;
                            }
                        }
                    }
                    else if(name=="LedgesLeft")
                    {
                        if(LedgesLeft.empty())
                            LedgesLeft=dataRaw;
                        else
                        {
                            unsigned int index=0;
                            while(index<LedgesLeft.size())
                            {
                                LedgesLeft[index]=LedgesLeft.at(index) || dataRaw.at(index);
                                index++;
                            }
                        }
                    }
                    else if(name=="LedgesBottom" || name=="LedgesDown")
                    {
                        if(LedgesBottom.empty())
                            LedgesBottom=dataRaw;
                        else
                        {
                            unsigned int index=0;
                            while(index<LedgesBottom.size())
                            {
                                LedgesBottom[index]=LedgesBottom.at(index) || dataRaw.at(index);
                                index++;
                            }
                        }
                    }
                    else if(name=="LedgesTop" || name=="LedgesUp")
                    {
                        if(LedgesTop.empty())
                            LedgesTop=dataRaw;
                        else
                        {
                            unsigned int index=0;
                            while(index<LedgesTop.size())
                            {
                                LedgesTop[index]=LedgesTop.at(index) || dataRaw.at(index);
                                index++;
                            }
                        }
                    }
                    else
                    {
                        if(!name.empty() && rawSize==(uint32_t)dataRaw.size())
                        {
                            unsigned int index=0;
                            while(index<CommonDatapack::commonDatapack.monstersCollision.size())
                            {
                                if(CommonDatapack::commonDatapack.monstersCollision.at(index).layer==name)
                                {
                                    mapLayerContentForMonsterCollision[name]=dataRaw;
                                    {
                                        const std::vector<std::string> &monsterTypeListText=CommonDatapack::commonDatapack.monstersCollision.at(index).monsterTypeList;
                                        unsigned int monsterTypeListIndex=0;
                                        while(monsterTypeListIndex<monsterTypeListText.size())
                                        {
                                            if(!vectorcontainsAtLeastOne(detectedMonsterCollisionMonsterType,monsterTypeListText.at(monsterTypeListIndex)))
                                                detectedMonsterCollisionMonsterType.push_back(monsterTypeListText.at(monsterTypeListIndex));
                                            monsterTypeListIndex++;
                                        }
                                    }
                                    if(!vectorcontainsAtLeastOne(detectedMonsterCollisionLayer,name))
                                        detectedMonsterCollisionLayer.push_back(name);
                                }
                                index++;
                            }
                            monsterList.push_back(dataRaw);
                        }
                    }
                }
                else
                {
                    error=std::string("base64 encoding layer corrupted \"")+name+"\": child->Name(): "+child->Name()+", file: "+file;
                    return false;
                }
            }
        }
        child = child->NextSiblingElement(XMLCACHEDSTRING_layer);
    }

    /*std::vector<char> null_data;
    null_data.resize(4);
    null_data[0]=0x00;
    null_data[1]=0x00;
    null_data[2]=0x00;
    null_data[3]=0x00;*/

    if(Walkable.size()>0)
        map_to_send_temp.parsed_layer.walkable	= new bool[map_to_send_temp.width*map_to_send_temp.height];
    else
        map_to_send_temp.parsed_layer.walkable	= NULL;
    map_to_send_temp.parsed_layer.monstersCollisionMap		= new uint8_t[map_to_send_temp.width*map_to_send_temp.height];
    if(Dirt.size()>0)
        map_to_send_temp.parsed_layer.dirt		= new bool[map_to_send_temp.width*map_to_send_temp.height];
    else
        map_to_send_temp.parsed_layer.dirt		= NULL;
    if(LedgesRight.size()>0 || LedgesLeft.size()>0 || LedgesBottom.size()>0 || LedgesTop.size()>0)
        map_to_send_temp.parsed_layer.ledges		= new uint8_t[map_to_send_temp.width*map_to_send_temp.height];
    else
        map_to_send_temp.parsed_layer.ledges		= NULL;

    uint32_t x=0;
    uint32_t y=0;

    char * WalkableBin=NULL;
    char * CollisionsBin=NULL;
    char * DirtBin=NULL;
    char * LedgesRightBin=NULL;
    char * LedgesLeftBin=NULL;
    char * LedgesBottomBin=NULL;
    char * LedgesTopBin=NULL;
    std::vector<std::vector<char> > MonsterCollisionBin;
    {
        if(rawSize==(uint32_t)Walkable.size())
            WalkableBin=Walkable.data();
        if(rawSize==(uint32_t)Collisions.size())
            CollisionsBin=Collisions.data();
        if(rawSize==(uint32_t)Dirt.size())
            DirtBin=Dirt.data();
        if(rawSize==(uint32_t)LedgesRight.size())
            LedgesRightBin=LedgesRight.data();
        if(rawSize==(uint32_t)LedgesLeft.size())
            LedgesLeftBin=LedgesLeft.data();
        if(rawSize==(uint32_t)LedgesBottom.size())
            LedgesBottomBin=LedgesBottom.data();
        if(rawSize==(uint32_t)LedgesTop.size())
            LedgesTopBin=LedgesTop.data();

        auto i=mapLayerContentForMonsterCollision.begin();
        while(i!=mapLayerContentForMonsterCollision.cend())
        {
            MonsterCollisionBin.push_back(i->second);
            ++i;
        }
    }

    bool walkable=false,collisions=false,monsterCollision=false,dirt=false,ledgesRight=false,ledgesLeft=false,ledgesBottom=false,ledgesTop=false;
    while(x<map_to_send_temp.width)
    {
        y=0;
        while(y<map_to_send_temp.height)
        {
            if(WalkableBin!=NULL)
                walkable=WalkableBin[x*4+y*map_to_send_temp.width*4+0]!=0x00 || WalkableBin[x*4+y*map_to_send_temp.width*4+1]!=0x00 || WalkableBin[x*4+y*map_to_send_temp.width*4+2]!=0x00 || WalkableBin[x*4+y*map_to_send_temp.width*4+3]!=0x00;
            else
                walkable=false;
            if(CollisionsBin!=NULL)
                collisions=CollisionsBin[x*4+y*map_to_send_temp.width*4+0]!=0x00 || CollisionsBin[x*4+y*map_to_send_temp.width*4+1]!=0x00 || CollisionsBin[x*4+y*map_to_send_temp.width*4+2]!=0x00 || CollisionsBin[x*4+y*map_to_send_temp.width*4+3]!=0x00;
            else
                collisions=false;
            if(DirtBin!=NULL)
                dirt=DirtBin[x*4+y*map_to_send_temp.width*4+0]!=0x00 || DirtBin[x*4+y*map_to_send_temp.width*4+1]!=0x00 || DirtBin[x*4+y*map_to_send_temp.width*4+2]!=0x00 || DirtBin[x*4+y*map_to_send_temp.width*4+3]!=0x00;
            else
                dirt=false;
            if(LedgesRightBin!=NULL)
                ledgesRight=LedgesRightBin[x*4+y*map_to_send_temp.width*4+0]!=0x00 || LedgesRightBin[x*4+y*map_to_send_temp.width*4+1]!=0x00 || LedgesRightBin[x*4+y*map_to_send_temp.width*4+2]!=0x00 || LedgesRightBin[x*4+y*map_to_send_temp.width*4+3]!=0x00;
            else
                ledgesRight=false;
            if(LedgesLeftBin!=NULL)
                ledgesLeft=LedgesLeftBin[x*4+y*map_to_send_temp.width*4+0]!=0x00 || LedgesLeftBin[x*4+y*map_to_send_temp.width*4+1]!=0x00 || LedgesLeftBin[x*4+y*map_to_send_temp.width*4+2]!=0x00 || LedgesLeftBin[x*4+y*map_to_send_temp.width*4+3]!=0x00;
            else
                ledgesLeft=false;
            if(LedgesBottomBin!=NULL)
                ledgesBottom=LedgesBottomBin[x*4+y*map_to_send_temp.width*4+0]!=0x00 || LedgesBottomBin[x*4+y*map_to_send_temp.width*4+1]!=0x00 || LedgesBottomBin[x*4+y*map_to_send_temp.width*4+2]!=0x00 || LedgesBottomBin[x*4+y*map_to_send_temp.width*4+3]!=0x00;
            else
                ledgesBottom=false;
            if(LedgesTopBin!=NULL)
                ledgesTop=LedgesTopBin[x*4+y*map_to_send_temp.width*4+0]!=0x00 || LedgesTopBin[x*4+y*map_to_send_temp.width*4+1]!=0x00 || LedgesTopBin[x*4+y*map_to_send_temp.width*4+2]!=0x00 || LedgesTopBin[x*4+y*map_to_send_temp.width*4+3]!=0x00;
            else
                ledgesTop=false;
            monsterCollision=false;
            unsigned int index=0;
            while(index<MonsterCollisionBin.size())
            {
                if(MonsterCollisionBin.at(index)[x*4+y*map_to_send_temp.width*4+0]!=0x00 || MonsterCollisionBin.at(index)[x*4+y*map_to_send_temp.width*4+1]!=0x00 || MonsterCollisionBin.at(index)[x*4+y*map_to_send_temp.width*4+2]!=0x00 || MonsterCollisionBin.at(index)[x*4+y*map_to_send_temp.width*4+3]!=0x00)
                {
                    monsterCollision=true;
                    break;
                }
                index++;
            }

            if(Walkable.size()>0)
                map_to_send_temp.parsed_layer.walkable[x+y*map_to_send_temp.width]=(walkable || monsterCollision) && !collisions && !dirt;
            if(Dirt.size()>0)
            {
                if(dirt)
                {
                    Map_to_send::DirtOnMap_Semi dirtOnMap_Semi;
                    dirtOnMap_Semi.point.x=static_cast<uint8_t>(x);
                    dirtOnMap_Semi.point.y=static_cast<uint8_t>(y);
                    map_to_send_temp.dirts.push_back(dirtOnMap_Semi);
                }
                map_to_send_temp.parsed_layer.dirt[x+y*map_to_send_temp.width]=dirt;
            }
            if(LedgesRight.size()>0 || LedgesLeft.size()>0 || LedgesBottom.size()>0 || LedgesTop.size()>0)
            {
                map_to_send_temp.parsed_layer.ledges[x+y*map_to_send_temp.width]=(uint8_t)ParsedLayerLedges_NoLedges;
                if(ledgesLeft)
                {
                    if(ledgesRight || ledgesBottom || ledgesTop)
                    {
                        std::cerr << "Multiple ledges at the same place, do colision for left" << std::endl;
                        map_to_send_temp.parsed_layer.walkable[x+y*map_to_send_temp.width]=false;
                    }
                    else if(map_to_send_temp.parsed_layer.walkable[x+y*map_to_send_temp.width])
                        map_to_send_temp.parsed_layer.ledges[x+y*map_to_send_temp.width]=(uint8_t)ParsedLayerLedges_LedgesLeft;
                }
                if(ledgesRight)
                {
                    if(ledgesLeft || ledgesBottom || ledgesTop)
                    {
                        std::cerr << "Multiple ledges at the same place, do colision for right" << std::endl;
                        map_to_send_temp.parsed_layer.walkable[x+y*map_to_send_temp.width]=false;
                    }
                    else if(map_to_send_temp.parsed_layer.walkable[x+y*map_to_send_temp.width])
                        map_to_send_temp.parsed_layer.ledges[x+y*map_to_send_temp.width]=(uint8_t)ParsedLayerLedges_LedgesRight;
                }
                if(ledgesTop)
                {
                    if(ledgesRight || ledgesBottom || ledgesLeft)
                    {
                        std::cerr << "Multiple ledges at the same place, do colision for top" << std::endl;
                        map_to_send_temp.parsed_layer.walkable[x+y*map_to_send_temp.width]=false;
                    }
                    else if(map_to_send_temp.parsed_layer.walkable[x+y*map_to_send_temp.width])
                        map_to_send_temp.parsed_layer.ledges[x+y*map_to_send_temp.width]=(uint8_t)ParsedLayerLedges_LedgesTop;
                }
                if(ledgesBottom)
                {
                    if(ledgesRight || ledgesLeft || ledgesTop)
                    {
                        std::cerr << "Multiple ledges at the same place, do colision for bottom" << std::endl;
                        map_to_send_temp.parsed_layer.walkable[x+y*map_to_send_temp.width]=false;
                    }
                    else if(map_to_send_temp.parsed_layer.walkable[x+y*map_to_send_temp.width])
                        map_to_send_temp.parsed_layer.ledges[x+y*map_to_send_temp.width]=(uint8_t)ParsedLayerLedges_LedgesBottom;
                }
            }
            y++;
        }
        x++;
    }

    if(Walkable.size()>0)
    {
        size_t index=0;
        {
            while(index<map_to_send_temp.teleport.size())
            {
                const Map_semi_teleport &map_semi_teleport=map_to_send_temp.teleport.at(index);
                if(map_semi_teleport.source_x<map_to_send_temp.width && map_semi_teleport.source_y<map_to_send_temp.height)
                    map_to_send_temp.parsed_layer.walkable[map_semi_teleport.source_x+map_semi_teleport.source_y*map_to_send_temp.width]=true;
                else
                    std::cerr << "teleporter out of map on " << file << ", source: " << std::to_string(map_semi_teleport.source_x) << "," << std::to_string(map_semi_teleport.source_y) << std::endl;
                index++;
            }
        }
        if(botIsNotWalkable)
        {
            index=0;
            while(index<map_to_send_temp.bots.size())
            {
                const Map_to_send::Bot_Semi &bot=map_to_send_temp.bots.at(index);
                if(bot.point.x<map_to_send_temp.width && bot.point.y<map_to_send_temp.height)
                {
                    if(bot.property_text.find(CACHEDSTRING_skin)!=bot.property_text.cend() && (bot.property_text.find(CACHEDSTRING_lookAt)==bot.property_text.cend() || bot.property_text.at(CACHEDSTRING_lookAt)!=CACHEDSTRING_move))
                        map_to_send_temp.parsed_layer.walkable[bot.point.x+bot.point.y*map_to_send_temp.width]=false;
                }
                else
                    std::cerr << "bot out of map on " << file << ", source: " << std::to_string(bot.point.x) << "," << std::to_string(bot.point.y) << std::endl;
                index++;
            }
            index=0;
            while(index<map_to_send_temp.items.size())
            {
                const Map_to_send::ItemOnMap_Semi &item_semi=map_to_send_temp.items.at(index);
                if(item_semi.point.x<map_to_send_temp.width && item_semi.point.y<map_to_send_temp.height)
                {
                    if(item_semi.visible && item_semi.infinite)
                        map_to_send_temp.parsed_layer.walkable[item_semi.point.x+item_semi.point.y*map_to_send_temp.width]=false;
                }
                else
                    std::cerr << "item out of map on " << file << ", source: " << std::to_string(item_semi.point.x) << "," << std::to_string(item_semi.point.y) << std::endl;
                index++;
            }
        }
    }

    //don't put code here !!!!!! put before the last block
    this->map_to_send=map_to_send_temp;

    std::string xmlExtra=file;
    stringreplaceAll(xmlExtra,".tmx",".xml");
    loadMonsterMap(xmlExtra,detectedMonsterCollisionMonsterType,detectedMonsterCollisionLayer);

    bool previousHaveMonsterWarn=false;
    {
        this->map_to_send.parsed_layer.monstersCollisionMap=new uint8_t[this->map_to_send.width*this->map_to_send.height];
        memset(this->map_to_send.parsed_layer.monstersCollisionMap,0,this->map_to_send.width*this->map_to_send.height);

        {
            auto i=mapLayerContentForMonsterCollision.begin();
            while(i!=mapLayerContentForMonsterCollision.cend())
            {
                if(zoneNumber.find(i->first)!=zoneNumber.cend())
                {
                    const std::string &zoneName=i->first;
                    const uint8_t &zoneId=zoneNumber.at(zoneName);
                    uint8_t x=0;
                    while(x<this->map_to_send.width)
                    {
                        uint8_t y=0;
                        while(y<this->map_to_send.height)
                        {
                            unsigned int value=reinterpret_cast<const unsigned int *>(i->second.data())[x+y*map_to_send_temp.width];
                            if(value!=0 && map_to_send_temp.parsed_layer.walkable!=NULL && map_to_send_temp.parsed_layer.walkable[x+y*this->map_to_send.width])
                            {
                                if(map_to_send_temp.parsed_layer.ledges==NULL || map_to_send_temp.parsed_layer.ledges[x+y*this->map_to_send.width]==(uint8_t)ParsedLayerLedges_NoLedges)
                                {
                                    if(this->map_to_send.parsed_layer.monstersCollisionMap[x+y*this->map_to_send.width]==0)
                                        this->map_to_send.parsed_layer.monstersCollisionMap[x+y*this->map_to_send.width]=zoneId;
                                    else if(this->map_to_send.parsed_layer.monstersCollisionMap[x+y*this->map_to_send.width]==zoneId)
                                    {}//ignore, same zone
                                    else
                                    {
                                        if(!previousHaveMonsterWarn)
                                        {
                                            std::cerr << "Have already monster at " << std::to_string(x) << "," << std::to_string(y) << " for " << file
                                                      << ", actual zone: " << std::to_string(this->map_to_send.parsed_layer.monstersCollisionMap[x+y*this->map_to_send.width])
                                                      << " (" << CommonDatapack::commonDatapack.monstersCollision.at(this->map_to_send.parsed_layer.monstersCollisionMap[x+y*this->map_to_send.width]).layer
                                                      << "), new zone: " << std::to_string(zoneId) << " (" << CommonDatapack::commonDatapack.monstersCollision.at(zoneId).layer << ")" << std::endl;
                                            previousHaveMonsterWarn=true;
                                        }
                                        this->map_to_send.parsed_layer.monstersCollisionMap[x+y*this->map_to_send.width]=zoneId;//overwrited by above layer
                                    }
                                }
                            }
                            y++;
                        }
                        x++;
                    }
                }
                ++i;
            }
        }

        {
            if(this->map_to_send.parsed_layer.monstersCollisionList.empty())
            {
                delete this->map_to_send.parsed_layer.monstersCollisionMap;
                this->map_to_send.parsed_layer.monstersCollisionMap=NULL;
            }
            if(this->map_to_send.parsed_layer.monstersCollisionList.size()==1 && this->map_to_send.parsed_layer.monstersCollisionList.front().actionOn.empty() && this->map_to_send.parsed_layer.monstersCollisionList.front().walkOn.empty())
            {
                this->map_to_send.parsed_layer.monstersCollisionList.clear();
                delete this->map_to_send.parsed_layer.monstersCollisionMap;
                this->map_to_send.parsed_layer.monstersCollisionMap=NULL;
            }
        }
    }

#ifdef DEBUG_MESSAGE_MAP_RAW
    if(Walkable.size()>0 || this->map_to_send.parsed_layer.monstersCollisionMap!=NULL || Collisions.size()>0 || Dirt.size()>0)
    {
        std::vector<char> null_data;
        null_data.resize(4);
        null_data[0]=0x00;
        null_data[1]=0x00;
        null_data[2]=0x00;
        null_data[3]=0x00;
        x=0;
        y=0;
        std::vector<std::string> layers_name;
        if(Walkable.size()>0)
            layers_name << "Walkable";
        if(this->map_to_send.parsed_layer.monstersCollisionMap!=NULL)
            layers_name << "Monster zone";
        if(Collisions.size()>0)
            layers_name << "Collisions";
        if(Dirt.size()>0)
            layers_name << "Dirt";
        if(LedgesRight.size()>0 || LedgesLeft.size()>0 || LedgesBottom.size()>0 || LedgesTop.size()>0)
            layers_name << "Ledges*";
        DebugClass::debugConsole("For "+fileName+": "+layers_name.join(" + ")+" = Walkable");
        while(y<this->map_to_send.height)
        {
            std::string line;
            if(Walkable.size()>0)
            {
                x=0;
                while(x<this->map_to_send.width)
                {
                    line += std::to_string(Walkable.mid(x*4+y*this->map_to_send.width*4,4)!=null_data);
                    x++;
                }
                line+=" ";
            }
            if(this->map_to_send.parsed_layer.monstersCollisionMap!=NULL)
            {
                x=0;
                while(x<this->map_to_send.width)
                {
                    line += std::to_string(this->map_to_send.parsed_layer.monstersCollisionMap[x+y*this->map_to_send.width]);
                    x++;
                }
                line+=" ";
            }
            if(Collisions.size()>0)
            {
                x=0;
                while(x<this->map_to_send.width)
                {
                    line += std::to_string(Collisions.mid(x*4+y*this->map_to_send.width*4,4)!=null_data);
                    x++;
                }
                line+=" ";
            }
            if(Dirt.size()>0)
            {
                x=0;
                while(x<this->map_to_send.width)
                {
                    line += std::to_string(Dirt.mid(x*4+y*this->map_to_send.width*4,4)!=null_data);
                    x++;
                }
                line+=" ";
            }
            if(LedgesRight.size()>0 || LedgesLeft.size()>0 || LedgesBottom.size()>0 || LedgesTop.size()>0)
            {
                x=0;
                while(x<this->map_to_send.width)
                {
                    line += std::to_string(this->map_to_send.parsed_layer.ledges[x+y*this->map_to_send.width]);
                    x++;
                }
                line+=" ";
            }
            x=0;
            while(x<this->map_to_send.width)
            {
                line += std::to_string(this->map_to_send.parsed_layer.walkable[x+y*this->map_to_send.width]);
                x++;
            }
            line.replace("0","_");
            DebugClass::debugConsole(line);
            y++;
        }
    }
    else
        DebugClass::debugConsole("No layer found!");
#endif

    #ifdef EPOLLCATCHCHALLENGERSERVER
    delete domDocument;
    #endif
    return true;
}

bool Map_loader::loadMonsterMap(const std::string &file, std::vector<std::string> detectedMonsterCollisionMonsterType, std::vector<std::string> detectedMonsterCollisionLayer)
{
    tinyxml2::XMLDocument *domDocument;

    //open and quick check the file
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=CommonDatapack::commonDatapack.xmlLoadedFile.cend())
        domDocument=&CommonDatapack::commonDatapack.xmlLoadedFile[file];
    else
    {
        domDocument=&CommonDatapack::commonDatapack.xmlLoadedFile[file];
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
    if(strcmp(this->map_to_send.xmlRoot->Name(),"map")!=0)
    {
        std::cerr << file+", loadMonsterMap(): \"map\" root balise not found for the xml file" << std::endl;
        return false;
    }

    //found the cave name
    std::vector<std::string> caveName;
    {
        unsigned int index=0;
        while(index<CommonDatapack::commonDatapack.monstersCollision.size())
        {
            if(CommonDatapack::commonDatapack.monstersCollision.at(index).layer.empty())
            {
                caveName=CommonDatapack::commonDatapack.monstersCollision.at(index).monsterTypeList;
                break;
            }
            index++;
        }
        if(caveName.empty())
            caveName.push_back(CACHEDSTRING_cave);
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
        while(index<CommonDatapack::commonDatapack.monstersCollision.size())
        {
            const MonstersCollision &monstersCollision=CommonDatapack::commonDatapack.monstersCollision.at(index);
            const std::vector<std::string> &searchList=monstersCollision.defautMonsterTypeList;
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
                if(CommonDatapack::commonDatapack.monstersCollision.at(index).layer.empty())
                {}
                //not cave
                else if(vectorcontainsAtLeastOne(detectedMonsterCollisionLayer,CommonDatapack::commonDatapack.monstersCollision.at(index).layer))
                {
                    if(zoneNumber.find(CommonDatapack::commonDatapack.monstersCollision.at(index).layer)==zoneNumber.cend())
                    {
                        zoneNumber[CommonDatapack::commonDatapack.monstersCollision.at(index).layer]=zoneNumberIndex;
                        this->map_to_send.parsed_layer.monstersCollisionList.push_back(MonstersCollisionValue());//create
                        tempZoneNumberIndex=zoneNumberIndex;
                        zoneNumberIndex++;
                    }
                    else
                        tempZoneNumberIndex=zoneNumber.at(CommonDatapack::commonDatapack.monstersCollision.at(index).layer);
                }
                {
                    MonstersCollisionValue *monstersCollisionValue=&this->map_to_send.parsed_layer.monstersCollisionList[tempZoneNumberIndex];
                    if(CommonDatapack::commonDatapack.monstersCollision.at(index).type==MonstersCollisionType_ActionOn)
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
                        while(event_index<monstersCollision.events.size())
                        {
                            const MonstersCollision::MonstersCollisionEvent &monstersCollisionEvent=monstersCollision.events.at(event_index);
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
        const tinyxml2::XMLElement *monsters=layer->FirstChildElement(XMLCACHEDSTRING_monster);
        while(monsters!=NULL)
        {
            if(monsters->Attribute(XMLCACHEDSTRING_id)!=NULL && ((monsters->Attribute(XMLCACHEDSTRING_minLevel)!=NULL && monsters->Attribute(XMLCACHEDSTRING_maxLevel)!=NULL) || monsters->Attribute(XMLCACHEDSTRING_level)!=NULL) && monsters->Attribute(XMLCACHEDSTRING_luck)!=NULL)
            {
                MapMonster mapMonster;
                mapMonster.id=stringtouint16(monsters->Attribute(XMLCACHEDSTRING_id),&ok);
                if(!ok)
                    std::cerr << "id is not a number: child->Name(): " << monsters->Name() << ", file: " << fileName << std::endl;
                if(ok)
                    if(CommonDatapack::commonDatapack.monsters.find(mapMonster.id)==CommonDatapack::commonDatapack.monsters.cend())
                    {
                        std::cerr << "monster " << mapMonster.id << " not found into the monster list: " << monsters->Name() << ", file: " << fileName << std::endl;
                        ok=false;
                    }
                if(monsters->Attribute(XMLCACHEDSTRING_minLevel)!=NULL && monsters->Attribute(XMLCACHEDSTRING_maxLevel)!=NULL)
                {
                    if(ok)
                    {
                        mapMonster.minLevel=stringtouint8(monsters->Attribute(XMLCACHEDSTRING_minLevel),&ok);
                        if(!ok)
                            std::cerr << "minLevel is not a number: child->Name(): " << monsters->Name() << ", file: " << fileName << std::endl;
                    }
                    if(ok)
                    {
                        mapMonster.maxLevel=stringtouint8(monsters->Attribute(XMLCACHEDSTRING_maxLevel),&ok);
                        if(!ok)
                            std::cerr << "maxLevel is not a number: child->Name(): " << monsters->Name() << ", file: " << fileName << std::endl;
                    }
                }
                else
                {
                    if(ok)
                    {
                        mapMonster.maxLevel=stringtouint8(monsters->Attribute(XMLCACHEDSTRING_level),&ok);
                        mapMonster.minLevel=mapMonster.maxLevel;
                        if(!ok)
                            std::cerr << "level is not a number: child->Name(): " << monsters->Name() << ", file: " << fileName << std::endl;
                    }
                }
                if(ok)
                {
                    std::string textLuck=std::string(monsters->Attribute(XMLCACHEDSTRING_luck));
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
            monsters = monsters->NextSiblingElement(XMLCACHEDSTRING_monster);
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

tinyxml2::XMLElement *Map_loader::getXmlCondition(const std::string &fileName,const std::string &file,const uint16_t &conditionId)
{
    (void)fileName;
    #ifdef ONLYMAPRENDER
    return NULL;
    #endif
    if(teleportConditionsUnparsed.find(file)!=teleportConditionsUnparsed.cend())
    {
        if(teleportConditionsUnparsed.at(file).find(conditionId)!=teleportConditionsUnparsed.at(file).cend())
            return teleportConditionsUnparsed.at(file).at(conditionId);
        else
            return NULL;
    }
    teleportConditionsUnparsed[file][conditionId];
    bool ok;
    tinyxml2::XMLDocument *domDocument;

    //open and quick check the file
    if(CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=CommonDatapack::commonDatapack.xmlLoadedFile.cend())
        domDocument=&CommonDatapack::commonDatapack.xmlLoadedFile[file];
    else
    {
        domDocument=&CommonDatapack::commonDatapack.xmlLoadedFile[file];
        const auto loadOkay = domDocument->LoadFile(file.c_str());
        if(loadOkay!=0)
        {
            std::cerr << file+", "+tinyxml2errordoc(domDocument) << std::endl;
            return NULL;
        }
    }
    const tinyxml2::XMLElement * root = domDocument->RootElement();
    if(root==NULL)
    {
        std::cerr << "no root balise found for the xml file " << file << std::endl;
        return NULL;
    }
    if(strcmp(root->Name(),"conditions")!=0)
    {
        std::cerr << "\"conditions\" root balise not found for the xml file " << file << std::endl;
        return NULL;
    }

    const tinyxml2::XMLElement *item = root->FirstChildElement(XMLCACHEDSTRING_condition);
    while(item!=NULL)
    {
        if(item->Attribute(XMLCACHEDSTRING_id)==NULL)
            std::cerr << "\"condition\" balise have not id attribute (" << file << ")" << std::endl;
        else if(item->Attribute(XMLCACHEDSTRING_type)==NULL)
            std::cerr << "\"condition\" balise have not type attribute (" << file << ")" << std::endl;
        else
        {
            const uint16_t &id=stringtouint16(item->Attribute(XMLCACHEDSTRING_id),&ok);
            if(!ok)
                std::cerr << "\"condition\" balise have id is not a number (" << file << ")" << std::endl;
            else
                teleportConditionsUnparsed[file][id]=const_cast<tinyxml2::XMLElement *>(item);
        }
        item = item->NextSiblingElement(XMLCACHEDSTRING_condition);
    }

    if(teleportConditionsUnparsed.find(file)!=teleportConditionsUnparsed.cend())
        if(teleportConditionsUnparsed.at(file).find(conditionId)!=teleportConditionsUnparsed.at(file).cend())
            return teleportConditionsUnparsed.at(file).at(conditionId);
    return NULL;
}

MapCondition Map_loader::xmlConditionToMapCondition(const std::string &conditionFile,
                                                    #ifdef CATCHCHALLENGER_XLMPARSER_TINYXML1
                                                    const tinyxml2::XMLElement *
                                                    #elif defined(CATCHCHALLENGER_XLMPARSER_TINYXML2)
                                                    const tinyxml2::XMLElement *
                                                    #endif
                                                    const conditionContent)
{
    #ifdef ONLYMAPRENDER
    return MapCondition();
    #endif
    bool ok;
    MapCondition condition;
    condition.type=MapConditionType_None;
    condition.data.fightBot=0;
    condition.data.item=0;
    condition.data.quest=0;
    if(conditionContent==NULL)
        return condition;
    const auto * const conditionContentTypeChar=conditionContent->Attribute(XMLCACHEDSTRING_type);
    if(conditionContentTypeChar==NULL)
        return condition;
    const std::string conditionContentType=std::string(conditionContentTypeChar);
    if(conditionContentType==CACHEDSTRING_quest)
    {
        if(conditionContent->Attribute(XMLCACHEDSTRING_quest)==NULL)
            std::cerr << "\"condition\" balise have type=quest but quest attribute not found, item, clan or fightBot ("
                      << conditionFile << ")" << std::endl;
        else
        {
            const uint16_t &quest=stringtouint16(conditionContent->Attribute(XMLCACHEDSTRING_quest),&ok);
            if(!ok)
                std::cerr << "\"condition\" balise have type=quest but quest attribute is not a number, item, clan or fightBot ("
                          << conditionFile << ")" << std::endl;
            else if(CommonDatapackServerSpec::commonDatapackServerSpec.quests.find(quest)==CommonDatapackServerSpec::commonDatapackServerSpec.quests.cend())
                std::cerr << "\"condition\" balise have type=quest but quest id is not found, item, clan or fightBot ("
                          << conditionFile << ")" << std::endl;
            else
            {
                condition.type=MapConditionType_Quest;
                condition.data.quest=quest;
            }
        }
    }
    else if(conditionContentType==CACHEDSTRING_item)
    {
        if(conditionContent->Attribute(XMLCACHEDSTRING_item)==NULL)
            std::cerr << "\"condition\" balise have type=item but item attribute not found, item, clan or fightBot (" << conditionFile <<  ")" << std::endl;
        else
        {
            const uint16_t &item=stringtouint16(conditionContent->Attribute(XMLCACHEDSTRING_item),&ok);
            if(!ok)
                std::cerr << "\"condition\" balise have type=item but item attribute is not a number, item, clan or fightBot (" << conditionFile <<  ")" << std::endl;
            else if(CommonDatapack::commonDatapack.items.item.find(item)==CommonDatapack::commonDatapack.items.item.cend())
                std::cerr << "\"condition\" balise have type=item but item id is not found, item, clan or fightBot (" << conditionFile <<  ")" << std::endl;
            else
            {
                condition.type=MapConditionType_Item;
                condition.data.item=item;
            }
        }
    }
    else if(conditionContentType==CACHEDSTRING_fightBot)
    {
        if(conditionContent->Attribute(XMLCACHEDSTRING_fightBot)==NULL)
            std::cerr << "\"condition\" balise have type=fightBot but fightBot attribute not found, item, clan or fightBot (" << conditionFile <<  ")" << std::endl;
        else
        {
            const uint16_t &fightBot=stringtouint16(conditionContent->Attribute(XMLCACHEDSTRING_fightBot),&ok);
            if(!ok)
                std::cerr << "\"condition\" balise have type=fightBot but fightBot attribute is not a number, item, clan or fightBot (" << conditionFile <<  ")" << std::endl;
            else if(CommonDatapackServerSpec::commonDatapackServerSpec.botFights.find(fightBot)==CommonDatapackServerSpec::commonDatapackServerSpec.botFights.cend())
                std::cerr << "\"condition\" balise have type=fightBot but fightBot id is not found, item, clan or fightBot (" << conditionFile <<  ")" << std::endl;
            else
            {
                condition.type=MapConditionType_FightBot;
                condition.data.fightBot=fightBot;
            }
        }
    }
    else if(conditionContentType==CACHEDSTRING_clan)
        condition.type=MapConditionType_Clan;
    else
        std::cerr << "\"condition\" balise have type but value is not quest, item, clan or fightBot (" << conditionFile <<  ")" << std::endl;
    return condition;
}
