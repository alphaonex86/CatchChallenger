#include "MapServerMini.h"
#include "../../client/libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../general/base/GeneralVariable.hpp"

QList<QColor> MapServerMini::colorsList;

MapServerMini::MapServerMini() :
    id(0),
    min_x(0),
    min_y(0),
    max_x(0),
    max_y(0)
{
    this->width=0;
    this->height=0;
}

bool MapServerMini::preload_other_pre()
{
    return true;
}

bool MapServerMini::preload_post_subdatapack()
{
    //items are now stored directly in BaseMap, no index resolution needed
    return true;
}
