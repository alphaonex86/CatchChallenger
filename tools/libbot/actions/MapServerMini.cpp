#include "MapServerMini.h"
#include "../../client/libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../general/base/GeneralVariable.hpp"

std::vector<MapServerMini::BlockColor> MapServerMini::colorsList;

MapServerMini::BlockColor::BlockColor() :
    r(0),
    g(0),
    b(0)
{
}

MapServerMini::BlockColor::BlockColor(const uint8_t &r,const uint8_t &g,const uint8_t &b) :
    r(r),
    g(g),
    b(b)
{
}

std::string MapServerMini::BlockColor::html() const
{
    static const char hexDigits[]="0123456789abcdef";
    std::string returnedVar("#000000");
    returnedVar[1]=hexDigits[(r>>4)&0x0F];
    returnedVar[2]=hexDigits[r&0x0F];
    returnedVar[3]=hexDigits[(g>>4)&0x0F];
    returnedVar[4]=hexDigits[g&0x0F];
    returnedVar[5]=hexDigits[(b>>4)&0x0F];
    returnedVar[6]=hexDigits[b&0x0F];
    return returnedVar;
}

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
