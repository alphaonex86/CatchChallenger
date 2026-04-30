#include "MapObjectItem.hpp"
#include "MapItem.hpp"
#include "../libcatchchallenger/ClientVariable.hpp"
#include <objectgroup.h>

#include <tile.h>
std::unordered_map<Tiled::ObjectGroup *,Tiled::MapRenderer *> MapObjectItem::mRendererList;
std::unordered_map<Tiled::MapObject *,MapObjectItem *> MapObjectItem::objectLink;

//async map (un)loading can leave a Tiled::Cell pointing at a Tileset whose
//refcount has dropped to zero (the SharedTileset in Map::mTilesets was the
//last owner, and the cell holds only a raw pointer). Cell::tile() would then
//deref the freed Tileset and crash. Returns true when cell is safe to access.
static bool cellTilesetIsValid(const Tiled::Cell &cell)
{
    if(cell.isEmpty())
        return true;
    Tiled::Tileset *ts=cell.tileset();
    if(ts==nullptr)
        return true;
    return MapItem::validTilesets_.find(ts)!=MapItem::validTilesets_.cend();
}

MapObjectItem::MapObjectItem(Tiled::MapObject *mapObject,
              QGraphicsItem *parent)
    : QGraphicsItem(parent)
    , mMapObject(mapObject)
{
    //setCacheMode(QGraphicsItem::ItemCoordinateCache);just change direction without move do bug
}

QRectF MapObjectItem::boundingRect() const
{
    //paint() draws non-door cell objects with painter translated by
    //(x*CLIENT_BASE_TILE_SIZE,y*CLIENT_BASE_TILE_SIZE) and with bottom-left tile anchor,
    //so the actual painted pixel scene rect is:
    //  (x*TS, y*TS - tileH) .. (x*TS + tileW, y*TS)
    //The existing renderer->boundingRect(mMapObject) returns tile-unit coordinates
    //because positions here are stored in tile-unit. Override to return the correct
    //pixel-unit rect so Qt doesn't cull the sprite out of the view.
    if(!cellTilesetIsValid(mMapObject->cell()))
        return QRectF();
    if(mMapObject->type()!="door" && !mMapObject->cell().isEmpty() && mMapObject->cell().tile()!=nullptr)
    {
        const QSize tileSize=mMapObject->cell().tile()->size();
        const qreal mx=static_cast<qreal>(mMapObject->x())*CLIENT_BASE_TILE_SIZE;
        const qreal my=static_cast<qreal>(mMapObject->y())*CLIENT_BASE_TILE_SIZE;
        return QRectF(mx-1,my-static_cast<qreal>(tileSize.height())-1,
                      static_cast<qreal>(tileSize.width())+2,static_cast<qreal>(tileSize.height())+2);
    }
    Tiled::ObjectGroup * objectGroup=mMapObject->objectGroup();
    if(mRendererList.find(objectGroup)!=mRendererList.cend())
        return mRendererList.at(objectGroup)->boundingRect(mMapObject);
    else
        return QRectF();
}

void MapObjectItem::paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *)
{
    if(!mMapObject->isVisible())
        return;
    //if anymore into a group
    if(mMapObject->objectGroup()==NULL)
        return;

    const QColor &color = mMapObject->objectGroup()->color();

    // Translation position
    float xx,yy;
    // Flag to detranslate after
    bool translate = false;
    // The object real position
    QPointF realPos;

    //skip the entire paint when the cell points at a freed tileset — a stale
    //Tileset* would crash inside Cell::tile() (and downstream drawMapObject).
    if(!cellTilesetIsValid(mMapObject->cell()))
        return;

    // Only doors are properly located
    if(mMapObject->type()!="door"){
        // Tile size (Width and Height) is 16 px
        const int TILE_SIZE = 16;
        // Pixel position is tile position * TILE_SIZE
        xx = (mMapObject->x()*TILE_SIZE);
        yy = (mMapObject->y()*TILE_SIZE);
        // Store the object tile position
        realPos = mMapObject->position();
        // Move the object to 0,0 (the translation fixes the position)
        mMapObject->setPosition(QPointF(0,0));
        // If the object doesn't have a size, we assign one (required by drawMapObject)
        if(mMapObject->size().isNull()){
            if(!mMapObject->cell().isEmpty() && mMapObject->cell().tile()!=nullptr){
                mMapObject->setSize(mMapObject->cell().tile()->size());
            }
            else{
                mMapObject->setPosition(realPos);
                return;
            }
        }
        // Translate the painter to the object's pixel position
        p->translate(xx,yy);
        // Set the translate flag
        translate = true;
    }
    //skip drawing when the cell tile's pixmap is not loaded yet — async map load
    //can deliver the MapObject before its tileset image is ready, and the
    //CellRenderer flush would otherwise drawPixmap a null/invalid QPixmap and crash.
    //Animated tiles use currentFrameTile() which may differ from cell.tile().
    if(!mMapObject->cell().isEmpty() && mMapObject->cell().tile()!=nullptr){
        const Tiled::Tile *renderTile=mMapObject->cell().tile();
        if(renderTile->isAnimated())
            renderTile=renderTile->currentFrameTile();
        if(renderTile==nullptr || renderTile->image().isNull()
           || renderTile->image().width()<=0 || renderTile->image().height()<=0
           || renderTile->imageStatus()!=Tiled::LoadingReady){
            if(translate){
                p->translate(-xx,-yy);
                mMapObject->setPosition(realPos);
            }
            return;
        }
    }
    if(mRendererList.find(mMapObject->objectGroup())==mRendererList.cend()){
        if(translate){
            p->translate(-xx,-yy);
            mMapObject->setPosition(realPos);
        }
        return;
    }
    Tiled::MapObjectColors colors;
    colors.main = color.isValid() ? color : Qt::transparent;
    colors.fill = colors.main;
    mRendererList.at(mMapObject->objectGroup())->drawMapObject(p, mMapObject, colors);
    if(translate){
        // If the painter was translated, we need to de-translate
        p->translate(-xx,-yy);
        // and restore the object tile position
        mMapObject->setPosition(realPos);
    }
}
