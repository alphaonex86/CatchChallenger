#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QSettings>

#include "../../client/tiled/tiled_mapobject.h"
#include "../../client/tiled/tiled_objectgroup.h"
#include "../../client/base/render/MapVisualiserThread.h"

#include "../../general/base/GeneralStructures.h"
#include "../../general/base/CommonMap.h"
#include "../../client/base/Map_client.h"
#include "../../general/base/Map_loader.h"

#include "../../client/tiled/tiled_isometricrenderer.h"
#include "../../client/tiled/tiled_map.h"
#include "../../client/tiled/tiled_mapobject.h"
#include "../../client/tiled/tiled_mapreader.h"
#include "../../client/tiled/tiled_mapwriter.h"
#include "../../client/tiled/tiled_objectgroup.h"
#include "../../client/tiled/tiled_orthogonalrenderer.h"
#include "../../client/tiled/tiled_tilelayer.h"
#include "../../client/tiled/tiled_tileset.h"

int main(int argc, char *argv[])
{
    // Avoid performance issues with X11 engine when rendering objects
#ifdef Q_WS_X11
    QApplication::setGraphicsSystem(QStringLiteral("raster"));
#endif

    QApplication a(argc, argv);

    a.setOrganizationDomain(QStringLiteral("catchchallenger"));
    a.setApplicationName(QStringLiteral("map-procedural-generation-terrain"));
    a.setApplicationVersion(QStringLiteral("1.0"));

    QSettings settings(QCoreApplication::applicationDirPath()+"/settings.xml",QSettings::NativeFormat);
    settings.beginGroup("terrain");
        settings.beginGroup("grass");
            if(!settings.contains("tsx"))
                settings.setValue("tsx","dest/base.tsx");
            if(!settings.contains("tile"))
                settings.setValue("tile",0);
        settings.endGroup();
    settings.endGroup();

    settings.sync();

    bool ok;
    QString tsx;
    uint32_t tile;
    settings.beginGroup("terrain");
        settings.beginGroup("grass");
            tsx=settings.value("tsx").toString();
            tile=settings.value("tile").toUInt(&ok);
            if(ok==false)
            {
                std::cerr << "tile not number into the config file" << std::endl;
                abort();
            }
        settings.endGroup();
    settings.endGroup();

    QDir mapDir("dest/map/");

    Tiled::Map tiledMap(Tiled::Map::Orientation::Orthogonal,42,42,16,16);
    Tiled::MapReader reader;
    Tiled::Tileset *tilesetBase=reader.readTileset(tsx);
    if(tilesetBase==NULL)
    {
        std::cerr << "File not found: " << QDir::currentPath().toStdString() << "/" << tsx.toStdString() << std::endl;
        abort();
    }
    if(tile>=(uint32_t)tilesetBase->tileCount())
    {
        std::cerr << "tile: " << tile << " too high number for: " << tsx.toStdString() << std::endl;
        abort();
    }
    tiledMap.addTileset(tilesetBase);
    tilesetBase->setFileName(mapDir.relativeFilePath(QDir::currentPath()+"/"+tilesetBase->fileName()));

    //layer
    Tiled::TileLayer *layerWalkable=new Tiled::TileLayer("Walkable",0,0,tiledMap.width(),tiledMap.height());
    tiledMap.addLayer(layerWalkable);

    //fill
    uint32_t y=0;
    while(y<(uint32_t)tiledMap.height())
    {
        uint32_t x=0;
        while(x<(uint32_t)tiledMap.width())
        {
            Tiled::Cell cell;//=layerWalkable.cellAt(x,y)
            cell.tile=tilesetBase->tileAt(tile);
            layerWalkable->setCell(x,y,cell);
            ++x;
        }
        ++y;
    }

    Tiled::MapWriter maprwriter;
    {
        QDir().mkpath("dest/map/");
    }
    maprwriter.writeMap(&tiledMap,"dest/map/test.tmx");

    return 0;
}
