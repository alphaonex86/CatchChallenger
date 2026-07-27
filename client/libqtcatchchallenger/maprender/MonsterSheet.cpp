#include "MonsterSheet.hpp"

#include "../../libcatchchallenger/ClientVariable.hpp"

//see MonsterSheet.hpp for the two sheet layouts. Only this one function looks at
//the image: the skin layout is the one whose tiles are 2:3 (16x24), which no square
//monster sheet can be. Anything else keeps the historic 32x32 grid untouched.
Tiled::SharedTileset MonsterSheet::create(const QString &name,const QImage &image)
{
    if(image.width()%3==0 && image.height()%4==0)
    {
        const int tileWidth=image.width()/3;
        const int tileHeight=image.height()/4;
        if(tileHeight*2==tileWidth*3)
            return Tiled::Tileset::create(name,tileWidth,tileHeight);
    }
    return Tiled::Tileset::create(name,32,32);
}

bool MonsterSheet::isSkinLayout(const Tiled::Tileset * const tileset)
{
    bool isSkinLayout=false;
    if(tileset!=NULL)
    {
        if(tileset->columnCount()==3)
            isSkinLayout=true;
    }
    return isSkinLayout;
}

int MonsterSheet::baseTile(const Tiled::Tileset * const tileset,const CatchChallenger::Direction &direction)
{
    int baseTile=-1;
    if(isSkinLayout(tileset))
    {
        //one direction per row, the idle frame is the middle column: same indexes as a player skin
        switch(direction)
        {
            case CatchChallenger::Direction_look_at_top:
            case CatchChallenger::Direction_move_at_top:
                baseTile=1;
            break;
            case CatchChallenger::Direction_look_at_right:
            case CatchChallenger::Direction_move_at_right:
                baseTile=4;
            break;
            case CatchChallenger::Direction_look_at_bottom:
            case CatchChallenger::Direction_move_at_bottom:
                baseTile=7;
            break;
            case CatchChallenger::Direction_look_at_left:
            case CatchChallenger::Direction_move_at_left:
                baseTile=10;
            break;
            default:
            break;
        }
    }
    else
    {
        //two directions per row pair, the idle frame is the second row of the pair
        switch(direction)
        {
            case CatchChallenger::Direction_look_at_top:
            case CatchChallenger::Direction_move_at_top:
                baseTile=2;
            break;
            case CatchChallenger::Direction_look_at_right:
            case CatchChallenger::Direction_move_at_right:
                baseTile=7;
            break;
            case CatchChallenger::Direction_look_at_bottom:
            case CatchChallenger::Direction_move_at_bottom:
                baseTile=6;
            break;
            case CatchChallenger::Direction_look_at_left:
            case CatchChallenger::Direction_move_at_left:
                baseTile=3;
            break;
            default:
            break;
        }
    }
    return baseTile;
}

int MonsterSheet::walkTile(const Tiled::Tileset * const tileset,const int &baseTile,const bool &stepAlternance)
{
    int walkTile=baseTile-2;
    if(isSkinLayout(tileset))
    {
        //a walk frame sits on each side of the idle one, alternate them like the player does
        if(stepAlternance)
            walkTile=baseTile-1;
        else
            walkTile=baseTile+1;
    }
    return walkTile;
}

qreal MonsterSheet::xOffset(const Tiled::Tileset * const tileset)
{
    qreal offset=0;
    if(tileset!=NULL)
        offset=(qreal)(CLIENT_BASE_TILE_SIZE-tileset->tileWidth())/(CLIENT_BASE_TILE_SIZE*2);
    return offset;
}
