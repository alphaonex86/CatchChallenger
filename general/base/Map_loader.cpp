#include "Map_loader.h"
#include "GeneralVariable.h"

#include "CommonDatapack.h"

#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QCoreApplication>
#include <QVariant>
#include <QMutex>
#include <QTime>
#include <QMutexLocker>

#include <zlib.h>

using namespace CatchChallenger;

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

bool Map_loader::tryLoadMap(const QString &fileName)
{
    error=QStringLiteral("");
    Map_to_send map_to_send;
    map_to_send.border.bottom.x_offset=0;
    map_to_send.border.top.x_offset=0;
    map_to_send.border.left.y_offset=0;
    map_to_send.border.right.y_offset=0;
    QByteArray xmlContent,Walkable,Collisions,Water,Grass,Dirt,LedgesRight,LedgesLeft,LedgesBottom,LedgesTop;
    bool ok;
    QDomDocument domDocument;

    //open and quick check the file
    if(CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.contains(fileName))
        domDocument=CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.value(fileName);
    else
    {
        QFile mapFile(fileName);
        if(!mapFile.open(QIODevice::ReadOnly))
        {
            error=mapFile.fileName()+QStringLiteral(": ")+mapFile.errorString();
            return false;
        }
        xmlContent=mapFile.readAll();
        mapFile.close();
        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            error=QStringLiteral("%1, Parse error at line %2, column %3: %4").arg(mapFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr);
            return false;
        }
        CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile[fileName]=domDocument;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!=QStringLiteral("map"))
    {
        error=QStringLiteral("\"map\" root balise not found for the xml file");
        return false;
    }

    //get the width
    if(!root.hasAttribute(QStringLiteral("width")))
    {
        error=QStringLiteral("the root node has not the attribute \"width\"");
        return false;
    }
    map_to_send.width=root.attribute(QStringLiteral("width")).toUInt(&ok);
    if(!ok)
    {
        error=QStringLiteral("the root node has wrong attribute \"width\"");
        return false;
    }

    //get the height
    if(!root.hasAttribute(QStringLiteral("height")))
    {
        error=QStringLiteral("the root node has not the attribute \"height\"");
        return false;
    }
    map_to_send.height=root.attribute(QStringLiteral("height")).toUInt(&ok);
    if(!ok)
    {
        error=QStringLiteral("the root node has wrong attribute \"height\"");
        return false;
    }

    //check the size
    if(map_to_send.width<1 || map_to_send.width>255)
    {
        error=QStringLiteral("the width should be greater or equal than 1, and lower or equal than 32000");
        return false;
    }
    if(map_to_send.height<1 || map_to_send.height>255)
    {
        error=QStringLiteral("the height should be greater or equal than 1, and lower or equal than 32000");
        return false;
    }

    //properties
    QDomElement child = root.firstChildElement(QStringLiteral("properties"));
    if(!child.isNull())
    {
        if(child.isElement())
        {
            QDomElement SubChild=child.firstChildElement(QStringLiteral("property"));
            while(!SubChild.isNull())
            {
                if(SubChild.isElement())
                {
                    if(SubChild.hasAttribute(QStringLiteral("name")) && SubChild.hasAttribute(QStringLiteral("value")))
                        map_to_send.property[SubChild.attribute(QStringLiteral("name"))]=SubChild.attribute(QStringLiteral("value"));
                    else
                    {
                        error=QStringLiteral("Missing attribute name or value: child.tagName(): %1 (at line: %2)").arg(SubChild.tagName()).arg(SubChild.lineNumber());
                        return false;
                    }
                }
                else
                {
                    error=QStringLiteral("Is not an element: child.tagName(): %1 (at line: %2)").arg(SubChild.tagName()).arg(SubChild.lineNumber());
                    return false;
                }
                SubChild = SubChild.nextSiblingElement(QStringLiteral("property"));
            }
        }
        else
        {
            error=QStringLiteral("Is not an element: child.tagName(): %1 (at line: %2)").arg(child.tagName()).arg(child.lineNumber());
            return false;
        }
    }

    // objectgroup
    child = root.firstChildElement(QStringLiteral("objectgroup"));
    while(!child.isNull())
    {
        if(!child.hasAttribute(QStringLiteral("name")))
            DebugClass::debugConsole(QStringLiteral("Has not attribute \"name\": child.tagName(): %1 (at line: %2)").arg(child.tagName()).arg(child.lineNumber()));
        else if(!child.isElement())
            DebugClass::debugConsole(QStringLiteral("Is not an element: child.tagName(): %1, name: %2 (at line: %3)").arg(child.tagName().arg(child.attribute(QStringLiteral("name"))).arg(child.lineNumber())));
        else
        {
            if(child.attribute(QStringLiteral("name"))==QStringLiteral("Moving"))
            {
                QDomElement SubChild=child.firstChildElement(QStringLiteral("object"));
                while(!SubChild.isNull())
                {
                    if(SubChild.isElement() && SubChild.hasAttribute(QStringLiteral("x")) && SubChild.hasAttribute(QStringLiteral("y")))
                    {
                        quint32 object_x=SubChild.attribute(QStringLiteral("x")).toUInt(&ok)/16;
                        if(!ok)
                            DebugClass::debugConsole(QStringLiteral("Wrong conversion with x: %1 (at line: %2), file: %3").arg(SubChild.tagName()).arg(SubChild.lineNumber()).arg(fileName));
                        else
                        {
                            /** the -1 is important to fix object layer bug into tiled!!!
                             * Don't remove! */
                            quint32 object_y=(SubChild.attribute(QStringLiteral("y")).toUInt(&ok)/16)-1;

                            if(!ok)
                                DebugClass::debugConsole(QStringLiteral("Wrong conversion with y: %1 (at line: %2), file: %3").arg(SubChild.tagName()).arg(SubChild.lineNumber()).arg(fileName));
                            else if(object_x>map_to_send.width || object_y>map_to_send.height)
                                DebugClass::debugConsole(QStringLiteral("Object out of the map: %1 (at line: %2), file: %3").arg(SubChild.tagName()).arg(SubChild.lineNumber()).arg(fileName));
                            else if(SubChild.hasAttribute(QStringLiteral("type")))
                            {
                                QString type=SubChild.attribute(QStringLiteral("type"));

                                QHash<QString,QVariant> property_text;
                                QDomElement prop=SubChild.firstChildElement(QStringLiteral("properties"));
                                if(!prop.isNull())
                                {
                                    #ifdef DEBUG_MESSAGE_MAP
                                    DebugClass::debugConsole(QStringLiteral("type: %1, object_x: %2, object_y: %3, prop.isNull()")
                                                 .arg(type)
                                                 .arg(object_x)
                                                 .arg(object_y)
                                                 );
                                    #endif
                                    QDomElement property=prop.firstChildElement(QStringLiteral("property"));
                                    while(!property.isNull())
                                    {
                                        if(property.hasAttribute(QStringLiteral("name")) && property.hasAttribute(QStringLiteral("value")))
                                            property_text[property.attribute(QStringLiteral("name"))]=property.attribute(QStringLiteral("value"));
                                        property = property.nextSiblingElement(QStringLiteral("property"));
                                    }
                                }
                                if(type==QStringLiteral("border-left") || type==QStringLiteral("border-right") || type==QStringLiteral("border-top") || type==QStringLiteral("border-bottom"))
                                {
                                    #ifdef DEBUG_MESSAGE_MAP
                                    DebugClass::debugConsole(QStringLiteral("type: %1, object_x: %2, object_y: %3, border")
                                         .arg(type)
                                         .arg(object_x)
                                         .arg(object_y)
                                         );
                                    #endif
                                    if(property_text.contains(QStringLiteral("map")))
                                    {
                                        if(type==QStringLiteral("border-left"))//border left
                                        {
                                            map_to_send.border.left.fileName=property_text.value(QStringLiteral("map")).toString();
                                            if(!map_to_send.border.left.fileName.endsWith(QStringLiteral(".tmx")) && !map_to_send.border.left.fileName.isEmpty())
                                                map_to_send.border.left.fileName+=QStringLiteral(".tmx");
                                            map_to_send.border.left.y_offset=object_y;
                                            #ifdef DEBUG_MESSAGE_MAP_BORDER
                                            DebugClass::debugConsole(QStringLiteral("map_to_send.border.left.fileName: %1, offset: %2").arg(map_to_send.border.left.fileName).arg(map_to_send.border.left.y_offset));
                                            #endif
                                        }
                                        else if(type==QStringLiteral("border-right"))//border right
                                        {
                                            map_to_send.border.right.fileName=property_text.value(QStringLiteral("map")).toString();
                                            if(!map_to_send.border.right.fileName.endsWith(QStringLiteral(".tmx")) && !map_to_send.border.right.fileName.isEmpty())
                                                map_to_send.border.right.fileName+=QStringLiteral(".tmx");
                                            map_to_send.border.right.y_offset=object_y;
                                            #ifdef DEBUG_MESSAGE_MAP_BORDER
                                            DebugClass::debugConsole(QStringLiteral("map_to_send.border.right.fileName: %1, offset: %2").arg(map_to_send.border.right.fileName).arg(map_to_send.border.right.y_offset));
                                            #endif
                                        }
                                        else if(type==QStringLiteral("border-top"))//border top
                                        {
                                            map_to_send.border.top.fileName=property_text.value(QStringLiteral("map")).toString();
                                            if(!map_to_send.border.top.fileName.endsWith(QStringLiteral(".tmx")) && !map_to_send.border.top.fileName.isEmpty())
                                                map_to_send.border.top.fileName+=QStringLiteral(".tmx");
                                            map_to_send.border.top.x_offset=object_x;
                                            #ifdef DEBUG_MESSAGE_MAP_BORDER
                                            DebugClass::debugConsole(QStringLiteral("map_to_send.border.top.fileName: %1, offset: %2").arg(map_to_send.border.top.fileName).arg(map_to_send.border.top.x_offset));
                                            #endif
                                        }
                                        else if(type==QStringLiteral("border-bottom"))//border bottom
                                        {
                                            map_to_send.border.bottom.fileName=property_text.value(QStringLiteral("map")).toString();
                                            if(!map_to_send.border.bottom.fileName.endsWith(QStringLiteral(".tmx")) && !map_to_send.border.bottom.fileName.isEmpty())
                                                map_to_send.border.bottom.fileName+=QStringLiteral(".tmx");
                                            map_to_send.border.bottom.x_offset=object_x;
                                            #ifdef DEBUG_MESSAGE_MAP_BORDER
                                            DebugClass::debugConsole(QStringLiteral("map_to_send.border.bottom.fileName: %1, offset: %2").arg(map_to_send.border.bottom.fileName).arg(map_to_send.border.bottom.x_offset));
                                            #endif
                                        }
                                        else
                                            DebugClass::debugConsole(QStringLiteral("Not at middle of border: child.tagName(): %1, object_x: %2, object_y: %3")
                                                 .arg(SubChild.tagName())
                                                 .arg(object_x)
                                                 .arg(object_y)
                                                 );
                                    }
                                    else
                                        DebugClass::debugConsole(QStringLiteral("Missing \"map\" properties for the border: %1 (at line: %2)").arg(SubChild.tagName()).arg(SubChild.lineNumber()));
                                }
                                else if(type==QStringLiteral("teleport on push") || type==QStringLiteral("teleport on it") || type==QStringLiteral("door"))
                                {
                                    #ifdef DEBUG_MESSAGE_MAP
                                    DebugClass::debugConsole(QStringLiteral("type: %1, object_x: %2, object_y: %3, tp")
                                         .arg(type)
                                         .arg(object_x)
                                         .arg(object_y)
                                         );
                                    #endif
                                    if(property_text.contains(QStringLiteral("map")) && property_text.contains(QStringLiteral("x")) && property_text.contains(QStringLiteral("y")))
                                    {
                                        Map_semi_teleport new_tp;
                                        new_tp.destination_x = property_text.value(QStringLiteral("x")).toUInt(&ok);
                                        if(ok)
                                        {
                                            new_tp.destination_y = property_text.value(QStringLiteral("y")).toUInt(&ok);
                                            if(ok)
                                            {
                                                if(property_text.contains(QStringLiteral("condition-file")) && property_text.contains(QStringLiteral("condition-id")))
                                                {
                                                    quint32 conditionId=property_text.value(QStringLiteral("condition-id")).toUInt(&ok);
                                                    if(!ok)
                                                        DebugClass::debugConsole(QStringLiteral("condition id is not a number, id: %1 (%2 at line: %3)").arg(property_text.value(QStringLiteral("condition-id")).toString()).arg(fileName).arg(SubChild.lineNumber()));
                                                    else
                                                    {
                                                        const QString &conditionFile=QFileInfo(QFileInfo(fileName).absolutePath()+QStringLiteral("/")+property_text.value(QStringLiteral("condition-file")).toString()).absoluteFilePath();
                                                        new_tp.conditionUnparsed=getXmlCondition(fileName,conditionFile,conditionId);
                                                        new_tp.condition=xmlConditionToMapCondition(conditionFile,new_tp.conditionUnparsed);
                                                    }
                                                }
                                                new_tp.source_x=object_x;
                                                new_tp.source_y=object_y;
                                                new_tp.map=property_text.value(QStringLiteral("map")).toString();
                                                if(!new_tp.map.endsWith(QStringLiteral(".tmx")) && !new_tp.map.isEmpty())
                                                    new_tp.map+=QStringLiteral(".tmx");
                                                map_to_send.teleport << new_tp;
                                                #ifdef DEBUG_MESSAGE_MAP_OBJECT
                                                DebugClass::debugConsole(QStringLiteral("Teleporter type: %1, to: %2 (%3,%4)").arg(type).arg(new_tp.map).arg(new_tp.source_x).arg(new_tp.source_y));
                                                #endif
                                            }
                                            else
                                                DebugClass::debugConsole(QStringLiteral("Bad convertion to int for y, type: %1, value: %2 (%3 at line: %4)").arg(type).arg(property_text.value(QStringLiteral("y")).toString()).arg(fileName).arg(SubChild.lineNumber()));
                                        }
                                        else
                                            DebugClass::debugConsole(QStringLiteral("Bad convertion to int for x, type: %1, value: %2 (%3 at line: %4)").arg(type).arg(property_text.value(QStringLiteral("x")).toString()).arg(fileName).arg(SubChild.lineNumber()));
                                    }
                                    else
                                        DebugClass::debugConsole(QStringLiteral("Missing map,x or y, type: %1 (at line: %2)").arg(type).arg(SubChild.lineNumber()));
                                }
                                else if(type==QStringLiteral("rescue"))
                                {
                                    #ifdef DEBUG_MESSAGE_MAP
                                    DebugClass::debugConsole(QStringLiteral("type: %1, object_x: %2, object_y: %3, rescue")
                                         .arg(type)
                                         .arg(object_x)
                                         .arg(object_y)
                                         );
                                    #endif
                                    Map_to_send::Map_Point tempPoint;
                                    tempPoint.x=object_x;
                                    tempPoint.y=object_y;
                                    map_to_send.rescue_points << tempPoint;
                                    map_to_send.bot_spawn_points << tempPoint;
                                }
                                else if(type==QStringLiteral("bot spawn"))
                                {
                                    #ifdef DEBUG_MESSAGE_MAP
                                    DebugClass::debugConsole(QStringLiteral("type: %1, object_x: %2, object_y: %3, bot spawn")
                                         .arg(type)
                                         .arg(object_x)
                                         .arg(object_y)
                                         );
                                    #endif
                                    Map_to_send::Map_Point tempPoint;
                                    tempPoint.x=object_x;
                                    tempPoint.y=object_y;
                                    map_to_send.bot_spawn_points << tempPoint;
                                }
                                else
                                {
                                    DebugClass::debugConsole(QStringLiteral("unknow type: %1, object_x: %2, object_y: %3 (moving), %4 (line: %5)")
                                         .arg(type)
                                         .arg(object_x)
                                         .arg(object_y)
                                         .arg(SubChild.tagName())
                                         .arg(SubChild.lineNumber())
                                         );
                                }

                            }
                            else
                                DebugClass::debugConsole(QStringLiteral("Missing attribute type missing: SubChild.tagName(): %1 (at line: %2)").arg(SubChild.tagName()).arg(SubChild.lineNumber()));
                        }
                    }
                    else
                        DebugClass::debugConsole(QStringLiteral("Is not Element: SubChild.tagName(): %1 (at line: %2)").arg(SubChild.tagName()).arg(SubChild.lineNumber()));
                    SubChild = SubChild.nextSiblingElement(QStringLiteral("object"));
                }
            }
            if(child.attribute(QStringLiteral("name"))==QStringLiteral("Object"))
            {
                QDomElement SubChild=child.firstChildElement(QStringLiteral("object"));
                while(!SubChild.isNull())
                {
                    if(SubChild.isElement() && SubChild.hasAttribute(QStringLiteral("x")) && SubChild.hasAttribute(QStringLiteral("y")))
                    {
                        quint32 object_x=SubChild.attribute(QStringLiteral("x")).toUInt(&ok)/16;
                        if(!ok)
                            DebugClass::debugConsole(QStringLiteral("Wrong conversion with x: %1 (at line: %2)").arg(SubChild.tagName()).arg(SubChild.lineNumber()));
                        else
                        {
                            /** the -1 is important to fix object layer bug into tiled!!!
                             * Don't remove! */
                            quint32 object_y=(SubChild.attribute(QStringLiteral("y")).toUInt(&ok)/16)-1;

                            if(!ok)
                                DebugClass::debugConsole(QStringLiteral("Wrong conversion with y: %1 (at line: %2)").arg(SubChild.tagName()).arg(SubChild.lineNumber()));
                            else if(object_x>map_to_send.width || object_y>map_to_send.height)
                                DebugClass::debugConsole(QStringLiteral("Object out of the map: %1 (at line: %2)").arg(SubChild.tagName()).arg(SubChild.lineNumber()));
                            else if(SubChild.hasAttribute(QStringLiteral("type")))
                            {
                                QString type=SubChild.attribute(QStringLiteral("type"));

                                QHash<QString,QVariant> property_text;
                                QDomElement prop=SubChild.firstChildElement(QStringLiteral("properties"));
                                if(!prop.isNull())
                                {
                                    #ifdef DEBUG_MESSAGE_MAP
                                    DebugClass::debugConsole(QStringLiteral("type: %1, object_x: %2, object_y: %3, prop.isNull()")
                                                 .arg(type)
                                                 .arg(object_x)
                                                 .arg(object_y)
                                                 );
                                    #endif
                                    QDomElement property=prop.firstChildElement(QStringLiteral("property"));
                                    while(!property.isNull())
                                    {
                                        if(property.hasAttribute(QStringLiteral("name")) && property.hasAttribute(QStringLiteral("value")))
                                            property_text[property.attribute(QStringLiteral("name"))]=property.attribute(QStringLiteral("value"));
                                        property = property.nextSiblingElement(QStringLiteral("property"));
                                    }
                                }
                                if(type==QStringLiteral("bot"))
                                {
                                    #ifdef DEBUG_MESSAGE_MAP
                                    DebugClass::debugConsole(QStringLiteral("type: %1, object_x: %2, object_y: %3")
                                         .arg(type)
                                         .arg(object_x)
                                         .arg(object_y)
                                         );
                                    #endif
                                    if(property_text.contains(QStringLiteral("skin")) && !property_text.value(QStringLiteral("skin")).toString().isEmpty() && !property_text.contains(QStringLiteral("lookAt")))
                                    {
                                        property_text[QStringLiteral("lookAt")]=QStringLiteral("bottom");
                                        DebugClass::debugConsole(QStringLiteral("skin but not lookAt, fixed by bottom: %1 (%2 at line: %3)").arg(SubChild.tagName()).arg(fileName).arg(SubChild.lineNumber()));
                                    }
                                    if(property_text.contains(QStringLiteral("file")) && property_text.contains(QStringLiteral("id")))
                                    {
                                        Map_to_send::Bot_Semi bot_semi;
                                        bot_semi.file=QFileInfo(QFileInfo(fileName).absolutePath()+QStringLiteral("/")+property_text.value(QStringLiteral("file")).toString()).absoluteFilePath();
                                        bot_semi.id=property_text.value(QStringLiteral("id")).toUInt(&ok);
                                        bot_semi.property_text=property_text;
                                        if(ok)
                                        {
                                            bot_semi.point.x=object_x;
                                            bot_semi.point.y=object_y;
                                            map_to_send.bots << bot_semi;
                                        }
                                    }
                                    else
                                        DebugClass::debugConsole(QStringLiteral("Missing \"bot\" properties for the bot: %1 (at line: %2)").arg(SubChild.tagName()).arg(SubChild.lineNumber()));
                                }
                                else
                                {
                                    DebugClass::debugConsole(QStringLiteral("unknow type: %1, object_x: %2, object_y: %3 (object), %4 (at line: %5)")
                                         .arg(type)
                                         .arg(object_x)
                                         .arg(object_y)
                                         .arg(SubChild.tagName())
                                         .arg(SubChild.lineNumber())
                                         );
                                }

                            }
                            else
                                DebugClass::debugConsole(QStringLiteral("Missing attribute type missing: SubChild.tagName(): %1 (at line: %2)").arg(SubChild.tagName()).arg(SubChild.lineNumber()));
                        }
                    }
                    else
                        DebugClass::debugConsole(QStringLiteral("Is not Element: SubChild.tagName(): %1 (at line: %2)").arg(SubChild.tagName()).arg(SubChild.lineNumber()));
                    SubChild = SubChild.nextSiblingElement(QStringLiteral("object"));
                }
            }
        }
        child = child.nextSiblingElement(QStringLiteral("objectgroup"));
    }



    // layer
    child = root.firstChildElement(QStringLiteral("layer"));
    while(!child.isNull())
    {
        if(!child.isElement())
        {
            error=QStringLiteral("Is Element: child.tagName(): %1").arg(child.tagName());
            return false;
        }
        else if(!child.hasAttribute(QStringLiteral("name")))
        {
            error=QStringLiteral("Has not attribute \"name\": child.tagName(): %1").arg(child.tagName());
            return false;
        }
        else
        {
            QDomElement data = child.firstChildElement(QStringLiteral("data"));
            QString name=child.attribute(QStringLiteral("name"));
            if(data.isNull())
            {
                error=QStringLiteral("Is Element for layer is null: %1 and name: %2").arg(data.tagName()).arg(name);
                return false;
            }
            else if(!data.isElement())
            {
                error=QStringLiteral("Is Element for layer child.tagName(): %1").arg(data.tagName());
                return false;
            }
            else if(!data.hasAttribute(QStringLiteral("encoding")))
            {
                error=QStringLiteral("Has not attribute \"base64\": child.tagName(): %1").arg(data.tagName());
                return false;
            }
            else if(!data.hasAttribute(QStringLiteral("compression")))
            {
                error=QStringLiteral("Has not attribute \"zlib\": child.tagName(): %1").arg(data.tagName());
                return false;
            }
            else if(data.attribute(QStringLiteral("encoding"))!=QStringLiteral("base64"))
            {
                error=QStringLiteral("only encoding base64 is supported");
                return false;
            }
            else if(!data.hasAttribute(QStringLiteral("compression")))
            {
                error=QStringLiteral("Only compression zlib is supported");
                return false;
            }
            else
            {
                QString text=data.text();
                #if QT_VERSION < 0x040800
                    const QString textData = QString::fromRawData(text.unicode(), text.size());
                    const QByteArray latin1Text = textData.toLatin1();
                #else
                    const QByteArray latin1Text = text.toLatin1();
                #endif
                const QByteArray &data=decompress(QByteArray::fromBase64(latin1Text),map_to_send.height*map_to_send.width*4);
                if((quint32)data.size()!=map_to_send.height*map_to_send.width*4)
                {
                    error=QStringLiteral("map binary size (%1) != %2x%3x4").arg(data.size()).arg(map_to_send.height).arg(map_to_send.width);
                    return false;
                }
                if(name==QStringLiteral("Walkable"))
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
                else if(name==QStringLiteral("Collisions"))
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
                else if(name==QStringLiteral("Water"))
                {
                    if(Water.isEmpty())
                        Water=data;
                    else
                    {
                        int index=0;
                        const int &layersize=Water.size();
                        while(index<layersize)
                        {
                            Water[index]=Water.at(index) || data.at(index);
                            index++;
                        }
                    }
                }
                else if(name==QStringLiteral("Grass"))
                {
                    if(Grass.isEmpty())
                        Grass=data;
                    else
                    {
                        int index=0;
                        const int &layersize=Grass.size();
                        while(index<layersize)
                        {
                            Grass[index]=Grass.at(index) || data.at(index);
                            index++;
                        }
                    }
                }
                else if(name==QStringLiteral("Dirt"))
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
                else if(name==QStringLiteral("LedgesRight"))
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
                else if(name==QStringLiteral("LedgesLeft"))
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
                else if(name==QStringLiteral("LedgesBottom") || name==QStringLiteral("LedgesDown"))
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
                else if(name==QStringLiteral("LedgesTop") || name==QStringLiteral("LedgesUp"))
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
            }
        }
        child = child.nextSiblingElement(QStringLiteral("layer"));
    }

    /*QByteArray null_data;
    null_data.resize(4);
    null_data[0]=0x00;
    null_data[1]=0x00;
    null_data[2]=0x00;
    null_data[3]=0x00;*/

    if(Walkable.size()>0)
        map_to_send.parsed_layer.walkable	= new bool[map_to_send.width*map_to_send.height];
    else
        map_to_send.parsed_layer.walkable	= NULL;
    if(Water.size()>0)
        map_to_send.parsed_layer.water		= new bool[map_to_send.width*map_to_send.height];
    else
        map_to_send.parsed_layer.water		= NULL;
    if(Grass.size()>0)
        map_to_send.parsed_layer.grass		= new bool[map_to_send.width*map_to_send.height];
    else
        map_to_send.parsed_layer.grass		= NULL;
    if(Dirt.size()>0)
        map_to_send.parsed_layer.dirt		= new bool[map_to_send.width*map_to_send.height];
    else
        map_to_send.parsed_layer.dirt		= NULL;
    if(LedgesRight.size()>0 || LedgesLeft.size()>0 || LedgesBottom.size()>0 || LedgesTop.size()>0)
        map_to_send.parsed_layer.ledges		= new quint8[map_to_send.width*map_to_send.height];
    else
        map_to_send.parsed_layer.ledges		= NULL;

    quint32 x=0;
    quint32 y=0;

    char * WalkableBin=NULL;
    char * WaterBin=NULL;
    char * CollisionsBin=NULL;
    char * GrassBin=NULL;
    char * DirtBin=NULL;
    char * LedgesRightBin=NULL;
    char * LedgesLeftBin=NULL;
    char * LedgesBottomBin=NULL;
    char * LedgesTopBin=NULL;
    {
        const quint32 rawSize=map_to_send.width*map_to_send.height*4;

        if(rawSize==(quint32)Walkable.size())
            WalkableBin=Walkable.data();
        if(rawSize==(quint32)Water.size())
            WaterBin=Water.data();
        if(rawSize==(quint32)Collisions.size())
            CollisionsBin=Collisions.data();
        if(rawSize==(quint32)Grass.size())
            GrassBin=Grass.data();
        if(rawSize==(quint32)Dirt.size())
            DirtBin=Dirt.data();
        if(rawSize==(quint32)LedgesRight.size())
            LedgesRightBin=LedgesRight.data();
        if(rawSize==(quint32)LedgesLeft.size())
            LedgesLeftBin=LedgesLeft.data();
        if(rawSize==(quint32)LedgesBottom.size())
            LedgesBottomBin=LedgesBottom.data();
        if(rawSize==(quint32)LedgesTop.size())
            LedgesTopBin=LedgesTop.data();
    }

    bool walkable=false,water=false,collisions=false,grass=false,dirt=false,ledgesRight=false,ledgesLeft=false,ledgesBottom=false,ledgesTop=false;
    while(x<map_to_send.width)
    {
        y=0;
        while(y<map_to_send.height)
        {
            if(WalkableBin!=NULL)
                walkable=WalkableBin[x*4+y*map_to_send.width*4+0]!=0x00 || WalkableBin[x*4+y*map_to_send.width*4+1]!=0x00 || WalkableBin[x*4+y*map_to_send.width*4+2]!=0x00 || WalkableBin[x*4+y*map_to_send.width*4+3]!=0x00;
            else
                walkable=false;
            if(WaterBin!=NULL)
                water=WaterBin[x*4+y*map_to_send.width*4+0]!=0x00 || WaterBin[x*4+y*map_to_send.width*4+1]!=0x00 || WaterBin[x*4+y*map_to_send.width*4+2]!=0x00 || WaterBin[x*4+y*map_to_send.width*4+3]!=0x00;
            else
                water=false;
            if(CollisionsBin!=NULL)
                collisions=CollisionsBin[x*4+y*map_to_send.width*4+0]!=0x00 || CollisionsBin[x*4+y*map_to_send.width*4+1]!=0x00 || CollisionsBin[x*4+y*map_to_send.width*4+2]!=0x00 || CollisionsBin[x*4+y*map_to_send.width*4+3]!=0x00;
            else
                collisions=false;
            if(GrassBin!=NULL)
                grass=GrassBin[x*4+y*map_to_send.width*4+0]!=0x00 || GrassBin[x*4+y*map_to_send.width*4+1]!=0x00 || GrassBin[x*4+y*map_to_send.width*4+2]!=0x00 || GrassBin[x*4+y*map_to_send.width*4+3]!=0x00;
            else
                grass=false;
            if(DirtBin!=NULL)
                dirt=DirtBin[x*4+y*map_to_send.width*4+0]!=0x00 || DirtBin[x*4+y*map_to_send.width*4+1]!=0x00 || DirtBin[x*4+y*map_to_send.width*4+2]!=0x00 || DirtBin[x*4+y*map_to_send.width*4+3]!=0x00;
            else
                dirt=false;
            if(LedgesRightBin!=NULL)
                ledgesRight=LedgesRightBin[x*4+y*map_to_send.width*4+0]!=0x00 || LedgesRightBin[x*4+y*map_to_send.width*4+1]!=0x00 || LedgesRightBin[x*4+y*map_to_send.width*4+2]!=0x00 || LedgesRightBin[x*4+y*map_to_send.width*4+3]!=0x00;
            else
                ledgesRight=false;
            if(LedgesLeftBin!=NULL)
                ledgesLeft=LedgesLeftBin[x*4+y*map_to_send.width*4+0]!=0x00 || LedgesLeftBin[x*4+y*map_to_send.width*4+1]!=0x00 || LedgesLeftBin[x*4+y*map_to_send.width*4+2]!=0x00 || LedgesLeftBin[x*4+y*map_to_send.width*4+3]!=0x00;
            else
                ledgesLeft=false;
            if(LedgesBottomBin!=NULL)
                ledgesBottom=LedgesBottomBin[x*4+y*map_to_send.width*4+0]!=0x00 || LedgesBottomBin[x*4+y*map_to_send.width*4+1]!=0x00 || LedgesBottomBin[x*4+y*map_to_send.width*4+2]!=0x00 || LedgesBottomBin[x*4+y*map_to_send.width*4+3]!=0x00;
            else
                ledgesBottom=false;
            if(LedgesTopBin!=NULL)
                ledgesTop=LedgesTopBin[x*4+y*map_to_send.width*4+0]!=0x00 || LedgesTopBin[x*4+y*map_to_send.width*4+1]!=0x00 || LedgesTopBin[x*4+y*map_to_send.width*4+2]!=0x00 || LedgesTopBin[x*4+y*map_to_send.width*4+3]!=0x00;
            else
                ledgesTop=false;

            /* drop mid() due to lack of performance
            if(x*4+y*map_to_send.width*4<(quint32)Walkable.size())
                walkable=Walkable.mid(x*4+y*map_to_send.width*4,4)!=null_data;
            else//if layer not found
                walkable=false;
            if(x*4+y*map_to_send.width*4<(quint32)Water.size())
                water=Water.mid(x*4+y*map_to_send.width*4,4)!=null_data;
            else//if layer not found
                water=false;
            if(x*4+y*map_to_send.width*4<(quint32)Collisions.size())
                collisions=Collisions.mid(x*4+y*map_to_send.width*4,4)!=null_data;
            else//if layer not found
                collisions=false;
            if(x*4+y*map_to_send.width*4<(quint32)Grass.size())
                grass=Grass.mid(x*4+y*map_to_send.width*4,4)!=null_data;
            else//if layer not found
                grass=false;
            if(x*4+y*map_to_send.width*4<(quint32)Dirt.size())
                dirt=Dirt.mid(x*4+y*map_to_send.width*4,4)!=null_data;
            else//if layer not found
                dirt=false;

            if(x*4+y*map_to_send.width*4<(quint32)LedgesRight.size())
                ledgesRight=LedgesRight.mid(x*4+y*map_to_send.width*4,4)!=null_data;
            else//if layer not found
                ledgesRight=false;
            if(x*4+y*map_to_send.width*4<(quint32)LedgesLeft.size())
                ledgesLeft=LedgesLeft.mid(x*4+y*map_to_send.width*4,4)!=null_data;
            else//if layer not found
                ledgesLeft=false;
            if(x*4+y*map_to_send.width*4<(quint32)LedgesBottom.size())
                ledgesBottom=LedgesBottom.mid(x*4+y*map_to_send.width*4,4)!=null_data;
            else//if layer not found
                ledgesBottom=false;
            if(x*4+y*map_to_send.width*4<(quint32)LedgesTop.size())
                ledgesTop=LedgesTop.mid(x*4+y*map_to_send.width*4,4)!=null_data;
            else//if layer not found
                ledgesTop=false;*/


            if(Walkable.size()>0)
                map_to_send.parsed_layer.walkable[x+y*map_to_send.width]=walkable && !water && !collisions && !dirt;
            if(Water.size()>0)
                map_to_send.parsed_layer.water[x+y*map_to_send.width]=water && !collisions;
            if(Grass.size()>0)
                map_to_send.parsed_layer.grass[x+y*map_to_send.width]=walkable && grass && !water && !collisions;
            if(Dirt.size()>0)
                map_to_send.parsed_layer.dirt[x+y*map_to_send.width]=dirt && !water;
            if(LedgesRight.size()>0 || LedgesLeft.size()>0 || LedgesBottom.size()>0 || LedgesTop.size()>0)
            {
                map_to_send.parsed_layer.ledges[x+y*map_to_send.width]=(quint8)ParsedLayerLedges_NoLedges;
                if(ledgesLeft)
                {
                    if(ledgesRight || ledgesBottom || ledgesTop)
                    {
                        DebugClass::debugConsole(QStringLiteral("Multiple ledges at the same place, do colision for left"));
                        map_to_send.parsed_layer.walkable[x+y*map_to_send.width]=false;
                    }
                    else
                        map_to_send.parsed_layer.ledges[x+y*map_to_send.width]=(quint8)ParsedLayerLedges_LedgesLeft;
                }
                if(ledgesRight)
                {
                    if(ledgesLeft || ledgesBottom || ledgesTop)
                    {
                        DebugClass::debugConsole(QStringLiteral("Multiple ledges at the same place, do colision for right"));
                        map_to_send.parsed_layer.walkable[x+y*map_to_send.width]=false;
                    }
                    else
                        map_to_send.parsed_layer.ledges[x+y*map_to_send.width]=(quint8)ParsedLayerLedges_LedgesRight;
                }
                if(ledgesTop)
                {
                    if(ledgesRight || ledgesBottom || ledgesLeft)
                    {
                        DebugClass::debugConsole(QStringLiteral("Multiple ledges at the same place, do colision for top"));
                        map_to_send.parsed_layer.walkable[x+y*map_to_send.width]=false;
                    }
                    else
                        map_to_send.parsed_layer.ledges[x+y*map_to_send.width]=(quint8)ParsedLayerLedges_LedgesTop;
                }
                if(ledgesBottom)
                {
                    if(ledgesRight || ledgesLeft || ledgesTop)
                    {
                        DebugClass::debugConsole(QStringLiteral("Multiple ledges at the same place, do colision for bottom"));
                        map_to_send.parsed_layer.walkable[x+y*map_to_send.width]=false;
                    }
                    else
                        map_to_send.parsed_layer.ledges[x+y*map_to_send.width]=(quint8)ParsedLayerLedges_LedgesBottom;
                }
            }
            y++;
        }
        x++;
    }

    const int &teleportlistsize=map_to_send.teleport.size();
    if(Walkable.size()>0)
    {
        int index=0;
        {
            while(index<teleportlistsize)
            {
                map_to_send.parsed_layer.walkable[map_to_send.teleport.at(index).source_x+map_to_send.teleport.at(index).source_y*map_to_send.width]=true;
                index++;
            }
        }
        index=0;
        {
            const int &listsize=map_to_send.bots.size();
            while(index<listsize)
            {
                map_to_send.parsed_layer.walkable[map_to_send.bots.at(index).point.x+map_to_send.bots.at(index).point.y*map_to_send.width]=false;
                index++;
            }
        }
    }
    if(Water.size()>0)
    {
        int index=0;
        while(index<teleportlistsize)
        {
            map_to_send.parsed_layer.water[map_to_send.teleport.at(index).source_x+map_to_send.teleport.at(index).source_y*map_to_send.width]=true;
            index++;
        }
    }

    //drop the empty layer
    {}

#ifdef DEBUG_MESSAGE_MAP_RAW
    if(Walkable.size()>0 || Water.size()>0 || Collisions.size()>0 || Grass.size()>0 || Dirt.size()>0)
    {
        QByteArray null_data;
        null_data.resize(4);
        null_data[0]=0x00;
        null_data[1]=0x00;
        null_data[2]=0x00;
        null_data[3]=0x00;
        x=0;
        y=0;
        QStringList layers_name;
        if(Walkable.size()>0)
            layers_name << "Walkable";
        if(Water.size()>0)
            layers_name << "Water";
        if(Collisions.size()>0)
            layers_name << "Collisions";
        if(Grass.size()>0)
            layers_name << "Grass";
        if(Dirt.size()>0)
            layers_name << "Dirt";
        if(LedgesRight.size()>0 || LedgesLeft.size()>0 || LedgesBottom.size()>0 || LedgesTop.size()>0)
            layers_name << "Ledges*";
        DebugClass::debugConsole("For "+fileName+": "+layers_name.join(" + ")+" = Walkable");
        while(y<map_to_send.height)
        {
            QString line;
            if(Walkable.size()>0)
            {
                x=0;
                while(x<map_to_send.width)
                {
                    line += QString::number(Walkable.mid(x*4+y*map_to_send.width*4,4)!=null_data);
                    x++;
                }
                line+=" ";
            }
            if(Water.size()>0)
            {
                x=0;
                while(x<map_to_send.width)
                {
                    line += QString::number(Water.mid(x*4+y*map_to_send.width*4,4)!=null_data);
                    x++;
                }
                line+=" ";
            }
            if(Collisions.size()>0)
            {
                x=0;
                while(x<map_to_send.width)
                {
                    line += QString::number(Collisions.mid(x*4+y*map_to_send.width*4,4)!=null_data);
                    x++;
                }
                line+=" ";
            }
            if(Grass.size()>0)
            {
                x=0;
                while(x<map_to_send.width)
                {
                    line += QString::number(Grass.mid(x*4+y*map_to_send.width*4,4)!=null_data);
                    x++;
                }
                line+=" ";
            }
            if(Dirt.size()>0)
            {
                x=0;
                while(x<map_to_send.width)
                {
                    line += QString::number(Dirt.mid(x*4+y*map_to_send.width*4,4)!=null_data);
                    x++;
                }
                line+=" ";
            }
            if(LedgesRight.size()>0 || LedgesLeft.size()>0 || LedgesBottom.size()>0 || LedgesTop.size()>0)
            {
                x=0;
                while(x<map_to_send.width)
                {
                    line += QString::number(map_to_send.parsed_layer.ledges.value(x+y*map_to_send.width);
                    x++;
                }
                line+=" ";
            }
            x=0;
            while(x<map_to_send.width)
            {
                line += QString::number(map_to_send.parsed_layer.walkable.value(x+y*map_to_send.width);
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

    //don't put code here !!!!!! put before the last block

    this->map_to_send=map_to_send;

    QString xmlExtra=fileName;
    xmlExtra.replace(QStringLiteral(".tmx"),QStringLiteral(".xml"));
    if(QFile::exists(xmlExtra))
        loadMonsterMap(xmlExtra);

    return true;
}

bool Map_loader::loadMonsterMap(const QString &fileName)
{
    bool ok;
    QDomDocument domDocument;

    //open and quick check the file
    if(CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.contains(fileName))
        domDocument=CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.value(fileName);
    else
    {
        QFile mapFile(fileName);
        QByteArray xmlContent;
        if(!mapFile.open(QIODevice::ReadOnly))
        {
            qDebug() << mapFile.fileName()+QStringLiteral(": ")+mapFile.errorString();
            return false;
        }
        xmlContent=mapFile.readAll();
        mapFile.close();
        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            qDebug() << QStringLiteral("%1, Parse error at line %2, column %3: %4").arg(mapFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr);
            return false;
        }
        CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile[fileName]=domDocument;
    }
    this->map_to_send.xmlRoot = domDocument.documentElement();
    if(this->map_to_send.xmlRoot.tagName()!=QStringLiteral("map"))
    {
        qDebug() << QStringLiteral("\"map\" root balise not found for the xml file");
        return false;
    }

    //grass
    quint32 tempGrassLuckTotal=0;
    QDomElement grass = this->map_to_send.xmlRoot.firstChildElement(QStringLiteral("grass"));
    if(!grass.isNull())
    {
        if(grass.isElement())
        {
            QDomElement monsters=grass.firstChildElement(QStringLiteral("monster"));
            while(!monsters.isNull())
            {
                if(monsters.isElement())
                {
                    if(monsters.hasAttribute(QStringLiteral("id")) && ((monsters.hasAttribute(QStringLiteral("minLevel")) && monsters.hasAttribute(QStringLiteral("maxLevel"))) || monsters.hasAttribute(QStringLiteral("level"))) && monsters.hasAttribute(QStringLiteral("luck")))
                    {
                        MapMonster mapMonster;
                        mapMonster.id=monsters.attribute(QStringLiteral("id")).toUInt(&ok);
                        if(!ok)
                            qDebug() << QStringLiteral("id is not a number: child.tagName(): %1 (at line: %2)").arg(monsters.tagName()).arg(monsters.lineNumber());
                        if(monsters.hasAttribute(QStringLiteral("minLevel")) && monsters.hasAttribute(QStringLiteral("maxLevel")))
                        {
                            if(ok)
                            {
                                mapMonster.minLevel=monsters.attribute(QStringLiteral("minLevel")).toUShort(&ok);
                                if(!ok)
                                    qDebug() << QStringLiteral("minLevel is not a number: child.tagName(): %1 (at line: %2)").arg(monsters.tagName()).arg(monsters.lineNumber());
                            }
                            if(ok)
                            {
                                mapMonster.maxLevel=monsters.attribute(QStringLiteral("maxLevel")).toUShort(&ok);
                                if(!ok)
                                    qDebug() << QStringLiteral("maxLevel is not a number: child.tagName(): %1 (at line: %2)").arg(monsters.tagName()).arg(monsters.lineNumber());
                            }
                        }
                        else
                        {
                            if(ok)
                            {
                                mapMonster.maxLevel=monsters.attribute(QStringLiteral("level")).toUShort(&ok);
                                mapMonster.minLevel=mapMonster.maxLevel;
                                if(!ok)
                                    qDebug() << QStringLiteral("level is not a number: child.tagName(): %1 (at line: %2)").arg(monsters.tagName()).arg(monsters.lineNumber());
                            }
                        }
                        if(ok)
                        {
                            QString textLuck=monsters.attribute(QStringLiteral("luck"));
                            textLuck.remove(QStringLiteral("%"));
                            mapMonster.luck=textLuck.toUShort(&ok);
                            if(!ok)
                                qDebug() << QStringLiteral("luck is not a number: child.tagName(): %1 (at line: %2)").arg(monsters.tagName()).arg(monsters.lineNumber());
                        }
                        if(ok)
                            if(mapMonster.minLevel>mapMonster.maxLevel)
                            {
                                qDebug() << QStringLiteral("min > max for the level: child.tagName(): %1 (at line: %2)").arg(monsters.tagName()).arg(monsters.lineNumber());
                                ok=false;
                            }
                        if(ok)
                            if(mapMonster.luck<=0)
                            {
                                qDebug() << QStringLiteral("luck is too low: child.tagName(): %1 (at line: %2)").arg(monsters.tagName()).arg(monsters.lineNumber());
                                ok=false;
                            }
                        if(ok)
                            if(mapMonster.minLevel<=0)
                            {
                                qDebug() << QStringLiteral("min level is too low: child.tagName(): %1 (at line: %2)").arg(monsters.tagName()).arg(monsters.lineNumber());
                                ok=false;
                            }
                        if(ok)
                            if(mapMonster.maxLevel<=0)
                            {
                                qDebug() << QStringLiteral("max level is too low: child.tagName(): %1 (at line: %2)").arg(monsters.tagName()).arg(monsters.lineNumber());
                                ok=false;
                            }
                        if(ok)
                            if(mapMonster.luck>100)
                            {
                                qDebug() << QStringLiteral("luck is greater than 100: child.tagName(): %1 (at line: %2)").arg(monsters.tagName()).arg(monsters.lineNumber());
                                ok=false;
                            }
                        if(ok)
                            if(mapMonster.minLevel>CATCHCHALLENGER_MONSTER_LEVEL_MAX)
                            {
                                qDebug() << QStringLiteral("min level is greater than %3: child.tagName(): %1 (at line: %2)").arg(monsters.tagName()).arg(monsters.lineNumber()).arg(CATCHCHALLENGER_MONSTER_LEVEL_MAX);
                                ok=false;
                            }
                        if(ok)
                            if(mapMonster.maxLevel>CATCHCHALLENGER_MONSTER_LEVEL_MAX)
                            {
                                qDebug() << QStringLiteral("max level is greater than %3: child.tagName(): %1 (at line: %2)").arg(monsters.tagName()).arg(monsters.lineNumber()).arg(CATCHCHALLENGER_MONSTER_LEVEL_MAX);
                                ok=false;
                            }
                        if(ok)
                        {
                            tempGrassLuckTotal+=mapMonster.luck;
                            map_to_send.grassMonster << mapMonster;
                        }
                    }
                    else
                        qDebug() << QStringLiteral("Missing attribute: child.tagName(): %1 (at line: %2)").arg(monsters.tagName()).arg(monsters.lineNumber());
                }
                else
                    qDebug() << QStringLiteral("Is not an element: child.tagName(): %1 (at line: %2)").arg(monsters.tagName()).arg(monsters.lineNumber());
                monsters = monsters.nextSiblingElement(QStringLiteral("monster"));
            }
        }
        else
            qDebug() << QStringLiteral("Is not an element: child.tagName(): %1 (at line: %2)").arg(grass.tagName()).arg(grass.lineNumber());
    }
    if(tempGrassLuckTotal!=100 && !map_to_send.grassMonster.isEmpty())
    {
        qDebug() << QStringLiteral("total luck is not egal to 100 (%3) for grass into %4, monsters dropped: child.tagName(): %1 (at line: %2)").arg(grass.tagName()).arg(grass.lineNumber()).arg(tempGrassLuckTotal).arg(fileName);
        map_to_send.grassMonster.clear();
    }

    //water
    quint32 tempWaterLuckTotal=0;
    QDomElement water = this->map_to_send.xmlRoot.firstChildElement(QStringLiteral("water"));
    if(!water.isNull())
    {
        if(water.isElement())
        {
            QDomElement monsters=water.firstChildElement(QStringLiteral("monster"));
            while(!monsters.isNull())
            {
                if(monsters.isElement())
                {
                    if(monsters.hasAttribute(QStringLiteral("id")) && ((monsters.hasAttribute(QStringLiteral("minLevel")) && monsters.hasAttribute(QStringLiteral("maxLevel"))) || monsters.hasAttribute(QStringLiteral("level"))) && monsters.hasAttribute(QStringLiteral("luck")))
                    {
                        MapMonster mapMonster;
                        mapMonster.id=monsters.attribute(QStringLiteral("id")).toUInt(&ok);
                        if(!ok)
                            qDebug() << QStringLiteral("id is not a number: child.tagName(): %1 (at line: %2)").arg(monsters.tagName()).arg(monsters.lineNumber());
                        if(monsters.hasAttribute(QStringLiteral("minLevel")) && monsters.hasAttribute(QStringLiteral("maxLevel")))
                        {
                            if(ok)
                            {
                                mapMonster.minLevel=monsters.attribute(QStringLiteral("minLevel")).toUShort(&ok);
                                if(!ok)
                                    qDebug() << QStringLiteral("minLevel is not a number: child.tagName(): %1 (at line: %2)").arg(monsters.tagName()).arg(monsters.lineNumber());
                            }
                            if(ok)
                            {
                                mapMonster.maxLevel=monsters.attribute(QStringLiteral("maxLevel")).toUShort(&ok);
                                if(!ok)
                                    qDebug() << QStringLiteral("maxLevel is not a number: child.tagName(): %1 (at line: %2)").arg(monsters.tagName()).arg(monsters.lineNumber());
                            }
                        }
                        else
                        {
                            if(ok)
                            {
                                mapMonster.maxLevel=monsters.attribute(QStringLiteral("level")).toUShort(&ok);
                                mapMonster.minLevel=mapMonster.maxLevel;
                                if(!ok)
                                    qDebug() << QStringLiteral("level is not a number: child.tagName(): %1 (at line: %2)").arg(monsters.tagName()).arg(monsters.lineNumber());
                            }
                        }
                        if(ok)
                        {
                            QString textLuck=monsters.attribute(QStringLiteral("luck"));
                            textLuck.remove(QStringLiteral("%"));
                            mapMonster.luck=textLuck.toUShort(&ok);
                            if(!ok)
                                qDebug() << QStringLiteral("luck is not a number: child.tagName(): %1 (at line: %2)").arg(monsters.tagName()).arg(monsters.lineNumber());
                        }
                        if(ok)
                            if(mapMonster.minLevel>mapMonster.maxLevel)
                            {
                                qDebug() << QStringLiteral("min > max for the level: child.tagName(): %1 (at line: %2)").arg(monsters.tagName()).arg(monsters.lineNumber());
                                ok=false;
                            }
                        if(ok)
                            if(mapMonster.luck<=0)
                            {
                                qDebug() << QStringLiteral("luck is too low: child.tagName(): %1 (at line: %2)").arg(monsters.tagName()).arg(monsters.lineNumber());
                                ok=false;
                            }
                        if(ok)
                            if(mapMonster.minLevel<=0)
                            {
                                qDebug() << QStringLiteral("min level is too low: child.tagName(): %1 (at line: %2)").arg(monsters.tagName()).arg(monsters.lineNumber());
                                ok=false;
                            }
                        if(ok)
                            if(mapMonster.maxLevel<=0)
                            {
                                qDebug() << QStringLiteral("max level is too low: child.tagName(): %1 (at line: %2)").arg(monsters.tagName()).arg(monsters.lineNumber());
                                ok=false;
                            }
                        if(ok)
                            if(mapMonster.luck>100)
                            {
                                qDebug() << QStringLiteral("luck is greater than 100: child.tagName(): %1 (at line: %2)").arg(monsters.tagName()).arg(monsters.lineNumber());
                                ok=false;
                            }
                        if(ok)
                            if(mapMonster.minLevel>CATCHCHALLENGER_MONSTER_LEVEL_MAX)
                            {
                                qDebug() << QStringLiteral("min level is greater than %3: child.tagName(): %1 (at line: %2)").arg(monsters.tagName()).arg(monsters.lineNumber()).arg(CATCHCHALLENGER_MONSTER_LEVEL_MAX);
                                ok=false;
                            }
                        if(ok)
                            if(mapMonster.maxLevel>CATCHCHALLENGER_MONSTER_LEVEL_MAX)
                            {
                                qDebug() << QStringLiteral("max level is greater than %3: child.tagName(): %1 (at line: %2)").arg(monsters.tagName()).arg(monsters.lineNumber()).arg(CATCHCHALLENGER_MONSTER_LEVEL_MAX);
                                ok=false;
                            }
                        if(ok)
                        {
                            tempWaterLuckTotal+=mapMonster.luck;
                            map_to_send.waterMonster << mapMonster;
                        }
                    }
                    else
                        qDebug() << QStringLiteral("Missing attribute: child.tagName(): %1 (at line: %2)").arg(monsters.tagName()).arg(monsters.lineNumber());
                }
                else
                    qDebug() << QStringLiteral("Is not an element: child.tagName(): %1 (at line: %2)").arg(monsters.tagName()).arg(monsters.lineNumber());
                monsters = monsters.nextSiblingElement(QStringLiteral("monster"));
            }
        }
        else
            qDebug() << QStringLiteral("Is not an element: child.tagName(): %1 (at line: %2)").arg(water.tagName()).arg(water.lineNumber());
    }
    if(tempWaterLuckTotal!=100 && !map_to_send.waterMonster.isEmpty())
    {
        qDebug() << QStringLiteral("total luck is not egal to 100 (%3) for water into %4, monsters dropped: child.tagName(): %1 (at line: %2)").arg(water.tagName()).arg(water.lineNumber()).arg(tempWaterLuckTotal).arg(fileName);
        map_to_send.waterMonster.clear();
    }

    //cave
    quint32 tempCaveLuckTotal=0;
    QDomElement cave = this->map_to_send.xmlRoot.firstChildElement(QStringLiteral("cave"));
    if(!cave.isNull())
    {
        if(cave.isElement())
        {
            QDomElement monsters=cave.firstChildElement(QStringLiteral("monster"));
            while(!monsters.isNull())
            {
                if(monsters.isElement())
                {
                    if(monsters.hasAttribute(QStringLiteral("id")) && ((monsters.hasAttribute(QStringLiteral("minLevel")) && monsters.hasAttribute(QStringLiteral("maxLevel"))) || monsters.hasAttribute(QStringLiteral("level"))) && monsters.hasAttribute(QStringLiteral("luck")))
                    {
                        MapMonster mapMonster;
                        mapMonster.id=monsters.attribute(QStringLiteral("id")).toUInt(&ok);
                        if(!ok)
                            qDebug() << QStringLiteral("id is not a number: child.tagName(): %1 (at line: %2)").arg(monsters.tagName()).arg(monsters.lineNumber());
                        if(monsters.hasAttribute(QStringLiteral("minLevel")) && monsters.hasAttribute(QStringLiteral("maxLevel")))
                        {
                            if(ok)
                            {
                                mapMonster.minLevel=monsters.attribute(QStringLiteral("minLevel")).toUShort(&ok);
                                if(!ok)
                                    qDebug() << QStringLiteral("minLevel is not a number: child.tagName(): %1 (at line: %2)").arg(monsters.tagName()).arg(monsters.lineNumber());
                            }
                            if(ok)
                            {
                                mapMonster.maxLevel=monsters.attribute(QStringLiteral("maxLevel")).toUShort(&ok);
                                if(!ok)
                                    qDebug() << QStringLiteral("maxLevel is not a number: child.tagName(): %1 (at line: %2)").arg(monsters.tagName()).arg(monsters.lineNumber());
                            }
                        }
                        else
                        {
                            if(ok)
                            {
                                mapMonster.maxLevel=monsters.attribute(QStringLiteral("level")).toUShort(&ok);
                                mapMonster.minLevel=mapMonster.maxLevel;
                                if(!ok)
                                    qDebug() << QStringLiteral("level is not a number: child.tagName(): %1 (at line: %2)").arg(monsters.tagName()).arg(monsters.lineNumber());
                            }
                        }
                        if(ok)
                        {
                            QString textLuck=monsters.attribute(QStringLiteral("luck"));
                            textLuck.remove("%");
                            mapMonster.luck=textLuck.toUShort(&ok);
                            if(!ok)
                                qDebug() << QStringLiteral("luck is not a number: child.tagName(): %1 (at line: %2)").arg(monsters.tagName()).arg(monsters.lineNumber());
                        }
                        if(ok)
                            if(mapMonster.minLevel>mapMonster.maxLevel)
                            {
                                qDebug() << QStringLiteral("min > max for the level: child.tagName(): %1 (at line: %2)").arg(monsters.tagName()).arg(monsters.lineNumber());
                                ok=false;
                            }
                        if(ok)
                            if(mapMonster.luck<=0)
                            {
                                qDebug() << QStringLiteral("luck is too low: child.tagName(): %1 (at line: %2)").arg(monsters.tagName()).arg(monsters.lineNumber());
                                ok=false;
                            }
                        if(ok)
                            if(mapMonster.minLevel<=0)
                            {
                                qDebug() << QStringLiteral("min level is too low: child.tagName(): %1 (at line: %2)").arg(monsters.tagName()).arg(monsters.lineNumber());
                                ok=false;
                            }
                        if(ok)
                            if(mapMonster.maxLevel<=0)
                            {
                                qDebug() << QStringLiteral("max level is too low: child.tagName(): %1 (at line: %2)").arg(monsters.tagName()).arg(monsters.lineNumber());
                                ok=false;
                            }
                        if(ok)
                            if(mapMonster.luck>100)
                            {
                                qDebug() << QStringLiteral("luck is greater than 100: child.tagName(): %1 (at line: %2)").arg(monsters.tagName()).arg(monsters.lineNumber());
                                ok=false;
                            }
                        if(ok)
                            if(mapMonster.minLevel>CATCHCHALLENGER_MONSTER_LEVEL_MAX)
                            {
                                qDebug() << QStringLiteral("min level is greater than %3: child.tagName(): %1 (at line: %2)").arg(monsters.tagName()).arg(monsters.lineNumber()).arg(CATCHCHALLENGER_MONSTER_LEVEL_MAX);
                                ok=false;
                            }
                        if(ok)
                            if(mapMonster.maxLevel>CATCHCHALLENGER_MONSTER_LEVEL_MAX)
                            {
                                qDebug() << QStringLiteral("max level is greater than %3: child.tagName(): %1 (at line: %2)").arg(monsters.tagName()).arg(monsters.lineNumber()).arg(CATCHCHALLENGER_MONSTER_LEVEL_MAX);
                                ok=false;
                            }
                        if(ok)
                        {
                            tempCaveLuckTotal+=mapMonster.luck;
                            map_to_send.caveMonster << mapMonster;
                        }
                    }
                    else
                        qDebug() << QStringLiteral("Missing attribute: child.tagName(): %1 (at line: %2)").arg(monsters.tagName()).arg(monsters.lineNumber());
                }
                else
                    qDebug() << QStringLiteral("Is not an element: child.tagName(): %1 (at line: %2)").arg(monsters.tagName()).arg(monsters.lineNumber());
                monsters = monsters.nextSiblingElement(QStringLiteral("monster"));
            }
        }
        else
            qDebug() << QStringLiteral("Is not an element: child.tagName(): %1 (at line: %2)").arg(cave.tagName()).arg(cave.lineNumber());
    }
    if(tempCaveLuckTotal!=100 && !map_to_send.caveMonster.isEmpty())
    {
        qDebug() << QStringLiteral("total luck is not egal to 100 (%3) for cave into %4, monsters dropped: child.tagName(): %1 (at line: %2)").arg(cave.tagName()).arg(cave.lineNumber()).arg(tempCaveLuckTotal).arg(fileName);
        map_to_send.caveMonster.clear();
    }

    return true;
}

QString Map_loader::errorString()
{
    return error;
}

QString Map_loader::resolvRelativeMap(const QString &fileName,const QString &link,const QString &datapackPath)
{
    if(link.isEmpty())
        return link;
    QString currentPath=QFileInfo(fileName).absolutePath();
    QFileInfo newmap(currentPath+QDir::separator()+link);
    QString newPath=newmap.absoluteFilePath();
    if(datapackPath.isEmpty() || newPath.startsWith(datapackPath))
    {
        newPath.remove(0,datapackPath.size());
        #if defined (DEBUG_MESSAGE_MAP_BORDER) || defined (DEBUG_MESSAGE_MAP_TP)
        DebugClass::debugConsole(QStringLiteral("map link resolved: %1 (%2)").arg(newPath).arg(link));
        #endif
        return newPath;
    }
    DebugClass::debugConsole(QStringLiteral("map link not resolved: %1, full path: %2, newPath: %3, datapackPath: %4").arg(link).arg(currentPath+QDir::separator()+link).arg(newPath).arg(datapackPath));
    return link;
}

QDomElement Map_loader::getXmlCondition(const QString &fileName,const QString &conditionFile,const quint32 &conditionId)
{
    if(CatchChallenger::CommonDatapack::commonDatapack.teleportConditionsUnparsed.contains(conditionFile))
    {
        if(CatchChallenger::CommonDatapack::commonDatapack.teleportConditionsUnparsed.value(conditionFile).contains(conditionId))
            return CatchChallenger::CommonDatapack::commonDatapack.teleportConditionsUnparsed.value(conditionFile).value(conditionId);
        else
            return QDomElement();
    }
    CatchChallenger::CommonDatapack::commonDatapack.teleportConditionsUnparsed[conditionFile][conditionId];
    bool ok;
    QDomDocument domDocument;

    //open and quick check the file
    if(CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.contains(conditionFile))
        domDocument=CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.value(conditionFile);
    else
    {
        QFile mapFile(conditionFile);
        QByteArray xmlContent;
        if(!mapFile.open(QIODevice::ReadOnly))
        {
            qDebug() << QStringLiteral("Into the file %1, unab le to open the condition file: ").arg(fileName)+mapFile.fileName()+QStringLiteral(": ")+mapFile.errorString();
            return QDomElement();
        }
        xmlContent=mapFile.readAll();
        mapFile.close();
        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            qDebug() << QStringLiteral("%1, Parse error at line %2, column %3: %4").arg(mapFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr);
            return QDomElement();
        }
        CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile[conditionFile]=domDocument;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!=QStringLiteral("conditions"))
    {
        qDebug() << QStringLiteral("\"conditions\" root balise not found for the xml file %1").arg(conditionFile);
        return QDomElement();
    }

    QDomElement item = root.firstChildElement(QStringLiteral("condition"));
    while(!item.isNull())
    {
        if(item.isElement())
        {
            if(!item.hasAttribute(QStringLiteral("id")))
                qDebug() << QStringLiteral("\"condition\" balise have not id attribute (%1 at %2)").arg(conditionFile).arg(item.lineNumber());
            else if(!item.hasAttribute(QStringLiteral("type")))
                qDebug() << QStringLiteral("\"condition\" balise have not type attribute (%1 at %2)").arg(conditionFile).arg(item.lineNumber());
            else
            {
                quint32 id=item.attribute(QStringLiteral("id")).toUInt(&ok);
                if(!ok)
                    qDebug() << QStringLiteral("\"condition\" balise have id is not a number (%1 at %2)").arg(conditionFile).arg(item.lineNumber());
                else
                    CatchChallenger::CommonDatapack::commonDatapack.teleportConditionsUnparsed[conditionFile][id]=item;
            }
        }
        item = item.nextSiblingElement(QStringLiteral("condition"));
    }
    if(CatchChallenger::CommonDatapack::commonDatapack.teleportConditionsUnparsed.contains(conditionFile))
        if(CatchChallenger::CommonDatapack::commonDatapack.teleportConditionsUnparsed.value(conditionFile).contains(conditionId))
            return CatchChallenger::CommonDatapack::commonDatapack.teleportConditionsUnparsed.value(conditionFile).value(conditionId);
    return QDomElement();
}

MapCondition Map_loader::xmlConditionToMapCondition(const QString &conditionFile,const QDomElement &conditionContent)
{
    bool ok;
    MapCondition condition;
    condition.type=MapConditionType_None;
    condition.value=0;
    if(conditionContent.attribute(QStringLiteral("type"))==QStringLiteral("quest"))
    {
        if(!conditionContent.hasAttribute(QStringLiteral("quest")))
            qDebug() << QStringLiteral("\"condition\" balise have type=quest but quest attribute not found, item, clan or fightBot (%1 at %2)").arg(conditionFile).arg(conditionContent.lineNumber());
        else
        {
            quint32 quest=conditionContent.attribute(QStringLiteral("quest")).toUInt(&ok);
            if(!ok)
                qDebug() << QStringLiteral("\"condition\" balise have type=quest but quest attribute is not a number, item, clan or fightBot (%1 at %2)").arg(conditionFile).arg(conditionContent.lineNumber());
            else if(!CatchChallenger::CommonDatapack::commonDatapack.quests.contains(quest))
                qDebug() << QStringLiteral("\"condition\" balise have type=quest but quest id is not found, item, clan or fightBot (%1 at %2)").arg(conditionFile).arg(conditionContent.lineNumber());
            else
            {
                condition.type=MapConditionType_Quest;
                condition.value=quest;
            }
        }
    }
    else if(conditionContent.attribute(QStringLiteral("type"))==QStringLiteral("item"))
    {
        if(!conditionContent.hasAttribute(QStringLiteral("item")))
            qDebug() << QStringLiteral("\"condition\" balise have type=item but item attribute not found, item, clan or fightBot (%1 at %2)").arg(conditionFile).arg(conditionContent.lineNumber());
        else
        {
            quint32 item=conditionContent.attribute(QStringLiteral("item")).toUInt(&ok);
            if(!ok)
                qDebug() << QStringLiteral("\"condition\" balise have type=item but item attribute is not a number, item, clan or fightBot (%1 at %2)").arg(conditionFile).arg(conditionContent.lineNumber());
            else if(!CatchChallenger::CommonDatapack::commonDatapack.items.item.contains(item))
                qDebug() << QStringLiteral("\"condition\" balise have type=item but item id is not found, item, clan or fightBot (%1 at %2)").arg(conditionFile).arg(conditionContent.lineNumber());
            else
            {
                condition.type=MapConditionType_Item;
                condition.value=item;
            }
        }
    }
    else if(conditionContent.attribute(QStringLiteral("type"))==QStringLiteral("fightBot"))
    {
        if(!conditionContent.hasAttribute(QStringLiteral("fightBot")))
            qDebug() << QStringLiteral("\"condition\" balise have type=fightBot but fightBot attribute not found, item, clan or fightBot (%1 at %2)").arg(conditionFile).arg(conditionContent.lineNumber());
        else
        {
            quint32 fightBot=conditionContent.attribute(QStringLiteral("fightBot")).toUInt(&ok);
            if(!ok)
                qDebug() << QStringLiteral("\"condition\" balise have type=fightBot but fightBot attribute is not a number, item, clan or fightBot (%1 at %2)").arg(conditionFile).arg(conditionContent.lineNumber());
            else if(!CatchChallenger::CommonDatapack::commonDatapack.botFights.contains(fightBot))
                qDebug() << QStringLiteral("\"condition\" balise have type=fightBot but fightBot id is not found, item, clan or fightBot (%1 at %2)").arg(conditionFile).arg(conditionContent.lineNumber());
            else
            {
                condition.type=MapConditionType_FightBot;
                condition.value=fightBot;
            }
        }
    }
    else if(conditionContent.attribute(QStringLiteral("type"))==QStringLiteral("clan"))
        condition.type=MapConditionType_Clan;
    else
        qDebug() << QStringLiteral("\"condition\" balise have type but value is not quest, item, clan or fightBot (%1 at %2)").arg(conditionFile).arg(conditionContent.lineNumber());
    return condition;
}
