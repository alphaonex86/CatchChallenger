#ifndef CATCHCHALLENGER_MAP_CLIENT_H
#define CATCHCHALLENGER_MAP_CLIENT_H

#include "../../general/base/CommonMap/CommonMap.hpp"
#include "../../general/base/GeneralStructures.hpp"
#include "../../general/base/Map_loader.hpp"
#include "../../general/base/GeneralType.hpp"

namespace CatchChallenger {
class Map_client : public CommonMap
{
public:
    Map_semi_border border_semi;
    uint32_t group;

    Map_client();
};
}

#endif // MAP_SERVER_H
