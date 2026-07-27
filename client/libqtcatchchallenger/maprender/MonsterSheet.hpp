#ifndef MONSTER_SHEET_H
#define MONSTER_SHEET_H

#include "../libtiled/tileset.h"
#include "../../../general/base/GeneralStructures.hpp"

#include <QImage>
#include <QString>

//monsters/<id>/overworld.png is accepted in two sheet layouts, told apart by the
//image size alone. Everything the renderer needs afterwards is read back from the
//loaded tileset, so this namespace holds ALL the format knowledge: never re-hardcode
//a tile index or the x correction at a call site.
//
// - skin layout, 3 columns x 4 rows of 2:3 tiles (48x96 of 16x24). Exactly the sheet
//   of skin/<x>/trainer.png, so a trainer.png can be copied over as-is: rows are
//   top/right/bottom/left, columns are walk/idle/walk, and BOTH walk frames are used.
// - monster layout, 2 columns x 4 rows of square tiles (64x128 of 32x32), read
//   top-walk/left-walk, top-idle/left-idle, bottom-walk/right-walk,
//   bottom-idle/right-idle. Only one walk frame per direction exists here.
namespace MonsterSheet
{
    //tileset sized for whichever layout the image is in; call loadFromImage() after
    Tiled::SharedTileset create(const QString &name,const QImage &image);
    bool isSkinLayout(const Tiled::Tileset * const tileset);
    //index of the standing frame for a direction, -1 when the direction is not a facing one
    int baseTile(const Tiled::Tileset * const tileset,const CatchChallenger::Direction &direction);
    //index of the walking frame to show next to that standing frame
    int walkTile(const Tiled::Tileset * const tileset,const int &baseTile,const bool &stepAlternance);
    //object x correction, in tiles: the object is anchored on its left edge, so a sheet
    //wider than the map cell has to be pulled back by half the excess to stay centred
    qreal xOffset(const Tiled::Tileset * const tileset);
}

#endif
