#include "Map_client.hpp"

using namespace CatchChallenger;
Map_client::Map_client()
{
    width=0;
    height=0;
    border.bottom.mapIndex=65535;
    border.bottom.x_offset=0;
    border.top.mapIndex=65535;
    border.top.x_offset=0;
    border.left.mapIndex=65535;
    border.left.y_offset=0;
    border.right.mapIndex=65535;
    border.right.y_offset=0;
}
