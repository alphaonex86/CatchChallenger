/*
 * mapwriter.cpp
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

#include "tiled_mapwriter.hpp"
#include "tiled_compression.hpp"
#include "tiled_gidmapper.hpp"
#include "tiled_map.hpp"
#include "tiled_mapobject.hpp"
#include "tiled_imagelayer.hpp"
#include "tiled_objectgroup.hpp"
#include "tiled_tile.hpp"
#include "tiled_tilelayer.hpp"
#include "tiled_tileset.hpp"
#include "tiled_terrain.hpp"

#include <QCoreApplication>
#include <QBuffer>
#include <QDir>
#include <QXmlStreamWriter>
#include <QString>

using namespace Tiled;
using namespace Tiled::Internal;

namespace Tiled {
namespace Internal {

class MapWriterPrivate
{
    Q_DECLARE_TR_FUNCTIONS(MapReader)

public:
    MapWriterPrivate();

    void writeMap(const Map *map, QIODevice *device,
                  const QString &path);

    void writeTileset(const Tileset *tileset, QIODevice *device,
                      const QString &path);

    bool openFile(QFile *file);

    QString mError;
    Map::LayerDataFormat mLayerDataFormat;
    bool mDtdEnabled;

private:
    void writeMap(QXmlStreamWriter &w, const Map *map);
    void writeTileset(QXmlStreamWriter &w, const Tileset *tileset,
                      unsigned firstGid);
    void writeTileLayer(QXmlStreamWriter &w, const TileLayer *tileLayer);
    void writeLayerAttributes(QXmlStreamWriter &w, const Layer *layer);
    void writeObjectGroup(QXmlStreamWriter &w, const ObjectGroup *objectGroup);
    void writeObject(QXmlStreamWriter &w, const MapObject *mapObject);
    void writeImageLayer(QXmlStreamWriter &w, const ImageLayer *imageLayer);
    void writeProperties(QXmlStreamWriter &w,
                         const Properties &properties);

    QDir mMapDir;     // The directory in which the map is being saved
    GidMapper mGidMapper;
    bool mUseAbsolutePaths;
};

} // namespace Internal
} // namespace Tiled


MapWriterPrivate::MapWriterPrivate()
    : mLayerDataFormat(Map::Base64Zlib)
    , mDtdEnabled(false)
    , mUseAbsolutePaths(false)
{
}

bool MapWriterPrivate::openFile(QFile *file)
{
    if (!file->open(QIODevice::WriteOnly)) {
        mError = tr("Could not open file for writing.");
        return false;
    }

    return true;
}

static QXmlStreamWriter *createWriter(QIODevice *device)
{
    QXmlStreamWriter *writer = new QXmlStreamWriter(device);
    writer->setAutoFormatting(true);
    writer->setAutoFormattingIndent(1);
    return writer;
}

void MapWriterPrivate::writeMap(const Map *map, QIODevice *device,
                                const QString &path)
{
    mMapDir = QDir(path);
    mUseAbsolutePaths = path.isEmpty();

    QXmlStreamWriter *writer = createWriter(device);
    writer->writeStartDocument();

    if (mDtdEnabled) {
        writer->writeDTD(QStringLiteral("<!DOCTYPE map SYSTEM \""
                                       "http://mapeditor.org/dtd/1.0/"
                                       "map.dtd\">"));
    }

    writeMap(*writer, map);
    writer->writeEndDocument();
    delete writer;
}

void MapWriterPrivate::writeTileset(const Tileset *tileset, QIODevice *device,
                                    const QString &path)
{
    mMapDir = QDir(path);
    mUseAbsolutePaths = path.isEmpty();

    QXmlStreamWriter *writer = createWriter(device);
    writer->writeStartDocument();

    if (mDtdEnabled) {
        writer->writeDTD(QStringLiteral("<!DOCTYPE tileset SYSTEM \""
                                       "http://mapeditor.org/dtd/1.0/"
                                       "map.dtd\">"));
    }

    writeTileset(*writer, tileset, 0);
    writer->writeEndDocument();
    delete writer;
}

void MapWriterPrivate::writeMap(QXmlStreamWriter &w, const Map *map)
{
    w.writeStartElement(QStringLiteral("map"));

    const QString orientation = orientationToString(map->orientation());

    w.writeAttribute(QStringLiteral("version"), QStringLiteral("1.0"));
    w.writeAttribute(QStringLiteral("orientation"), orientation);
    w.writeAttribute(QStringLiteral("width"), QString::number(map->width()));
    w.writeAttribute(QStringLiteral("height"), QString::number(map->height()));
    w.writeAttribute(QStringLiteral("tilewidth"),
                     QString::number(map->tileWidth()));
    w.writeAttribute(QStringLiteral("tileheight"),
                     QString::number(map->tileHeight()));

    if (map->backgroundColor().isValid()) {
        w.writeAttribute(QStringLiteral("backgroundcolor"),
                         map->backgroundColor().name());
    }

    writeProperties(w, map->properties());

    mGidMapper.clear();
    unsigned firstGid = 1;
    foreach (Tileset *tileset, map->tilesets()) {
        writeTileset(w, tileset, firstGid);
        mGidMapper.insert(firstGid, tileset);
        firstGid += tileset->tileCount();
    }

    foreach (const Layer *layer, map->layers()) {
        const Layer::TypeFlag type = layer->layerType();
        if (type == Layer::TileLayerType)
            writeTileLayer(w, static_cast<const TileLayer*>(layer));
        else if (type == Layer::ObjectGroupType)
            writeObjectGroup(w, static_cast<const ObjectGroup*>(layer));
        else if (type == Layer::ImageLayerType)
            writeImageLayer(w, static_cast<const ImageLayer*>(layer));
    }

    w.writeEndElement();
}

static QString makeTerrainAttribute(const Tile *tile)
{
    QString terrain;
    for (int i = 0; i < 4; ++i ) {
        if (i > 0)
            terrain += QStringLiteral(",");
        int t = tile->cornerTerrainId(i);
        if (t > -1)
            terrain += QString::number(t);
    }
    return terrain;
}

void MapWriterPrivate::writeTileset(QXmlStreamWriter &w, const Tileset *tileset,
                                    unsigned firstGid)
{
    w.writeStartElement(QStringLiteral("tileset"));
    if (firstGid > 0)
        w.writeAttribute(QStringLiteral("firstgid"), QString::number(firstGid));

    const QString &fileName = tileset->fileName();
    if (!fileName.isEmpty()) {
        QString source = fileName;
        if (!mUseAbsolutePaths)
            source = mMapDir.relativeFilePath(source);
        w.writeAttribute(QStringLiteral("source"), source);

        // Tileset is external, so no need to write any of the stuff below
        w.writeEndElement();
        return;
    }

    w.writeAttribute(QStringLiteral("name"), tileset->name());
    w.writeAttribute(QStringLiteral("tilewidth"),
                     QString::number(tileset->tileWidth()));
    w.writeAttribute(QStringLiteral("tileheight"),
                     QString::number(tileset->tileHeight()));
    const int tileSpacing = tileset->tileSpacing();
    const int margin = tileset->margin();
    if (tileSpacing != 0)
        w.writeAttribute(QStringLiteral("spacing"),
                         QString::number(tileSpacing));
    if (margin != 0)
        w.writeAttribute(QStringLiteral("margin"), QString::number(margin));

    const QPoint offset = tileset->tileOffset();
    if (!offset.isNull()) {
        w.writeStartElement(QStringLiteral("tileoffset"));
        w.writeAttribute(QStringLiteral("x"), QString::number(offset.x()));
        w.writeAttribute(QStringLiteral("y"), QString::number(offset.y()));
        w.writeEndElement();
    }

    // Write the tileset properties
    writeProperties(w, tileset->properties());

    // Write the image element
    const QString &imageSource = tileset->imageSource();
    if (!imageSource.isEmpty()) {
        w.writeStartElement(QStringLiteral("image"));
        QString source = imageSource;
        if (!mUseAbsolutePaths)
            source = mMapDir.relativeFilePath(source);
        w.writeAttribute(QStringLiteral("source"), source);

        const QColor transColor = tileset->transparentColor();
        if (transColor.isValid())
            w.writeAttribute(QStringLiteral("trans"), transColor.name().mid(1));

        if (tileset->imageWidth() > 0)
            w.writeAttribute(QStringLiteral("width"),
                             QString::number(tileset->imageWidth()));
        if (tileset->imageHeight() > 0)
            w.writeAttribute(QStringLiteral("height"),
                             QString::number(tileset->imageHeight()));

        w.writeEndElement();
    }

    // Write the terrain types
    if (tileset->terrainCount() > 0) {
        w.writeStartElement(QStringLiteral("terraintypes"));
        for (int i = 0; i < tileset->terrainCount(); ++i) {
            Terrain* t = tileset->terrain(i);
            w.writeStartElement(QStringLiteral("terrain"));

            w.writeAttribute(QStringLiteral("name"), t->name());
            w.writeAttribute(QStringLiteral("tile"), QString::number(t->imageTileId()));

            writeProperties(w, t->properties());

            w.writeEndElement();
        }
        w.writeEndElement();
    }

    // Write the properties for those tiles that have them
    for (int i = 0; i < tileset->tileCount(); ++i) {
        const Tile *tile = tileset->tileAt(i);
        const Properties properties = tile->properties();
        unsigned terrain = tile->terrain();
        float probability = tile->terrainProbability();

        if (!properties.isEmpty() || terrain != 0xFFFFFFFF || probability != -1.f || imageSource.isEmpty()) {
            w.writeStartElement(QStringLiteral("tile"));
            w.writeAttribute(QStringLiteral("id"), QString::number(i));
            if (terrain != 0xFFFFFFFF)
                w.writeAttribute(QStringLiteral("terrain"), makeTerrainAttribute(tile));
            if (probability != -1.f)
                w.writeAttribute(QStringLiteral("probability"), QString::number(static_cast<double>(probability)));
            if (!properties.isEmpty())
                writeProperties(w, properties);
            if (imageSource.isEmpty()) {
                w.writeStartElement(QStringLiteral("image"));
                w.writeAttribute(QStringLiteral("format"),
                                 QStringLiteral("png"));

                w.writeStartElement(QStringLiteral("data"));
                w.writeAttribute(QStringLiteral("encoding"),
                                 QStringLiteral("base64"));

                QBuffer buffer;
                tile->image().save(&buffer, "png");
                w.writeCharacters(QString::fromLatin1(buffer.data().toBase64()));

                w.writeEndElement();

                w.writeEndElement();
            }
            w.writeEndElement();
        }
    }

    w.writeEndElement();
}

void MapWriterPrivate::writeTileLayer(QXmlStreamWriter &w,
                                      const TileLayer *tileLayer)
{
    w.writeStartElement(QStringLiteral("layer"));
    writeLayerAttributes(w, tileLayer);
    writeProperties(w, tileLayer->properties());

    QString encoding;
    QString compression;

    if (mLayerDataFormat == Map::Base64Zstandard) {
        encoding = QStringLiteral("base64");
        compression = QStringLiteral("zstd");
    }
    else
        abort();

    w.writeStartElement(QStringLiteral("data"));
    if (!encoding.isEmpty())
        w.writeAttribute(QStringLiteral("encoding"), encoding);
    if (!compression.isEmpty())
        w.writeAttribute(QStringLiteral("compression"), compression);

    if (mLayerDataFormat == Map::XML) {
        for (int y = 0; y < tileLayer->height(); ++y) {
            for (int x = 0; x < tileLayer->width(); ++x) {
                const unsigned gid = mGidMapper.cellToGid(tileLayer->cellAt(x, y));
                w.writeStartElement(QStringLiteral("tile"));
                w.writeAttribute(QStringLiteral("gid"), QString::number(gid));
                w.writeEndElement();
            }
        }
    } else {
        QByteArray tileData;
        tileData.reserve(tileLayer->height() * tileLayer->width() * 4);

        for (int y = 0; y < tileLayer->height(); ++y) {
            for (int x = 0; x < tileLayer->width(); ++x) {
                const unsigned gid = mGidMapper.cellToGid(tileLayer->cellAt(x, y));
                tileData.append((char) (gid));
                tileData.append((char) (gid >> 8));
                tileData.append((char) (gid >> 16));
                tileData.append((char) (gid >> 24));
            }
        }

        if (mLayerDataFormat == Map::Base64Zstandard)
            tileData = compressData(tileData, Zstandard);

        w.writeCharacters(QStringLiteral("\n   "));
        w.writeCharacters(QString::fromLatin1(tileData.toBase64()));
        w.writeCharacters(QStringLiteral("\n  "));
    }

    w.writeEndElement(); // </data>
    w.writeEndElement(); // </layer>
}

void MapWriterPrivate::writeLayerAttributes(QXmlStreamWriter &w,
                                            const Layer *layer)
{
    w.writeAttribute(QStringLiteral("name"), layer->name());
    w.writeAttribute(QStringLiteral("width"), QString::number(layer->width()));
    w.writeAttribute(QStringLiteral("height"),
                     QString::number(layer->height()));
    const int x = layer->x();
    const int y = layer->y();
    const qreal opacity = static_cast<double>(layer->opacity());
    if (x != 0)
        w.writeAttribute(QStringLiteral("x"), QString::number(x));
    if (y != 0)
        w.writeAttribute(QStringLiteral("y"), QString::number(y));
    if (!layer->isVisible())
        w.writeAttribute(QStringLiteral("visible"), QStringLiteral("0"));
    if (opacity != qreal(1))
        w.writeAttribute(QStringLiteral("opacity"), QString::number(opacity));
}

void MapWriterPrivate::writeObjectGroup(QXmlStreamWriter &w,
                                        const ObjectGroup *objectGroup)
{
    w.writeStartElement(QStringLiteral("objectgroup"));

    if (objectGroup->color().isValid())
        w.writeAttribute(QStringLiteral("color"),
                         objectGroup->color().name());

    writeLayerAttributes(w, objectGroup);
    writeProperties(w, objectGroup->properties());

    foreach (const MapObject *mapObject, objectGroup->objects())
        writeObject(w, mapObject);

    w.writeEndElement();
}

class TileToPixelCoordinates
{
public:
    TileToPixelCoordinates(Map *map)
    {
        if (map->orientation() == Map::Isometric) {
            // Isometric needs special handling, since the pixel values are
            // based solely on the tile height.
            mMultiplierX = map->tileHeight();
            mMultiplierY = map->tileHeight();
        } else {
            mMultiplierX = map->tileWidth();
            mMultiplierY = map->tileHeight();
        }
    }

    QPoint operator() (qreal x, qreal y) const
    {
        return QPoint(qRound(x * mMultiplierX),
                      qRound(y * mMultiplierY));
    }

private:
    int mMultiplierX;
    int mMultiplierY;
};

void MapWriterPrivate::writeObject(QXmlStreamWriter &w,
                                   const MapObject *mapObject)
{
    w.writeStartElement(QStringLiteral("object"));
    const QString &name = mapObject->name();
    const QString &type = mapObject->type();
    if (!name.isEmpty())
        w.writeAttribute(QStringLiteral("name"), name);
    if (!type.isEmpty())
        w.writeAttribute(QStringLiteral("type"), type);

    if (!mapObject->cell().isEmpty()) {
        const unsigned gid = mGidMapper.cellToGid(mapObject->cell());
        w.writeAttribute(QStringLiteral("gid"), QString::number(gid));
    }

    // Convert from tile to pixel coordinates
    const ObjectGroup *objectGroup = mapObject->objectGroup();
    const TileToPixelCoordinates toPixel(objectGroup->map());

    QPoint pos = toPixel(mapObject->x(), mapObject->y());
    QPoint size = toPixel(mapObject->width(), mapObject->height());

    w.writeAttribute(QStringLiteral("x"), QString::number(pos.x()));
    w.writeAttribute(QStringLiteral("y"), QString::number(pos.y()));

    if (size.x() != 0)
        w.writeAttribute(QStringLiteral("width"), QString::number(size.x()));
    if (size.y() != 0)
        w.writeAttribute(QStringLiteral("height"), QString::number(size.y()));

    const qreal rotation = mapObject->rotation();
    if (rotation != 0.0)
        w.writeAttribute(QStringLiteral("rotation"), QString::number(rotation));

    if (!mapObject->isVisible())
        w.writeAttribute(QStringLiteral("visible"), QStringLiteral("0"));

    writeProperties(w, mapObject->properties());

    const QPolygonF &polygon = mapObject->polygon();
    if (!polygon.isEmpty()) {
        if (mapObject->shape() == MapObject::Polygon)
            w.writeStartElement(QStringLiteral("polygon"));
        else
            w.writeStartElement(QStringLiteral("polyline"));

        QString points;
        foreach (const QPointF &point, polygon) {
            const QPoint pos = toPixel(point.x(), point.y());
            points.append(QString::number(pos.x()));
            points.append(QLatin1Char(','));
            points.append(QString::number(pos.y()));
            points.append(QLatin1Char(' '));
        }
        points.chop(1);
        w.writeAttribute(QStringLiteral("points"), points);
        w.writeEndElement();
    }

    if (mapObject->shape() == MapObject::Ellipse)
        w.writeEmptyElement(QStringLiteral("ellipse"));

    w.writeEndElement();
}

void MapWriterPrivate::writeImageLayer(QXmlStreamWriter &w,
                                        const ImageLayer *imageLayer)
{
    w.writeStartElement(QStringLiteral("imagelayer"));
    writeLayerAttributes(w, imageLayer);

    // Write the image element
    const QString &imageSource = imageLayer->imageSource();
    if (!imageSource.isEmpty()) {
        w.writeStartElement(QStringLiteral("image"));
        QString source = imageSource;
        if (!mUseAbsolutePaths)
            source = mMapDir.relativeFilePath(source);
        w.writeAttribute(QStringLiteral("source"), source);

        const QColor transColor = imageLayer->transparentColor();
        if (transColor.isValid())
            w.writeAttribute(QStringLiteral("trans"), transColor.name().mid(1));

        w.writeEndElement();
    }

    writeProperties(w, imageLayer->properties());

    w.writeEndElement();
}

void MapWriterPrivate::writeProperties(QXmlStreamWriter &w,
                                       const Properties &properties)
{
    if (properties.isEmpty())
        return;

    w.writeStartElement(QStringLiteral("properties"));

    Properties::const_iterator it = properties.constBegin();
    Properties::const_iterator it_end = properties.constEnd();
    for (; it != it_end; ++it) {
        w.writeStartElement(QStringLiteral("property"));
        w.writeAttribute(QStringLiteral("name"), it.key());

        const QString &value = it.value();
        if (value.contains(QLatin1Char('\n'))) {
            w.writeCharacters(value);
        } else {
            w.writeAttribute(QStringLiteral("value"), it.value());
        }
        w.writeEndElement();
    }

    w.writeEndElement();
}


MapWriter::MapWriter()
    : d(new MapWriterPrivate)
{
}

MapWriter::~MapWriter()
{
    delete d;
}

void MapWriter::writeMap(const Map *map, QIODevice *device,
                         const QString &path)
{
    d->writeMap(map, device, path);
}

bool MapWriter::writeMap(const Map *map, const QString &fileName)
{
    QFile file(fileName);
    if (!d->openFile(&file))
        return false;

    writeMap(map, &file, QFileInfo(fileName).absolutePath());

    if (file.error() != QFile::NoError) {
        d->mError = file.errorString();
        return false;
    }

    return true;
}

void MapWriter::writeTileset(const Tileset *tileset, QIODevice *device,
                             const QString &path)
{
    d->writeTileset(tileset, device, path);
}

bool MapWriter::writeTileset(const Tileset *tileset, const QString &fileName)
{
    QFile file(fileName);
    if (!d->openFile(&file))
        return false;

    writeTileset(tileset, &file, QFileInfo(fileName).absolutePath());

    if (file.error() != QFile::NoError) {
        d->mError = file.errorString();
        return false;
    }

    return true;
}

QString MapWriter::errorString() const
{
    return d->mError;
}

void MapWriter::setLayerDataFormat(Map::LayerDataFormat format)
{
    d->mLayerDataFormat = format;
}

Map::LayerDataFormat MapWriter::layerDataFormat() const
{
    return d->mLayerDataFormat;
}

void MapWriter::setDtdEnabled(bool enabled)
{
    d->mDtdEnabled = enabled;
}

bool MapWriter::isDtdEnabled() const
{
    return d->mDtdEnabled;
}
