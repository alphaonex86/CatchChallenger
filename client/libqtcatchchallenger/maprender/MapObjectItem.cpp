#include "MapObjectItem.hpp"
#include <libtiled/objectgroup.h>

std::unordered_map<Tiled::ObjectGroup *,Tiled::MapRenderer *> MapObjectItem::mRendererList;
std::unordered_map<Tiled::MapObject *,MapObjectItem *> MapObjectItem::objectLink;

MapObjectItem::MapObjectItem(Tiled::MapObject *mapObject,
              QGraphicsItem *parent)
    : QGraphicsItem(parent)
    , mMapObject(mapObject)
{
    //setCacheMode(QGraphicsItem::ItemCoordinateCache);just change direction without move do bug
}

QRectF MapObjectItem::boundingRect() const
{
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
            mMapObject->setSize(mMapObject->cell().tile()->size());
        }
        // Translate the painter to the object's pixel position
        p->translate(xx,yy);
        // Set the translate flag
        translate = true;
    }
    mRendererList.at(mMapObject->objectGroup())->drawMapObject(p, mMapObject,
                             color.isValid() ? color : Qt::transparent);
    if(translate){
        // If the painter was translated, we need to de-translate
        p->translate(-xx,-yy);
        // and restore the object tile position
        mMapObject->setPosition(realPos);
    }
}
