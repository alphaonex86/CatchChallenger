#include "objectgroupitem.h"

ObjectGroupItem::ObjectGroupItem(ObjectGroup *objectGroup,QGraphicsItem *parent) :
    QGraphicsItem(parent)
{
        setFlag(QGraphicsItem::ItemHasNoContents);

        // Create a child item for each object
        foreach (MapObject *object, objectGroup->objects())
                if(object){
                        new ObjectItem(object, this);
                }

}
