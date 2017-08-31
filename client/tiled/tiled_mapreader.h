/*
 * mapreader.h
 * Copyright 2008-2010, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
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

#ifndef MAPREADER_H
#define MAPREADER_H

#include <QImage>

class QFile;

namespace Tiled {

class Map;
class Tileset;

namespace Internal {
class MapReaderPrivate;
}

/**
 * A fast QXmlStreamReader based reader for the TMX and TSX formats.
 *
 * Can be subclassed when special handling of external images and tilesets is
 * needed.
 */
class MapReader
{
public:
    MapReader();
    ~MapReader();

    /**
     * Reads a TMX map from the given \a device. Optionally a \a path can
     * be given, which will be used to resolve relative references to external
     * images and tilesets.
     *
     * Returns 0 and sets errorString() when reading failed.
     *
     * The caller takes ownership over the newly created map.
     */
    Map *readMap(QIODevice *device, const QString &path = QString());

    /**
     * Reads a TMX map from the given \a fileName.
     * \overload
     */
    Map *readMap(const QString &fileName);

    /**
     * Reads a TSX tileset from the given \a device. Optionally a \a path can
     * be given, which will be used to resolve relative references to external
     * images.
     *
     * Returns 0 and sets errorString() when reading failed.
     *
     * The caller takes ownership over the newly created tileset.
     */
    Tileset *readTileset(QIODevice *device, const QString &path = QString());

    /**
     * Reads a TSX tileset from the given \a fileName.
     * \overload
     */
    Tileset *readTileset(const QString &fileName);

    /**
     * Sets whether this map reader is lazy. By default a map reader is not
     * lazy.
     *
     * A lazy map reader will not attempt to load any external tilesets or
     * images found in the map. Instead it will only create the necessary
     * structures for holding all the map information. The external tilesets
     * and images can be resolved later.
     *
     * Note that this also means that no errors about invalid tile ids will be
     * given, since a lazy reader has no way to know whether an id is invalid.
     */
    void setLazy(bool lazy);

    /**
     * Returns whether this map reader is lazy.
     * @see setLazy
     */
    bool isLazy() const;

    /**
     * Returns the error message for the last occurred error.
     */
    QString errorString() const;

protected:
    /**
     * Called for each \a reference to an external file. Should return the path
     * to be used when loading this file. \a mapPath contains the path to the
     * map or tileset that is currently being loaded.
     */
    virtual QString resolveReference(const QString &reference,
                                     const QString &mapPath);

    /**
     * Called when an external image is encountered while a tileset is loaded.
     */
    virtual QImage readExternalImage(const QString &source);

    /**
     * Called when an external tileset is encountered while a map is loaded.
     * The default implementation just calls readTileset() on a new MapReader.
     *
     * If an error occurred, the \a error parameter should be set to the error
     * message.
     */
    virtual Tileset *readExternalTileset(const QString &source,
                                         QString *error);

public:
    static const QString text_map;
    static const QString text_width;
    static const QString text_height;
    static const QString text_tilewidth;
    static const QString text_tileheight;
    static const QString text_orientation;
    static const QString text_backgroundcolor;
    static const QString text_properties;
    static const QString text_tileset;
    static const QString text_layer;
    static const QString text_objectgroup;
    static const QString text_imagelayer;
    static const QString text_source;
    static const QString text_name;
    static const QString text_spacing;
    static const QString text_margin;
    static const QString text_tile;
    static const QString text_tileoffset;
    static const QString text_x;
    static const QString text_y;
    static const QString text_image;
    static const QString text_terraintypes;
    static const QString text_id;
    static const QString text_comma;
    static const QString text_probability;
    static const QString text_trans;
    static const QString text_dies;
    static const QString text_format;
    static const QString text_data;
    static const QString text_encoding;
    static const QString text_base64;
    static const QString text_firstgid;
    static const QString text_terrain;
    static const QString text_opacity;
    static const QString text_visible;
    static const QString text_compression;
    static const QString text_csv;
    static const QString text_gzip;
    static const QString text_zlib;
    static const QString text_gid;
    static const QString text_color;
    static const QString text_object;
    static const QString text_type;
    static const QString text_rotation;
    static const QString text_polygon;
    static const QString text_polyline;
    static const QString text_ellipse;
    static const QString text_points;
    static const QString text_space;
    static const QString text_property;
    static const QString text_value;
    static const QString text_slash;
private:
    friend class Internal::MapReaderPrivate;
    Internal::MapReaderPrivate *d;
};

} // namespace Tiled

#endif // MAPREADER_H
