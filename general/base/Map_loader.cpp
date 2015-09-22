#include "Map_loader.h"
#include "GeneralVariable.h"
#include "CommonDatapackServerSpec.h"

#include "CommonDatapack.h"

#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QCoreApplication>
#include <QVariant>
#include <QMutex>
#include <QTime>
#include <QMutexLocker>

#include <iostream>
#include <zlib.h>

using namespace CatchChallenger;

std::unordered_map<std::string/*file*/, std::unordered_map<uint32_t/*id*/,QDomElement> > Map_loader::teleportConditionsUnparsed;

const std::string Map_loader::text_map="map";
const std::string Map_loader::text_width="width";
const std::string Map_loader::text_height="height";
const std::string Map_loader::text_properties="properties";
const std::string Map_loader::text_property="property";
const std::string Map_loader::text_name="name";
const std::string Map_loader::text_value="value";
const std::string Map_loader::text_objectgroup="objectgroup";
const std::string Map_loader::text_Moving="Moving";
const std::string Map_loader::text_object="object";
const std::string Map_loader::text_x="x";
const std::string Map_loader::text_y="y";
const std::string Map_loader::text_type="type";
const std::string Map_loader::text_borderleft="border-left";
const std::string Map_loader::text_borderright="border-right";
const std::string Map_loader::text_bordertop="border-top";
const std::string Map_loader::text_borderbottom="border-bottom";
const std::string Map_loader::text_teleport_on_push="teleport on push";
const std::string Map_loader::text_teleport_on_it="teleport on it";
const std::string Map_loader::text_door="door";
const std::string Map_loader::text_condition_file="condition-file";
const std::string Map_loader::text_condition_id="condition-id";
const std::string Map_loader::text_rescue="rescue";
const std::string Map_loader::text_bot_spawn="bot spawn";
const std::string Map_loader::text_Object="Object";
const std::string Map_loader::text_bot="bot";
const std::string Map_loader::text_skin="skin";
const std::string Map_loader::text_lookAt="lookAt";
const std::string Map_loader::text_bottom="bottom";
const std::string Map_loader::text_file="file";
const std::string Map_loader::text_id="id";
const std::string Map_loader::text_layer="layer";
const std::string Map_loader::text_encoding="encoding";
const std::string Map_loader::text_compression="compression";
const std::string Map_loader::text_base64="base64";
const std::string Map_loader::text_zlib="zlib";
const std::string Map_loader::text_Walkable="Walkable";
const std::string Map_loader::text_Collisions="Collisions";
const std::string Map_loader::text_Water="Water";
const std::string Map_loader::text_Grass="Grass";
const std::string Map_loader::text_Dirt="Dirt";
const std::string Map_loader::text_LedgesRight="LedgesRight";
const std::string Map_loader::text_LedgesLeft="LedgesLeft";
const std::string Map_loader::text_LedgesBottom="LedgesBottom";
const std::string Map_loader::text_LedgesDown="LedgesDown";
const std::string Map_loader::text_LedgesTop="LedgesTop";
const std::string Map_loader::text_LedgesUp="LedgesUp";
const std::string Map_loader::text_grass="grass";
const std::string Map_loader::text_monster="monster";
const std::string Map_loader::text_minLevel="minLevel";
const std::string Map_loader::text_maxLevel="maxLevel";
const std::string Map_loader::text_level="level";
const std::string Map_loader::text_luck="luck";
const std::string Map_loader::text_water="water";
const std::string Map_loader::text_cave="cave";
const std::string Map_loader::text_condition="condition";
const std::string Map_loader::text_quest="quest";
const std::string Map_loader::text_item="item";
const std::string Map_loader::text_fightBot="fightBot";
const std::string Map_loader::text_clan="clan";
const std::string Map_loader::text_dottmx=".tmx";
const std::string Map_loader::text_dotxml=".xml";
const std::string Map_loader::text_slash="/";
const std::string Map_loader::text_percent="%";
const std::string Map_loader::text_data="data";
const std::string Map_loader::text_dotcomma=";";
const std::string Map_loader::text_false="false";
const std::string Map_loader::text_true="true";
const std::string Map_loader::text_visible="visible";
const std::string Map_loader::text_infinite="infinite";
const std::string Map_loader::text_tilewidth="tilewidth";
const std::string Map_loader::text_tileheight="tileheight";

/// \todo put at walkable the tp on push

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
        std::cerr << "Unknown error while (de)compressing data!" << std::endl;
    }
}

uint32_t Map_loader::decompressZlib(const char * const input, const uint32_t &intputSize, char * const output, const uint32_t &outputSize)
{
    z_stream strm;

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.next_in = (Bytef *) input;
    strm.avail_in = intputSize;
    strm.next_out = (Bytef *) output;
    strm.avail_out = outputSize;

    int ret = inflateInit2(&strm, 15 + 32);

    if (ret != Z_OK) {
    logZlibError(ret);
    return 0;
    }

    do {
    ret = inflate(&strm, Z_SYNC_FLUSH);

    switch (ret) {
        case Z_NEED_DICT:
        case Z_STREAM_ERROR:
        ret = Z_DATA_ERROR;
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
        inflateEnd(&strm);
        logZlibError(ret);
        return 0;
    }

    if (ret != Z_OK && ret != Z_STREAM_END) {
        if((strm.next_out-reinterpret_cast<unsigned char * const>(output))>outputSize)
        {
            logZlibError(Z_STREAM_ERROR);
            return 0;
        }
        logZlibError(Z_STREAM_ERROR);
        return 0;
    }
    }
    while (ret != Z_STREAM_END);

    if (strm.avail_in != 0) {
    logZlibError(Z_DATA_ERROR);
    return 0;
    }

    inflateEnd(&strm);

    return outputSize-strm.avail_out;
}

Map_loader::Map_loader()
{
}

Map_loader::~Map_loader()
{
}

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

bool Map_loader::tryLoadMap(const std::string &fileName)
{
    error.clear();
    Map_to_send map_to_send_temp;
    map_to_send_temp.border.bottom.x_offset=0;
    map_to_send_temp.border.top.x_offset=0;
    map_to_send_temp.border.left.y_offset=0;
    map_to_send_temp.border.right.y_offset=0;

    std::vector<std::string> detectedMonsterCollisionMonsterType,detectedMonsterCollisionLayer;
    QByteArray Walkable,Collisions,Dirt,LedgesRight,LedgesLeft,LedgesBottom,LedgesTop;
    std::vector<QByteArray> monsterList;
    std::map<std::string/*layer*/,const char *> mapLayerContentForMonsterCollision;
    bool ok;
    QDomDocument domDocument;

    //open and quick check the file
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CommonDatapack::commonDatapack.xmlLoadedFile.contains(fileName))
        domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.value(fileName);
    else
    {
        #endif
        QFile mapFile(QString::fromStdString(fileName));
        if(!mapFile.open(QIODevice::ReadOnly))
        {
            error=mapFile.fileName().toStdString()+": "+mapFile.errorString().toStdString();
            return false;
        }
        const QByteArray &xmlContent=mapFile.readAll();
        mapFile.close();
        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            error=mapFile.fileName().toStdString()+", Parse error at line "+std::to_string(errorLine)+" column "+std::to_string(errorColumn)+": "+errorStr.toStdString();
            return false;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
        CommonDatapack::commonDatapack.xmlLoadedFile[fileName]=domDocument;
    }
    #endif
    const QDomElement &root = domDocument.documentElement();
    if(root.tagName().toStdString()!=Map_loader::text_map)
    {
        error="\"map\" root balise not found for the xml file";
        return false;
    }

    //get the width
    if(!root.hasAttribute(QString::fromStdString(Map_loader::text_width)))
    {
        error="the root node has not the attribute \"width\"";
        return false;
    }
    map_to_send_temp.width=root.attribute(QString::fromStdString(Map_loader::text_width)).toUInt(&ok);
    if(!ok)
    {
        error="the root node has wrong attribute \"width\"";
        return false;
    }

    //get the height
    if(!root.hasAttribute(QString::fromStdString(Map_loader::text_height)))
    {
        error="the root node has not the attribute \"height\"";
        return false;
    }
    map_to_send_temp.height=root.attribute(QString::fromStdString(Map_loader::text_height)).toUInt(&ok);
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
    QDomElement child = root.firstChildElement(QString::fromStdString(Map_loader::text_properties));
    if(!child.isNull())
    {
        if(child.isElement())
        {
            QDomElement SubChild=child.firstChildElement(QString::fromStdString(Map_loader::text_property));
            while(!SubChild.isNull())
            {
                if(SubChild.isElement())
                {
                    if(SubChild.hasAttribute(QString::fromStdString(Map_loader::text_name)) && SubChild.hasAttribute(QString::fromStdString(Map_loader::text_value)))
                        map_to_send_temp.property[SubChild.attribute(QString::fromStdString(Map_loader::text_name)).toStdString()]=SubChild.attribute(QString::fromStdString(Map_loader::text_value)).toStdString();
                    else
                    {
                        error="Missing attribute name or value: child.tagName(): "+SubChild.tagName().toStdString()+" (at line: "+std::to_string(SubChild.lineNumber())+")";
                        return false;
                    }
                }
                else
                {
                    error="Is not an element: child.tagName(): "+SubChild.tagName().toStdString()+" (at line: "+std::to_string(SubChild.lineNumber())+")";
                    return false;
                }
                SubChild = SubChild.nextSiblingElement(QString::fromStdString(Map_loader::text_property));
            }
        }
        else
        {
            error="Is not an element: child.tagName(): "+child.tagName().toStdString()+" (at line: "+std::to_string(child.lineNumber())+")";
            return false;
        }
    }

    int tilewidth=16;
    int tileheight=16;
    if(root.hasAttribute(QString::fromStdString(Map_loader::text_tilewidth)))
    {
        tilewidth=root.attribute(QString::fromStdString(Map_loader::text_tilewidth)).toUShort(&ok);
        if(!ok)
        {
            std::cerr << "Unable to open the file: " << fileName << ", tilewidth is not a number" << std::endl;
            tilewidth=16;
        }
    }
    if(root.hasAttribute(QString::fromStdString(Map_loader::text_tileheight)))
    {
        tileheight=root.attribute(QString::fromStdString(Map_loader::text_tileheight)).toUShort(&ok);
        if(!ok)
        {
            std::cerr << "Unable to open the file: " << fileName << ", tilewidth is not a number" << std::endl;
            tileheight=16;
        }
    }

    // objectgroup
    child = root.firstChildElement(QString::fromStdString(Map_loader::text_objectgroup));
    while(!child.isNull())
    {
        if(!child.hasAttribute(QString::fromStdString(Map_loader::text_name)))
            std::cerr << "Has not attribute \"name\": child.tagName(): " << child.tagName().toStdString() << " (at line: " << child.lineNumber() << ")";
        else if(!child.isElement())
            std::cerr << "Is not an element: name: " << child.attribute(QString::fromStdString(Map_loader::text_name)).toStdString()
                      << " child.tagName(): " << child.tagName().toStdString() << " (at line: " << child.lineNumber() << ")";
        else
        {
            if(child.attribute(QString::fromStdString(Map_loader::text_name))==QString::fromStdString(Map_loader::text_Moving))
            {
                QDomElement SubChild=child.firstChildElement(QString::fromStdString(Map_loader::text_object));
                while(!SubChild.isNull())
                {
                    if(SubChild.isElement() && SubChild.hasAttribute(QString::fromStdString(Map_loader::text_x)) && SubChild.hasAttribute(QString::fromStdString(Map_loader::text_y)))
                    {
                        const uint32_t &object_x=SubChild.attribute(QString::fromStdString(Map_loader::text_x)).toUInt(&ok)/tilewidth;
                        if(!ok)
                            std::cerr << "Wrong conversion with x: child.tagName(): " << SubChild.tagName().toStdString()
                                        << " (at line: " << std::to_string(SubChild.lineNumber())
                                        << "), file: " << fileName << std::endl;
                        else
                        {
                            /** the -1 is important to fix object layer bug into tiled!!!
                             * Don't remove! */
                            const uint32_t &object_y=(SubChild.attribute(QString::fromStdString(Map_loader::text_y)).toUInt(&ok)/tileheight)-1;

                            if(!ok)
                                std::cerr << "Wrong conversion with y: child.tagName(): " << SubChild.tagName().toStdString()
                                            << " (at line: " << std::to_string(SubChild.lineNumber())
                                            << "), file: " << fileName << std::endl;
                            else if(object_x>map_to_send_temp.width || object_y>map_to_send_temp.height)
                                std::cerr << "Object out of the map: child.tagName(): " << SubChild.tagName().toStdString()
                                            << " (at line: " << std::to_string(SubChild.lineNumber())
                                            << "), file: " << fileName << std::endl;
                            else if(SubChild.hasAttribute(QString::fromStdString(Map_loader::text_type)))
                            {
                                const std::string &type=SubChild.attribute(QString::fromStdString(Map_loader::text_type)).toStdString();

                                std::unordered_map<std::string,std::string> property_text;
                                const QDomElement &prop=SubChild.firstChildElement(QString::fromStdString(Map_loader::text_properties));
                                if(!prop.isNull())
                                {
                                    #ifdef DEBUG_MESSAGE_MAP
                                    std::cerr << "Wrong conversion with y: child.tagName(): " << prop.tagName().toStdString()
                                                << " (at line: " << std::to_string(prop.lineNumber())
                                                << "), file: " << fileName << std::endl;
                                    #endif
                                    QDomElement property=prop.firstChildElement(QString::fromStdString(Map_loader::text_property));
                                    while(!property.isNull())
                                    {
                                        if(property.hasAttribute(QString::fromStdString(Map_loader::text_name)) && property.hasAttribute(QString::fromStdString(Map_loader::text_value)))
                                            property_text[property.attribute(QString::fromStdString(Map_loader::text_name)).toStdString()]=property.attribute(QString::fromStdString(Map_loader::text_value)).toStdString();
                                        property = property.nextSiblingElement(QString::fromStdString(Map_loader::text_property));
                                    }
                                }
                                if(type==Map_loader::text_borderleft || type==Map_loader::text_borderright || type==Map_loader::text_bordertop || type==Map_loader::text_borderbottom)
                                {
                                    #ifdef DEBUG_MESSAGE_MAP
                                    DebugClass::debugConsole(std::stringLiteral("type: %1, object_x: %2, object_y: %3, border")
                                         .arg(type)
                                         .arg(object_x)
                                         .arg(object_y)
                                         );
                                    #endif
                                    if(property_text.find(Map_loader::text_map)!=property_text.cend())
                                    {
                                        if(type==Map_loader::text_borderleft)//border left
                                        {
                                            const std::string &borderMap=property_text.at(Map_loader::text_map);
                                            if(!borderMap.empty())
                                            {
                                                if(map_to_send_temp.border.left.fileName.empty())
                                                {
                                                    map_to_send_temp.border.left.fileName=borderMap;
                                                    if(!stringEndsWith(map_to_send_temp.border.left.fileName,Map_loader::text_dottmx) && !map_to_send_temp.border.left.fileName.empty())
                                                        map_to_send_temp.border.left.fileName+=Map_loader::text_dottmx;
                                                    map_to_send_temp.border.left.y_offset=object_y;
                                                }
                                                else
                                                    std::cerr << "The border " << SubChild.tagName().toStdString() << " " << type << " is already set (at line: " << SubChild.lineNumber() << "), file: " << fileName << std::endl;
                                            }
                                            else
                                                std::cerr << "The border " << SubChild.tagName().toStdString() << " " << type << " can't be empty (at line: " << SubChild.lineNumber() << "), file: " << fileName << std::endl;
                                        }
                                        else if(type==Map_loader::text_borderright)//border right
                                        {
                                            const std::string &borderMap=property_text.at(Map_loader::text_map);
                                            if(!borderMap.empty())
                                            {
                                                if(map_to_send_temp.border.right.fileName.empty())
                                                {
                                                    map_to_send_temp.border.right.fileName=borderMap;
                                                    if(!stringEndsWith(map_to_send_temp.border.right.fileName,Map_loader::text_dottmx) && !map_to_send_temp.border.right.fileName.empty())
                                                        map_to_send_temp.border.right.fileName+=Map_loader::text_dottmx;
                                                    map_to_send_temp.border.right.y_offset=object_y;
                                                }
                                                else
                                                    std::cerr << "The border " << SubChild.tagName().toStdString() << " " << type << " is already set (at line: " << SubChild.lineNumber() << "), file: " << fileName << std::endl;
                                            }
                                            else
                                                std::cerr << "The border " << SubChild.tagName().toStdString() << " " << type << " can't be empty (at line: " << SubChild.lineNumber() << "), file: " << fileName << std::endl;
                                        }
                                        else if(type==Map_loader::text_bordertop)//border top
                                        {
                                            const std::string &borderMap=property_text.at(Map_loader::text_map);
                                            if(!borderMap.empty())
                                            {
                                                if(map_to_send_temp.border.top.fileName.empty())
                                                {
                                                    map_to_send_temp.border.top.fileName=borderMap;
                                                    if(!stringEndsWith(map_to_send_temp.border.top.fileName,Map_loader::text_dottmx) && !map_to_send_temp.border.top.fileName.empty())
                                                        map_to_send_temp.border.top.fileName+=Map_loader::text_dottmx;
                                                    map_to_send_temp.border.top.x_offset=object_x;
                                                }
                                                else
                                                    std::cerr << "The border " << SubChild.tagName().toStdString() << " " << type << " is already set (at line: " << SubChild.lineNumber() << "), file: " << fileName << std::endl;
                                            }
                                            else
                                                std::cerr << "The border " << SubChild.tagName().toStdString() << " " << type << " can't be empty (at line: " << SubChild.lineNumber() << "), file: " << fileName << std::endl;
                                        }
                                        else if(type==Map_loader::text_borderbottom)//border bottom
                                        {
                                            const std::string &borderMap=property_text.at(Map_loader::text_map);
                                            if(!borderMap.empty())
                                            {
                                                if(map_to_send_temp.border.bottom.fileName.empty())
                                                {
                                                    map_to_send_temp.border.bottom.fileName=borderMap;
                                                    if(!stringEndsWith(map_to_send_temp.border.bottom.fileName,Map_loader::text_dottmx) && !map_to_send_temp.border.bottom.fileName.empty())
                                                        map_to_send_temp.border.bottom.fileName+=Map_loader::text_dottmx;
                                                    map_to_send_temp.border.bottom.x_offset=object_x;
                                                }
                                                else
                                                    std::cerr << "The border " << SubChild.tagName().toStdString() << " " << type << " is already set (at line: " << SubChild.lineNumber() << "), file: " << fileName << std::endl;
                                            }
                                            else
                                                std::cerr << "The border " << SubChild.tagName().toStdString() << " " << type << " can't be empty (at line: " << SubChild.lineNumber() << "), file: " << fileName << std::endl;
                                        }
                                        else
                                            std::cerr << "Not at middle of border: child.tagName(): " << SubChild.tagName().toStdString() << ", object_x: " << object_x << ", object_y: " << object_y << ", file: " << fileName << std::endl;
                                    }
                                    else
                                        std::cerr << "Missing \"map\" properties for the border: " << SubChild.tagName().toStdString() << " (at line: " << SubChild.lineNumber() << ")" << std::endl;
                                }
                                else if(type==Map_loader::text_teleport_on_push || type==Map_loader::text_teleport_on_it || type==Map_loader::text_door)
                                {
                                    if(property_text.find(Map_loader::text_map)!=property_text.cend() && property_text.find(Map_loader::text_x)!=property_text.cend() && property_text.find(Map_loader::text_y)!=property_text.cend())
                                    {
                                        Map_semi_teleport new_tp;
                                        new_tp.condition.type=MapConditionType_None;
                                        new_tp.condition.value=0;
                                        new_tp.destination_x = stringtouint8(property_text.at(Map_loader::text_x),&ok);
                                        if(ok)
                                        {
                                            new_tp.destination_y = stringtouint8(property_text.at(Map_loader::text_y),&ok);
                                            if(ok)
                                            {
                                                if(property_text.find(Map_loader::text_condition_file)!=property_text.cend() && property_text.find(Map_loader::text_condition_id)!=property_text.cend())
                                                {
                                                    uint32_t conditionId=stringtouint32(property_text.at(Map_loader::text_condition_id),&ok);
                                                    if(!ok)
                                                        std::cerr << "condition id is not a number, id: " << property_text.at("condition-id") << " (" << fileName << " at line: " << SubChild.lineNumber() << ")" << std::endl;
                                                    else
                                                    {
                                                        std::string conditionFile=QFileInfo(
                                                                    QFileInfo(QString::fromStdString(fileName)).absolutePath()+
                                                                    QString::fromStdString(Map_loader::text_slash)+
                                                                    QString::fromStdString(property_text.at(Map_loader::text_condition_file))
                                                                    ).absoluteFilePath().toStdString();
                                                        if(!stringEndsWith(conditionFile,Map_loader::text_dotxml))
                                                            conditionFile+=Map_loader::text_dotxml;
                                                        new_tp.conditionUnparsed=getXmlCondition(fileName,conditionFile,conditionId);
                                                        new_tp.condition=xmlConditionToMapCondition(conditionFile,new_tp.conditionUnparsed);
                                                    }
                                                }
                                                new_tp.source_x=object_x;
                                                new_tp.source_y=object_y;
                                                new_tp.map=property_text.at(Map_loader::text_map);
                                                if(!stringEndsWith(new_tp.map,Map_loader::text_dottmx) && !new_tp.map.empty())
                                                    new_tp.map+=Map_loader::text_dottmx;
                                                map_to_send_temp.teleport.push_back(new_tp);
                                            }
                                            else
                                                std::cerr << "Bad convertion to int for y, type: " << type << ", value: " << property_text.at("y") << " (" << fileName << " at line: " << SubChild.lineNumber() << ")" << std::endl;
                                        }
                                        else
                                            std::cerr << "Bad convertion to int for x, type: " << type << ", value: " << property_text.at("x") << " (" << fileName << " at line: " << SubChild.lineNumber() << ")" << std::endl;
                                    }
                                    else
                                        std::cerr << "Missing map,x or y, type: " << type << " (at line: " << SubChild.lineNumber() << "), file: " << fileName << std::endl;
                                }
                                else if(type==Map_loader::text_rescue)
                                {
                                    Map_to_send::Map_Point tempPoint;
                                    tempPoint.x=object_x;
                                    tempPoint.y=object_y;
                                    map_to_send_temp.rescue_points.push_back(tempPoint);
                                    //map_to_send_temp.bot_spawn_points << tempPoint;
                                }
                                else
                                {
                                    std::cerr << "Unknown type: " << type
                                              << ", object_x: " << object_x
                                              << ", object_y: " << object_y
                                              << " (moving), " << SubChild.tagName().toStdString()
                                              << " (line: " << SubChild.lineNumber()
                                              << "), file: " << fileName << std::endl;
                                }
                            }
                            else
                                std::cerr << "Missing attribute type missing: SubChild.tagName(): " << SubChild.tagName().toStdString() << " (at line: " << SubChild.lineNumber() << "), file: " << fileName << std::endl;
                        }
                    }
                    else
                        std::cerr << "Is not Element: SubChild.tagName(): " << SubChild.tagName().toStdString() << " (at line: " << SubChild.lineNumber() << "), file: " << fileName << std::endl;
                    SubChild = SubChild.nextSiblingElement(QString::fromStdString(Map_loader::text_object));
                }
            }
            if(child.attribute(QString::fromStdString(Map_loader::text_name)).toStdString()==Map_loader::text_Object)
            {
                QDomElement SubChild=child.firstChildElement(QString::fromStdString(Map_loader::text_object));
                while(!SubChild.isNull())
                {
                    if(SubChild.isElement() && SubChild.hasAttribute(QString::fromStdString(Map_loader::text_x)) && SubChild.hasAttribute(QString::fromStdString(Map_loader::text_y)))
                    {
                        const uint32_t &object_x=SubChild.attribute(QString::fromStdString(Map_loader::text_x)).toUInt(&ok)/tilewidth;
                        if(!ok)
                            std::cerr << "Wrong conversion with x: child.tagName(): " << SubChild.tagName().toStdString()
                                        << " (at line: " << std::to_string(SubChild.lineNumber())
                                        << "), file: " << fileName << std::endl;
                        else
                        {
                            /** the -1 is important to fix object layer bug into tiled!!!
                             * Don't remove! */
                            const uint32_t &object_y=(SubChild.attribute(QString::fromStdString(Map_loader::text_y)).toUInt(&ok)/tileheight)-1;

                            if(!ok)
                                std::cerr << "Wrong conversion with y: child.tagName(): " << SubChild.tagName().toStdString()
                                            << " (at line: " << std::to_string(SubChild.lineNumber())
                                            << "), file: " << fileName << std::endl;
                            else if(object_x>map_to_send_temp.width || object_y>map_to_send_temp.height)
                                std::cerr << "Object out of the map: child.tagName(): " << SubChild.tagName().toStdString()
                                            << " (at line: " << std::to_string(SubChild.lineNumber())
                                            << "), file: " << fileName << std::endl;
                            else if(SubChild.hasAttribute(QString::fromStdString(Map_loader::text_type)))
                            {
                                const std::string &type=SubChild.attribute(QString::fromStdString(Map_loader::text_type)).toStdString();

                                std::unordered_map<std::string,std::string> property_text;
                                const QDomElement &prop=SubChild.firstChildElement(QString::fromStdString(Map_loader::text_properties));
                                if(!prop.isNull())
                                {
                                    QDomElement property=prop.firstChildElement(QString::fromStdString(Map_loader::text_property));
                                    while(!property.isNull())
                                    {
                                        if(property.hasAttribute(QString::fromStdString(Map_loader::text_name)) && property.hasAttribute(QString::fromStdString(Map_loader::text_value)))
                                            property_text[property.attribute(QString::fromStdString(Map_loader::text_name)).toStdString()]=property.attribute(QString::fromStdString(Map_loader::text_value)).toStdString();
                                        property = property.nextSiblingElement(QString::fromStdString(Map_loader::text_property));
                                    }
                                }
                                if(type==Map_loader::text_bot)
                                {
                                    if(property_text.find(Map_loader::text_skin)!=property_text.cend() && !property_text.at(Map_loader::text_skin).empty() && property_text.find(Map_loader::text_lookAt)==property_text.cend())
                                    {
                                        property_text[Map_loader::text_lookAt]=Map_loader::text_bottom;
                                        std::cerr << "skin but not lookAt, fixed by bottom: " << SubChild.tagName().toStdString() << " (" << fileName << " at line: " << SubChild.lineNumber() << ")" << std::endl;
                                    }
                                    if(property_text.find(Map_loader::text_file)!=property_text.cend() && property_text.find(Map_loader::text_id)!=property_text.cend())
                                    {
                                        Map_to_send::Bot_Semi bot_semi;
                                        bot_semi.file=QFileInfo(
                                                        QString::fromStdString(
                                                            QFileInfo(QString::fromStdString(fileName)).absolutePath().toStdString()+
                                                            Map_loader::text_slash+
                                                            property_text.at(Map_loader::text_file)
                                                        )
                                                    ).absoluteFilePath().toStdString();
                                        bot_semi.id=stringtouint16(property_text.at(Map_loader::text_id),&ok);
                                        bot_semi.property_text=property_text;
                                        if(ok)
                                        {
                                            bot_semi.point.x=object_x;
                                            bot_semi.point.y=object_y;
                                            map_to_send_temp.bots.push_back(bot_semi);
                                        }
                                    }
                                    else
                                        std::cerr << "Missing \"bot\" properties for the bot: " << SubChild.tagName().toStdString()
                                                  << " (at line: " << SubChild.lineNumber()
                                                  << "), file: " << fileName << std::endl;
                                }
                                else if(type==Map_loader::text_object)
                                {
                                    if(property_text.find(Map_loader::text_item)!=property_text.cend())
                                    {
                                        Map_to_send::ItemOnMap_Semi item_semi;
                                        item_semi.infinite=false;
                                        if(property_text.find(Map_loader::text_infinite)!=property_text.cend() && property_text.at(Map_loader::text_infinite)==Map_loader::text_true)
                                            item_semi.infinite=true;
                                        item_semi.visible=true;
                                        if(property_text.find(Map_loader::text_visible)!=property_text.cend() && property_text.at(Map_loader::text_visible)==Map_loader::text_false)
                                            item_semi.visible=false;
                                        item_semi.item=stringtouint16(property_text.at(Map_loader::text_item),&ok);
                                        if(ok)
                                        {
                                            item_semi.point.x=object_x;
                                            item_semi.point.y=object_y;
                                            map_to_send_temp.items.push_back(item_semi);
                                        }
                                    }
                                    else
                                        std::cerr << "Missing \"bot\" properties for the bot: " << SubChild.tagName().toStdString()
                                                  << " (at line: " << SubChild.lineNumber()
                                                  << "), file: " << fileName
                                                  << std::endl;
                                }
                                else
                                {
                                    std::cerr << "Unknown type: " << type
                                              << ", object_x: " << object_x
                                              << ", object_y: " << object_y
                                              << " (moving), " << SubChild.tagName().toStdString()
                                              << " (line: " << SubChild.lineNumber()
                                              << "), file: " << fileName
                                              << std::endl;
                                }
                            }
                            else
                                std::cerr << "Missing attribute type missing: SubChild.tagName(): " << SubChild.tagName().toStdString() << " (at line: " << SubChild.lineNumber() << "), file: " << fileName << std::endl;
                        }
                    }
                    else
                        std::cerr << "Is not Element: SubChild.tagName(): " << SubChild.tagName().toStdString() << " (at line: " << SubChild.lineNumber() << "), file: " << fileName << std::endl;
                    SubChild = SubChild.nextSiblingElement(QString::fromStdString(Map_loader::text_object));
                }
            }
        }
        child = child.nextSiblingElement(QString::fromStdString(Map_loader::text_objectgroup));
    }

    const uint32_t &rawSize=map_to_send_temp.width*map_to_send_temp.height*4;

    // layer
    child = root.firstChildElement(QString::fromStdString(Map_loader::text_layer));
    while(!child.isNull())
    {
        if(!child.isElement())
        {
            error="Is Element: child.tagName(): "+child.tagName().toStdString()+", file: "+fileName;
            return false;
        }
        else if(!child.hasAttribute(QString::fromStdString(Map_loader::text_name)))
        {
            error="Has not attribute \"name\": child.tagName(): "+child.tagName().toStdString()+", file: "+fileName;
            return false;
        }
        else
        {
            const QDomElement &data=child.firstChildElement(QString::fromStdString(Map_loader::text_data));
            const std::string name=child.attribute(QString::fromStdString(Map_loader::text_name)).toStdString();
            if(data.isNull())
            {
                error="Is Element for layer is null: "+data.tagName().toStdString()+" and name: "+name+", file: "+fileName;
                return false;
            }
            else if(!data.isElement())
            {
                error="Is Element for layer child.tagName(): "+data.tagName().toStdString()+", file: "+fileName;
                return false;
            }
            else if(!data.hasAttribute(QString::fromStdString(Map_loader::text_encoding)))
            {
                error="Has not attribute \"base64\": child.tagName(): "+data.tagName().toStdString()+", file: "+fileName;
                return false;
            }
            else if(!data.hasAttribute(QString::fromStdString(Map_loader::text_compression)))
            {
                error="Has not attribute \"zlib\": child.tagName(): "+data.tagName().toStdString()+", file: "+fileName;
                return false;
            }
            else if(data.attribute(QString::fromStdString(Map_loader::text_encoding)).toStdString()!=Map_loader::text_base64)
            {
                error="only encoding base64 is supported, file: file: "+fileName;
                return false;
            }
            else if(data.attribute(QString::fromStdString(Map_loader::text_compression)).toStdString()!=Map_loader::text_zlib)
            {
                error="Only compression zlib is supported, file: file: "+fileName;
                return false;
            }
            else
            {
                const QByteArray compressedData=QByteArray::fromBase64(data.text().toLatin1());
                QByteArray dataRaw;
                dataRaw.resize(map_to_send_temp.height*map_to_send_temp.width*4);
                dataRaw.resize(decompressZlib(compressedData.constData(),compressedData.size(),dataRaw.data(),dataRaw.size()));
                if((uint32_t)dataRaw.size()!=map_to_send_temp.height*map_to_send_temp.width*4)
                {
                    error="map binary size ("+std::to_string(dataRaw.size())+") != "+std::to_string(map_to_send_temp.height)+"x"+std::to_string(map_to_send_temp.width)+"x4";
                    return false;
                }
                if(name==Map_loader::text_Walkable)
                {
                    if(Walkable.isEmpty())
                        Walkable=dataRaw;
                    else
                    {
                        const int &layersize=Walkable.size();
                        int index=0;
                        while(index<layersize)
                        {
                            Walkable[index]=Walkable.at(index) || dataRaw.at(index);
                            index++;
                        }
                    }
                }
                else if(name==Map_loader::text_Collisions)
                {
                    if(Collisions.isEmpty())
                        Collisions=dataRaw;
                    else
                    {
                        int index=0;
                        const int &layersize=Collisions.size();
                        while(index<layersize)
                        {
                            Collisions[index]=Collisions.at(index) || dataRaw.at(index);
                            index++;
                        }
                    }
                }
                else if(name==Map_loader::text_Dirt)
                {
                    if(Dirt.isEmpty())
                        Dirt=dataRaw;
                    else
                    {
                        int index=0;
                        const int &layersize=Dirt.size();
                        while(index<layersize)
                        {
                            Dirt[index]=Dirt.at(index) || dataRaw.at(index);
                            index++;
                        }
                    }
                }
                else if(name==Map_loader::text_LedgesRight)
                {
                    if(LedgesRight.isEmpty())
                        LedgesRight=dataRaw;
                    else
                    {
                        int index=0;
                        const int &layersize=LedgesRight.size();
                        while(index<layersize)
                        {
                            LedgesRight[index]=LedgesRight.at(index) || dataRaw.at(index);
                            index++;
                        }
                    }
                }
                else if(name==Map_loader::text_LedgesLeft)
                {
                    if(LedgesLeft.isEmpty())
                        LedgesLeft=dataRaw;
                    else
                    {
                        int index=0;
                        const int &layersize=LedgesLeft.size();
                        while(index<layersize)
                        {
                            LedgesLeft[index]=LedgesLeft.at(index) || dataRaw.at(index);
                            index++;
                        }
                    }
                }
                else if(name==Map_loader::text_LedgesBottom || name==Map_loader::text_LedgesDown)
                {
                    if(LedgesBottom.isEmpty())
                        LedgesBottom=dataRaw;
                    else
                    {
                        int index=0;
                        const int &layersize=LedgesBottom.size();
                        while(index<layersize)
                        {
                            LedgesBottom[index]=LedgesBottom.at(index) || dataRaw.at(index);
                            index++;
                        }
                    }
                }
                else if(name==Map_loader::text_LedgesTop || name==Map_loader::text_LedgesUp)
                {
                    if(LedgesTop.isEmpty())
                        LedgesTop=dataRaw;
                    else
                    {
                        int index=0;
                        const int &layersize=LedgesTop.size();
                        while(index<layersize)
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
                                mapLayerContentForMonsterCollision[name]=dataRaw.constData();
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
        }
        child = child.nextSiblingElement(QString ::fromStdString(Map_loader::text_layer));
    }

    /*QByteArray null_data;
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
    std::vector<const char *> MonsterCollisionBin;
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
                    dirtOnMap_Semi.point.x=x;
                    dirtOnMap_Semi.point.y=y;
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
                    else
                        map_to_send_temp.parsed_layer.ledges[x+y*map_to_send_temp.width]=(uint8_t)ParsedLayerLedges_LedgesLeft;
                }
                if(ledgesRight)
                {
                    if(ledgesLeft || ledgesBottom || ledgesTop)
                    {
                        std::cerr << "Multiple ledges at the same place, do colision for right" << std::endl;
                        map_to_send_temp.parsed_layer.walkable[x+y*map_to_send_temp.width]=false;
                    }
                    else
                        map_to_send_temp.parsed_layer.ledges[x+y*map_to_send_temp.width]=(uint8_t)ParsedLayerLedges_LedgesRight;
                }
                if(ledgesTop)
                {
                    if(ledgesRight || ledgesBottom || ledgesLeft)
                    {
                        std::cerr << "Multiple ledges at the same place, do colision for top" << std::endl;
                        map_to_send_temp.parsed_layer.walkable[x+y*map_to_send_temp.width]=false;
                    }
                    else
                        map_to_send_temp.parsed_layer.ledges[x+y*map_to_send_temp.width]=(uint8_t)ParsedLayerLedges_LedgesTop;
                }
                if(ledgesBottom)
                {
                    if(ledgesRight || ledgesLeft || ledgesTop)
                    {
                        std::cerr << "Multiple ledges at the same place, do colision for bottom" << std::endl;
                        map_to_send_temp.parsed_layer.walkable[x+y*map_to_send_temp.width]=false;
                    }
                    else
                        map_to_send_temp.parsed_layer.ledges[x+y*map_to_send_temp.width]=(uint8_t)ParsedLayerLedges_LedgesBottom;
                }
            }
            y++;
        }
        x++;
    }

    const int &teleportlistsize=map_to_send_temp.teleport.size();
    if(Walkable.size()>0)
    {
        int index=0;
        {
            while(index<teleportlistsize)
            {
                map_to_send_temp.parsed_layer.walkable[map_to_send_temp.teleport.at(index).source_x+map_to_send_temp.teleport.at(index).source_y*map_to_send_temp.width]=true;
                index++;
            }
        }
        index=0;
        {
            const int &listsize=map_to_send_temp.bots.size();
            while(index<listsize)
            {
                map_to_send_temp.parsed_layer.walkable[map_to_send_temp.bots.at(index).point.x+map_to_send_temp.bots.at(index).point.y*map_to_send_temp.width]=false;
                index++;
            }
        }
    }

    //don't put code here !!!!!! put before the last block
    this->map_to_send=map_to_send_temp;

    std::string xmlExtra=fileName;
    stringreplaceAll(xmlExtra,Map_loader::text_dottmx,Map_loader::text_dotxml);
    if(QFile::exists(QString::fromStdString(xmlExtra)))
        loadMonsterMap(xmlExtra,detectedMonsterCollisionMonsterType,detectedMonsterCollisionLayer);

    {
        this->map_to_send.parsed_layer.monstersCollisionMap=new uint8_t[this->map_to_send.width*this->map_to_send.height];
        {
            uint8_t x=0;
            while(x<this->map_to_send.width)
            {
                uint8_t y=0;
                while(y<this->map_to_send.height)
                {
                    this->map_to_send.parsed_layer.monstersCollisionMap[x+y*this->map_to_send.width]=0;
                    y++;
                }
                x++;
            }
        }

        {
            auto i=mapLayerContentForMonsterCollision.begin();
            while(i!=mapLayerContentForMonsterCollision.cend())
            {
                if(zoneNumber.find(i->first)!=zoneNumber.cend())
                {
                    const uint8_t &zoneId=zoneNumber.at(i->first);
                    uint8_t x=0;
                    while(x<this->map_to_send.width)
                    {
                        uint8_t y=0;
                        while(y<this->map_to_send.height)
                        {
                            if(i->second[x*4+y*map_to_send_temp.width*4+0]!=0x00 || i->second[x*4+y*map_to_send_temp.width*4+1]!=0x00 || i->second[x*4+y*map_to_send_temp.width*4+2]!=0x00 || i->second[x*4+y*map_to_send_temp.width*4+3]!=0x00)
                            {
                                if(this->map_to_send.parsed_layer.monstersCollisionMap[x+y*this->map_to_send.width]==0)
                                    this->map_to_send.parsed_layer.monstersCollisionMap[x+y*this->map_to_send.width]=zoneId;
                                else if(this->map_to_send.parsed_layer.monstersCollisionMap[x+y*this->map_to_send.width]==zoneId)
                                {}//ignore, same zone
                                else
                                {
                                    std::cerr << "Have already monster at " << std::to_string(x) << "," << std::to_string(y) << " for " << fileName
                                              << ", actual zone: " << std::to_string(this->map_to_send.parsed_layer.monstersCollisionMap[x+y*this->map_to_send.width])
                                              << " (" << CommonDatapack::commonDatapack.monstersCollision.at(this->map_to_send.parsed_layer.monstersCollisionMap[x+y*this->map_to_send.width]).layer
                                              << "), new zone: " << zoneId << " (" << CommonDatapack::commonDatapack.monstersCollision.at(zoneId).layer << ")";
                                    this->map_to_send.parsed_layer.monstersCollisionMap[x+y*this->map_to_send.width]=zoneId;//overwrited by above layer
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
        QByteArray null_data;
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
                    line += std::string::number(Walkable.mid(x*4+y*this->map_to_send.width*4,4)!=null_data);
                    x++;
                }
                line+=" ";
            }
            if(this->map_to_send.parsed_layer.monstersCollisionMap!=NULL)
            {
                x=0;
                while(x<this->map_to_send.width)
                {
                    line += std::string::number(this->map_to_send.parsed_layer.monstersCollisionMap[x+y*this->map_to_send.width]);
                    x++;
                }
                line+=" ";
            }
            if(Collisions.size()>0)
            {
                x=0;
                while(x<this->map_to_send.width)
                {
                    line += std::string::number(Collisions.mid(x*4+y*this->map_to_send.width*4,4)!=null_data);
                    x++;
                }
                line+=" ";
            }
            if(Dirt.size()>0)
            {
                x=0;
                while(x<this->map_to_send.width)
                {
                    line += std::string::number(Dirt.mid(x*4+y*this->map_to_send.width*4,4)!=null_data);
                    x++;
                }
                line+=" ";
            }
            if(LedgesRight.size()>0 || LedgesLeft.size()>0 || LedgesBottom.size()>0 || LedgesTop.size()>0)
            {
                x=0;
                while(x<this->map_to_send.width)
                {
                    line += std::string::number(this->map_to_send.parsed_layer.ledges[x+y*this->map_to_send.width]);
                    x++;
                }
                line+=" ";
            }
            x=0;
            while(x<this->map_to_send.width)
            {
                line += std::string::number(this->map_to_send.parsed_layer.walkable[x+y*this->map_to_send.width]);
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

    return true;
}

bool Map_loader::loadMonsterMap(const std::string &fileName, std::vector<std::string> detectedMonsterCollisionMonsterType, std::vector<std::string> detectedMonsterCollisionLayer)
{
    QDomDocument domDocument;

    //open and quick check the file
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CommonDatapack::commonDatapack.xmlLoadedFile.contains(fileName))
        domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.value(fileName);
    else
    {
        #endif
        QFile mapFile(QString::fromStdString(fileName));
        if(!mapFile.open(QIODevice::ReadOnly))
        {
            std::cerr << mapFile.fileName().toStdString() << ": " << mapFile.errorString().toStdString() << std::endl;
            return false;
        }
        const QByteArray &xmlContent=mapFile.readAll();
        mapFile.close();
        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            std::cerr << mapFile.fileName().toStdString() << ", Parse error at line " << std::to_string(errorLine) << ", column " << std::to_string(errorColumn) << ": " << errorStr.toStdString() << std::endl;
            return false;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
        CommonDatapack::commonDatapack.xmlLoadedFile[fileName]=domDocument;
    }
    #endif
    this->map_to_send.xmlRoot = domDocument.documentElement();
    if(this->map_to_send.xmlRoot.tagName().toStdString()!=Map_loader::text_map)
    {
        std::cerr << "\"map\" root balise not found for the xml file" << std::endl;
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
            detectedMonsterCollisionMonsterType.push_back(Map_loader::text_cave);
    }

    //load the found monster type
    std::unordered_map<std::string/*monsterType*/,std::vector<MapMonster> > monsterTypeList;
    {
        unsigned int index=0;
        while(index<detectedMonsterCollisionMonsterType.size())
        {
            if(!detectedMonsterCollisionMonsterType.at(index).empty())
                if(monsterTypeList.find(detectedMonsterCollisionMonsterType.at(index))==monsterTypeList.cend())
                    monsterTypeList[detectedMonsterCollisionMonsterType.at(index)]=loadSpecificMonster(fileName,detectedMonsterCollisionMonsterType.at(index));
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
    const QDomElement &layer = map_to_send.xmlRoot.firstChildElement(QString::fromStdString(monsterType));
    if(!layer.isNull())
    {
        if(layer.isElement())
        {
            QDomElement monsters=layer.firstChildElement(QString::fromStdString(Map_loader::text_monster));
            while(!monsters.isNull())
            {
                if(monsters.isElement())
                {
                    if(monsters.hasAttribute(QString::fromStdString(Map_loader::text_id)) && ((monsters.hasAttribute(QString::fromStdString(Map_loader::text_minLevel)) && monsters.hasAttribute(QString::fromStdString(Map_loader::text_maxLevel))) || monsters.hasAttribute(QString::fromStdString(Map_loader::text_level))) && monsters.hasAttribute(QString::fromStdString(Map_loader::text_luck)))
                    {
                        MapMonster mapMonster;
                        mapMonster.id=monsters.attribute(QString::fromStdString(Map_loader::text_id)).toUInt(&ok);
                        if(!ok)
                            std::cerr << "id is not a number: child.tagName(): " << monsters.tagName().toStdString() << " (at line: " << monsters.lineNumber() << "), file: " << fileName << std::endl;
                        if(ok)
                            if(CommonDatapack::commonDatapack.monsters.find(mapMonster.id)==CommonDatapack::commonDatapack.monsters.cend())
                            {
                                std::cerr << "monster " << mapMonster.id << " not found into the monster list: " << monsters.tagName().toStdString() << " (at line: " << monsters.lineNumber() << "), file: " << fileName << std::endl;
                                ok=false;
                            }
                        if(monsters.hasAttribute(QString::fromStdString(Map_loader::text_minLevel)) && monsters.hasAttribute(QString::fromStdString(Map_loader::text_maxLevel)))
                        {
                            if(ok)
                            {
                                mapMonster.minLevel=monsters.attribute(QString::fromStdString(Map_loader::text_minLevel)).toUShort(&ok);
                                if(!ok)
                                    std::cerr << "minLevel is not a number: child.tagName(): " << monsters.tagName().toStdString() << " (at line: " << monsters.lineNumber() << "), file: " << fileName << std::endl;
                            }
                            if(ok)
                            {
                                mapMonster.maxLevel=monsters.attribute(QString::fromStdString(Map_loader::text_maxLevel)).toUShort(&ok);
                                if(!ok)
                                    std::cerr << "maxLevel is not a number: child.tagName(): " << monsters.tagName().toStdString() << " (at line: " << monsters.lineNumber() << "), file: " << fileName << std::endl;
                            }
                        }
                        else
                        {
                            if(ok)
                            {
                                mapMonster.maxLevel=monsters.attribute(QString::fromStdString(Map_loader::text_level)).toUShort(&ok);
                                mapMonster.minLevel=mapMonster.maxLevel;
                                if(!ok)
                                    std::cerr << "level is not a number: child.tagName(): " << monsters.tagName().toStdString() << " (at line: " << monsters.lineNumber() << "), file: " << fileName << std::endl;
                            }
                        }
                        if(ok)
                        {
                            std::string textLuck=monsters.attribute(QString::fromStdString(Map_loader::text_luck)).toStdString();
                            stringreplaceAll(textLuck,Map_loader::text_percent,"");
                            mapMonster.luck=stringtouint8(textLuck,&ok);
                            if(!ok)
                                std::cerr << "luck is not a number: child.tagName(): " << monsters.tagName().toStdString() << " (at line: " << monsters.lineNumber() << "), file: " << fileName << std::endl;
                        }
                        if(ok)
                            if(mapMonster.minLevel>mapMonster.maxLevel)
                            {
                                std::cerr << "min > max for the level: child.tagName(): " << monsters.tagName().toStdString() << " (at line: " << monsters.lineNumber() << "), file: " << fileName << std::endl;
                                ok=false;
                            }
                        if(ok)
                            if(mapMonster.luck<=0)
                            {
                                std::cerr << "luck is too low: child.tagName(): " << monsters.tagName().toStdString() << " (at line: " << monsters.lineNumber() << "), file: " << fileName << std::endl;
                                ok=false;
                            }
                        if(ok)
                            if(mapMonster.minLevel<=0)
                            {
                                std::cerr << "min level is too low: child.tagName(): " << monsters.tagName().toStdString() << " (at line: " << monsters.lineNumber() << "), file: " << fileName << std::endl;
                                ok=false;
                            }
                        if(ok)
                            if(mapMonster.maxLevel<=0)
                            {
                                std::cerr << "max level is too low: child.tagName(): " << monsters.tagName().toStdString() << " (at line: " << monsters.lineNumber() << "), file: " << fileName << std::endl;
                                ok=false;
                            }
                        if(ok)
                            if(mapMonster.luck>100)
                            {
                                std::cerr << "luck is greater than 100: child.tagName(): " << monsters.tagName().toStdString() << " (at line: " << monsters.lineNumber() << "), file: " << fileName << std::endl;
                                ok=false;
                            }
                        if(ok)
                            if(mapMonster.minLevel>CATCHCHALLENGER_MONSTER_LEVEL_MAX)
                            {
                                std::cerr << "min level is greater than " << CATCHCHALLENGER_MONSTER_LEVEL_MAX << ": child.tagName(): " << monsters.tagName().toStdString() << " (at line: " << monsters.lineNumber() << "), file: " << fileName << std::endl;
                                ok=false;
                            }
                        if(ok)
                            if(mapMonster.maxLevel>CATCHCHALLENGER_MONSTER_LEVEL_MAX)
                            {
                                std::cerr << "max level is greater than " << CATCHCHALLENGER_MONSTER_LEVEL_MAX << ": child.tagName(): " << monsters.tagName().toStdString() << " (at line: " << monsters.lineNumber() << "), file: " << fileName << std::endl;
                                ok=false;
                            }
                        if(ok)
                        {
                            tempLuckTotal+=mapMonster.luck;
                            monsterTypeList.push_back(mapMonster);
                        }
                    }
                    else
                        std::cerr << "Missing attribute: child.tagName(): " << monsters.tagName().toStdString() << " (at line: " << monsters.lineNumber() << "), file: " << fileName << std::endl;
                }
                else
                    std::cerr << "Is not an element: child.tagName(): " << monsters.tagName().toStdString() << " (at line: " << monsters.lineNumber() << "), file: " << fileName << std::endl;
                monsters = monsters.nextSiblingElement(QString::fromStdString(Map_loader::text_monster));
            }
            if(monsterTypeList.empty())
                std::cerr << "map have empty monster layer:" << fileName << "type:" << monsterType;
        }
        else
            std::cerr << "Is not an element: child.tagName(): " << layer.tagName().toStdString() << " (at line: " << layer.lineNumber() << "), file: " << fileName << std::endl;
        if(tempLuckTotal!=100)
        {
            std::cerr << "total luck is not egal to 100 (" << tempLuckTotal << ") for grass, monsters dropped: child.tagName(): " << layer.tagName().toStdString() << " (at line: " << std::to_string(layer.lineNumber()) << "), file: " << fileName << std::endl;
            monsterTypeList.clear();
        }
        else
            return monsterTypeList;
    }
    /*else//do ignore for cave
    qDebug() << std::stringLiteral("A layer on map is found, but no matching monster list into the meta (%3), monsters dropped: child.tagName(): %1 (at line: %2)").arg(layer.tagName()).arg(layer.lineNumber()).arg(fileName);*/
    return monsterTypeList;
}

std::string Map_loader::errorString()
{
    return error;
}

std::string Map_loader::resolvRelativeMap(const std::string &fileName,const std::string &link,const std::string &datapackPath)
{
    if(link.empty())
        return link;
    const std::string &currentPath=QFileInfo(QString::fromStdString(fileName)).absolutePath().toStdString();
    QFileInfo newmap(QString::fromStdString(currentPath)+QDir::separator()+QString::fromStdString(link));
    std::string newPath=newmap.absoluteFilePath().toStdString();
    if(datapackPath.empty() || stringStartWith(newPath,datapackPath))
    {
        newPath.erase(0,datapackPath.size());
        return newPath;
    }
    std::cerr << "map link not resolved: " << link
              << ", full path: " << currentPath << QDir::separator().toLatin1() << link
              << ", newPath: " << newPath
              << ", datapackPath: " << datapackPath << std::endl;
    return link;
}

QDomElement Map_loader::getXmlCondition(const std::string &fileName,const std::string &conditionFile,const uint32_t &conditionId)
{
    #ifdef ONLYMAPRENDER
    return QDomElement();
    #endif
    if(teleportConditionsUnparsed.find(conditionFile)!=teleportConditionsUnparsed.cend())
    {
        if(teleportConditionsUnparsed.at(conditionFile).find(conditionId)!=teleportConditionsUnparsed.at(conditionFile).cend())
            return teleportConditionsUnparsed.at(conditionFile).at(conditionId);
        else
            return QDomElement();
    }
    teleportConditionsUnparsed[conditionFile][conditionId];
    bool ok;
    QDomDocument domDocument;

    //open and quick check the file
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CommonDatapack::commonDatapack.xmlLoadedFile.contains(conditionFile))
        domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.value(conditionFile);
    else
    {
        #endif
        QFile mapFile(QString::fromStdString(conditionFile));
        if(!mapFile.open(QIODevice::ReadOnly))
        {
            std::cerr << "Into the file " << fileName << ", unable to open the condition file: " << mapFile.fileName().toStdString() << ": " << mapFile.errorString().toStdString() << std::endl;
            return QDomElement();
        }
        const QByteArray &xmlContent=mapFile.readAll();
        mapFile.close();
        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            std::cerr << mapFile.fileName().toStdString() << ", Parse error at line " << errorLine << ", column " << errorColumn << ": " << errorStr.toStdString() << std::endl;
            return QDomElement();
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
        CommonDatapack::commonDatapack.xmlLoadedFile[conditionFile]=domDocument;
    }
    #endif
    const QDomElement &root = domDocument.documentElement();
    if(root.tagName()!=QLatin1String("conditions"))
    {
        std::cerr << "\"conditions\" root balise not found for the xml file " << conditionFile << std::endl;
        return QDomElement();
    }

    QDomElement item = root.firstChildElement(QString::fromStdString(Map_loader::text_condition));
    while(!item.isNull())
    {
        if(item.isElement())
        {
            if(!item.hasAttribute(QString::fromStdString(Map_loader::text_id)))
                std::cerr << "\"condition\" balise have not id attribute (" << conditionFile << " at " << item.lineNumber() << ")" << std::endl;
            else if(!item.hasAttribute(QString::fromStdString(Map_loader::text_type)))
                std::cerr << "\"condition\" balise have not type attribute (" << conditionFile << " at " << item.lineNumber() << ")" << std::endl;
            else
            {
                const uint32_t &id=item.attribute(QString::fromStdString(Map_loader::text_id)).toUInt(&ok);
                if(!ok)
                    std::cerr << "\"condition\" balise have id is not a number (" << conditionFile << " at " << item.lineNumber() << ")" << std::endl;
                else
                    teleportConditionsUnparsed[conditionFile][id]=item;
            }
        }
        item = item.nextSiblingElement(QString::fromStdString(Map_loader::text_condition));
    }
    if(teleportConditionsUnparsed.find(conditionFile)!=teleportConditionsUnparsed.cend())
        if(teleportConditionsUnparsed.at(conditionFile).find(conditionId)!=teleportConditionsUnparsed.at(conditionFile).cend())
            return teleportConditionsUnparsed.at(conditionFile).at(conditionId);
    return QDomElement();
}

MapCondition Map_loader::xmlConditionToMapCondition(const std::string &conditionFile,const QDomElement &conditionContent)
{
    #ifdef ONLYMAPRENDER
    return MapCondition();
    #endif
    bool ok;
    MapCondition condition;
    condition.type=MapConditionType_None;
    condition.value=0;
    if(conditionContent.attribute(QString::fromStdString(Map_loader::text_type)).toStdString()==Map_loader::text_quest)
    {
        if(!conditionContent.hasAttribute(QString::fromStdString(Map_loader::text_quest)))
            std::cerr << "\"condition\" balise have type=quest but quest attribute not found, item, clan or fightBot (" << conditionFile << " at " << conditionContent.lineNumber() << ")" << std::endl;
        else
        {
            const uint32_t &quest=stringtouint32(conditionContent.attribute(QString::fromStdString(Map_loader::text_quest)).toStdString(),&ok);
            if(!ok)
                std::cerr << "\"condition\" balise have type=quest but quest attribute is not a number, item, clan or fightBot (" << conditionFile << " at " << conditionContent.lineNumber() << ")" << std::endl;
            else if(CommonDatapackServerSpec::commonDatapackServerSpec.quests.find(quest)==CommonDatapackServerSpec::commonDatapackServerSpec.quests.cend())
                std::cerr << "\"condition\" balise have type=quest but quest id is not found, item, clan or fightBot (" << conditionFile << " at " << conditionContent.lineNumber() << ")" << std::endl;
            else
            {
                condition.type=MapConditionType_Quest;
                condition.value=quest;
            }
        }
    }
    else if(conditionContent.attribute(QString::fromStdString(Map_loader::text_type)).toStdString()==Map_loader::text_item)
    {
        if(!conditionContent.hasAttribute(QString::fromStdString(Map_loader::text_item)))
            std::cerr << "\"condition\" balise have type=item but item attribute not found, item, clan or fightBot (" << conditionFile << " at " << conditionContent.lineNumber() << ")" << std::endl;
        else
        {
            const uint32_t &item=conditionContent.attribute(QString::fromStdString(Map_loader::text_item)).toUInt(&ok);
            if(!ok)
                std::cerr << "\"condition\" balise have type=item but item attribute is not a number, item, clan or fightBot (" << conditionFile << " at " << conditionContent.lineNumber() << ")" << std::endl;
            else if(CommonDatapack::commonDatapack.items.item.find(item)==CommonDatapack::commonDatapack.items.item.cend())
                std::cerr << "\"condition\" balise have type=item but item id is not found, item, clan or fightBot (" << conditionFile << " at " << conditionContent.lineNumber() << ")" << std::endl;
            else
            {
                condition.type=MapConditionType_Item;
                condition.value=item;
            }
        }
    }
    else if(conditionContent.attribute(QString::fromStdString(Map_loader::text_type)).toStdString()==Map_loader::text_fightBot)
    {
        if(!conditionContent.hasAttribute(QString::fromStdString(Map_loader::text_fightBot)))
            std::cerr << "\"condition\" balise have type=fightBot but fightBot attribute not found, item, clan or fightBot (" << conditionFile << " at " << conditionContent.lineNumber() << ")" << std::endl;
        else
        {
            const uint32_t &fightBot=conditionContent.attribute(QString::fromStdString(Map_loader::text_fightBot)).toUInt(&ok);
            if(!ok)
                std::cerr << "\"condition\" balise have type=fightBot but fightBot attribute is not a number, item, clan or fightBot (" << conditionFile << " at " << conditionContent.lineNumber() << ")" << std::endl;
            else if(CommonDatapackServerSpec::commonDatapackServerSpec.botFights.find(fightBot)==CommonDatapackServerSpec::commonDatapackServerSpec.botFights.cend())
                std::cerr << "\"condition\" balise have type=fightBot but fightBot id is not found, item, clan or fightBot (" << conditionFile << " at " << conditionContent.lineNumber() << ")" << std::endl;
            else
            {
                condition.type=MapConditionType_FightBot;
                condition.value=fightBot;
            }
        }
    }
    else if(conditionContent.attribute(QString::fromStdString(Map_loader::text_type)).toStdString()==Map_loader::text_clan)
        condition.type=MapConditionType_Clan;
    else
        std::cerr << "\"condition\" balise have type but value is not quest, item, clan or fightBot (" << conditionFile << " at " << conditionContent.lineNumber() << ")" << std::endl;
    return condition;
}
