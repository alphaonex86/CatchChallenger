#include "Map_loader.h"
#include "tinyXML2/customtinyxml2.h"
#include "FacilityLibGeneral.h"
#include "CommonDatapack.h"

#include <iostream>
#include <zstd.h>      // presumes zstd library is installed

using namespace CatchChallenger;

bool Map_loader::tryLoadMap(const std::string &file,const bool &botIsNotWalkable)
{
    error.clear();
    Map_to_send map_to_send_temp;
    map_to_send_temp.border.bottom.x_offset=0;
    map_to_send_temp.border.top.x_offset=0;
    map_to_send_temp.border.left.y_offset=0;
    map_to_send_temp.border.right.y_offset=0;
    map_to_send_temp.xmlRoot=NULL;
    map_to_send_temp.monstersCollisionMap=NULL;
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
    if(root->Name()==NULL)
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
    if(root->Attribute("width")==NULL)
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
    const tinyxml2::XMLElement * child = root->FirstChildElement("properties");
    if(child!=NULL)
    {
        const tinyxml2::XMLElement * SubChild=child->FirstChildElement("property");
        while(SubChild!=NULL)
        {
            if(SubChild->Attribute("name")!=NULL && SubChild->Attribute("value")!=NULL)
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
    if(root->Attribute("tilewidth")!=NULL)
    {
        tilewidth=stringtouint8(root->Attribute("tilewidth"),&ok);
        if(!ok)
        {
            std::cerr << "Unable to open the file: " << file << ", tilewidth is not a number" << std::endl;
            tilewidth=16;
        }
    }
    if(root->Attribute("tileheight")!=NULL)
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
    while(child!=NULL)
    {
        if(child->Attribute("name")==NULL)
            std::cerr << "Has not attribute \"name\": child->Name(): " << child->Name() << ")";
        else
        {
            if(strcmp(child->Attribute("name"),"Moving")==0)
            {
                const tinyxml2::XMLElement *SubChild=child->FirstChildElement("object");
                while(SubChild!=NULL)
                {
                    if(SubChild->Attribute("x")!=NULL && SubChild->Attribute("y")!=NULL)
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
                            else if(SubChild->Attribute("type")==NULL)
                                std::cerr << "Missing attribute type missing: SubChild->Name(): " << SubChild->Name() << ", file: " << file << std::endl;
                            else
                            {
                                const std::string type(SubChild->Attribute("type"));

                                std::unordered_map<std::string,std::string> property_text;
                                const tinyxml2::XMLElement *prop=SubChild->FirstChildElement("properties");
                                if(prop!=NULL)
                                {
                                    #ifdef DEBUG_MESSAGE_MAP
                                    std::cerr << "Wrong conversion with y: child->Name(): " << prop->Name()
                                                << " (at line: " << std::to_string(prop->Row())
                                                << "), file: " << fileName << std::endl;
                                    #endif
                                    const tinyxml2::XMLElement *property=prop->FirstChildElement("property");
                                    while(property!=NULL)
                                    {
                                        if(property->Attribute("name")!=NULL && property->Attribute("value")!=NULL)
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
                                    if(property_text.find("map")!=property_text.cend() && property_text.find("x")!=property_text.cend() &&
                                            property_text.find("y")!=property_text.cend())
                                    {
                                        Map_semi_teleport new_tp;
                                        new_tp.source_x=static_cast<uint8_t>(object_x);
                                        new_tp.source_y=static_cast<uint8_t>(object_y);
                                        new_tp.condition.type=MapConditionType_None;
                                        new_tp.condition.data.fightBot=0;
                                        new_tp.condition.data.item=0;
                                        new_tp.condition.data.quest=0;
                                        new_tp.conditionUnparsed=NULL;
                                        new_tp.destination_x = stringtouint8(property_text.at("x"),&ok);
                                        if(ok)
                                        {
                                            new_tp.destination_y = stringtouint8(property_text.at("y"),&ok);
                                            if(ok)
                                            {
                                                //std::cerr << "CACHEDSTRING_condition_file: " << CACHEDSTRING_condition_file << std::endl;
                                                //std::cerr << "CACHEDSTRING_condition_id: " << CACHEDSTRING_condition_id << std::endl;
                                                if(property_text.find("condition_file")!=property_text.cend() &&
                                                        property_text.find("condition_id")!=property_text.cend())
                                                {
                                                    uint32_t conditionId=stringtouint32(property_text.at("condition_id"),&ok);
                                                    if(!ok)
                                                        std::cerr << "condition id is not a number, id: " << property_text.at("condition_id")
                                                                  << " (" << file << ")" << std::endl;
                                                    else
                                                    {
                                                        std::string conditionFile=FSabsoluteFilePath(
                                                                    FSabsolutePath(file)+
                                                                    "/"+
                                                                    property_text.at("condition_file")
                                                                    );
                                                        if(!stringEndsWith(conditionFile,".xml"))
                                                            conditionFile+=".xml";
                                                        new_tp.conditionUnparsed=getXmlCondition(file,conditionFile,conditionId);
                                                        new_tp.condition=xmlConditionToMapCondition(conditionFile,new_tp.conditionUnparsed);
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
                    SubChild = SubChild->NextSiblingElement("object");
                }
            }
            if(strcmp(child->Attribute("name"),"Object")==0)
            {
                const tinyxml2::XMLElement * SubChild=child->FirstChildElement("object");
                while(SubChild!=NULL)
                {
                    if(SubChild->Attribute("x")!=NULL && SubChild->Attribute("y")!=NULL)
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
                            else if(SubChild->Attribute("type")!=NULL)
                            {
                                const std::string &type=SubChild->Attribute("type");

                                std::unordered_map<std::string,std::string> property_text;
                                const tinyxml2::XMLElement * prop=SubChild->FirstChildElement("properties");
                                if(prop!=NULL)
                                {
                                    const tinyxml2::XMLElement * property=prop->FirstChildElement("property");
                                    while(property!=NULL)
                                    {
                                        if(property->Attribute("name")!=NULL && property->Attribute("value")!=NULL)
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
                                        bot_semi.file=FSabsoluteFilePath(
                                                            FSabsolutePath(file)+
                                                            "/"+
                                                            property_text.at("file")
                                                    );
                                        bot_semi.id=stringtouint16(property_text.at("id"),&ok);
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
                    SubChild = SubChild->NextSiblingElement("object");
                }
            }
        }
        child = child->NextSiblingElement("objectgroup");
    }

    const uint32_t &rawSize=map_to_send_temp.width*map_to_send_temp.height*4;

    // layer
    child = root->FirstChildElement("layer");
    while(child!=NULL)
    {
        if(child->Attribute("name")==NULL)
        {
            error=std::string("Has not attribute \"name\": child->Name(): ")+child->Name()+", file: "+file;
            return false;
        }
        else
        {
            const tinyxml2::XMLElement *data=child->FirstChildElement("data");
            const std::string name=child->Attribute("name");
            if(data==NULL)
            {
                error=std::string("Is Element for layer is null: ")+data->Name()+" and name: "+name+", file: "+file;
                return false;
            }
            else if(data->Attribute("encoding")==NULL)
            {
                error=std::string("Has not attribute \"base64\": child->Name(): ")+data->Name()+", file: "+file;
                return false;
            }
            else if(data->Attribute("compression")==NULL)
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
                if(data->GetText()!=NULL)
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
        child = child->NextSiblingElement("layer");
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
                    if(bot.property_text.find("skin")!=bot.property_text.cend() &&
                            (bot.property_text.find("lookAt")==bot.property_text.cend() || bot.property_text.at("lookAt")!="move"))
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

    #ifdef EPOLLCATCHCHALLENGERSERVER
    delete domDocument;
    #endif
    return true;
}
