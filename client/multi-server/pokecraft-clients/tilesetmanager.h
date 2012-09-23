#ifndef TILESETMANAGER_H
#define TILESETMANAGER_H

#include <QtCore>

#include "tileset.h"
#include "Singleton.h"

class tilesetmanager : public Singleton<tilesetmanager>
{
        friend class Singleton<tilesetmanager>;
public:
    tilesetmanager();
    ~tilesetmanager();
    Tiled::Tileset* getTileset(QString filePath = QString(""));
    void addTileset(Tiled::Tileset *tileset);
    void removeTileset(Tiled::Tileset *tileset);
private:
    QList<Tiled::Tileset*>  tilesetlist;
};

//tilesetmanager::tilesetmanager tilesetmanager::mInstance=NULL;

#endif // TILESETMANAGER_H
