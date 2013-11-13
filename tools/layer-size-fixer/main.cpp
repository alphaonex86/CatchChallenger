#include <QApplication>
#include <QCoreApplication>
#include <QFile>
#include <QDebug>

#include "../../client/tiled/tiled_mapreader.h"
#include "../../client/tiled/tiled_mapwriter.h"
#include "../../client/tiled/tiled_map.h"
#include "../../client/tiled/tiled_layer.h"
#include "../../client/tiled/tiled_tilelayer.h"

int fixLayer(QString file)
{
    Tiled::MapReader reader;
    Tiled::MapWriter write;
    Tiled::Map *map=reader.readMap(file);
    if(map==NULL)
    {
        qDebug() << "Can't read" << file << reader.errorString();
        return 86;
    }
    int index=0;
    while(index<map->layerCount())
    {
        if(map->layerAt(index)->isTileLayer())
            if(map->layerAt(index)->asTileLayer()->width()!=map->width() || map->layerAt(index)->asTileLayer()->height()!=map->height())
                map->layerAt(index)->asTileLayer()->resize(QSize(map->width(),map->height()),QPoint(0,0));
        index++;
    }
    write.setLayerDataFormat(Tiled::Map::Base64Gzip);
    if(!write.writeMap(map,file))
    {
        qDebug() << "Can't write" << file << write.errorString();
        return 87;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QStringList args=QCoreApplication::arguments();
    if(args.size()<=1)
    {
        qDebug() << "Wrong args count (need just on arg, the file to parse)";
        return 85;
    }
    bool haveError=false;
    int index=1;
    while(index<args.size())
    {
        if(fixLayer(args.at(index))!=0)
            haveError=true;
        index++;
    }
    if(haveError)
        return 85;
    else
        return 0;
}
