#include "tilesetmanager.h"

tilesetmanager::tilesetmanager()
{
}

tilesetmanager::~tilesetmanager()
{
        qDeleteAll(tilesetlist);
        tilesetlist.clear();
}

Tiled::Tileset* tilesetmanager::getTileset(QString filePath)
{
        foreach(Tiled::Tileset *tileset,tilesetlist)
        {
                if(tileset->fileName() == filePath)
                {
                        //return tileset;
                }
        }
        return 0;
}
void tilesetmanager::addTileset(Tiled::Tileset *tileset)
{
        if(tileset)
        {
                tilesetlist.append(tileset);
        }
}

void tilesetmanager::removeTileset(Tiled::Tileset *tileset)
{
    if(tileset)
        tilesetlist.removeOne(tileset);
}
