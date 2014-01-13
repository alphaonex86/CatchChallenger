/*
 * mapreader.cpp
 * Copyright 2008-2010, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
 * Copyright 2010, Jeff Bland <jksb@member.fsf.org>
 * Copyright 2010, Dennis Honeyman <arcticuno@gmail.com>
 *
 * This file is part of libtiled.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "tiled_mapreader.h"
#include "tiled_compression.h"
#include "tiled_gidmapper.h"
#include "tiled_imagelayer.h"
#include "tiled_objectgroup.h"
#include "tiled_map.h"
#include "tiled_mapobject.h"
#include "tiled_tile.h"
#include "tiled_tilelayer.h"
#include "tiled_tileset.h"
#include "tiled_terrain.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QVector>
#include <QXmlStreamReader>

using namespace Tiled;
using namespace Tiled::Internal;

namespace Tiled {
namespace Internal {

class MapReaderPrivate
{
    Q_DECLARE_TR_FUNCTIONS(MapReader)

    friend class Tiled::MapReader;

public:
    MapReaderPrivate(MapReader *mapReader):
        mLazy(false),
        p(mapReader),
        mMap(0),
        mReadingExternalTileset(false)
    {}

    Map *readMap(QIODevice *device, const QString &path);
    Tileset *readTileset(QIODevice *device, const QString &path);

    bool openFile(QFile *file);

    QString errorString() const;

    bool mLazy;

private:
    void readUnknownElement();

    Map *readMap();

    Tileset *readTileset();
    void readTilesetTile(Tileset *tileset);
    void readTilesetImage(Tileset *tileset);
    void readTilesetTerrainTypes(Tileset *tileset);
    QImage readImage();

    TileLayer *readLayer();
    void readLayerData(TileLayer *tileLayer);
    void decodeBinaryLayerData(TileLayer *tileLayer,
                               const QStringRef &text,
                               const QStringRef &compression);
    void decodeCSVLayerData(TileLayer *tileLayer, const QString &text);

    /**
     * Returns the cell for the given global tile ID. Errors are raised with
     * the QXmlStreamReader.
     *
     * @param gid the global tile ID
     * @return the cell data associated with the given global tile ID, or an
     *         empty cell if not found
     */
    Cell cellForGid(unsigned gid);

    ImageLayer *readImageLayer();
    void readImageLayerImage(ImageLayer *imageLayer);

    ObjectGroup *readObjectGroup();
    MapObject *readObject();
    QPolygonF readPolygon();

    Properties readProperties();
    void readProperty(Properties *properties);

    MapReader *p;

    QString mError;
    QString mPath;
    Map *mMap;
    QList<Tileset*> mCreatedTilesets;
    GidMapper mGidMapper;
    bool mReadingExternalTileset;

    QXmlStreamReader xml;
};

} // namespace Internal
} // namespace Tiled

Map *MapReaderPrivate::readMap(QIODevice *device, const QString &path)
{
    mError.clear();
    mPath = path;
    Map *map = 0;

    xml.setDevice(device);

    if (xml.readNextStartElement() && xml.name() == QStringLiteral("map")) {
        map = readMap();
    } else {
        xml.raiseError(tr("Not a map file."));
    }

    mGidMapper.clear();
    return map;
}

Tileset *MapReaderPrivate::readTileset(QIODevice *device, const QString &path)
{
    mError.clear();
    mPath = path;
    Tileset *tileset = 0;
    mReadingExternalTileset = true;

    xml.setDevice(device);

    if (xml.readNextStartElement() && xml.name() == QStringLiteral("tileset"))
        tileset = readTileset();
    else
        xml.raiseError(tr("Not a tileset file."));

    mReadingExternalTileset = false;
    return tileset;
}

QString MapReaderPrivate::errorString() const
{
    if (!mError.isEmpty()) {
        return mError;
    } else {
        return tr("%3\n\nLine %1, column %2")
                .arg(xml.lineNumber())
                .arg(xml.columnNumber())
                .arg(xml.errorString());
    }
}

bool MapReaderPrivate::openFile(QFile *file)
{
    if (!file->exists()) {
        mError = tr("File not found: %1").arg(file->fileName());
        return false;
    } else if (!file->open(QFile::ReadOnly | QFile::Text)) {
        mError = tr("Unable to read file: %1").arg(file->fileName());
        return false;
    }

    return true;
}

void MapReaderPrivate::readUnknownElement()
{
    qDebug().nospace() << "Unknown element (fixme): " << xml.name()
                       << " at line " << xml.lineNumber()
                       << ", column " << xml.columnNumber();
    xml.skipCurrentElement();
}

Map *MapReaderPrivate::readMap()
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QStringLiteral("map"));

    const QXmlStreamAttributes atts = xml.attributes();
    const int mapWidth =
            atts.value(QStringLiteral("width")).toString().toInt();
    const int mapHeight =
            atts.value(QStringLiteral("height")).toString().toInt();
    const int tileWidth =
            atts.value(QStringLiteral("tilewidth")).toString().toInt();
    const int tileHeight =
            atts.value(QStringLiteral("tileheight")).toString().toInt();

    const QString orientationString =
            atts.value(QStringLiteral("orientation")).toString();
    const Map::Orientation orientation =
            orientationFromString(orientationString);

    if (orientation == Map::Unknown) {
        xml.raiseError(tr("Unsupported map orientation: \"%1\"")
                       .arg(orientationString));
    }

    mMap = new Map(orientation, mapWidth, mapHeight, tileWidth, tileHeight);
    mCreatedTilesets.clear();

    QStringRef bgColorString = atts.value(QStringLiteral("backgroundcolor"));
    if (!bgColorString.isEmpty())
        mMap->setBackgroundColor(QColor(bgColorString.toString()));

    while (xml.readNextStartElement()) {
        if (xml.name() == QStringLiteral("properties"))
            mMap->mergeProperties(readProperties());
        else if (xml.name() == QStringLiteral("tileset"))
            mMap->addTileset(readTileset());
        else if (xml.name() == QStringLiteral("layer"))
            mMap->addLayer(readLayer());
        else if (xml.name() == QStringLiteral("objectgroup"))
            mMap->addLayer(readObjectGroup());
        else if (xml.name() == QStringLiteral("imagelayer"))
            mMap->addLayer(readImageLayer());
        else
            readUnknownElement();
    }

    // Clean up in case of error
    if (xml.hasError()) {
        // The tilesets are not owned by the map
        qDeleteAll(mCreatedTilesets);
        mCreatedTilesets.clear();

        delete mMap;
        mMap = 0;
    }

    return mMap;
}

Tileset *MapReaderPrivate::readTileset()
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QStringLiteral("tileset"));

    const QXmlStreamAttributes atts = xml.attributes();
    const QString source = atts.value(QStringLiteral("source")).toString();
    const unsigned firstGid =
            atts.value(QStringLiteral("firstgid")).toString().toUInt();

    Tileset *tileset = 0;

    if (source.isEmpty()) { // Not an external tileset
        const QString name =
                atts.value(QStringLiteral("name")).toString();
        const int tileWidth =
                atts.value(QStringLiteral("tilewidth")).toString().toInt();
        const int tileHeight =
                atts.value(QStringLiteral("tileheight")).toString().toInt();
        const int tileSpacing =
                atts.value(QStringLiteral("spacing")).toString().toInt();
        const int margin =
                atts.value(QStringLiteral("margin")).toString().toInt();

        if (tileWidth < 0 || tileHeight < 0
            || (firstGid == 0 && !mReadingExternalTileset)) {
            xml.raiseError(tr("Invalid tileset parameters for tileset"
                              " '%1'").arg(name));
        } else {
            tileset = new Tileset(name, tileWidth, tileHeight,
                                  tileSpacing, margin);

            mCreatedTilesets.append(tileset);

            while (xml.readNextStartElement()) {
                if (xml.name() == QStringLiteral("tile")) {
                    readTilesetTile(tileset);
                } else if (xml.name() == QStringLiteral("tileoffset")) {
                    const QXmlStreamAttributes oa = xml.attributes();
                    int x = oa.value(QStringLiteral("x")).toString().toInt();
                    int y = oa.value(QStringLiteral("y")).toString().toInt();
                    tileset->setTileOffset(QPoint(x, y));
                    xml.skipCurrentElement();
                } else if (xml.name() == QStringLiteral("properties")) {
                    tileset->mergeProperties(readProperties());
                } else if (xml.name() == QStringLiteral("image")) {
                    if (tileWidth == 0 || tileHeight == 0) {
                        xml.raiseError(tr("Invalid tileset parameters for tileset"
                                          " '%1'").arg(name));
                    }
                    readTilesetImage(tileset);
                } else if (xml.name() == QStringLiteral("terraintypes")) {
                    readTilesetTerrainTypes(tileset);
                } else {
                    readUnknownElement();
                }
            }
        }
    } else { // External tileset
        const QString absoluteSource = p->resolveReference(source, mPath);

        if (mLazy) {
            // Don't load it right now, but remember the source
            tileset = new Tileset(QString(), 0, 0);
            tileset->setFileName(absoluteSource);
        } else {
            QString error;
            tileset = p->readExternalTileset(absoluteSource, &error);

            if (!tileset) {
                xml.raiseError(tr("Error while loading tileset '%1': %2")
                               .arg(absoluteSource, error));
            }
        }

        xml.skipCurrentElement();
    }

    if (tileset && !mReadingExternalTileset)
        mGidMapper.insert(firstGid, tileset);

    return tileset;
}

void MapReaderPrivate::readTilesetTile(Tileset *tileset)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QStringLiteral("tile"));

    const QXmlStreamAttributes atts = xml.attributes();
    const int id = atts.value(QStringLiteral("id")).toString().toInt();

    // In lazy mode we don't know in advance how many tiles a certain tileset
    // will have. Make sure the tileset is at least big enough to hold the
    // properties for all tiles.
    if (mLazy && id >= tileset->tileCount())
        tileset->resize(id + 1);

    if (id < 0) {
        xml.raiseError(tr("Invalid tile ID: %1").arg(id));
        return;
    } else if (id == tileset->tileCount()) {
        tileset->addTile(QPixmap());
    } else if (id > tileset->tileCount()) {
        xml.raiseError(tr("Invalid (nonconsecutive) tile ID: %1").arg(id));
        return;
    }
    Tile *tile = tileset->tileAt(id);

    // Read tile quadrant terrain ids
    QString terrain = atts.value(QStringLiteral("terrain")).toString();
    if (!terrain.isEmpty()) {
        QStringList quadrants = terrain.split(QStringLiteral(","));
        if (quadrants.size() == 4) {
            for (int i = 0; i < 4; ++i) {
                int t = quadrants[i].isEmpty() ? -1 : quadrants[i].toInt();
                tile->setCornerTerrain(i, t);
            }
        }
    }

    // Read tile probability
    QString probability = atts.value(QStringLiteral("probability")).toString();
    if (!probability.isEmpty()) {
        tile->setTerrainProbability(probability.toFloat());
    }

    while (xml.readNextStartElement()) {
        if (xml.name() == QStringLiteral("properties")) {
            tile->mergeProperties(readProperties());
        } else if (xml.name() == QStringLiteral("image")) {
            // TODO: Support individual tile images in lazy mode
            tileset->setTileImage(id, QPixmap::fromImage(readImage()));
        } else {
            readUnknownElement();
        }
    }
}

void MapReaderPrivate::readTilesetImage(Tileset *tileset)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QStringLiteral("image"));

    const QXmlStreamAttributes atts = xml.attributes();
    QString source = atts.value(QStringLiteral("source")).toString();
    QString trans = atts.value(QStringLiteral("trans")).toString();

    if (!trans.isEmpty()) {
        if (!trans.startsWith(QStringLiteral("#")))
            trans.prepend(QStringLiteral("#"));
        tileset->setTransparentColor(QColor(trans));
    }

    source = p->resolveReference(source, mPath);

    // Set the width that the tileset had when the map was saved
    const int width = atts.value(QStringLiteral("width")).toString().toInt();
    mGidMapper.setTilesetWidth(tileset, width);

    if (mLazy && !source.isEmpty()) {
        // Don't load the image right now, but remember the source
        tileset->setImageSource(source);
        xml.skipCurrentElement();
    } else {
        if (!tileset->loadFromImage(readImage(), source))
            xml.raiseError(tr("Error loading tileset image:\n'%1'").arg(source));
    }
}

QImage MapReaderPrivate::readImage()
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QStringLiteral("image"));

    const QXmlStreamAttributes atts = xml.attributes();
    QString source = atts.value(QStringLiteral("source")).toString();
    QString format = atts.value(QStringLiteral("format")).toString();

    if (source.isEmpty()) {
        while (xml.readNextStartElement()) {
            if (xml.name() == QStringLiteral("data")) {
                const QXmlStreamAttributes atts = xml.attributes();
                QString encoding = atts.value(QStringLiteral("encoding"))
                    .toString();
                QByteArray data = xml.readElementText().toLatin1();
                if (encoding == QStringLiteral("base64")) {
                    data = QByteArray::fromBase64(data);
                }
                xml.skipCurrentElement();
                return QImage::fromData(data, format.toLatin1());
            } else {
                readUnknownElement();
            }
        }
    } else {
        xml.skipCurrentElement();

        source = p->resolveReference(source, mPath);
        QImage image = p->readExternalImage(source);
        if (image.isNull())
            xml.raiseError(tr("Error loading image:\n'%1'").arg(source));
        if(image.format()!=QImage::Format_ARGB32 || image.format()!=QImage::Format_RGB32)
            image=image.convertToFormat(QImage::Format_ARGB32);
        return image;
    }
    return QImage();
}

void MapReaderPrivate::readTilesetTerrainTypes(Tileset *tileset)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QStringLiteral("terraintypes"));

    while (xml.readNextStartElement()) {
        if (xml.name() == QStringLiteral("terrain")) {
            const QXmlStreamAttributes atts = xml.attributes();
            QString name = atts.value(QStringLiteral("name")).toString();
            int tile = atts.value(QStringLiteral("tile")).toString().toInt();

            Terrain *terrain = tileset->addTerrain(name, tile);

            while (xml.readNextStartElement()) {
                if (xml.name() == QStringLiteral("properties"))
                    terrain->mergeProperties(readProperties());
                else
                    readUnknownElement();
            }
        } else {
            readUnknownElement();
        }
    }
}

static void readLayerAttributes(Layer *layer,
                                const QXmlStreamAttributes &atts)
{
    const QStringRef opacityRef = atts.value(QStringLiteral("opacity"));
    const QStringRef visibleRef = atts.value(QStringLiteral("visible"));

    bool ok;
    const float opacity = opacityRef.toString().toFloat(&ok);
    if (ok)
        layer->setOpacity(opacity);

    const int visible = visibleRef.toString().toInt(&ok);
    if (ok)
        layer->setVisible(visible);
}

TileLayer *MapReaderPrivate::readLayer()
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QStringLiteral("layer"));

    const QXmlStreamAttributes atts = xml.attributes();
    const QString name = atts.value(QStringLiteral("name")).toString();
    const int x = atts.value(QStringLiteral("x")).toString().toInt();
    const int y = atts.value(QStringLiteral("y")).toString().toInt();
    const int width = atts.value(QStringLiteral("width")).toString().toInt();
    const int height = atts.value(QStringLiteral("height")).toString().toInt();

    TileLayer *tileLayer = new TileLayer(name, x, y, width, height);
    readLayerAttributes(tileLayer, atts);

    while (xml.readNextStartElement()) {
        if (xml.name() == QStringLiteral("properties"))
            tileLayer->mergeProperties(readProperties());
        else if (xml.name() == QStringLiteral("data"))
            readLayerData(tileLayer);
        else
            readUnknownElement();
    }

    return tileLayer;
}

void MapReaderPrivate::readLayerData(TileLayer *tileLayer)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QStringLiteral("data"));

    const QXmlStreamAttributes atts = xml.attributes();
    QStringRef encoding = atts.value(QStringLiteral("encoding"));
    QStringRef compression = atts.value(QStringLiteral("compression"));

    bool respect = true; // TODO: init from preferences
    if (respect) {
        if (encoding.isEmpty())
            mMap->setLayerDataFormat(Map::XML);
        else if (encoding == QStringLiteral("csv"))
            mMap->setLayerDataFormat(Map::CSV);
        else if (encoding == QStringLiteral("base64")) {
            if (compression.isEmpty())
                mMap->setLayerDataFormat(Map::Base64);
            else if (compression == QStringLiteral("gzip"))
                mMap->setLayerDataFormat(Map::Base64Gzip);
            else if (compression == QStringLiteral("zlib"))
                mMap->setLayerDataFormat(Map::Base64Zlib);
        }
        // else, error handled below
    }

    int x = 0;
    int y = 0;

    while (xml.readNext() != QXmlStreamReader::Invalid) {
        if (xml.isEndElement())
            break;
        else if (xml.isStartElement()) {
            if (xml.name() == QStringLiteral("tile")) {
                if (y >= tileLayer->height()) {
                    xml.raiseError(tr("Too many <tile> elements"));
                    continue;
                }

                const QXmlStreamAttributes atts = xml.attributes();
                unsigned gid = atts.value(QStringLiteral("gid")).toString().toUInt();
                tileLayer->setCell(x, y, cellForGid(gid));

                x++;
                if (x >= tileLayer->width()) {
                    x = 0;
                    y++;
                }

                xml.skipCurrentElement();
            } else {
                readUnknownElement();
            }
        } else if (xml.isCharacters() && !xml.isWhitespace()) {
            if (encoding == QStringLiteral("base64")) {
                decodeBinaryLayerData(tileLayer,
                                      xml.text(),
                                      compression);
            } else if (encoding == QStringLiteral("csv")) {
                decodeCSVLayerData(tileLayer, xml.text().toString());
            } else {
                xml.raiseError(tr("Unknown encoding: %1")
                               .arg(encoding.toString()));
                continue;
            }
        }
    }
}

void MapReaderPrivate::decodeBinaryLayerData(TileLayer *tileLayer,
                                             const QStringRef &text,
                                             const QStringRef &compression)
{
#if QT_VERSION < 0x040800
    const QString textData = QString::fromRawData(text.unicode(), text.size());
    const QByteArray latin1Text = textData.toLatin1();
#else
    const QByteArray latin1Text = text.toLatin1();
#endif
    QByteArray tileData = QByteArray::fromBase64(latin1Text);
    const int size = (tileLayer->width() * tileLayer->height()) * 4;

    if (compression == QStringLiteral("zlib")
        || compression == QStringLiteral("gzip")) {
        tileData = decompress(tileData, size);
    } else if (!compression.isEmpty()) {
        xml.raiseError(tr("Compression method '%1' not supported")
                       .arg(compression.toString()));
        return;
    }

    if (size != tileData.length()) {
        xml.raiseError(tr("Corrupt layer data for layer '%1'")
                       .arg(tileLayer->name()));
        return;
    }

    const unsigned char *data =
            reinterpret_cast<const unsigned char*>(tileData.constData());
    int x = 0;
    int y = 0;

    for (int i = 0; i < size - 3; i += 4) {
        const unsigned gid = data[i] |
                             data[i + 1] << 8 |
                             data[i + 2] << 16 |
                             data[i + 3] << 24;

        tileLayer->setCell(x, y, cellForGid(gid));

        x++;
        if (x == tileLayer->width()) {
            x = 0;
            y++;
        }
    }
}

void MapReaderPrivate::decodeCSVLayerData(TileLayer *tileLayer, const QString &text)
{
    QString trimText = text.trimmed();
    QStringList tiles = trimText.split(QStringLiteral(","));

    if (tiles.length() != tileLayer->width() * tileLayer->height()) {
        xml.raiseError(tr("Corrupt layer data for layer '%1'")
                       .arg(tileLayer->name()));
        return;
    }

    for (int y = 0; y < tileLayer->height(); y++) {
        for (int x = 0; x < tileLayer->width(); x++) {
            bool conversionOk;
            const unsigned gid = tiles.at(y * tileLayer->width() + x)
                    .toUInt(&conversionOk);
            if (!conversionOk) {
                xml.raiseError(
                        tr("Unable to parse tile at (%1,%2) on layer '%3'")
                               .arg(x + 1).arg(y + 1).arg(tileLayer->name()));
                return;
            }
            tileLayer->setCell(x, y, cellForGid(gid));
        }
    }
}

Cell MapReaderPrivate::cellForGid(unsigned gid)
{
    bool ok;
    const Cell result = mGidMapper.gidToCell(gid, ok, mLazy);

    if (!ok) {
        if (mGidMapper.isEmpty())
            xml.raiseError(tr("Tile used but no tilesets specified"));
        else
            xml.raiseError(tr("Invalid tile: %1").arg(gid));
    }

    return result;
}

ObjectGroup *MapReaderPrivate::readObjectGroup()
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QStringLiteral("objectgroup"));

    const QXmlStreamAttributes atts = xml.attributes();
    const QString name = atts.value(QStringLiteral("name")).toString();
    const int x = atts.value(QStringLiteral("x")).toString().toInt();
    const int y = atts.value(QStringLiteral("y")).toString().toInt();
    const int width = atts.value(QStringLiteral("width")).toString().toInt();
    const int height = atts.value(QStringLiteral("height")).toString().toInt();

    ObjectGroup *objectGroup = new ObjectGroup(name, x, y, width, height);
    readLayerAttributes(objectGroup, atts);

    const QString color = atts.value(QStringLiteral("color")).toString();
    if (!color.isEmpty())
        objectGroup->setColor(color);

    while (xml.readNextStartElement()) {
        if (xml.name() == QStringLiteral("object"))
            objectGroup->addObject(readObject());
        else if (xml.name() == QStringLiteral("properties"))
            objectGroup->mergeProperties(readProperties());
        else
            readUnknownElement();
    }

    return objectGroup;
}

ImageLayer *MapReaderPrivate::readImageLayer()
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QStringLiteral("imagelayer"));

    const QXmlStreamAttributes atts = xml.attributes();
    const QString name = atts.value(QStringLiteral("name")).toString();
    const int x = atts.value(QStringLiteral("x")).toString().toInt();
    const int y = atts.value(QStringLiteral("y")).toString().toInt();
    const int width = atts.value(QStringLiteral("width")).toString().toInt();
    const int height = atts.value(QStringLiteral("height")).toString().toInt();

    ImageLayer *imageLayer = new ImageLayer(name, x, y, width, height);
    readLayerAttributes(imageLayer, atts);

    while (xml.readNextStartElement()) {
        if (xml.name() == QStringLiteral("image"))
            readImageLayerImage(imageLayer);
        else if (xml.name() == QStringLiteral("properties"))
            imageLayer->mergeProperties(readProperties());
        else
            readUnknownElement();
    }

    return imageLayer;
}

void MapReaderPrivate::readImageLayerImage(ImageLayer *imageLayer)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QStringLiteral("image"));

    const QXmlStreamAttributes atts = xml.attributes();
    QString source = atts.value(QStringLiteral("source")).toString();
    QString trans = atts.value(QStringLiteral("trans")).toString();

    if (!trans.isEmpty()) {
        if (!trans.startsWith(QStringLiteral("#")))
            trans.prepend(QStringLiteral("#"));
        imageLayer->setTransparentColor(QColor(trans));
    }

    source = p->resolveReference(source, mPath);

    // TODO: Support image layers in lazy mode
    const QImage imageLayerImage = p->readExternalImage(source);
    if (!imageLayer->loadFromImage(imageLayerImage, source))
        xml.raiseError(tr("Error loading image layer image:\n'%1'").arg(source));

    xml.skipCurrentElement();
}

static QPointF pixelToTileCoordinates(Map *map, int x, int y)
{
    const int tileHeight = map->tileHeight();
    const int tileWidth = map->tileWidth();

    if (map->orientation() == Map::Isometric) {
        // Isometric needs special handling, since the pixel values are based
        // solely on the tile height.
        return QPointF((qreal) x / tileHeight,
                       (qreal) y / tileHeight);
    } else {
        return QPointF((qreal) x / tileWidth,
                       (qreal) y / tileHeight);
    }
}

MapObject *MapReaderPrivate::readObject()
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QStringLiteral("object"));

    const QXmlStreamAttributes atts = xml.attributes();
    const QString name = atts.value(QStringLiteral("name")).toString();
    const unsigned gid = atts.value(QStringLiteral("gid")).toString().toUInt();
    const int x = atts.value(QStringLiteral("x")).toString().toInt();
    const int y = atts.value(QStringLiteral("y")).toString().toInt();
    const int width = atts.value(QStringLiteral("width")).toString().toInt();
    const int height = atts.value(QStringLiteral("height")).toString().toInt();
    const QString type = atts.value(QStringLiteral("type")).toString();
    const QStringRef visibleRef = atts.value(QStringLiteral("visible"));

    const QPointF pos = pixelToTileCoordinates(mMap, x, y);
    const QPointF size = pixelToTileCoordinates(mMap, width, height);

    MapObject *object = new MapObject(name, type, pos, QSizeF(size.x(),
                                                              size.y()));

    bool ok;
    const qreal rotation = atts.value(QStringLiteral("rotation")).toString().toDouble(&ok);
    if (ok)
        object->setRotation(rotation);

    if (gid)
        object->setCell(cellForGid(gid));

    const int visible = visibleRef.toString().toInt(&ok);
    if (ok)
        object->setVisible(visible);

    while (xml.readNextStartElement()) {
        if (xml.name() == QStringLiteral("properties")) {
            object->mergeProperties(readProperties());
        } else if (xml.name() == QStringLiteral("polygon")) {
            object->setPolygon(readPolygon());
            object->setShape(MapObject::Polygon);
        } else if (xml.name() == QStringLiteral("polyline")) {
            object->setPolygon(readPolygon());
            object->setShape(MapObject::Polyline);
        } else if (xml.name() == QStringLiteral("ellipse")) {
            xml.skipCurrentElement();
            object->setShape(MapObject::Ellipse);
        } else {
            readUnknownElement();
        }
    }

    return object;
}

QPolygonF MapReaderPrivate::readPolygon()
{
    Q_ASSERT(xml.isStartElement() && (xml.name() == QStringLiteral("polygon") ||
                                      xml.name() == QStringLiteral("polyline")));

    const QXmlStreamAttributes atts = xml.attributes();
    const QString points = atts.value(QStringLiteral("points")).toString();
    const QStringList pointsList = points.split(QStringLiteral(" "),
                                                QString::SkipEmptyParts);

    QPolygonF polygon;
    bool ok = true;

    foreach (const QString &point, pointsList) {
        const int commaPos = point.indexOf(QStringLiteral(","));
        if (commaPos == -1) {
            ok = false;
            break;
        }

        const int x = point.left(commaPos).toInt(&ok);
        if (!ok)
            break;
        const int y = point.mid(commaPos + 1).toInt(&ok);
        if (!ok)
            break;

        polygon.append(pixelToTileCoordinates(mMap, x, y));
    }

    if (!ok)
        xml.raiseError(tr("Invalid points data for polygon"));

    xml.skipCurrentElement();
    return polygon;
}

Properties MapReaderPrivate::readProperties()
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QStringLiteral("properties"));

    Properties properties;

    while (xml.readNextStartElement()) {
        if (xml.name() == QStringLiteral("property"))
            readProperty(&properties);
        else
            readUnknownElement();
    }

    return properties;
}

void MapReaderPrivate::readProperty(Properties *properties)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QStringLiteral("property"));

    const QXmlStreamAttributes atts = xml.attributes();
    QString propertyName = atts.value(QStringLiteral("name")).toString();
    QString propertyValue = atts.value(QStringLiteral("value")).toString();

    while (xml.readNext() != QXmlStreamReader::Invalid) {
        if (xml.isEndElement()) {
            break;
        } else if (xml.isCharacters() && !xml.isWhitespace()) {
            if (propertyValue.isEmpty())
                propertyValue = xml.text().toString();
        } else if (xml.isStartElement()) {
            readUnknownElement();
        }
    }

    properties->insert(propertyName, propertyValue);
}


MapReader::MapReader()
    : d(new MapReaderPrivate(this))
{
}

MapReader::~MapReader()
{
    delete d;
}

Map *MapReader::readMap(QIODevice *device, const QString &path)
{
    return d->readMap(device, path);
}

Map *MapReader::readMap(const QString &fileName)
{
    QFile file(fileName);
    if (!d->openFile(&file))
        return 0;

    return readMap(&file, QFileInfo(fileName).absolutePath());
}

Tileset *MapReader::readTileset(QIODevice *device, const QString &path)
{
    return d->readTileset(device, path);
}

Tileset *MapReader::readTileset(const QString &fileName)
{
    QFile file(fileName);
    if (!d->openFile(&file))
        return 0;

    const QString &fileNameAbsolutePath=QFileInfo(fileName).absolutePath();
    if(Tileset::preloadedTileset.contains(fileNameAbsolutePath))
        return Tileset::preloadedTileset[fileNameAbsolutePath];

    Tileset *tileset = readTileset(&file, fileNameAbsolutePath);
    if (tileset)
        tileset->setFileName(fileName);

    return tileset;
}

void MapReader::setLazy(bool lazy)
{
    d->mLazy = lazy;
}

bool MapReader::isLazy() const
{
    return d->mLazy;
}

QString MapReader::errorString() const
{
    return d->errorString();
}

QString MapReader::resolveReference(const QString &reference,
                                    const QString &mapPath)
{
    if (QDir::isRelativePath(reference))
        return mapPath + QStringLiteral("/") + reference;
    else
        return reference;
}

QImage MapReader::readExternalImage(const QString &source)
{
    return QImage(source);
}

Tileset *MapReader::readExternalTileset(const QString &source,
                                        QString *error)
{
    MapReader reader;

    Tileset *tileset = reader.readTileset(source);
    if (!tileset)
        *error = reader.errorString();
    else
        d->mCreatedTilesets.append(tileset);

    return tileset;
}
