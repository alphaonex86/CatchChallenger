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
#include <QMultiHash>

#include <zlib.h>

using namespace CatchChallenger;

std::unordered_map<std::string/*file*/, std::unordered_map<uint32_t/*id*/,QDomElement> > Map_loader::teleportConditionsUnparsed;

const std::string Map_loader::text_map=QLatin1Literal("map");
const std::string Map_loader::text_width=QLatin1Literal("width");
const std::string Map_loader::text_height=QLatin1Literal("height");
const std::string Map_loader::text_properties=QLatin1Literal("properties");
const std::string Map_loader::text_property=QLatin1Literal("property");
const std::string Map_loader::text_name=QLatin1Literal("name");
const std::string Map_loader::text_value=QLatin1Literal("value");
const std::string Map_loader::text_objectgroup=QLatin1Literal("objectgroup");
const std::string Map_loader::text_Moving=QLatin1Literal("Moving");
const std::string Map_loader::text_object=QLatin1Literal("object");
const std::string Map_loader::text_x=QLatin1Literal("x");
const std::string Map_loader::text_y=QLatin1Literal("y");
const std::string Map_loader::text_type=QLatin1Literal("type");
const std::string Map_loader::text_borderleft=QLatin1Literal("border-left");
const std::string Map_loader::text_borderright=QLatin1Literal("border-right");
const std::string Map_loader::text_bordertop=QLatin1Literal("border-top");
const std::string Map_loader::text_borderbottom=QLatin1Literal("border-bottom");
const std::string Map_loader::text_teleport_on_push=QLatin1Literal("teleport on push");
const std::string Map_loader::text_teleport_on_it=QLatin1Literal("teleport on it");
const std::string Map_loader::text_door=QLatin1Literal("door");
const std::string Map_loader::text_condition_file=QLatin1Literal("condition-file");
const std::string Map_loader::text_condition_id=QLatin1Literal("condition-id");
const std::string Map_loader::text_rescue=QLatin1Literal("rescue");
const std::string Map_loader::text_bot_spawn=QLatin1Literal("bot spawn");
const std::string Map_loader::text_Object=QLatin1Literal("Object");
const std::string Map_loader::text_bot=QLatin1Literal("bot");
const std::string Map_loader::text_skin=QLatin1Literal("skin");
const std::string Map_loader::text_lookAt=QLatin1Literal("lookAt");
const std::string Map_loader::text_bottom=QLatin1Literal("bottom");
const std::string Map_loader::text_file=QLatin1Literal("file");
const std::string Map_loader::text_id=QLatin1Literal("id");
const std::string Map_loader::text_layer=QLatin1Literal("layer");
const std::string Map_loader::text_encoding=QLatin1Literal("encoding");
const std::string Map_loader::text_compression=QLatin1Literal("compression");
const std::string Map_loader::text_base64=QLatin1Literal("base64");
const std::string Map_loader::text_Walkable=QLatin1Literal("Walkable");
const std::string Map_loader::text_Collisions=QLatin1Literal("Collisions");
const std::string Map_loader::text_Water=QLatin1Literal("Water");
const std::string Map_loader::text_Grass=QLatin1Literal("Grass");
const std::string Map_loader::text_Dirt=QLatin1Literal("Dirt");
const std::string Map_loader::text_LedgesRight=QLatin1Literal("LedgesRight");
const std::string Map_loader::text_LedgesLeft=QLatin1Literal("LedgesLeft");
const std::string Map_loader::text_LedgesBottom=QLatin1Literal("LedgesBottom");
const std::string Map_loader::text_LedgesDown=QLatin1Literal("LedgesDown");
const std::string Map_loader::text_LedgesTop=QLatin1Literal("LedgesTop");
const std::string Map_loader::text_LedgesUp=QLatin1Literal("LedgesUp");
const std::string Map_loader::text_grass=QLatin1Literal("grass");
const std::string Map_loader::text_monster=QLatin1Literal("monster");
const std::string Map_loader::text_minLevel=QLatin1Literal("minLevel");
const std::string Map_loader::text_maxLevel=QLatin1Literal("maxLevel");
const std::string Map_loader::text_level=QLatin1Literal("level");
const std::string Map_loader::text_luck=QLatin1Literal("luck");
const std::string Map_loader::text_water=QLatin1Literal("water");
const std::string Map_loader::text_cave=QLatin1Literal("cave");
const std::string Map_loader::text_condition=QLatin1Literal("condition");
const std::string Map_loader::text_quest=QLatin1Literal("quest");
const std::string Map_loader::text_item=QLatin1Literal("item");
const std::string Map_loader::text_fightBot=QLatin1Literal("fightBot");
const std::string Map_loader::text_clan=QLatin1Literal("clan");
const std::string Map_loader::text_dottmx=QLatin1Literal(".tmx");
const std::string Map_loader::text_dotxml=QLatin1Literal(".xml");
const std::string Map_loader::text_slash=QLatin1Literal("/");
const std::string Map_loader::text_percent=QLatin1Literal("%");
const std::string Map_loader::text_data=QLatin1Literal("data");
const std::string Map_loader::text_dotcomma=QLatin1Literal(";");
const std::string Map_loader::text_false=QLatin1Literal("false");
const std::string Map_loader::text_true=QLatin1Literal("true");
const std::string Map_loader::text_visible=QLatin1Literal("visible");
const std::string Map_loader::text_infinite=QLatin1Literal("infinite");
const std::string Map_loader::text_tilewidth=QLatin1Literal("tilewidth");
const std::string Map_loader::text_tileheight=QLatin1Literal("tileheight");

/// \todo put at walkable the tp on push

static void logZlibError(int error)
{
    switch (error)
    {
    case Z_MEM_ERROR:
        qDebug() << "Out of memory while (de)compressing data!";
        break;
    case Z_VERSION_ERROR:
        qDebug() << "Incompatible zlib version!";
        break;
    case Z_NEED_DICT:
    case Z_DATA_ERROR:
        qDebug() << "Incorrect zlib compressed data!";
        break;
    default:
        qDebug() << "Unknown error while (de)compressing data!";
    }
}

QByteArray Map_loader::decompress(const QByteArray &data, int expectedSize)
{
    QByteArray out;
    out.resize(expectedSize);
    z_stream strm;

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.next_in = (Bytef *) data.data();
    strm.avail_in = data.length();
    strm.next_out = (Bytef *) out.data();
    strm.avail_out = out.size();

    int ret = inflateInit2(&strm, 15 + 32);

    if (ret != Z_OK) {
    logZlibError(ret);
    return QByteArray();
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
        return QByteArray();
    }

    if (ret != Z_STREAM_END) {
        if(out.size()>16*1024*1024)
        {
            logZlibError(Z_STREAM_ERROR);
            return QByteArray();
        }
        int oldSize = out.size();
        out.resize(out.size() * 2);

        strm.next_out = (Bytef *)(out.data() + oldSize);
        strm.avail_out = oldSize;
    }
    }
    while (ret != Z_STREAM_END);

    if (strm.avail_in != 0) {
    logZlibError(Z_DATA_ERROR);
    return QByteArray();
    }

    const int outLength = out.size() - strm.avail_out;
    inflateEnd(&strm);

    out.resize(outLength);
    return out;
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
    QMap<std::string/*layer*/,const char *> mapLayerContentForMonsterCollision;
    bool ok;
    QDomDocument domDocument;

    //open and quick check the file
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CommonDatapack::commonDatapack.xmlLoadedFile.contains(fileName))
        domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.value(fileName);
    else
    {
        #endif
        QFile mapFile(fileName);
        if(!mapFile.open(QIODevice::ReadOnly))
        {
            error=mapFile.fileName()+QLatin1String(": ")+mapFile.errorString();
            return false;
        }
        const QByteArray &xmlContent=mapFile.readAll();
        mapFile.close();
        std::string errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            error=std::stringLiteral("%1, Parse error at line %2, column %3: %4").arg(mapFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr);
            return false;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
        CommonDatapack::commonDatapack.xmlLoadedFile[fileName]=domDocument;
    }
    #endif
    const QDomElement &root = domDocument.documentElement();
    if(root.tagName()!=Map_loader::text_map)
    {
        error=std::stringLiteral("\"map\" root balise not found for the xml file");
        return false;
    }

    //get the width
    if(!root.hasAttribute(Map_loader::text_width))
    {
        error=std::stringLiteral("the root node has not the attribute \"width\"");
        return false;
    }
    map_to_send_temp.width=root.attribute(Map_loader::text_width).toUInt(&ok);
    if(!ok)
    {
        error=std::stringLiteral("the root node has wrong attribute \"width\"");
        return false;
    }

    //get the height
    if(!root.hasAttribute(Map_loader::text_height))
    {
        error=std::stringLiteral("the root node has not the attribute \"height\"");
        return false;
    }
    map_to_send_temp.height=root.attribute(Map_loader::text_height).toUInt(&ok);
    if(!ok)
    {
        error=std::stringLiteral("the root node has wrong attribute \"height\"");
        return false;
    }

    //check the size
    if(map_to_send_temp.width<1 || map_to_send_temp.width>255)
    {
        error=std::stringLiteral("the width should be greater or equal than 1, and lower or equal than 32000");
        return false;
    }
    if(map_to_send_temp.height<1 || map_to_send_temp.height>255)
    {
        error=std::stringLiteral("the height should be greater or equal than 1, and lower or equal than 32000");
        return false;
    }

    //properties
    QDomElement child = root.firstChildElement(Map_loader::text_properties);
    if(!child.isNull())
    {
        if(child.isElement())
        {
            QDomElement SubChild=child.firstChildElement(Map_loader::text_property);
            while(!SubChild.isNull())
            {
                if(SubChild.isElement())
                {
                    if(SubChild.hasAttribute(Map_loader::text_name) && SubChild.hasAttribute(Map_loader::text_value))
                        map_to_send_temp.property[SubChild.attribute(Map_loader::text_name)]=SubChild.attribute(Map_loader::text_value);
                    else
                    {
                        error=std::stringLiteral("Missing attribute name or value: child.tagName(): %1 (at line: %2)").arg(SubChild.tagName()).arg(SubChild.lineNumber());
                        return false;
                    }
                }
                else
                {
                    error=std::stringLiteral("Is not an element: child.tagName(): %1 (at line: %2)").arg(SubChild.tagName()).arg(SubChild.lineNumber());
                    return false;
                }
                SubChild = SubChild.nextSiblingElement(Map_loader::text_property);
            }
        }
        else
        {
            error=std::stringLiteral("Is not an element: child.tagName(): %1 (at line: %2)").arg(child.tagName()).arg(child.lineNumber());
            return false;
        }
    }

    int tilewidth=16;
    int tileheight=16;
    if(root.hasAttribute(Map_loader::text_tilewidth))
    {
        tilewidth=root.attribute(Map_loader::text_tilewidth).toUShort(&ok);
        if(!ok)
        {
            qDebug() << std::stringLiteral("Unable to open the file: %1, tilewidth is not a number").arg(fileName);
            tilewidth=16;
        }
    }
    if(root.hasAttribute(Map_loader::text_tileheight))
    {
        tileheight=root.attribute(Map_loader::text_tileheight).toUShort(&ok);
        if(!ok)
        {
            qDebug() << std::stringLiteral("Unable to open the file: %1, tilewidth is not a number").arg(fileName);
            tileheight=16;
        }
    }

    // objectgroup
    child = root.firstChildElement(Map_loader::text_objectgroup);
    while(!child.isNull())
    {
        if(!child.hasAttribute(Map_loader::text_name))
            DebugClass::debugConsole(std::stringLiteral("Has not attribute \"name\": child.tagName(): %1 (at line: %2)").arg(child.tagName()).arg(child.lineNumber()));
        else if(!child.isElement())
            DebugClass::debugConsole(std::stringLiteral("Is not an element: child.tagName(): %1, name: %2 (at line: %3)").arg(child.tagName().arg(child.attribute(Map_loader::text_name)).arg(child.lineNumber())));
        else
        {
            if(child.attribute(Map_loader::text_name)==Map_loader::text_Moving)
            {
                QDomElement SubChild=child.firstChildElement(Map_loader::text_object);
                while(!SubChild.isNull())
                {
                    if(SubChild.isElement() && SubChild.hasAttribute(Map_loader::text_x) && SubChild.hasAttribute(Map_loader::text_y))
                    {
                        const uint32_t &object_x=SubChild.attribute(Map_loader::text_x).toUInt(&ok)/tilewidth;
                        if(!ok)
                            DebugClass::debugConsole(std::stringLiteral("Wrong conversion with x: %1 (at line: %2), file: %3").arg(SubChild.tagName()).arg(SubChild.lineNumber()).arg(fileName));
                        else
                        {
                            /** the -1 is important to fix object layer bug into tiled!!!
                             * Don't remove! */
                            const uint32_t &object_y=(SubChild.attribute(Map_loader::text_y).toUInt(&ok)/tileheight)-1;

                            if(!ok)
                                DebugClass::debugConsole(std::stringLiteral("Wrong conversion with y: %1 (at line: %2), file: %3").arg(SubChild.tagName()).arg(SubChild.lineNumber()).arg(fileName));
                            else if(object_x>map_to_send_temp.width || object_y>map_to_send_temp.height)
                                DebugClass::debugConsole(std::stringLiteral("Object out of the map: %1 (at line: %2), file: %3").arg(SubChild.tagName()).arg(SubChild.lineNumber()).arg(fileName));
                            else if(SubChild.hasAttribute(Map_loader::text_type))
                            {
                                const std::string &type=SubChild.attribute(Map_loader::text_type);

                                std::unordered_map<std::string,QVariant> property_text;
                                const QDomElement &prop=SubChild.firstChildElement(Map_loader::text_properties);
                                if(!prop.isNull())
                                {
                                    #ifdef DEBUG_MESSAGE_MAP
                                    DebugClass::debugConsole(std::stringLiteral("type: %1, object_x: %2, object_y: %3, !prop.isNull()")
                                                 .arg(type)
                                                 .arg(object_x)
                                                 .arg(object_y)
                                                 );
                                    #endif
                                    QDomElement property=prop.firstChildElement(Map_loader::text_property);
                                    while(!property.isNull())
                                    {
                                        if(property.hasAttribute(Map_loader::text_name) && property.hasAttribute(Map_loader::text_value))
                                            property_text[property.attribute(Map_loader::text_name)]=property.attribute(Map_loader::text_value);
                                        property = property.nextSiblingElement(Map_loader::text_property);
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
                                    if(property_text.contains(Map_loader::text_map))
                                    {
                                        if(type==Map_loader::text_borderleft)//border left
                                        {
                                            const std::string &borderMap=property_text.value(Map_loader::text_map).toString();
                                            if(!borderMap.isEmpty())
                                            {
                                                if(map_to_send_temp.border.left.fileName.isEmpty())
                                                {
                                                    map_to_send_temp.border.left.fileName=borderMap;
                                                    if(!map_to_send_temp.border.left.fileName.endsWith(Map_loader::text_dottmx) && !map_to_send_temp.border.left.fileName.isEmpty())
                                                        map_to_send_temp.border.left.fileName+=Map_loader::text_dottmx;
                                                    map_to_send_temp.border.left.y_offset=object_y;
                                                    #ifdef DEBUG_MESSAGE_MAP_BORDER
                                                    DebugClass::debugConsole(std::stringLiteral("map_to_send.border.left.fileName: %1, offset: %2").arg(map_to_send.border.left.fileName).arg(map_to_send.border.left.y_offset));
                                                    #endif
                                                }
                                                else
                                                    DebugClass::debugConsole(std::stringLiteral("The border %1 %2 is already set (at line: %3), file: %4").arg(SubChild.tagName()).arg(type).arg(SubChild.lineNumber()).arg(fileName));
                                            }
                                            else
                                                DebugClass::debugConsole(std::stringLiteral("The border %1 %2 can't be empty (at line: %3), file: %4").arg(SubChild.tagName()).arg(type).arg(SubChild.lineNumber()).arg(fileName));
                                        }
                                        else if(type==Map_loader::text_borderright)//border right
                                        {
                                            const std::string &borderMap=property_text.value(Map_loader::text_map).toString();
                                            if(!borderMap.isEmpty())
                                            {
                                                if(map_to_send_temp.border.right.fileName.isEmpty())
                                                {
                                                    map_to_send_temp.border.right.fileName=borderMap;
                                                    if(!map_to_send_temp.border.right.fileName.endsWith(Map_loader::text_dottmx) && !map_to_send_temp.border.right.fileName.isEmpty())
                                                        map_to_send_temp.border.right.fileName+=QLatin1String(".tmx");
                                                    map_to_send_temp.border.right.y_offset=object_y;
                                                    #ifdef DEBUG_MESSAGE_MAP_BORDER
                                                    DebugClass::debugConsole(std::stringLiteral("map_to_send.border.right.fileName: %1, offset: %2").arg(map_to_send.border.right.fileName).arg(map_to_send.border.right.y_offset));
                                                    #endif
                                                }
                                                else
                                                    DebugClass::debugConsole(std::stringLiteral("The border %1 %2 is already set (at line: %3), file: %4").arg(SubChild.tagName()).arg(type).arg(SubChild.lineNumber()).arg(fileName));
                                            }
                                            else
                                                DebugClass::debugConsole(std::stringLiteral("The border %1 %2 can't be empty (at line: %3), file: %4").arg(SubChild.tagName()).arg(type).arg(SubChild.lineNumber()).arg(fileName));
                                        }
                                        else if(type==Map_loader::text_bordertop)//border top
                                        {
                                            const std::string &borderMap=property_text.value(Map_loader::text_map).toString();
                                            if(!borderMap.isEmpty())
                                            {
                                                if(map_to_send_temp.border.top.fileName.isEmpty())
                                                {
                                                    map_to_send_temp.border.top.fileName=borderMap;
                                                    if(!map_to_send_temp.border.top.fileName.endsWith(Map_loader::text_dottmx) && !map_to_send_temp.border.top.fileName.isEmpty())
                                                        map_to_send_temp.border.top.fileName+=Map_loader::text_dottmx;
                                                    map_to_send_temp.border.top.x_offset=object_x;
                                                    #ifdef DEBUG_MESSAGE_MAP_BORDER
                                                    DebugClass::debugConsole(std::stringLiteral("map_to_send.border.top.fileName: %1, offset: %2").arg(map_to_send.border.top.fileName).arg(map_to_send.border.top.x_offset));
                                                    #endif
                                                }
                                                else
                                                    DebugClass::debugConsole(std::stringLiteral("The border %1 %2 is already set (at line: %3), file: %4").arg(SubChild.tagName()).arg(type).arg(SubChild.lineNumber()).arg(fileName));
                                            }
                                            else
                                                DebugClass::debugConsole(std::stringLiteral("The border %1 %2 can't be empty (at line: %3), file: %4").arg(SubChild.tagName()).arg(type).arg(SubChild.lineNumber()).arg(fileName));
                                        }
                                        else if(type==Map_loader::text_borderbottom)//border bottom
                                        {
                                            const std::string &borderMap=property_text.value(Map_loader::text_map).toString();
                                            if(!borderMap.isEmpty())
                                            {
                                                if(map_to_send_temp.border.bottom.fileName.isEmpty())
                                                {
                                                    map_to_send_temp.border.bottom.fileName=borderMap;
                                                    if(!map_to_send_temp.border.bottom.fileName.endsWith(Map_loader::text_dottmx) && !map_to_send_temp.border.bottom.fileName.isEmpty())
                                                        map_to_send_temp.border.bottom.fileName+=Map_loader::text_dottmx;
                                                    map_to_send_temp.border.bottom.x_offset=object_x;
                                                    #ifdef DEBUG_MESSAGE_MAP_BORDER
                                                    DebugClass::debugConsole(std::stringLiteral("map_to_send.border.bottom.fileName: %1, offset: %2").arg(map_to_send.border.bottom.fileName).arg(map_to_send.border.bottom.x_offset));
                                                    #endif
                                                }
                                                else
                                                    DebugClass::debugConsole(std::stringLiteral("The border %1 %2 is already set (at line: %3), file: %4").arg(SubChild.tagName()).arg(type).arg(SubChild.lineNumber()).arg(fileName));
                                            }
                                            else
                                                DebugClass::debugConsole(std::stringLiteral("The border %1 %2 can't be empty (at line: %3), file: %4").arg(SubChild.tagName()).arg(type).arg(SubChild.lineNumber()).arg(fileName));
                                        }
                                        else
                                            DebugClass::debugConsole(std::stringLiteral("Not at middle of border: child.tagName(): %1, object_x: %2, object_y: %3, file: %4")
                                                 .arg(SubChild.tagName())
                                                 .arg(object_x)
                                                 .arg(object_y)
                                                 .arg(fileName)
                                                 );
                                    }
                                    else
                                        DebugClass::debugConsole(std::stringLiteral("Missing \"map\" properties for the border: %1 (at line: %2)").arg(SubChild.tagName()).arg(SubChild.lineNumber()));
                                }
                                else if(type==Map_loader::text_teleport_on_push || type==Map_loader::text_teleport_on_it || type==Map_loader::text_door)
                                {
                                    #ifdef DEBUG_MESSAGE_MAP
                                    DebugClass::debugConsole(std::stringLiteral("type: %1, object_x: %2, object_y: %3, tp")
                                         .arg(type)
                                         .arg(object_x)
                                         .arg(object_y)
                                         );
                                    #endif
                                    if(property_text.contains(Map_loader::text_map) && property_text.contains(Map_loader::text_x) && property_text.contains(Map_loader::text_y))
                                    {
                                        Map_semi_teleport new_tp;
                                        new_tp.condition.type=MapConditionType_None;
                                        new_tp.condition.value=0;
                                        new_tp.destination_x = property_text.value(Map_loader::text_x).toUInt(&ok);
                                        if(ok)
                                        {
                                            new_tp.destination_y = property_text.value(Map_loader::text_y).toUInt(&ok);
                                            if(ok)
                                            {
                                                if(property_text.contains(Map_loader::text_condition_file) && property_text.contains(Map_loader::text_condition_id))
                                                {
                                                    uint32_t conditionId=property_text.value(Map_loader::text_condition_id).toUInt(&ok);
                                                    if(!ok)
                                                        DebugClass::debugConsole(std::stringLiteral("condition id is not a number, id: %1 (%2 at line: %3)").arg(property_text.value(QLatin1String("condition-id")).toString()).arg(fileName).arg(SubChild.lineNumber()));
                                                    else
                                                    {
                                                        std::string conditionFile=QFileInfo(QFileInfo(fileName).absolutePath()+Map_loader::text_slash+property_text.value(Map_loader::text_condition_file).toString()).absoluteFilePath();
                                                        if(!conditionFile.endsWith(Map_loader::text_dotxml))
                                                            conditionFile+=Map_loader::text_dotxml;
                                                        new_tp.conditionUnparsed=getXmlCondition(fileName,conditionFile,conditionId);
                                                        new_tp.condition=xmlConditionToMapCondition(conditionFile,new_tp.conditionUnparsed);
                                                    }
                                                }
                                                new_tp.source_x=object_x;
                                                new_tp.source_y=object_y;
                                                new_tp.map=property_text.value(Map_loader::text_map).toString();
                                                if(!new_tp.map.endsWith(Map_loader::text_dottmx) && !new_tp.map.isEmpty())
                                                    new_tp.map+=Map_loader::text_dottmx;
                                                map_to_send_temp.teleport << new_tp;
                                                #ifdef DEBUG_MESSAGE_MAP_OBJECT
                                                DebugClass::debugConsole(std::stringLiteral("Teleporter type: %1, to: %2 (%3,%4)").arg(type).arg(new_tp.map).arg(new_tp.source_x).arg(new_tp.source_y));
                                                #endif
                                            }
                                            else
                                                DebugClass::debugConsole(std::stringLiteral("Bad convertion to int for y, type: %1, value: %2 (%3 at line: %4)").arg(type).arg(property_text.value(QLatin1String("y")).toString()).arg(fileName).arg(SubChild.lineNumber()));
                                        }
                                        else
                                            DebugClass::debugConsole(std::stringLiteral("Bad convertion to int for x, type: %1, value: %2 (%3 at line: %4)").arg(type).arg(property_text.value(QLatin1String("x")).toString()).arg(fileName).arg(SubChild.lineNumber()));
                                    }
                                    else
                                        DebugClass::debugConsole(std::stringLiteral("Missing map,x or y, type: %1 (at line: %2), file: %3").arg(type).arg(SubChild.lineNumber()).arg(fileName));
                                }
                                else if(type==Map_loader::text_rescue)
                                {
                                    #ifdef DEBUG_MESSAGE_MAP
                                    DebugClass::debugConsole(std::stringLiteral("type: %1, object_x: %2, object_y: %3, rescue")
                                         .arg(type)
                                         .arg(object_x)
                                         .arg(object_y)
                                         );
                                    #endif
                                    Map_to_send::Map_Point tempPoint;
                                    tempPoint.x=object_x;
                                    tempPoint.y=object_y;
                                    map_to_send_temp.rescue_points << tempPoint;
                                    //map_to_send_temp.bot_spawn_points << tempPoint;
                                }
                                else if(type==Map_loader::text_bot_spawn)
                                {
                                    /*#ifdef DEBUG_MESSAGE_MAP
                                    DebugClass::debugConsole(std::stringLiteral("type: %1, object_x: %2, object_y: %3, bot spawn")
                                         .arg(type)
                                         .arg(object_x)
                                         .arg(object_y)
                                         );
                                    #endif
                                    Map_to_send::Map_Point tempPoint;
                                    tempPoint.x=object_x;
                                    tempPoint.y=object_y;
                                    map_to_send_temp.bot_spawn_points << tempPoint;*/
                                }
                                else
                                {
                                    DebugClass::debugConsole(std::stringLiteral("Unknown type: %1, object_x: %2, object_y: %3 (moving), %4 (line: %5), file: %6")
                                         .arg(type)
                                         .arg(object_x)
                                         .arg(object_y)
                                         .arg(SubChild.tagName())
                                         .arg(SubChild.lineNumber())
                                         .arg(fileName)
                                         );
                                }

                            }
                            else
                                DebugClass::debugConsole(std::stringLiteral("Missing attribute type missing: SubChild.tagName(): %1 (at line: %2), file: %3").arg(SubChild.tagName()).arg(SubChild.lineNumber()).arg(fileName));
                        }
                    }
                    else
                        DebugClass::debugConsole(std::stringLiteral("Is not Element: SubChild.tagName(): %1 (at line: %2), file: %3").arg(SubChild.tagName()).arg(SubChild.lineNumber()).arg(fileName));
                    SubChild = SubChild.nextSiblingElement(Map_loader::text_object);
                }
            }
            if(child.attribute(Map_loader::text_name)==Map_loader::text_Object)
            {
                QDomElement SubChild=child.firstChildElement(Map_loader::text_object);
                while(!SubChild.isNull())
                {
                    if(SubChild.isElement() && SubChild.hasAttribute(Map_loader::text_x) && SubChild.hasAttribute(Map_loader::text_y))
                    {
                        const uint32_t &object_x=SubChild.attribute(Map_loader::text_x).toUInt(&ok)/tilewidth;
                        if(!ok)
                            DebugClass::debugConsole(std::stringLiteral("Wrong conversion with x: %1 (at line: %2): %3").arg(SubChild.tagName()).arg(SubChild.lineNumber()).arg(fileName));
                        else
                        {
                            /** the -1 is important to fix object layer bug into tiled!!!
                             * Don't remove! */
                            const uint32_t &object_y=(SubChild.attribute(Map_loader::text_y).toUInt(&ok)/tileheight)-1;

                            if(!ok)
                                DebugClass::debugConsole(std::stringLiteral("Wrong conversion with y: %1 (at line: %2), file: %3").arg(SubChild.tagName()).arg(SubChild.lineNumber()).arg(fileName));
                            else if(object_x>map_to_send_temp.width || object_y>map_to_send_temp.height)
                                DebugClass::debugConsole(std::stringLiteral("Object out of the map: %1 (at line: %2), file: %3").arg(SubChild.tagName()).arg(SubChild.lineNumber()).arg(fileName));
                            else if(SubChild.hasAttribute(Map_loader::text_type))
                            {
                                const std::string &type=SubChild.attribute(Map_loader::text_type);

                                std::unordered_map<std::string,QVariant> property_text;
                                const QDomElement &prop=SubChild.firstChildElement(Map_loader::text_properties);
                                if(!prop.isNull())
                                {
                                    #ifdef DEBUG_MESSAGE_MAP
                                    DebugClass::debugConsole(std::stringLiteral("type: %1, object_x: %2, object_y: %3, !prop.isNull()")
                                                 .arg(type)
                                                 .arg(object_x)
                                                 .arg(object_y)
                                                 );
                                    #endif
                                    QDomElement property=prop.firstChildElement(Map_loader::text_property);
                                    while(!property.isNull())
                                    {
                                        if(property.hasAttribute(Map_loader::text_name) && property.hasAttribute(Map_loader::text_value))
                                            property_text[property.attribute(Map_loader::text_name)]=property.attribute(Map_loader::text_value);
                                        property = property.nextSiblingElement(Map_loader::text_property);
                                    }
                                }
                                if(type==Map_loader::text_bot)
                                {
                                    #ifdef DEBUG_MESSAGE_MAP
                                    DebugClass::debugConsole(std::stringLiteral("type: %1, object_x: %2, object_y: %3")
                                         .arg(type)
                                         .arg(object_x)
                                         .arg(object_y)
                                         );
                                    #endif
                                    if(property_text.contains(Map_loader::text_skin) && !property_text.value(Map_loader::text_skin).toString().isEmpty() && !property_text.contains(Map_loader::text_lookAt))
                                    {
                                        property_text[Map_loader::text_lookAt]=Map_loader::text_bottom;
                                        DebugClass::debugConsole(std::stringLiteral("skin but not lookAt, fixed by bottom: %1 (%2 at line: %3)").arg(SubChild.tagName()).arg(fileName).arg(SubChild.lineNumber()));
                                    }
                                    if(property_text.contains(Map_loader::text_file) && property_text.contains(Map_loader::text_id))
                                    {
                                        Map_to_send::Bot_Semi bot_semi;
                                        bot_semi.file=QFileInfo(QFileInfo(fileName).absolutePath()+Map_loader::text_slash+property_text.value(Map_loader::text_file).toString()).absoluteFilePath();
                                        bot_semi.id=property_text.value(Map_loader::text_id).toUInt(&ok);
                                        bot_semi.property_text=property_text;
                                        if(ok)
                                        {
                                            bot_semi.point.x=object_x;
                                            bot_semi.point.y=object_y;
                                            map_to_send_temp.bots << bot_semi;
                                        }
                                    }
                                    else
                                        DebugClass::debugConsole(std::stringLiteral("Missing \"bot\" properties for the bot: %1 (at line: %2), file: %3").arg(SubChild.tagName()).arg(SubChild.lineNumber()).arg(fileName));
                                }
                                else if(type==Map_loader::text_object)
                                {
                                    #ifdef DEBUG_MESSAGE_MAP
                                    DebugClass::debugConsole(std::stringLiteral("type: %1, object_x: %2, object_y: %3")
                                         .arg(type)
                                         .arg(object_x)
                                         .arg(object_y)
                                         );
                                    #endif
                                    if(property_text.contains(Map_loader::text_item))
                                    {
                                        Map_to_send::ItemOnMap_Semi item_semi;
                                        item_semi.infinite=false;
                                        if(property_text.contains(Map_loader::text_infinite) && property_text.value(Map_loader::text_infinite)==Map_loader::text_true)
                                            item_semi.infinite=true;
                                        item_semi.visible=true;
                                        if(property_text.contains(Map_loader::text_visible) && property_text.value(Map_loader::text_visible)==Map_loader::text_false)
                                            item_semi.visible=false;
                                        item_semi.item=property_text.value(Map_loader::text_item).toUInt(&ok);
                                        if(ok)
                                        {
                                            item_semi.point.x=object_x;
                                            item_semi.point.y=object_y;
                                            map_to_send_temp.items << item_semi;
                                        }
                                    }
                                    else
                                        DebugClass::debugConsole(std::stringLiteral("Missing \"bot\" properties for the bot: %1 (at line: %2), file: %3").arg(SubChild.tagName()).arg(SubChild.lineNumber()).arg(fileName));
                                }
                                else
                                {
                                    DebugClass::debugConsole(std::stringLiteral("unknow type: %1, object_x: %2, object_y: %3 (object), %4 (at line: %5), file: %6")
                                         .arg(type)
                                         .arg(object_x)
                                         .arg(object_y)
                                         .arg(SubChild.tagName())
                                         .arg(SubChild.lineNumber())
                                         .arg(fileName)
                                         );
                                }

                            }
                            else
                                DebugClass::debugConsole(std::stringLiteral("Missing attribute type missing: SubChild.tagName(): %1 (at line: %2), file: %3").arg(SubChild.tagName()).arg(SubChild.lineNumber()).arg(fileName));
                        }
                    }
                    else
                        DebugClass::debugConsole(std::stringLiteral("Is not Element: SubChild.tagName(): %1 (at line: %2)").arg(SubChild.tagName()).arg(SubChild.lineNumber()));
                    SubChild = SubChild.nextSiblingElement(Map_loader::text_object);
                }
            }
        }
        child = child.nextSiblingElement(Map_loader::text_objectgroup);
    }

    const uint32_t &rawSize=map_to_send_temp.width*map_to_send_temp.height*4;

    // layer
    child = root.firstChildElement(Map_loader::text_layer);
    while(!child.isNull())
    {
        if(!child.isElement())
        {
            error=std::stringLiteral("Is Element: child.tagName(): %1, file: %2").arg(child.tagName()).arg(fileName);
            return false;
        }
        else if(!child.hasAttribute(Map_loader::text_name))
        {
            error=std::stringLiteral("Has not attribute \"name\": child.tagName(): %1, file: %2").arg(child.tagName()).arg(fileName);
            return false;
        }
        else
        {
            const QDomElement &data = child.firstChildElement(Map_loader::text_data);
            const std::string &name=child.attribute(Map_loader::text_name);
            if(data.isNull())
            {
                error=std::stringLiteral("Is Element for layer is null: %1 and name: %2, file: %3").arg(data.tagName()).arg(name).arg(fileName);
                return false;
            }
            else if(!data.isElement())
            {
                error=std::stringLiteral("Is Element for layer child.tagName(): %1, file: %2").arg(data.tagName()).arg(fileName);
                return false;
            }
            else if(!data.hasAttribute(Map_loader::text_encoding))
            {
                error=std::stringLiteral("Has not attribute \"base64\": child.tagName(): %1, file: %2").arg(data.tagName()).arg(fileName);
                return false;
            }
            else if(!data.hasAttribute(Map_loader::text_compression))
            {
                error=std::stringLiteral("Has not attribute \"zlib\": child.tagName(): %1, file: %2").arg(data.tagName()).arg(fileName);
                return false;
            }
            else if(data.attribute(Map_loader::text_encoding)!=Map_loader::text_base64)
            {
                error=std::stringLiteral("only encoding base64 is supported, file: %1").arg(fileName);
                return false;
            }
            else if(!data.hasAttribute(Map_loader::text_compression))
            {
                error=std::stringLiteral("Only compression zlib is supported, file: %1").arg(fileName);
                return false;
            }
            else
            {
                std::string text=data.text();
                #if QT_VERSION < 0x040800
                    const std::string textData = std::string::fromRawData(text.unicode(), text.size());
                    const QByteArray latin1Text = textData.toLatin1();
                #else
                    const QByteArray latin1Text = text.toLatin1();
                #endif
                const QByteArray &data=decompress(QByteArray::fromBase64(latin1Text),map_to_send_temp.height*map_to_send_temp.width*4);
                if((uint32_t)data.size()!=map_to_send_temp.height*map_to_send_temp.width*4)
                {
                    error=std::stringLiteral("map binary size (%1) != %2x%3x4").arg(data.size()).arg(map_to_send_temp.height).arg(map_to_send_temp.width);
                    return false;
                }
                if(name==Map_loader::text_Walkable)
                {
                    if(Walkable.isEmpty())
                        Walkable=data;
                    else
                    {
                        const int &layersize=Walkable.size();
                        int index=0;
                        while(index<layersize)
                        {
                            Walkable[index]=Walkable.at(index) || data.at(index);
                            index++;
                        }
                    }
                }
                else if(name==Map_loader::text_Collisions)
                {
                    if(Collisions.isEmpty())
                        Collisions=data;
                    else
                    {
                        int index=0;
                        const int &layersize=Collisions.size();
                        while(index<layersize)
                        {
                            Collisions[index]=Collisions.at(index) || data.at(index);
                            index++;
                        }
                    }
                }
                else if(name==Map_loader::text_Dirt)
                {
                    if(Dirt.isEmpty())
                        Dirt=data;
                    else
                    {
                        int index=0;
                        const int &layersize=Dirt.size();
                        while(index<layersize)
                        {
                            Dirt[index]=Dirt.at(index) || data.at(index);
                            index++;
                        }
                    }
                }
                else if(name==Map_loader::text_LedgesRight)
                {
                    if(LedgesRight.isEmpty())
                        LedgesRight=data;
                    else
                    {
                        int index=0;
                        const int &layersize=LedgesRight.size();
                        while(index<layersize)
                        {
                            LedgesRight[index]=LedgesRight.at(index) || data.at(index);
                            index++;
                        }
                    }
                }
                else if(name==Map_loader::text_LedgesLeft)
                {
                    if(LedgesLeft.isEmpty())
                        LedgesLeft=data;
                    else
                    {
                        int index=0;
                        const int &layersize=LedgesLeft.size();
                        while(index<layersize)
                        {
                            LedgesLeft[index]=LedgesLeft.at(index) || data.at(index);
                            index++;
                        }
                    }
                }
                else if(name==Map_loader::text_LedgesBottom || name==Map_loader::text_LedgesDown)
                {
                    if(LedgesBottom.isEmpty())
                        LedgesBottom=data;
                    else
                    {
                        int index=0;
                        const int &layersize=LedgesBottom.size();
                        while(index<layersize)
                        {
                            LedgesBottom[index]=LedgesBottom.at(index) || data.at(index);
                            index++;
                        }
                    }
                }
                else if(name==Map_loader::text_LedgesTop || name==Map_loader::text_LedgesUp)
                {
                    if(LedgesTop.isEmpty())
                        LedgesTop=data;
                    else
                    {
                        int index=0;
                        const int &layersize=LedgesTop.size();
                        while(index<layersize)
                        {
                            LedgesTop[index]=LedgesTop.at(index) || data.at(index);
                            index++;
                        }
                    }
                }
                else
                {
                    if(!name.isEmpty() && rawSize==(uint32_t)data.size())
                    {
                        int index=0;
                        while(index<CommonDatapack::commonDatapack.monstersCollision.size())
                        {
                            if(CommonDatapack::commonDatapack.monstersCollision.at(index).layer==name)
                            {
                                mapLayerContentForMonsterCollision[name]=data.constData();
                                {
                                    const std::stringList &monsterTypeListText=CommonDatapack::commonDatapack.monstersCollision.at(index).monsterTypeList;
                                    int monsterTypeListIndex=0;
                                    while(monsterTypeListIndex<monsterTypeListText.size())
                                    {
                                        if(!detectedMonsterCollisionMonsterType.contains(monsterTypeListText.at(monsterTypeListIndex)))
                                            detectedMonsterCollisionMonsterType << monsterTypeListText.at(monsterTypeListIndex);
                                        monsterTypeListIndex++;
                                    }
                                }
                                if(!detectedMonsterCollisionLayer.contains(name))
                                    detectedMonsterCollisionLayer << name;
                            }
                            index++;
                        }
                        monsterList << data;
                    }
                }
            }
        }
        child = child.nextSiblingElement(Map_loader::text_layer);
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

        QMapIterator<std::string/*layer*/,const char *> i(mapLayerContentForMonsterCollision);
        while (i.hasNext()) {
            i.next();
            MonsterCollisionBin << i.value();
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
            int index=0;
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
                    map_to_send_temp.dirts << dirtOnMap_Semi;
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
                        DebugClass::debugConsole(std::stringLiteral("Multiple ledges at the same place, do colision for left"));
                        map_to_send_temp.parsed_layer.walkable[x+y*map_to_send_temp.width]=false;
                    }
                    else
                        map_to_send_temp.parsed_layer.ledges[x+y*map_to_send_temp.width]=(uint8_t)ParsedLayerLedges_LedgesLeft;
                }
                if(ledgesRight)
                {
                    if(ledgesLeft || ledgesBottom || ledgesTop)
                    {
                        DebugClass::debugConsole(std::stringLiteral("Multiple ledges at the same place, do colision for right"));
                        map_to_send_temp.parsed_layer.walkable[x+y*map_to_send_temp.width]=false;
                    }
                    else
                        map_to_send_temp.parsed_layer.ledges[x+y*map_to_send_temp.width]=(uint8_t)ParsedLayerLedges_LedgesRight;
                }
                if(ledgesTop)
                {
                    if(ledgesRight || ledgesBottom || ledgesLeft)
                    {
                        DebugClass::debugConsole(std::stringLiteral("Multiple ledges at the same place, do colision for top"));
                        map_to_send_temp.parsed_layer.walkable[x+y*map_to_send_temp.width]=false;
                    }
                    else
                        map_to_send_temp.parsed_layer.ledges[x+y*map_to_send_temp.width]=(uint8_t)ParsedLayerLedges_LedgesTop;
                }
                if(ledgesBottom)
                {
                    if(ledgesRight || ledgesLeft || ledgesTop)
                    {
                        DebugClass::debugConsole(std::stringLiteral("Multiple ledges at the same place, do colision for bottom"));
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
    xmlExtra.replace(Map_loader::text_dottmx,Map_loader::text_dotxml);
    if(QFile::exists(xmlExtra))
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
            QMapIterator<std::string/*layer*/,const char *> i(mapLayerContentForMonsterCollision);
            while (i.hasNext()) {
                i.next();
                if(zoneNumber.contains(i.key()))
                {
                    const uint8_t &zoneId=zoneNumber.value(i.key());
                    uint8_t x=0;
                    while(x<this->map_to_send.width)
                    {
                        uint8_t y=0;
                        while(y<this->map_to_send.height)
                        {
                            if(i.value()[x*4+y*map_to_send_temp.width*4+0]!=0x00 || i.value()[x*4+y*map_to_send_temp.width*4+1]!=0x00 || i.value()[x*4+y*map_to_send_temp.width*4+2]!=0x00 || i.value()[x*4+y*map_to_send_temp.width*4+3]!=0x00)
                            {
                                if(this->map_to_send.parsed_layer.monstersCollisionMap[x+y*this->map_to_send.width]==0)
                                    this->map_to_send.parsed_layer.monstersCollisionMap[x+y*this->map_to_send.width]=zoneId;
                                else if(this->map_to_send.parsed_layer.monstersCollisionMap[x+y*this->map_to_send.width]==zoneId)
                                {}//ignore, same zone
                                else
                                {
                                    DebugClass::debugConsole(std::stringLiteral("Have already monster at %1,%2 for %3, actual zone: %4 (%5), new zone: %6 (%7)")
                                             .arg(x).arg(y).arg(fileName)
                                             .arg(this->map_to_send.parsed_layer.monstersCollisionMap[x+y*this->map_to_send.width])
                                             .arg(CommonDatapack::commonDatapack.monstersCollision.at(this->map_to_send.parsed_layer.monstersCollisionMap[x+y*this->map_to_send.width]).layer)
                                             .arg(zoneId)
                                             .arg(CommonDatapack::commonDatapack.monstersCollision.at(zoneId).layer)
                                            );
                                    this->map_to_send.parsed_layer.monstersCollisionMap[x+y*this->map_to_send.width]=zoneId;//overwrited by above layer
                                }
                            }
                            y++;
                        }
                        x++;
                    }
                }
            }
        }

        {
            if(this->map_to_send.parsed_layer.monstersCollisionList.isEmpty())
            {
                delete this->map_to_send.parsed_layer.monstersCollisionMap;
                this->map_to_send.parsed_layer.monstersCollisionMap=NULL;
            }
            if(this->map_to_send.parsed_layer.monstersCollisionList.size()==1 && this->map_to_send.parsed_layer.monstersCollisionList.first().actionOn.isEmpty() && this->map_to_send.parsed_layer.monstersCollisionList.first().walkOn.isEmpty())
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
        std::stringList layers_name;
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
        QFile mapFile(fileName);
        if(!mapFile.open(QIODevice::ReadOnly))
        {
            qDebug() << mapFile.fileName()+QLatin1String(": ")+mapFile.errorString();
            return false;
        }
        const QByteArray &xmlContent=mapFile.readAll();
        mapFile.close();
        std::string errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            qDebug() << std::stringLiteral("%1, Parse error at line %2, column %3: %4").arg(mapFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr);
            return false;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
        CommonDatapack::commonDatapack.xmlLoadedFile[fileName]=domDocument;
    }
    #endif
    this->map_to_send.xmlRoot = domDocument.documentElement();
    if(this->map_to_send.xmlRoot.tagName()!=Map_loader::text_map)
    {
        qDebug() << std::stringLiteral("\"map\" root balise not found for the xml file");
        return false;
    }

    //found the cave name
    std::stringList caveName;
    {
        int index=0;
        while(index<CommonDatapack::commonDatapack.monstersCollision.size())
        {
            if(CommonDatapack::commonDatapack.monstersCollision.at(index).layer.isEmpty())
            {
                caveName=CommonDatapack::commonDatapack.monstersCollision.at(index).monsterTypeList;
                break;
            }
            index++;
        }
        if(caveName.isEmpty())
            detectedMonsterCollisionMonsterType << Map_loader::text_cave;
    }

    //load the found monster type
    std::unordered_map<std::string/*monsterType*/,std::vector<MapMonster> > monsterTypeList;
    {
        int index=0;
        while(index<detectedMonsterCollisionMonsterType.size())
        {
            if(!detectedMonsterCollisionMonsterType.at(index).isEmpty())
                if(!monsterTypeList.contains(detectedMonsterCollisionMonsterType.at(index)))
                    monsterTypeList[detectedMonsterCollisionMonsterType.at(index)]=loadSpecificMonster(fileName,detectedMonsterCollisionMonsterType.at(index));
            index++;
        }
    }

    this->map_to_send.parsed_layer.monstersCollisionList.clear();
    this->map_to_send.parsed_layer.monstersCollisionList << MonstersCollisionValue();//cave
    //found the zone number
    zoneNumber.clear();
    uint8_t zoneNumberIndex=1;
    {
        int index=0;
        while(index<CommonDatapack::commonDatapack.monstersCollision.size())
        {
            const MonstersCollision &monstersCollision=CommonDatapack::commonDatapack.monstersCollision.at(index);
            const std::stringList &searchList=monstersCollision.defautMonsterTypeList;
            int index_search=0;
            while(index_search<searchList.size())
            {
                if(monsterTypeList.contains(searchList.at(index_search)))
                    break;
                index_search++;
            }
            if(index_search<searchList.size())
            {
                uint8_t tempZoneNumberIndex=0;
                //cave
                if(CommonDatapack::commonDatapack.monstersCollision.at(index).layer.isEmpty())
                {}
                //not cave
                else if(detectedMonsterCollisionLayer.contains(CommonDatapack::commonDatapack.monstersCollision.at(index).layer))
                {
                    if(!zoneNumber.contains(CommonDatapack::commonDatapack.monstersCollision.at(index).layer))
                    {
                        zoneNumber[CommonDatapack::commonDatapack.monstersCollision.at(index).layer]=zoneNumberIndex;
                        this->map_to_send.parsed_layer.monstersCollisionList << MonstersCollisionValue();//create
                        tempZoneNumberIndex=zoneNumberIndex;
                        zoneNumberIndex++;
                    }
                    else
                        tempZoneNumberIndex=zoneNumber.value(CommonDatapack::commonDatapack.monstersCollision.at(index).layer);
                }
                {
                    MonstersCollisionValue *monstersCollisionValue=&this->map_to_send.parsed_layer.monstersCollisionList[tempZoneNumberIndex];
                    if(CommonDatapack::commonDatapack.monstersCollision.at(index).type==MonstersCollisionType_ActionOn)
                    {
                        monstersCollisionValue->actionOn << index;
                        monstersCollisionValue->actionOnMonsters << monsterTypeList.value(searchList.at(index_search));
                    }
                    else
                    {
                        monstersCollisionValue->walkOn << index;
                        MonstersCollisionValue::MonstersCollisionContent monstersCollisionContent;
                        monstersCollisionContent.defaultMonsters=monsterTypeList.value(searchList.at(index_search));
                        int event_index=0;
                        while(event_index<monstersCollision.events.size())
                        {
                            const MonstersCollision::MonstersCollisionEvent &monstersCollisionEvent=monstersCollision.events.at(event_index);
                            const std::stringList &searchList=monstersCollisionEvent.monsterTypeList;
                            int index_search=0;
                            while(index_search<searchList.size())
                            {
                                if(monsterTypeList.contains(searchList.at(index_search)))
                                    break;
                                index_search++;
                            }
                            if(index_search<searchList.size())
                            {
                                MonstersCollisionValue::MonstersCollisionValueOnCondition monstersCollisionValueOnCondition;
                                monstersCollisionValueOnCondition.event=monstersCollisionEvent.event;
                                monstersCollisionValueOnCondition.event_value=monstersCollisionEvent.event_value;
                                monstersCollisionValueOnCondition.monsters=monsterTypeList.value(searchList.at(index_search));
                                monstersCollisionContent.conditions << monstersCollisionValueOnCondition;
                            }
                            event_index++;
                        }
                        monstersCollisionValue->walkOnMonsters << monstersCollisionContent;
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
    const QDomElement &layer = map_to_send.xmlRoot.firstChildElement(monsterType);
    if(!layer.isNull())
    {
        if(layer.isElement())
        {
            QDomElement monsters=layer.firstChildElement(Map_loader::text_monster);
            while(!monsters.isNull())
            {
                if(monsters.isElement())
                {
                    if(monsters.hasAttribute(Map_loader::text_id) && ((monsters.hasAttribute(Map_loader::text_minLevel) && monsters.hasAttribute(Map_loader::text_maxLevel)) || monsters.hasAttribute(Map_loader::text_level)) && monsters.hasAttribute(Map_loader::text_luck))
                    {
                        MapMonster mapMonster;
                        mapMonster.id=monsters.attribute(Map_loader::text_id).toUInt(&ok);
                        if(!ok)
                            qDebug() << std::stringLiteral("id is not a number: child.tagName(): %1 (at line: %2), file: %3").arg(monsters.tagName()).arg(monsters.lineNumber()).arg(fileName);
                        if(ok)
                            if(!CommonDatapack::commonDatapack.monsters.contains(mapMonster.id))
                            {
                                qDebug() << std::stringLiteral("monster %4 not found into the monster list: %1 (at line: %2), file: %3").arg(monsters.tagName()).arg(monsters.lineNumber()).arg(fileName).arg(mapMonster.id);
                                ok=false;
                            }
                        if(monsters.hasAttribute(Map_loader::text_minLevel) && monsters.hasAttribute(Map_loader::text_maxLevel))
                        {
                            if(ok)
                            {
                                mapMonster.minLevel=monsters.attribute(Map_loader::text_minLevel).toUShort(&ok);
                                if(!ok)
                                    qDebug() << std::stringLiteral("minLevel is not a number: child.tagName(): %1 (at line: %2), file: %3").arg(monsters.tagName()).arg(monsters.lineNumber()).arg(fileName);
                            }
                            if(ok)
                            {
                                mapMonster.maxLevel=monsters.attribute(Map_loader::text_maxLevel).toUShort(&ok);
                                if(!ok)
                                    qDebug() << std::stringLiteral("maxLevel is not a number: child.tagName(): %1 (at line: %2), file: %3").arg(monsters.tagName()).arg(monsters.lineNumber()).arg(fileName);
                            }
                        }
                        else
                        {
                            if(ok)
                            {
                                mapMonster.maxLevel=monsters.attribute(Map_loader::text_level).toUShort(&ok);
                                mapMonster.minLevel=mapMonster.maxLevel;
                                if(!ok)
                                    qDebug() << std::stringLiteral("level is not a number: child.tagName(): %1 (at line: %2), file: %3").arg(monsters.tagName()).arg(monsters.lineNumber()).arg(fileName);
                            }
                        }
                        if(ok)
                        {
                            std::string textLuck=monsters.attribute(Map_loader::text_luck);
                            textLuck.remove(Map_loader::text_percent);
                            mapMonster.luck=textLuck.toUShort(&ok);
                            if(!ok)
                                qDebug() << std::stringLiteral("luck is not a number: child.tagName(): %1 (at line: %2), file: %3").arg(monsters.tagName()).arg(monsters.lineNumber()).arg(fileName);
                        }
                        if(ok)
                            if(mapMonster.minLevel>mapMonster.maxLevel)
                            {
                                qDebug() << std::stringLiteral("min > max for the level: child.tagName(): %1 (at line: %2), file: %3").arg(monsters.tagName()).arg(monsters.lineNumber()).arg(fileName);
                                ok=false;
                            }
                        if(ok)
                            if(mapMonster.luck<=0)
                            {
                                qDebug() << std::stringLiteral("luck is too low: child.tagName(): %1 (at line: %2), file: %3").arg(monsters.tagName()).arg(monsters.lineNumber()).arg(fileName);
                                ok=false;
                            }
                        if(ok)
                            if(mapMonster.minLevel<=0)
                            {
                                qDebug() << std::stringLiteral("min level is too low: child.tagName(): %1 (at line: %2), file: %3").arg(monsters.tagName()).arg(monsters.lineNumber()).arg(fileName);
                                ok=false;
                            }
                        if(ok)
                            if(mapMonster.maxLevel<=0)
                            {
                                qDebug() << std::stringLiteral("max level is too low: child.tagName(): %1 (at line: %2), file: %3").arg(monsters.tagName()).arg(monsters.lineNumber()).arg(fileName);
                                ok=false;
                            }
                        if(ok)
                            if(mapMonster.luck>100)
                            {
                                qDebug() << std::stringLiteral("luck is greater than 100: child.tagName(): %1 (at line: %2), file: %3").arg(monsters.tagName()).arg(monsters.lineNumber()).arg(fileName);
                                ok=false;
                            }
                        if(ok)
                            if(mapMonster.minLevel>CATCHCHALLENGER_MONSTER_LEVEL_MAX)
                            {
                                qDebug() << std::stringLiteral("min level is greater than %3: child.tagName(): %1 (at line: %2), file: %3").arg(monsters.tagName()).arg(monsters.lineNumber()).arg(CATCHCHALLENGER_MONSTER_LEVEL_MAX).arg(fileName);
                                ok=false;
                            }
                        if(ok)
                            if(mapMonster.maxLevel>CATCHCHALLENGER_MONSTER_LEVEL_MAX)
                            {
                                qDebug() << std::stringLiteral("max level is greater than %3: child.tagName(): %1 (at line: %2), file: %3").arg(monsters.tagName()).arg(monsters.lineNumber()).arg(CATCHCHALLENGER_MONSTER_LEVEL_MAX).arg(fileName);
                                ok=false;
                            }
                        if(ok)
                        {
                            tempLuckTotal+=mapMonster.luck;
                            monsterTypeList << mapMonster;
                        }
                    }
                    else
                        qDebug() << std::stringLiteral("Missing attribute: child.tagName(): %1 (at line: %2), file: %3").arg(monsters.tagName()).arg(monsters.lineNumber()).arg(fileName);
                }
                else
                    qDebug() << std::stringLiteral("Is not an element: child.tagName(): %1 (at line: %2), file: %3").arg(monsters.tagName()).arg(monsters.lineNumber()).arg(fileName);
                monsters = monsters.nextSiblingElement(Map_loader::text_monster);
            }
            if(monsterTypeList.isEmpty())
                qDebug() << "map have empty monster layer:" << fileName << "type:" << monsterType;
        }
        else
            qDebug() << std::stringLiteral("Is not an element: child.tagName(): %1 (at line: %2), file: %3").arg(layer.tagName()).arg(layer.lineNumber()).arg(fileName);
        if(tempLuckTotal!=100)
        {
            qDebug() << std::stringLiteral("total luck is not egal to 100 (%3) for grass into %4, monsters dropped: child.tagName(): %1 (at line: %2), file: %3").arg(layer.tagName()).arg(layer.lineNumber()).arg(tempLuckTotal).arg(fileName).arg(fileName);
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
    if(link.isEmpty())
        return link;
    const std::string &currentPath=QFileInfo(fileName).absolutePath();
    QFileInfo newmap(currentPath+QDir::separator()+link);
    std::string newPath=newmap.absoluteFilePath();
    if(datapackPath.isEmpty() || newPath.startsWith(datapackPath))
    {
        newPath.remove(0,datapackPath.size());
        #if defined (DEBUG_MESSAGE_MAP_BORDER) || defined (DEBUG_MESSAGE_MAP_TP)
        DebugClass::debugConsole(std::stringLiteral("map link resolved: %1 (%2)").arg(newPath).arg(link));
        #endif
        return newPath;
    }
    DebugClass::debugConsole(std::stringLiteral("map link not resolved: %1, full path: %2, newPath: %3, datapackPath: %4").arg(link).arg(currentPath+QDir::separator()+link).arg(newPath).arg(datapackPath));
    return link;
}

QDomElement Map_loader::getXmlCondition(const std::string &fileName,const std::string &conditionFile,const uint32_t &conditionId)
{
    #ifdef ONLYMAPRENDER
    return QDomElement();
    #endif
    if(teleportConditionsUnparsed.contains(conditionFile))
    {
        if(teleportConditionsUnparsed.value(conditionFile).contains(conditionId))
            return teleportConditionsUnparsed.value(conditionFile).value(conditionId);
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
        QFile mapFile(conditionFile);
        if(!mapFile.open(QIODevice::ReadOnly))
        {
            qDebug() << std::stringLiteral("Into the file %1, unable to open the condition file: ").arg(fileName)+mapFile.fileName()+QLatin1String(": ")+mapFile.errorString();
            return QDomElement();
        }
        const QByteArray &xmlContent=mapFile.readAll();
        mapFile.close();
        std::string errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            qDebug() << std::stringLiteral("%1, Parse error at line %2, column %3: %4").arg(mapFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr);
            return QDomElement();
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
        CommonDatapack::commonDatapack.xmlLoadedFile[conditionFile]=domDocument;
    }
    #endif
    const QDomElement &root = domDocument.documentElement();
    if(root.tagName()!=QLatin1String("conditions"))
    {
        qDebug() << std::stringLiteral("\"conditions\" root balise not found for the xml file %1").arg(conditionFile);
        return QDomElement();
    }

    QDomElement item = root.firstChildElement(Map_loader::text_condition);
    while(!item.isNull())
    {
        if(item.isElement())
        {
            if(!item.hasAttribute(Map_loader::text_id))
                qDebug() << std::stringLiteral("\"condition\" balise have not id attribute (%1 at %2)").arg(conditionFile).arg(item.lineNumber());
            else if(!item.hasAttribute(Map_loader::text_type))
                qDebug() << std::stringLiteral("\"condition\" balise have not type attribute (%1 at %2)").arg(conditionFile).arg(item.lineNumber());
            else
            {
                const uint32_t &id=item.attribute(Map_loader::text_id).toUInt(&ok);
                if(!ok)
                    qDebug() << std::stringLiteral("\"condition\" balise have id is not a number (%1 at %2)").arg(conditionFile).arg(item.lineNumber());
                else
                    teleportConditionsUnparsed[conditionFile][id]=item;
            }
        }
        item = item.nextSiblingElement(Map_loader::text_condition);
    }
    if(teleportConditionsUnparsed.contains(conditionFile))
        if(teleportConditionsUnparsed.value(conditionFile).contains(conditionId))
            return teleportConditionsUnparsed.value(conditionFile).value(conditionId);
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
    if(conditionContent.attribute(Map_loader::text_type)==Map_loader::text_quest)
    {
        if(!conditionContent.hasAttribute(Map_loader::text_quest))
            qDebug() << std::stringLiteral("\"condition\" balise have type=quest but quest attribute not found, item, clan or fightBot (%1 at %2)").arg(conditionFile).arg(conditionContent.lineNumber());
        else
        {
            const uint32_t &quest=conditionContent.attribute(Map_loader::text_quest).toUInt(&ok);
            if(!ok)
                qDebug() << std::stringLiteral("\"condition\" balise have type=quest but quest attribute is not a number, item, clan or fightBot (%1 at %2)").arg(conditionFile).arg(conditionContent.lineNumber());
            else if(!CommonDatapackServerSpec::commonDatapackServerSpec.quests.contains(quest))
                qDebug() << std::stringLiteral("\"condition\" balise have type=quest but quest id is not found, item, clan or fightBot (%1 at %2)").arg(conditionFile).arg(conditionContent.lineNumber());
            else
            {
                condition.type=MapConditionType_Quest;
                condition.value=quest;
            }
        }
    }
    else if(conditionContent.attribute(Map_loader::text_type)==Map_loader::text_item)
    {
        if(!conditionContent.hasAttribute(Map_loader::text_item))
            qDebug() << std::stringLiteral("\"condition\" balise have type=item but item attribute not found, item, clan or fightBot (%1 at %2)").arg(conditionFile).arg(conditionContent.lineNumber());
        else
        {
            const uint32_t &item=conditionContent.attribute(Map_loader::text_item).toUInt(&ok);
            if(!ok)
                qDebug() << std::stringLiteral("\"condition\" balise have type=item but item attribute is not a number, item, clan or fightBot (%1 at %2)").arg(conditionFile).arg(conditionContent.lineNumber());
            else if(!CommonDatapack::commonDatapack.items.item.contains(item))
                qDebug() << std::stringLiteral("\"condition\" balise have type=item but item id is not found, item, clan or fightBot (%1 at %2)").arg(conditionFile).arg(conditionContent.lineNumber());
            else
            {
                condition.type=MapConditionType_Item;
                condition.value=item;
            }
        }
    }
    else if(conditionContent.attribute(Map_loader::text_type)==Map_loader::text_fightBot)
    {
        if(!conditionContent.hasAttribute(Map_loader::text_fightBot))
            qDebug() << std::stringLiteral("\"condition\" balise have type=fightBot but fightBot attribute not found, item, clan or fightBot (%1 at %2)").arg(conditionFile).arg(conditionContent.lineNumber());
        else
        {
            const uint32_t &fightBot=conditionContent.attribute(Map_loader::text_fightBot).toUInt(&ok);
            if(!ok)
                qDebug() << std::stringLiteral("\"condition\" balise have type=fightBot but fightBot attribute is not a number, item, clan or fightBot (%1 at %2)").arg(conditionFile).arg(conditionContent.lineNumber());
            else if(!CommonDatapackServerSpec::commonDatapackServerSpec.botFights.contains(fightBot))
                qDebug() << std::stringLiteral("\"condition\" balise have type=fightBot but fightBot id is not found, item, clan or fightBot (%1 at %2)").arg(conditionFile).arg(conditionContent.lineNumber());
            else
            {
                condition.type=MapConditionType_FightBot;
                condition.value=fightBot;
            }
        }
    }
    else if(conditionContent.attribute(Map_loader::text_type)==Map_loader::text_clan)
        condition.type=MapConditionType_Clan;
    else
        qDebug() << std::stringLiteral("\"condition\" balise have type but value is not quest, item, clan or fightBot (%1 at %2)").arg(conditionFile).arg(conditionContent.lineNumber());
    return condition;
}
