#ifndef CATCHCHALLENGER_ItemOnMap_H
#define CATCHCHALLENGER_ItemOnMap_H

#include "../GeneralStructures.hpp"

namespace CatchChallenger {

class ItemOnMap
{
public:
    CATCHCHALLENGER_TYPE_ITEM item;
    bool infinite;
    #ifdef CATCHCHALLENGER_CACHE_HPS
    template <class B>
    void serialize(B& buf) const {
        buf << item << infinite;
    }
    template <class B>
    void parse(B& buf) {
        buf >> item >> infinite;
    }
    #endif
};

}

#endif // MAP_H
