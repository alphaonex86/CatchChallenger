#include "../base/ServerStructures.h"
#include "../base/GlobalData.h"

#ifndef POKECRAFT_BASESERVERFIGHT_H
#define POKECRAFT_BASESERVERFIGHT_H

namespace Pokecraft {
class BaseServerFight
{
protected:
    virtual void preload_monsters();
    virtual void preload_skills();
    virtual void preload_buff();
    virtual void unload_buff();
    virtual void unload_skills();
    virtual void unload_monsters();
};
}

#endif
