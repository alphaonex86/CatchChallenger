#include "Map_loader.hpp"
#include "../tinyXML2/customtinyxml2.hpp"
#include "FacilityLibGeneral.hpp"
#include "CommonDatapack.hpp"
#include "cpp11addition.hpp"

#include <iostream>
#include <zstd.h>
#include <map>

using namespace CatchChallenger;

bool Map_loader::tryLoadMap(const std::string &file, CommonMap &mapFinal, const bool &botIsNotWalkable)
{
    std::unordered_map<std::string,CATCHCHALLENGER_TYPE_ITEM> tempNameToItemId=CommonDatapack::commonDatapack.get_tempNameToItemId();
    std::unordered_map<std::string,CATCHCHALLENGER_TYPE_MONSTER> tempNameToMonsterId=CommonDatapack::commonDatapack.get_tempNameToMonsterId();
    if(tempNameToItemId.empty())
    {
        std::cerr << "no item name loaded (abort)" << std::endl;
        abort();
    }
    if(tempNameToMonsterId.empty())
    {
        std::cerr << "no monster name loaded (abort)" << std::endl;
        abort();
    }

    error.clear();
    Map_to_send map_to_send_temp;
    map_to_send_temp.border.bottom.x_offset=0;
    map_to_send_temp.border.top.x_offset=0;
    map_to_send_temp.border.left.y_offset=0;
    map_to_send_temp.border.right.y_offset=0;
    map_to_send_temp.xmlRoot=nullptr;
    map_to_send_temp.width=0;
    map_to_send_temp.height=0;

    std::vector<std::string> detectedMonsterCollisionMonsterType,detectedMonsterCollisionLayer;
    std::vector<char> Walkable,Collisions,Dirt,LedgesRight,LedgesLeft,LedgesBottom,LedgesTop;
    std::vector<std::vector<char> > monsterList;
    std::map<std::string/*layer*/,std::vector<char> > mapLayerContentForMonsterCollision;
    bool ok;
    tinyxml2::XMLDocument *domDocument;

    //open and quick check the file
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CommonDatapack::commonDatapack.get_xmlLoadedFile().find(file)!=CommonDatapack::commonDatapack.get_xmlLoadedFile().cend())
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
            error=file+", "+tinyxml2errordoc(domDocument);
            return false;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
    }
    #endif
    const tinyxml2::XMLElement * root = domDocument->RootElement();
    if(root==nullptr)
    {
        error=file+", tryLoadMap(): no root balise found for the xml file";
        return false;
    }
    if(root->Name()==nullptr)
    {
        error=file+", tryLoadMap(): \"map\" root balise not found 2 for the xml file";
        return false;
    }
    if(strcmp(root->Name(),"map")!=0)
    {
        error=file+", tryLoadMap(): \"map\" root balise not found for the xml file";
        return false;
    }

    //get the width
    if(root->Attribute("width")==nullptr)
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
    if(root->Attribute("height")==nullptr)
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
    if(map_to_send_temp.width<1 || map_to_send_temp.width>127)
    {
        error="the width should be greater or equal than 1, and lower or equal to 127";
        return false;
    }
    if(map_to_send_temp.height<1 || map_to_send_temp.height>127)
    {
        error="the height should be greater or equal than 1, and lower or equal to 127";
        return false;
    }

    //properties
    const tinyxml2::XMLElement * child = root->FirstChildElement("properties");
    if(child!=nullptr)
    {
        const tinyxml2::XMLElement * SubChild=child->FirstChildElement("property");
        while(SubChild!=nullptr)
        {
            if(SubChild->Attribute("name")!=nullptr && SubChild->Attribute("value")!=nullptr)
                map_to_send_temp.property[std::string(SubChild->Attribute("name"))]=
                        std::string(SubChild->Attribute("value"));
            else
            {
                error=std::string("Missing attribute name or value: child->Name(): ")+std::string(SubChild->Name())+")";
                return false;
            }
            SubChild = SubChild->NextSiblingElement("property");
        }
    }

    int8_t tilewidth=16;
    int8_t tileheight=16;
    if(root->Attribute("tilewidth")!=nullptr)
    {
        tilewidth=stringtouint8(root->Attribute("tilewidth"),&ok);
        if(!ok)
        {
            std::cerr << "Unable to open the file: " << file << ", tilewidth is not a number" << std::endl;
            tilewidth=16;
        }
    }
    if(root->Attribute("tileheight")!=nullptr)
    {
        tileheight=stringtouint8(root->Attribute("tileheight"),&ok);
        if(!ok)
        {
            std::cerr << "Unable to open the file: " << file << ", tilewidth is not a number" << std::endl;
            tileheight=16;
        }
    }

    // objectgroup
    child = root->FirstChildElement("objectgroup");
    while(child!=nullptr)
    {
        if(child->Attribute("name")==nullptr)
            std::cerr << "Has not attribute \"name\": child->Name(): " << child->Name() << ")";
        else
        {
            if(strcmp(child->Attribute("name"),"Moving")==0)
            {
                const tinyxml2::XMLElement *SubChild=child->FirstChildElement("object");
                while(SubChild!=nullptr)
                {
                    if(SubChild->Attribute("x")!=nullptr && SubChild->Attribute("y")!=nullptr)
                    {
                        const uint32_t &object_x=stringtouint32(SubChild->Attribute("x"),&ok)/tilewidth;
                        if(!ok)
                            std::cerr << "Wrong conversion with x: child->Name(): " << SubChild->Name()
                                        << ", file: " << file << std::endl;
                        else
                        {
                            /** the -1 is important to fix object layer bug into tiled!!!
                             * Don't remove! */
                            const uint32_t &object_y=(stringtouint32(SubChild->Attribute("y"),&ok)/tileheight)-1;

                            if(!ok)
                                std::cerr << "Wrong conversion with y: child->Name(): " << SubChild->Name()
                                            << ", file: " << file << std::endl;
                            else if(object_x>map_to_send_temp.width || object_y>map_to_send_temp.height)
                                std::cerr << "Object out of the map: child->Name(): " << SubChild->Name()
                                            << ", file: " << file << std::endl;
                            else if(SubChild->Attribute("type")==nullptr && SubChild->Attribute("class")==nullptr)
                                std::cerr << "Missing attribute type missing: SubChild->Name(): " << SubChild->Name() << ", file: " << file << std::endl;
                            else
                            {
                                std::string type;
                                if(SubChild->Attribute("type")!=nullptr)
                                    type=std::string(SubChild->Attribute("type"));
                                else
                                    type=std::string(SubChild->Attribute("class"));

                                std::unordered_map<std::string,std::string> property_text;
                                const tinyxml2::XMLElement *prop=SubChild->FirstChildElement("properties");
                                if(prop!=nullptr)
                                {
                                    #ifdef DEBUG_MESSAGE_MAP
                                    std::cerr << "Wrong conversion with y: child->Name(): " << prop->Name()
                                                << " (at line: " << std::to_string(prop->Row())
                                                << "), file: " << fileName << std::endl;
                                    #endif
                                    const tinyxml2::XMLElement *property=prop->FirstChildElement("property");
                                    while(property!=nullptr)
                                    {
                                        if(property->Attribute("name")!=nullptr && property->Attribute("value")!=nullptr)
                                            property_text[std::string(property->Attribute("name"))]=
                                                    std::string(property->Attribute("value"));
                                        property = property->NextSiblingElement("property");
                                    }
                                }
                                if(type=="border-left" || type=="border-right" || type=="border-top" || type=="border-bottom")
                                {
                                    #ifdef DEBUG_MESSAGE_MAP
                                    DebugClass::debugConsole(std::stringLiteral("type: %1, object_x: %2, object_y: %3, border")
                                         .arg(type)
                                         .arg(object_x)
                                         .arg(object_y)
                                         );
                                    #endif
                                    if(property_text.find("map")!=property_text.cend())
                                    {
                                        if(type=="border-left")//border left
                                        {
                                            const std::string &borderMap=property_text.at("map");
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
                                            const std::string &borderMap=property_text.at("map");
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
                                            const std::string &borderMap=property_text.at("map");
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
                                            const std::string &borderMap=property_text.at("map");
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
                                    if(property_text.find("map")==property_text.cend())
                                    {
                                        const auto &pos=file.find_last_of('/');
                                        if(pos==std::string::npos)
                                            property_text["map"]=file;
                                        else
                                            property_text["map"]=file.substr(pos+1);
                                    }
                                    if(property_text.find("x")!=property_text.cend() &&
                                            property_text.find("y")!=property_text.cend())
                                    {
                                        Map_semi_teleport new_tp;
                                        new_tp.source_x=static_cast<uint8_t>(object_x);
                                        new_tp.source_y=static_cast<uint8_t>(object_y);
                                        new_tp.condition.type=MapConditionType_None;
                                        new_tp.condition.mapindex=65535;
                                        new_tp.condition.data.fightBot=0;
                                        new_tp.condition.data.item=0;
                                        new_tp.condition.data.quest=0;
                                        new_tp.destination_x = stringtouint8(property_text.at("x"),&ok);
                                        if(ok)
                                        {
                                            new_tp.destination_y = stringtouint8(property_text.at("y"),&ok);
                                            if(ok)
                                            {
                                                //std::cerr << "CACHEDSTRING_condition_file: " << CACHEDSTRING_condition_file << std::endl;
                                                //std::cerr << "CACHEDSTRING_condition_id: " << CACHEDSTRING_condition_id << std::endl;
                                                if(property_text.find("condition-file")!=property_text.cend() &&
                                                        property_text.find("condition-id")!=property_text.cend())
                                                {
                                                    uint32_t conditionId=stringtouint32(property_text.at("condition-id"),&ok);
                                                    if(!ok)
                                                        std::cerr << "condition id is not a number, id: " << property_text.at("condition-id")
                                                                  << " (" << file << ")" << std::endl;
                                                    else
                                                    {
                                                        std::string conditionFile=FSabsoluteFilePath(
                                                                    FSabsolutePath(file)+
                                                                    "/"+
                                                                    property_text.at("condition-file")
                                                                    );
                                                        if(!stringEndsWith(conditionFile,".xml"))
                                                            conditionFile+=".xml";
                                                        /*new_tp.conditionUnparsed=getXmlCondition(file,conditionFile,conditionId);
                                                        new_tp.condition=xmlConditionToMapCondition(conditionFile,new_tp.conditionUnparsed);*/
                                                    }
                                                }
                                                new_tp.map=property_text.at("map");
                                                if(!stringEndsWith(new_tp.map,".tmx") && !new_tp.map.empty())
                                                    new_tp.map+=".tmx";
                                                map_to_send_temp.teleport.push_back(new_tp);
                                            }
                                            else
                                                std::cerr << "Bad convertion to int for y, type: " << type << ", value: " << property_text.at("y") << " (" << file << ")" << std::endl;
                                        }
                                        else
                                            std::cerr << "Bad convertion to int for x, type: " << type << ", value: " << property_text.at("x") << " (" << file << ")" << std::endl;
                                    }
                                    else
                                        std::cerr << "Missing map,x or y, type: " << type << ", file: " << file << std::endl;
                                }
                                else
                                {
                                    if(!mapFinal.parseUnknownMoving(type,object_x,object_y,property_text))
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
                    SubChild = SubChild->NextSiblingElement("object");
                }
            }
            if(strcmp(child->Attribute("name"),"Object")==0)
            {
                const tinyxml2::XMLElement * SubChild=child->FirstChildElement("object");
                while(SubChild!=nullptr)
                {
                    if(SubChild->Attribute("x")!=nullptr && SubChild->Attribute("y")!=nullptr)
                    {
                        const uint32_t &object_x=stringtouint32(SubChild->Attribute("x"),&ok)/tilewidth;
                        if(!ok)
                            std::cerr << "Wrong conversion with x: child->Name(): " << SubChild->Name()
                                        << ", file: " << file << std::endl;
                        else
                        {
                            /** the -1 is important to fix object layer bug into tiled!!!
                             * Don't remove! */
                            const uint32_t &object_y=(stringtouint32(SubChild->Attribute("y"),&ok)/tileheight)-1;

                            if(!ok)
                                std::cerr << "Wrong conversion with y: child->Name(): " << SubChild->Name()
                                            << ", file: " << file << std::endl;
                            else if(object_x>map_to_send_temp.width || object_y>map_to_send_temp.height)
                                std::cerr << "Object out of the map: child->Name(): " << SubChild->Name()
                                            << ", file: " << file << std::endl;
                            else if(SubChild->Attribute("type")!=nullptr)
                            {
                                const std::string &type=SubChild->Attribute("type");

                                std::unordered_map<std::string,std::string> property_text;
                                const tinyxml2::XMLElement * prop=SubChild->FirstChildElement("properties");
                                if(prop!=nullptr)
                                {
                                    const tinyxml2::XMLElement * property=prop->FirstChildElement("property");
                                    while(property!=nullptr)
                                    {
                                        if(property->Attribute("name")!=nullptr && property->Attribute("value")!=nullptr)
                                            property_text[std::string(property->Attribute("name"))]=
                                                    std::string(property->Attribute("value"));
                                        property = property->NextSiblingElement("property");
                                    }
                                }
                                if(type=="bot")
                                {
                                    if(property_text.find("skin")!=property_text.cend() && !property_text.at("skin").empty() &&
                                            property_text.find("lookAt")==property_text.cend())
                                    {
                                        property_text["lookAt"]="bottom";
                                        std::cerr << "skin but not lookAt, fixed by bottom: " << SubChild->Name() << " (" << file << ")" << std::endl;
                                    }
                                    if(property_text.find("file")!=property_text.cend() && property_text.find("id")!=property_text.cend())
                                    {
                                        if(!stringEndsWith(property_text["file"],".xml"))
                                            property_text["file"]+=".xml";
                                        Map_to_send::Bot_Semi bot_semi;
                                        bot_semi.id=stringtouint8(property_text.at("id"),&ok);
                                        bot_semi.property_text=property_text;
                                        if(ok)
                                        {
                                            bot_semi.point.first=static_cast<uint8_t>(object_x);
                                            bot_semi.point.second=static_cast<uint8_t>(object_y);
                                            map_to_send_temp.bots.push_back(bot_semi);
                                        }
                                    }
                                    else
                                    {

                                        std::cerr << "Missing \"bot\" properties ";
                                        if(property_text.find("file")==property_text.cend())
                                            std::cerr << "file, ";
                                        if(property_text.find("id")==property_text.cend())
                                            std::cerr << "id, ";
                                        std::cerr << " for the bot: " << SubChild->Name()
                                                  << ", file: " << file << ", text: " << SubChild->Value() << std::endl;
                                    }
                                }
                                else if(type=="object")
                                {
                                    if(property_text.find("item")!=property_text.cend())
                                    {
                                        Map_to_send::ItemOnMap_Semi item_semi;
                                        item_semi.infinite=false;
                                        if(property_text.find("infinite")!=property_text.cend() &&
                                                property_text.at("infinite")=="true")
                                            item_semi.infinite=true;
                                        item_semi.visible=true;
                                        if(property_text.find("visible")!=property_text.cend() &&
                                                property_text.at("visible")=="false")
                                            item_semi.visible=false;
                                        item_semi.item=stringtouint16(property_text.at("item"),&ok);
                                        if(ok)
                                        {
                                            item_semi.point.first=static_cast<uint8_t>(object_x);
                                            item_semi.point.second=static_cast<uint8_t>(object_y);
                                            map_to_send_temp.items.push_back(item_semi);
                                        }
                                        else if(CommonDatapack::commonDatapack.get_tempNameToItemId().find(property_text.at("item"))!=CommonDatapack::commonDatapack.get_tempNameToItemId().cend())
                                       {
                                            item_semi.item=CommonDatapack::commonDatapack.get_tempNameToItemId().at(property_text.at("item"));
                                            item_semi.point.first=static_cast<uint8_t>(object_x);
                                            item_semi.point.second=static_cast<uint8_t>(object_y);
                                            map_to_send_temp.items.push_back(item_semi);
                                       }
                                    }
                                    else
                                        std::cerr << "Missing \"object\" properties (item) for the bot: " << SubChild->Name()
                                                  << ", file: " << file << ", text: " << SubChild->Value() << std::endl;
                                }
                                else if(type=="StrengthBoulder")//not implemented
                                {}
                                else
                                {
                                    if(!mapFinal.parseUnknownObject(type,object_x,object_y,property_text))
                                        std::cerr << "Unknown type: " << type
                                              << ", object_x: " << object_x
                                              << ", object_y: " << object_y
                                              << " (object), " << SubChild->Name()
                                              << ", file: " << file
                                              << std::endl;
                                }
                            }
                            /* ignore, can be informationnal text for mapper
                             * else
                                std::cerr << "Missing attribute type missing: SubChild->Name(): " << SubChild->Name() << ", file: " << file << std::endl;*/
                        }
                    }
                    else
                        std::cerr << "Is not Element: SubChild->Name(): " << SubChild->Name() << ", file: " << file << std::endl;
                    SubChild = SubChild->NextSiblingElement("object");
                }
            }
        }
        child = child->NextSiblingElement("objectgroup");
    }

    const uint32_t &rawSize=map_to_send_temp.width*map_to_send_temp.height*4;

    // layer
    child = root->FirstChildElement("layer");
    while(child!=nullptr)
    {
        if(child->Attribute("name")==nullptr)
        {
            error=std::string("Has not attribute \"name\": child->Name(): ")+child->Name()+", file: "+file;
            return false;
        }
        else
        {
            const tinyxml2::XMLElement *data=child->FirstChildElement("data");
            const std::string name=child->Attribute("name");
            if(data==nullptr)
            {
                error=std::string("Is Element for layer is null: data==nullptr and name: ")+name+", file: "+file;
                return false;
            }
            else if(data->Attribute("encoding")==nullptr)
            {
                error=std::string("Has not attribute \"base64\": child->Name(): ")+data->Name()+", file: "+file;
                return false;
            }
            else if(data->Attribute("compression")==nullptr)
            {
                error=std::string("Has not attribute \"zlib\": child->Name(): ")+data->Name()+", file: "+file;
                return false;
            }
            else if(strcmp(data->Attribute("encoding"),"base64")!=0)
            {
                error=std::string("only encoding base64 is supported, file: ")+file;
                return false;
            }
            else if(
                    #ifdef TILED_ZLIB
                    strcmp(data->Attribute("compression"),"zlib")!=0
                    &&
                    #endif
                    strcmp(data->Attribute("compression"),"zstd")!=0
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
                if(data->GetText()!=nullptr)
                base64text=FacilityLibGeneral::dropPrefixAndSuffixLowerThen33(data->GetText());
                const std::vector<char> &compressedData=base64toBinary(base64text);
                if(!compressedData.empty())
                {
                    std::vector<char> dataRaw;
                    dataRaw.resize(map_to_send_temp.height*map_to_send_temp.width*4);
                    int32_t decompressedSize=0;
                    #ifdef TILED_ZLIB
                    if(strcmp(data->Attribute("compression"),"zlib")==0)
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
                        if(data->GetText()!=nullptr)
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
                            while(index<CommonDatapack::commonDatapack.get_monstersCollision().size())
                            {
                                const MonstersCollisionTemp &monstersCollisionTemp=CommonDatapack::commonDatapack.get_monstersCollisionTemp().at(index);
                                if(monstersCollisionTemp.layer==name)
                                {
                                    mapLayerContentForMonsterCollision[name]=dataRaw;
                                    {
                                        const std::vector<std::string> &monsterTypeListText=monstersCollisionTemp.monsterTypeList;
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
        child = child->NextSiblingElement("layer");
    }

    /*std::vector<char> null_data;
    null_data.resize(4);
    null_data[0]=0x00;
    null_data[1]=0x00;
    null_data[2]=0x00;
    null_data[3]=0x00;*/

    std::vector<uint8_t> simplifiedMap;
    simplifiedMap.resize(map_to_send_temp.width*map_to_send_temp.height);

    uint32_t x=0;
    uint32_t y=0;

    char * WalkableBin=nullptr;
    char * CollisionsBin=nullptr;
    char * DirtBin=nullptr;
    char * LedgesRightBin=nullptr;
    char * LedgesLeftBin=nullptr;
    char * LedgesBottomBin=nullptr;
    char * LedgesTopBin=nullptr;
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
            if(WalkableBin!=nullptr)
                walkable=WalkableBin[x*4+y*map_to_send_temp.width*4+0]!=0x00 || WalkableBin[x*4+y*map_to_send_temp.width*4+1]!=0x00 || WalkableBin[x*4+y*map_to_send_temp.width*4+2]!=0x00 || WalkableBin[x*4+y*map_to_send_temp.width*4+3]!=0x00;
            else
                walkable=false;
            if(CollisionsBin!=nullptr)
                collisions=CollisionsBin[x*4+y*map_to_send_temp.width*4+0]!=0x00 || CollisionsBin[x*4+y*map_to_send_temp.width*4+1]!=0x00 || CollisionsBin[x*4+y*map_to_send_temp.width*4+2]!=0x00 || CollisionsBin[x*4+y*map_to_send_temp.width*4+3]!=0x00;
            else
                collisions=false;
            if(DirtBin!=nullptr)
                dirt=DirtBin[x*4+y*map_to_send_temp.width*4+0]!=0x00 || DirtBin[x*4+y*map_to_send_temp.width*4+1]!=0x00 || DirtBin[x*4+y*map_to_send_temp.width*4+2]!=0x00 || DirtBin[x*4+y*map_to_send_temp.width*4+3]!=0x00;
            else
                dirt=false;
            if(LedgesRightBin!=nullptr)
                ledgesRight=LedgesRightBin[x*4+y*map_to_send_temp.width*4+0]!=0x00 || LedgesRightBin[x*4+y*map_to_send_temp.width*4+1]!=0x00 || LedgesRightBin[x*4+y*map_to_send_temp.width*4+2]!=0x00 || LedgesRightBin[x*4+y*map_to_send_temp.width*4+3]!=0x00;
            else
                ledgesRight=false;
            if(LedgesLeftBin!=nullptr)
                ledgesLeft=LedgesLeftBin[x*4+y*map_to_send_temp.width*4+0]!=0x00 || LedgesLeftBin[x*4+y*map_to_send_temp.width*4+1]!=0x00 || LedgesLeftBin[x*4+y*map_to_send_temp.width*4+2]!=0x00 || LedgesLeftBin[x*4+y*map_to_send_temp.width*4+3]!=0x00;
            else
                ledgesLeft=false;
            if(LedgesBottomBin!=nullptr)
                ledgesBottom=LedgesBottomBin[x*4+y*map_to_send_temp.width*4+0]!=0x00 || LedgesBottomBin[x*4+y*map_to_send_temp.width*4+1]!=0x00 || LedgesBottomBin[x*4+y*map_to_send_temp.width*4+2]!=0x00 || LedgesBottomBin[x*4+y*map_to_send_temp.width*4+3]!=0x00;
            else
                ledgesBottom=false;
            if(LedgesTopBin!=nullptr)
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

            if(dirt)
            {
                /*Map_to_send::DirtOnMap_Semi dirtOnMap_Semi;
                dirtOnMap_Semi.point.x=static_cast<uint8_t>(x);
                dirtOnMap_Semi.point.y=static_cast<uint8_t>(y);
                map_to_send_temp.dirts.push_back(dirtOnMap_Semi); why have this?*/
                simplifiedMap[x+y*map_to_send_temp.width]=249;
            }
            else if(ledgesLeft)
                simplifiedMap[x+y*map_to_send_temp.width]=250;
            else if(ledgesRight)
                simplifiedMap[x+y*map_to_send_temp.width]=251;
            else if(ledgesTop)
                simplifiedMap[x+y*map_to_send_temp.width]=252;
            else if(ledgesBottom)
                simplifiedMap[x+y*map_to_send_temp.width]=253;
            else if((walkable || monsterCollision) && !collisions && !dirt &&
                    !ledgesBottom && !ledgesRight && !ledgesLeft && !ledgesTop)
                simplifiedMap[x+y*map_to_send_temp.width]=0;
            else
                simplifiedMap[x+y*map_to_send_temp.width]=254;

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
                    simplifiedMap[map_semi_teleport.source_x+map_semi_teleport.source_y*map_to_send_temp.width]=0;
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
                if(bot.point.first<map_to_send_temp.width && bot.point.second<map_to_send_temp.height)
                {
                    if(bot.property_text.find("skin")!=bot.property_text.cend() &&
                            (bot.property_text.find("lookAt")==bot.property_text.cend() || bot.property_text.at("lookAt")!="move"))
                        simplifiedMap[bot.point.first+bot.point.second*map_to_send_temp.width]=254;
                }
                else
                    std::cerr << "bot out of map on " << file << ", source: " << std::to_string(bot.point.first) << "," << std::to_string(bot.point.second) << std::endl;
                index++;
            }
            index=0;
            while(index<map_to_send_temp.items.size())
            {
                const Map_to_send::ItemOnMap_Semi &item_semi=map_to_send_temp.items.at(index);
                if(item_semi.point.first<map_to_send_temp.width && item_semi.point.second<map_to_send_temp.height)
                {
                    if(item_semi.visible && item_semi.infinite)
                        simplifiedMap[item_semi.point.first+item_semi.point.second*map_to_send_temp.width]=254;
                }
                else
                    std::cerr << "item out of map on " << file << ", source: " << std::to_string(item_semi.point.first) << "," << std::to_string(item_semi.point.second) << std::endl;
                index++;
            }
        }
    }

    std::string xmlExtra=file;
    stringreplaceAll(xmlExtra,".tmx",".xml");
    loadExtraXml(mapFinal,xmlExtra,map_to_send_temp.bots,detectedMonsterCollisionMonsterType,detectedMonsterCollisionLayer,map_to_send_temp.zoneName);

    bool previousHaveMonsterWarn=false;
    {
        //link to monster zone id
        {
            auto i=mapLayerContentForMonsterCollision.begin();
            while(i!=mapLayerContentForMonsterCollision.cend())
            {
                const std::string &zoneName=i->first;
                if(zoneNumber.find(zoneName)!=zoneNumber.cend())
                {
                    const uint8_t &zoneId=zoneNumber.at(zoneName);
                    uint8_t x=0;
                    while(x<this->map_to_send.width)
                    {
                        uint8_t y=0;
                        while(y<this->map_to_send.height)
                        {
                            unsigned int value=reinterpret_cast<const unsigned int *>(i->second.data())[x+y*map_to_send_temp.width];
                            const uint8_t &val=simplifiedMap[x+y*this->map_to_send.width];
                            if(value!=0 && val<200)
                            {
                                if(val==0)
                                    simplifiedMap[x+y*this->map_to_send.width]=zoneId;
                                else if(val==zoneId)
                                {}//ignore, same zone
                                else
                                {
                                    if(!previousHaveMonsterWarn)
                                    {
                                        std::cerr << "Have already monster at " << std::to_string(x) << "," << std::to_string(y) << " for " << file
                                                  << ", actual zone: " << std::to_string(val)
                                                  << " (" << CommonDatapack::commonDatapack.get_monstersCollisionTemp()
                                                     .at(val).layer
                                                  << "), new zone: " << std::to_string(zoneId) << " (" << CommonDatapack::commonDatapack.get_monstersCollisionTemp().at(zoneId).layer
                                                  << ")" << std::endl;
                                        previousHaveMonsterWarn=true;
                                    }
                                    simplifiedMap[x+y*this->map_to_send.width]=zoneId;//overwrited by above layer
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
    }

    #ifdef EPOLLCATCHCHALLENGERSERVER
    delete domDocument;
    #endif
    return true;
}
