/*
 * tileset.h
 * Copyright 2008-2009, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
 * Copyrigth 2009, Edward Hutchins <eah1@yahoo.com>
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

#ifndef TILESET_H
#define TILESET_H

#include "tiled_object.h"

#include <QColor>
#include <QList>
#include <QVector>
#include <QHash>
#include <QPoint>
#include <QString>
#include <QPixmap>

#include <iostream>
#include <string>

class QImage;

namespace Tiled {

class Tile;
class Terrain;

/**
 * A tileset, representing a set of tiles.
 *
 * This class currently only supports loading tiles from a tileset image, using
 * loadFromImage(). There is no way to add or remove arbitrary tiles.
 */
class Tileset : public Object
{
public:
    static QHash<QString,Tileset *> preloadedTileset;
    /**
     * Constructor.
     *
     * @param name        the name of the tileset
     * @param tileWidth   the width of the tiles in the tileset
     * @param tileHeight  the height of the tiles in the tileset
     * @param tileSpacing the spacing between the tiles in the tileset image
     * @param margin      the margin around the tiles in the tileset image
     */
    Tileset(const QString &name, int tileWidth, int tileHeight,
            int tileSpacing = 0, int margin = 0):
        Object(TilesetType),
        mName(name),
        mTileWidth(tileWidth),
        mTileHeight(tileHeight),
        mTileSpacing(tileSpacing),
        mMargin(margin),
        mImageWidth(0),
        mImageHeight(0),
        mColumnCount(0),
        mTerrainDistancesDirty(false)
    {
        Q_ASSERT(tileSpacing >= 0);
        Q_ASSERT(margin >= 0);
    }

    /**
     * Destructor.
     */
    ~Tileset();

    /**
     * Returns the name of this tileset.
     */
    const QString &name() const { return mName; }

    /**
     * Sets the name of this tileset.
     */
    void setName(const QString &name) { mName = name; }

    /**
     * Returns the file name of this tileset. When the tileset isn't an
     * external tileset, the file name is empty.
     */
    const QString &fileName() const { return mFileName; }

    /**
     * Sets the filename of this tileset.
     */
    void setFileName(const QString &fileName) { mFileName = fileName; }

    /**
     * Returns whether this tileset is external.
     */
    bool isExternal() const { return !mFileName.isEmpty(); }

    /**
     * Returns the maximum width of the tiles in this tileset.
     */
    int tileWidth() const { return mTileWidth; }

    /**
     * Returns the maximum height of the tiles in this tileset.
     */
    int tileHeight() const { return mTileHeight; }

    /**
     * Returns the maximum size of the tiles in this tileset.
     */
    QSize tileSize() const { return QSize(mTileWidth, mTileHeight); }

    /**
     * Returns the spacing between the tiles in the tileset image.
     */
    int tileSpacing() const { return mTileSpacing; }

    /**
     * Returns the margin around the tiles in the tileset image.
     */
    int margin() const { return mMargin; }

    /**
     * Returns the offset that is applied when drawing the tiles in this
     * tileset.
     */
    QPoint tileOffset() const { return mTileOffset; }

    /**
     * @see tileOffset
     */
    void setTileOffset(QPoint offset) { mTileOffset = offset; }

    /**
     * Returns a const reference to the list of tiles in this tileset.
     */
    const QList<Tile*> &tiles() const { return mTiles; }

    /**
     * Returns the tile for the given tile ID.
     * The tile ID is local to this tileset, which means the IDs are in range
     * [0, tileCount() - 1].
     */
    Tile *tileAt(int id) const;

    /**
     * Returns the number of tiles in this tileset.
     */
    int tileCount() const { return mTiles.size(); }

    /**
     * Resizes this tileset, making sure the number of tiles equals
     * \a tileCount.
     *
     * When the tileset is currently larger, existing tiles will
     * be dropped. When it is smaller, the new tiles will be created with a
     * null pixmap.
     *
     * \warning This function is basically only meant to be used by the
     *          MapReader when operating in lazy mode.
     */
    void resize(int tileCount);

    /**
     * Returns the number of tile columns in the tileset image.
     */
    int columnCount() const { return mColumnCount; }

    /**
     * Returns the width of the tileset image.
     */
    int imageWidth() const { return mImageWidth; }

    /**
     * Returns the height of the tileset image.
     */
    int imageHeight() const { return mImageHeight; }

    /**
     * Returns the transparent color, or an invalid color if no transparent
     * color is used.
     */
    QColor transparentColor() const { return mTransparentColor; }

    /**
     * Sets the transparent color. Pixels with this color will be masked out
     * when loadFromImage() is called.
     */
    void setTransparentColor(const QColor &c) { mTransparentColor = c; }

    /**
     * Load this tileset from the given tileset \a image. This will replace
     * existing tile images in this tileset with new ones. If the new image
     * contains more tiles than exist in the tileset new tiles will be
     * appended, if there are fewer tiles the excess images will be blanked.
     *
     * The tile width and height of this tileset must be higher than 0.
     *
     * @param image    the image to load the tiles from
     * @param fileName the file name of the image, which will be remembered
     *                 as the image source of this tileset.
     * @return <code>true</code> if loading was successful, otherwise
     *         returns <code>false</code>
     */
    bool loadFromImage(const QImage &image, const QString &fileName);
    bool loadFromImage(const std::string &fileName);
    /**
     * This checks if there is a similar tileset in the given list.
     * It is needed for replacing this tileset by its similar copy.
     */
    Tileset *findSimilarTileset(const QList<Tileset*> &tilesets) const;

    /**
     * Returns the file name of the external image that contains the tiles in
     * this tileset. Is an empty string when this tileset doesn't have a
     * tileset image.
     */
    const QString &imageSource() const { return mImageSource; }

    /**
     * Sets the file name of the external image that contains the tiles in this
     * tilesets, without actually loading the image.
     *
     * \warning This function is basically only meant to be used by the
     *          MapReader when operating in lazy mode.
     */
    void setImageSource(const QString &source) { mImageSource = source; }

    /**
     * Returns the column count that this tileset would have if the tileset
     * image would have the given \a width. This takes into account the tile
     * size, margin and spacing.
     */
    int columnCountForWidth(int width) const;

    /**
     * Returns a const reference to the list of terrains in this tileset.
     */
    const QList<Terrain*> &terrains() const { return mTerrainTypes; }

    /**
     * Returns the number of terrain types in this tileset.
     */
    int terrainCount() const { return mTerrainTypes.size(); }

    /**
     * Returns the terrain type at the given \a index.
     */
    Terrain *terrain(int index) const { return index >= 0 ? mTerrainTypes[index] : 0; }

    /**
     * Adds a new terrain type.
     *
     * @param name      the name of the terrain
     * @param imageTile the id of the tile that represents the terrain visually
     * @return the created Terrain instance
     */
    Terrain *addTerrain(const QString &name, int imageTileId);

    /**
     * Adds the \a terrain type at the given \a index.
     *
     * The terrain should already have this tileset associated with it.
     */
    void insertTerrain(int index, Terrain *terrain);

    /**
     * Removes the terrain type at the given \a index and returns it. The
     * caller becomes responsible for the lifetime of the terrain type.
     *
     * This will cause the terrain ids of subsequent terrains to shift up to
     * fill the space and the terrain information of all tiles in this tileset
     * will be updated accordingly.
     */
    Terrain *takeTerrainAt(int index);

    /**
     * Returns the transition penalty(/distance) between 2 terrains. -1 if no transition is possible.
     */
    int terrainTransitionPenalty(int terrainType0, int terrainType1);

    /**
     * Add a new tile to the end of the tileset
     */
    void addTile(const QPixmap &image);

    /**
     * Set a tile's image
     */
    void setTileImage(int index, const QPixmap &image);

    /**
     * Used by the Tile class when its terrain information changes.
     */
    void markTerrainDistancesDirty() { mTerrainDistancesDirty = true; }

private:
    /**
     * Detaches from the external image. Should be called everytime the tileset
     * is changed.
     */
    void detachExternalImage();

    /**
     * Sets tile size to the maximum size.
     */
    void updateTileSize();

    /**
     * Calculates the transition distance matrix for all terrain types.
     */
    void recalculateTerrainDistances();

    QString mName;
    QString mFileName;
    QString mImageSource;
    QColor mTransparentColor;
    int mTileWidth;
    int mTileHeight;
    int mTileSpacing;
    int mMargin;
    QPoint mTileOffset;
    int mImageWidth;
    int mImageHeight;
    int mColumnCount;
    QList<Tile*> mTiles;
    QList<Terrain*> mTerrainTypes;
    bool mTerrainDistancesDirty;
};

} // namespace Tiled

#endif // TILESET_H
