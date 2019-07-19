/*
 * tile.h
 * Copyright 2008-2012, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
 * Copyright 2009, Edward Hutchins <eah1@yahoo.com>
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

#ifndef TILE_H
#define TILE_H

#include "tiled_object.h"
#include "tiled_tileset.h"

#include <QPixmap>

namespace Tiled {

/**
 * Returns the given \a terrain with the \a corner modified to \a terrainId.
 */
inline unsigned setTerrainCorner(unsigned terrain, int corner, int terrainId)
{
    unsigned mask = 0xFF << (3 - corner) * 8;
    unsigned insert = terrainId << (3 - corner) * 8;
    return (terrain & ~mask) | (insert & mask);
}

class Tile : public Object
{
public:
    Tile(const QPixmap &image, int id, Tileset *tileset):
        Object(TileType),
        mId(id),
        mTileset(tileset),
        mImage(image),
        mTerrain(-1),
        mTerrainProbability(-1.f)
    {}

    /**
     * Returns ID of this tile within its tileset.
     */
    int id() const { return mId; }

    /**
     * Returns the tileset that this tile is part of.
     */
    Tileset *tileset() const { return mTileset; }

    /**
     * Returns the image of this tile.
     */
    const QPixmap &image() const { return mImage; }

    /**
     * Sets the image of this tile.
     */
    void setImage(const QPixmap &image) { mImage = image; }

    // TODO: Methods below now returns tileset's tile width, which is a
    // temporary hack to work around problems with lazy-loaded tiles leading to
    // not updating the max tile size of the layer properly.

    /**
     * Returns the width of this tile.
     */
    int width() const { return mTileset->tileWidth(); }

    /**
     * Returns the height of this tile.
     */
    int height() const { return mTileset->tileHeight(); }

    /**
     * Returns the size of this tile.
     */
    QSize size() const { return mTileset->tileSize(); }

    /**
     * Returns the Terrain of a given corner.
     */
    Terrain *terrainAtCorner(int corner) const;

    /**
     * Returns the terrain id at a given corner.
     */
    int cornerTerrainId(int corner) const { unsigned t = (terrain() >> (3 - corner)*8) & 0xFF; return t == 0xFF ? -1 : (int)t; }

    /**
     * Set the terrain type of a given corner.
     */
    void setCornerTerrain(int corner, int terrainId)
    { setTerrain(setTerrainCorner(mTerrain, corner, terrainId)); }

    /**
     * Returns the terrain for each corner of this tile.
     */
    unsigned terrain() const { return mTerrain; }

    /**
     * Set the terrain for each corner of the tile.
     */
    void setTerrain(unsigned terrain);

    /**
     * Returns the probability of this terrain type appearing while painting (0-100%).
     */
    float terrainProbability() const { return mTerrainProbability; }

    /**
     * Set the probability of this terrain type appearing while painting (0-100%).
     */
    void setTerrainProbability(float probability) { mTerrainProbability = probability; }

private:
    int mId;
    Tileset *mTileset;
    QPixmap mImage;
    unsigned mTerrain;
    float mTerrainProbability;
};

} // namespace Tiled

#endif // TILE_H
