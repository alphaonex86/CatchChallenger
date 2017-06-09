#include <QApplication>
#include <QSettings>
#include <QTime>
#include <iostream>

#include "../../client/tiled/tiled_mapwriter.h"
#include "../../client/tiled/tiled_mapobject.h"
#include "../../client/tiled/tiled_objectgroup.h"
#include "../../client/tiled/tiled_tile.h"

#include "znoise/headers/Simplex.hpp"
#include "VoronioForTiledMapTmx.h"
#include "LoadMap.h"
#include "TransitionTerrain.h"

#include <unordered_set>

/*To do: Tree/Grass, Rivers
http://www-cs-students.stanford.edu/~amitp/game-programming/polygon-map-generation/*/
/*template<typename T, size_t C, size_t R>  Matrix { std::array<T, C*R> };
 * operator[](int c, int r) { return arr[C*r+c]; }*/

struct MapTemplate
{
    const Tiled::Map * tiledMap;
    std::vector<uint8_t> templateLayerNumberToMapLayerNumber;
    uint8_t width,height,x,y;
};

void loadTypeToMap(std::vector</*heigh*/std::vector</*moisure*/MapTemplate> > &templateResolver,
                   const unsigned int heigh/*heigh, starting at 0*/,
                   const unsigned int moisure/*moisure, starting at 0*/,
                   const MapTemplate &templateMap
                   )
{
    if(templateResolver.size()<4)
    {
        MapTemplate mapTemplate;
        mapTemplate.height=0;
        mapTemplate.width=0;
        mapTemplate.tiledMap=NULL;
        mapTemplate.x=0;
        mapTemplate.y=0;
        templateResolver.resize(4);
        for(int i=0;i<4;i++)
        {
            std::fill(templateResolver[i].begin(),templateResolver[i].end(),mapTemplate);
            templateResolver[i].resize(6);
        }
    }
    if(templateResolver.size()<(heigh+1))
        templateResolver.resize((heigh+1));
    if(templateResolver[heigh].size()<(moisure+1))
        templateResolver[heigh].resize((moisure+1));
    templateResolver[heigh][moisure]=templateMap;
}

bool detectBorder(Tiled::TileLayer *layer,MapTemplate *templateMap)
{
    uint8_t min_x=layer->width(),min_y=layer->height(),max_x=0,max_y=0;
    uint8_t y=0;
    while(y<layer->height())
    {
        uint8_t x=0;
        while(x<layer->width())
        {
            if(!layer->cellAt(x,y).isEmpty())
            {
                if(x<min_x)
                    min_x=x;
                if(y<min_y)
                    min_y=y;
                if(x>max_x)
                    max_x=x;
                if(y>max_y)
                    max_y=y;
            }
            x++;
        }
        y++;
    }
    if(max_x<min_x)
        return false;
    templateMap->width=max_x-min_x+1;
    templateMap->height=max_y-min_y+1;
    templateMap->x=min_x;
    templateMap->y=min_y;
    return true;
}

MapTemplate tiledMapToMapTemplate(const Tiled::Map *templateMap,const Tiled::Map &worldMap)
{
    uint8_t lastDimensionLayerUsed;
    MapTemplate returnedVar;
    returnedVar.tiledMap=templateMap;
    returnedVar.templateLayerNumberToMapLayerNumber.resize(templateMap->layerCount());
    std::unordered_set<uint8_t> alreadyBlockedLayer;
    uint8_t templateLayerIndex=0;
    while(templateLayerIndex<templateMap->layerCount())
    {
        Tiled::Layer * layer=templateMap->layerAt(templateLayerIndex);
        if(layer->isTileLayer())
        {
            Tiled::TileLayer * tileLayer=static_cast<Tiled::TileLayer *>(layer);
            //search into the world map
            uint8_t worldLayerIndex=0;
            while(worldLayerIndex<worldMap.layerCount())
            {
                Tiled::Layer * layer=worldMap.layerAt(worldLayerIndex);
                if(layer->isTileLayer())
                {
                    Tiled::TileLayer * worldTileLayer=static_cast<Tiled::TileLayer *>(layer);
                    //search into the world map
                    if(alreadyBlockedLayer.find(worldLayerIndex)==alreadyBlockedLayer.cend() && worldTileLayer->name()==tileLayer->name())
                    {
                        alreadyBlockedLayer.insert(worldLayerIndex);
                        returnedVar.templateLayerNumberToMapLayerNumber[templateLayerIndex]=worldLayerIndex;
                        break;
                    }
                }
                worldLayerIndex++;
            }
            if(worldLayerIndex>=worldMap.layerCount())
            {
                std::cerr << "a template layer is not found: " << tileLayer->name().toStdString() << " (abort)" << std::endl;
                abort();
            }
            if(tileLayer->name()=="Collisions")
            {
                if(detectBorder(tileLayer,&returnedVar))
                    lastDimensionLayerUsed=0;
            }
            else if(tileLayer->name()=="Grass" && lastDimensionLayerUsed>0)
            {
                if(detectBorder(tileLayer,&returnedVar))
                    lastDimensionLayerUsed=1;
            }
            else if(tileLayer->name()=="OnGrass" && lastDimensionLayerUsed>1)
            {
                if(detectBorder(tileLayer,&returnedVar))
                    lastDimensionLayerUsed=2;
            }
        }
        templateLayerIndex++;
    }
    return returnedVar;
}

void brushTheMap(Tiled::Map &worldMap,const Tiled::Map *brush,const MapTemplate &selectedTemplate,const unsigned int x,const unsigned int y)
{
    unsigned int layerIndex=0;
    while(layerIndex<(unsigned int)brush->layerCount())
    {
        const Tiled::Layer * const layer=brush->layerAt(layerIndex);
        if(layer->isTileLayer())
        {
            const Tiled::TileLayer * const castedLayer=static_cast<const Tiled::TileLayer * const>(layer);
            Tiled::TileLayer * const worldLayer=static_cast<Tiled::TileLayer *>(worldMap.layerAt(selectedTemplate.templateLayerNumberToMapLayerNumber.at(layerIndex)));
            unsigned int index_y=0;
            while(index_y<(unsigned int)brush->height())
            {
                unsigned int index_x=0;
                while(index_x<(unsigned int)brush->width())
                {
                    const Tiled::Cell &cell=castedLayer->cellAt(index_x,index_y);
                    if(!cell.isEmpty())
                        worldLayer->setCell(index_x+x,index_y+y,cell);
                    index_x++;
                }
                index_y++;
            }
        }
        layerIndex++;
    }
}

void addVegetation(Tiled::Map &tiledMap,const VoronioForTiledMapTmx::PolygonZoneMap &vd)
{
    /*const Tiled::Map *flowers=LoadMap::readMap("template/flowers.tmx");
    const Tiled::Map *grass=LoadMap::readMap("template/grass.tmx");
    const Tiled::Map *tree1=LoadMap::readMap("template/tree-1.tmx");
    const Tiled::Map *tree3=LoadMap::readMap("template/tree-3.tmx");*/
    const Tiled::Map *tree2=LoadMap::readMap("template/tree-2.tmx");
    MapTemplate t2=tiledMapToMapTemplate(tree2,tiledMap);
    //resolv form zone to template
    std::vector<std::vector</*moisure*/MapTemplate> > templateResolver;
    loadTypeToMap(templateResolver,0,0,t2);

    unsigned int y=0;
    while(y<(unsigned int)tiledMap.height())
    {
        unsigned int x=0;
        while(x<(unsigned int)tiledMap.width())
        {
            //resolve into zone
            const VoronioForTiledMapTmx::PolygonZoneIndex &zoneIndex=vd.tileToPolygonZoneIndex[x+y*tiledMap.width()];
            const VoronioForTiledMapTmx::PolygonZone &zone=vd.zones[zoneIndex.index];
            //resolve into MapTemplate
            if(zone.height>0)
            {
                const MapTemplate &selectedTemplate=templateResolver.at(zone.height-1).at(zone.moisure-1);
                if(selectedTemplate.tiledMap!=NULL)
                {
                    if(selectedTemplate.width==0 || selectedTemplate.height==0)
                        abort();
                    if(x%selectedTemplate.width==0 && y%selectedTemplate.height==0)
                        brushTheMap(tiledMap,selectedTemplate.tiledMap,selectedTemplate,x,y);
                }
            }
            x++;
        }
        y++;
    }
}

int main(int argc, char *argv[])
{
    // Avoid performance issues with X11 engine when rendering objects
#ifdef Q_WS_X11
    QApplication::setGraphicsSystem(QStringLiteral("raster"));
#endif

    QApplication a(argc, argv);
    QTime t;

    a.setOrganizationDomain(QStringLiteral("catchchallenger"));
    a.setApplicationName(QStringLiteral("map-procedural-generation-terrain"));
    a.setApplicationVersion(QStringLiteral("1.0"));

    QSettings settings(QCoreApplication::applicationDirPath()+"/settings.xml",QSettings::NativeFormat);
    settings.beginGroup("terrain");
    /*do the settings table:
        altitude
        heat
        Give type

        For each type:
        tsx
        tile*/
        //do tile to zone converter
        settings.beginGroup("water");
            if(!settings.contains("tsx"))
                settings.setValue("tsx","animation.tsx");
            if(!settings.contains("tile"))
                settings.setValue("tile",0);
        settings.endGroup();
        settings.beginGroup("grass");
            if(!settings.contains("tsx"))
                settings.setValue("tsx","terra.tsx");
            if(!settings.contains("tile"))
                settings.setValue("tile",0);
        settings.endGroup();
        settings.beginGroup("montain");
            if(!settings.contains("tsx"))
                settings.setValue("tsx","terra.tsx");
            if(!settings.contains("tile"))
                settings.setValue("tile",353);
        settings.endGroup();
        settings.beginGroup("transition");
            settings.beginGroup("waterborder");
                if(!settings.contains("from_type"))
                    settings.setValue("from_type","water");
                if(!settings.contains("to_type"))
                    settings.setValue("to_type","grass,grass2");
                if(!settings.contains("transition_tsx"))
                    settings.setValue("transition_tsx","terra.tsx");
                if(!settings.contains("transition_tile"))
                    settings.setValue("transition_tile","184,185,186,218,250,249,248,216,281,282,314,313");
                if(!settings.contains("replace_tile"))
                    settings.setValue("replace_tile",false);
            settings.endGroup();
            settings.beginGroup("monstainborder");
                if(!settings.contains("from_type"))
                    settings.setValue("from_type","mountain");
                if(!settings.contains("to_type"))
                    settings.setValue("to_type","grass");
                if(!settings.contains("transition_tsx"))
                    settings.setValue("transition_tsx","terra.tsx");
                if(!settings.contains("transition_tile"))
                    settings.setValue("transition_tile","320,321,322,354,386,385,384,352,323,324,356,355");//missing moutain piece
                if(!settings.contains("collision_tsx"))
                    settings.setValue("collision_tsx","terra.tsx");
                if(!settings.contains("collision_tile"))
                    settings.setValue("collision_tile","21,22,23,55,87,86,85,53,82,83,115,114");//the mountain borden
                if(!settings.contains("replace_tile"))
                    settings.setValue("replace_tile",true);//put the nearest tile into current (grass replace mountain)
            settings.endGroup();
        settings.endGroup();
    settings.endGroup();
    if(!settings.contains("resize_TerrainMap"))
        settings.setValue("resize_TerrainMap",4);
    if(!settings.contains("resize_TerrainHeat"))
        settings.setValue("resize_TerrainHeat",4);
    if(!settings.contains("scale_TerrainMap"))
        settings.setValue("scale_TerrainMap",(double)0.01f);
    if(!settings.contains("scale_TerrainHeat"))
        settings.setValue("scale_TerrainHeat",(double)0.01f);
    if(!settings.contains("mapWidth"))
        settings.setValue("mapWidth",44);
    if(!settings.contains("mapHeight"))
        settings.setValue("mapHeight",44);
    if(!settings.contains("mapXCount"))
        settings.setValue("mapXCount",3);
    if(!settings.contains("mapYCount"))
        settings.setValue("mapYCount",3);
    if(!settings.contains("seed"))
        settings.setValue("seed",0);
    if(!settings.contains("displayzone"))
        settings.setValue("displayzone",false);

    settings.sync();

    bool ok;
    LoadMap::Terrain grass,water,montain;
    std::vector<LoadMap::TerrainTransition> terrainTransitionList;
    settings.beginGroup("terrain");
        settings.beginGroup("grass");
            grass.tsx=settings.value("tsx").toString();
            grass.tileId=settings.value("tile").toUInt(&ok);
            if(ok==false)
            {
                std::cerr << "tile not number into the config file" << std::endl;
                abort();
            }
            grass.baseX=1;
            grass.baseY=1;
        settings.endGroup();
        settings.beginGroup("water");
            water.tsx=settings.value("tsx").toString();
            water.tileId=settings.value("tile").toUInt(&ok);
            if(ok==false)
            {
                std::cerr << "tile not number into the config file" << std::endl;
                abort();
            }
            water.baseX=4;
            water.baseY=4;
        settings.endGroup();
        settings.beginGroup("montain");
            montain.tsx=settings.value("tsx").toString();
            montain.tileId=settings.value("tile").toUInt(&ok);
            if(ok==false)
            {
                std::cerr << "tile not number into the config file" << std::endl;
                abort();
            }
            montain.baseX=8;
            montain.baseY=8;
        settings.endGroup();
    settings.endGroup();
    const unsigned int mapWidth=settings.value("mapWidth").toUInt(&ok);
    if(ok==false)
    {
        std::cerr << "mapWidth not number into the config file" << std::endl;
        abort();
    }
    const unsigned int mapHeight=settings.value("mapHeight").toUInt(&ok);
    if(ok==false)
    {
        std::cerr << "mapHeight not number into the config file" << std::endl;
        abort();
    }
    const unsigned int mapXCount=settings.value("mapXCount").toUInt(&ok);
    if(ok==false)
    {
        std::cerr << "mapXCount not number into the config file" << std::endl;
        abort();
    }
    const unsigned int mapYCount=settings.value("mapYCount").toUInt(&ok);
    if(ok==false)
    {
        std::cerr << "mapYCount not number into the config file" << std::endl;
        abort();
    }
    const unsigned int seed=settings.value("seed").toUInt(&ok);
    if(ok==false)
    {
        std::cerr << "seed not number into the config file" << std::endl;
        abort();
    }
    srand(seed);
    const bool displayzone=settings.value("displayzone").toBool();
    settings.beginGroup("terrain");
    settings.beginGroup("transition");
        const QStringList &groupsNames=settings.childGroups();
        {
            unsigned int index=0;
            while(index<(unsigned int)groupsNames.size())
            {
                settings.beginGroup(groupsNames.at(index));
                    LoadMap::TerrainTransition terrainTransition;
                    //before map creation
                    terrainTransition.replace_tile=settings.value("replace_tile").toBool();
                    //tempory value
                    terrainTransition.tmp_from_type=LoadMap::stringToTerrainInt(settings.value("from_type").toString());
                    {
                        const QStringList &to_type_list=settings.value("to_type").toString().split(',');
                        unsigned int to_type_index=0;
                        while(to_type_index<(unsigned int)to_type_list.size())
                        {
                            terrainTransition.tmp_to_type.push_back(LoadMap::stringToTerrainInt(to_type_list.at(to_type_index)));
                            to_type_index++;
                        }
                    }
                    terrainTransition.tmp_transition_tsx=settings.value("transition_tsx").toString();
                    {
                        bool ok;
                        const QString &tmp_transition_tile_settings=settings.value("transition_tile").toString();
                        if(!tmp_transition_tile_settings.isEmpty())
                        {
                            const QStringList &tmp_transition_tile_list=tmp_transition_tile_settings.split(',');
                            unsigned int tmp_transition_tile_index=0;
                            while(tmp_transition_tile_index<(unsigned int)tmp_transition_tile_list.size())
                            {
                                const QString &s=tmp_transition_tile_list.at(tmp_transition_tile_index);
                                terrainTransition.tmp_transition_tile.push_back(s.toInt(&ok));
                                if(!ok)
                                {
                                    std::cerr << "into transition_tile some is not a number: " << s.toStdString() << " from: " << tmp_transition_tile_settings.toStdString() << " (abort)" << std::endl;
                                    abort();
                                }
                                tmp_transition_tile_index++;
                            }
                        }
                        while(terrainTransition.tmp_transition_tile.size()<=12)
                            terrainTransition.tmp_transition_tile.push_back(-1);
                    }
                    terrainTransition.tmp_collision_tsx=settings.value("collision_tsx").toString();
                    {
                        bool ok;
                        const QString &tmp_collision_tile_settings=settings.value("collision_tile").toString();
                        if(!tmp_collision_tile_settings.isEmpty())
                        {
                            const QStringList &tmp_collision_tile_list=tmp_collision_tile_settings.split(',');
                            unsigned int tmp_transition_tile_index=0;
                            while(tmp_transition_tile_index<(unsigned int)tmp_collision_tile_list.size())
                            {
                                const QString &s=tmp_collision_tile_list.at(tmp_transition_tile_index);
                                terrainTransition.tmp_collision_tile.push_back(s.toInt(&ok));
                                if(!ok)
                                {
                                    std::cerr << "into transition_tile some is not a number: " << s.toStdString() << " from: " << tmp_collision_tile_settings.toStdString() << " (abort)" << std::endl;
                                    abort();
                                }
                                tmp_transition_tile_index++;
                            }
                        }
                        while(terrainTransition.tmp_collision_tile.size()<=12)
                            terrainTransition.tmp_collision_tile.push_back(-1);
                    }
                    terrainTransitionList.push_back(terrainTransition);
                settings.endGroup();
                index++;
            }
        }
    settings.endGroup();
    settings.endGroup();

    {
        const unsigned int totalWidth=mapWidth*mapXCount;
        const unsigned int totalHeight=mapHeight*mapYCount;
        t.start();
        const Grid &grid = VoronioForTiledMapTmx::generateGrid(totalWidth,totalHeight,seed,1000);
        qDebug("generateGrid took %d ms", t.elapsed());
        t.start();

        /*Tiled::ObjectGroup *layerPoint=new Tiled::ObjectGroup("Point",0,0,tiledMap.width(),tiledMap.height());
        tiledMap.addLayer(layerPoint);
        layerPoint->setVisible(false);

        for (const auto &p : grid)
        {
            Tiled::MapObject *object = new Tiled::MapObject("P","",QPointF(
                                                                ((float)p.x()/VoronioForTiledMapTmx::SCALE),
                                                                ((float)p.y()/VoronioForTiledMapTmx::SCALE)
                                                                ),
                                                            QSizeF(0.0,0.0));
            object->setPolygon(QPolygonF(QVector<QPointF>()<<QPointF(0.05,0.0)<<QPointF(0.05,0.0)<<QPointF(0.05,0.05)<<QPointF(0.0,0.05)));
            object->setShape(Tiled::MapObject::Polygon);
            layerPoint->addObject(object);
        }
        qDebug("do point took %d ms", t.elapsed());*/

        const float noiseMapScale=0.005f/((mapXCount+mapYCount)/2);
        Simplex heighmap(seed+500);
        Simplex moisuremap(seed+5200);

        t.start();
        VoronioForTiledMapTmx::PolygonZoneMap vd = VoronioForTiledMapTmx::computeVoronoi(grid,totalWidth,totalHeight,2);
        if(vd.zones.size()!=grid.size())
            abort();
        qDebug("computVoronoi took %d ms", t.elapsed());

        {
            Tiled::Map tiledMap(Tiled::Map::Orientation::Orthogonal,totalWidth,totalHeight,16,16);
            QHash<QString,Tiled::Tileset *> cachedTileset;
            LoadMap::loadTileset(water,cachedTileset,tiledMap);LoadMap::loadTileset(grass,cachedTileset,tiledMap);LoadMap::loadTileset(montain,cachedTileset,tiledMap);
            LoadMap::load_terrainTransitionList(grass,water,montain,cachedTileset,terrainTransitionList,tiledMap);
            std::vector<std::vector<Tiled::Tile *> > arrayTerrainTile;
            LoadMap::addTerrainTile(arrayTerrainTile,grass,montain);
            if(displayzone)
            {
                std::vector<std::vector<Tiled::ObjectGroup *> > arrayTerrainPolygon;
                Tiled::ObjectGroup *layerZoneWaterPolygon=LoadMap::addDebugLayer(tiledMap,arrayTerrainPolygon,true);
                std::vector<std::vector<Tiled::ObjectGroup *> > arrayTerrainTile;
                Tiled::ObjectGroup *layerZoneWaterTile=LoadMap::addDebugLayer(tiledMap,arrayTerrainTile,false);
                LoadMap::addPolygoneTerrain(arrayTerrainPolygon,layerZoneWaterPolygon,arrayTerrainTile,layerZoneWaterTile,grid,vd,heighmap,moisuremap,noiseMapScale,tiledMap.width(),tiledMap.height());
            }
            {
                t.start();
                std::vector<std::vector<Tiled::TileLayer *> > arrayTerrain;
                Tiled::TileLayer *layerZoneWater=LoadMap::addTerrainLayer(tiledMap,arrayTerrain);
                LoadMap::addTerrain(arrayTerrain,layerZoneWater,grid,vd,heighmap,moisuremap,noiseMapScale,tiledMap.width(),tiledMap.height(),water,arrayTerrainTile);
                TransitionTerrain::addTransitionOnMap(tiledMap,terrainTransitionList);
                qDebug("Transitions took %d ms", t.elapsed());
            }
            {
                t.start();
                addVegetation(tiledMap,vd);
                qDebug("Vegetation took %d ms", t.elapsed());
            }
            {
                Tiled::ObjectGroup *layerZoneChunk=new Tiled::ObjectGroup("Chunk",0,0,tiledMap.width(),tiledMap.height());
                layerZoneChunk->setColor(QColor("#ffe000"));
                tiledMap.addLayer(layerZoneChunk);

                unsigned int mapY=0;
                while(mapY<mapYCount)
                {
                    unsigned int mapX=0;
                    while(mapX<mapXCount)
                    {
                        Tiled::MapObject *object = new Tiled::MapObject(QString::number(mapX)+","+QString::number(mapY),"",QPointF(0,0), QSizeF(0.0,0.0));
                        object->setPolygon(QPolygonF(QRectF(mapX*mapWidth,mapY*mapHeight,mapWidth,mapHeight)));
                        object->setShape(Tiled::MapObject::Polygon);
                        layerZoneChunk->addObject(object);
                        mapX++;
                    }
                    mapY++;
                }
                layerZoneChunk->setVisible(false);
            }
            Tiled::MapWriter maprwriter;maprwriter.writeMap(&tiledMap,QCoreApplication::applicationDirPath()+"/dest/map/all.tmx");
        }
/* rewrite a real all.tmx split        {
            unsigned int mapY=0;
            while(mapY<mapYCount)
            {
                unsigned int mapX=0;
                while(mapX<mapXCount)
                {
                    Tiled::Map tiledMap(Tiled::Map::Orientation::Orthogonal,mapWidth,mapHeight,16,16);
                    QHash<QString,Tiled::Tileset *> cachedTileset;
                    loadTileset(water,cachedTileset,tiledMap);loadTileset(grass,cachedTileset,tiledMap);loadTileset(montain,cachedTileset,tiledMap);
                    load_terrainTransitionList(grass,water,montain,cachedTileset,terrainTransitionList,tiledMap);
                    if(displayzone)
                    {
                        std::vector<std::vector<Tiled::ObjectGroup *> > arrayTerrainPolygon;
                        Tiled::ObjectGroup *layerZoneWaterPolygon=addDebugLayer(tiledMap,arrayTerrainPolygon,true);
                        std::vector<std::vector<Tiled::ObjectGroup *> > arrayTerrainTile;
                        Tiled::ObjectGroup *layerZoneWaterTile=addDebugLayer(tiledMap,arrayTerrainTile,false);
                        addPolygoneTerrain(arrayTerrainPolygon,layerZoneWaterPolygon,arrayTerrainTile,layerZoneWaterTile,grid,vd,heighmap,moisuremap,noiseMapScale,tiledMap.width(),tiledMap.height(),-(mapWidth*mapX),-(mapHeight*mapY));
                    }
                    Tiled::MapWriter maprwriter;
                    maprwriter.writeMap(&tiledMap,QCoreApplication::applicationDirPath()+"/dest/map/"+QString::number(mapX)+"."+QString::number(mapY)+".tmx");

                    mapX++;
                }
                mapY++;
            }
        }*/
    }

    return 0;
}
