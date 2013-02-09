#include "../base/ServerStructures.h"

#ifndef CATCHCHALLENGER_BASESERVERCRAFTING_H
#define CATCHCHALLENGER_BASESERVERCRAFTING_H

namespace CatchChallenger {
class BaseServerCrafting
{
protected:
    virtual void preload_the_plant();
    virtual void preload_the_plant_on_map();
    virtual void preload_crafting_recipes();
    virtual void preload_shop();
    virtual void unload_the_plant();
    virtual void unload_the_plant_on_map();
    virtual void unload_crafting_recipes();
    virtual void unload_shop();

    virtual void remove_plant_on_map(const QString &map,const quint8 &x,const quint8 &y);
};
}

#endif
