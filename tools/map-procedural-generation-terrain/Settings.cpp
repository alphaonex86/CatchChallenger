#include "Settings.h"

#include "MapPlants.h"

#include <iostream>

void Settings::putDefaultSettings(QSettings &settings)
{
    //do tile to zone converter
    if(!settings.contains("scale_TerrainMap"))
        settings.setValue("scale_TerrainMap",(double)1.0f);
    if(!settings.contains("scale_TerrainMoisure"))
        settings.setValue("scale_TerrainMoisure",(double)1.0f);
    if(!settings.contains("scale_Zone"))
        settings.setValue("scale_Zone",(double)1.0f);
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
    if(!settings.contains("dotransition"))
        settings.setValue("dotransition",true);
    if(!settings.contains("dovegetation"))
        settings.setValue("dovegetation",true);
    if(!settings.contains("dominimap"))
        settings.setValue("dominimap",true);
    if(!settings.contains("miniMapDivisor"))
        settings.setValue("miniMapDivisor",4.0);
    if(!settings.contains("tileStep"))
        settings.setValue("tileStep",4);

    settings.sync();
}

void Settings::loadSettings(QSettings &settings, unsigned int &mapWidth, unsigned int &mapHeight, unsigned int &mapXCount, unsigned int &mapYCount, unsigned int &seed, bool &displayzone,
                  bool &dotransition, bool &dovegetation, unsigned int &tileStep, float &scale_TerrainMap, float &scale_TerrainMoisure, float &scale_Zone,bool &dominimap, float &miniMapDivisor)
{
    bool ok;
    {
        for(int height=0;height<5;height++)
            for(int moisure=0;moisure<6;moisure++)
            {
                LoadMap::terrainList[height][moisure].tile=NULL;
                LoadMap::terrainList[height][moisure].tileLayer=NULL;
                LoadMap::terrainList[height][moisure].tmp_tileId=0;
                LoadMap::terrainList[height][moisure].outsideBorder=true;
            }
        settings.beginGroup("terrain");
        const QStringList &groupsNames=settings.childGroups();
        unsigned int index=0;
        while(index<(unsigned int)groupsNames.size())
        {
            const QString &terrainName=groupsNames.at(index);
            settings.beginGroup(terrainName);
                const QString &tsx=settings.value("tsx").toString();
                const unsigned int &tileId=settings.value("tileId").toUInt(&ok);
                if(!ok)
                {
                    std::cerr << "tileId not number: " << settings.value("tileId").toString().toStdString() << std::endl;
                    abort();
                }
                const QString &layerString=settings.value("layer").toString();

                QStringList heightmoisurelist=settings.value("heightmoisurelist").toString().split(';');
                unsigned int heightmoisurelistIndex=0;
                while(heightmoisurelistIndex<(unsigned int)heightmoisurelist.size())
                {
                    QStringList heightmoisure=heightmoisurelist.at(heightmoisurelistIndex).split(',');
                    if(heightmoisure.size()!=2)
                    {
                        std::cerr << "heightmoisure.size()!=2" << std::endl;
                        abort();
                    }
                    const unsigned int &height=heightmoisure.at(0).toUInt(&ok);
                    if(!ok)
                    {
                        std::cerr << "height not a number" << std::endl;
                        abort();
                    }
                    if(height>4)
                    {
                        std::cerr << "height not in valid range" << std::endl;
                        abort();
                    }
                    const unsigned int &moisure=heightmoisure.at(1).toUInt(&ok);
                    if(!ok)
                    {
                        std::cerr << "moisure not a number" << std::endl;
                        abort();
                    }
                    if(moisure>6 && moisure<1)
                    {
                        std::cerr << "moisure not in valid range" << std::endl;
                        abort();
                    }

                    //before map creation
                    const bool outsideBorder=settings.value("outsideBorder").toBool();
                    //tempory value
                    const QString &tmp_transition_tsx=settings.value("transition_tsx").toString();
                    std::vector<uint32_t> tmp_transition_tile;
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
                                tmp_transition_tile.push_back(s.toInt(&ok));
                                if(!ok)
                                {
                                    std::cerr << "into transition_tile some is not a number: " << s.toStdString() << " from: " << tmp_transition_tile_settings.toStdString() << " (abort)" << std::endl;
                                    abort();
                                }
                                tmp_transition_tile_index++;
                            }
                        }
                        if(tmp_transition_tile.size()!=12)
                        {
                            std::cerr << "into transition_tile number should be 12, 8 border + 4 curved border (abort)" << std::endl;
                            abort();
                        }
                    }

                    LoadMap::Terrain &terrain=LoadMap::terrainList[height][moisure-1];
                    terrain.tmp_tsx=tsx;
                    terrain.tmp_tileId=tileId;
                    terrain.tmp_layerString=layerString;
                    terrain.terrainName=terrainName;

                    terrain.outsideBorder=outsideBorder;
                    terrain.tmp_transition_tsx=tmp_transition_tsx;
                    terrain.tmp_transition_tile=tmp_transition_tile;

                    heightmoisurelistIndex++;
                }
            settings.endGroup();
            index++;
        }
        settings.endGroup();
    }
    {
        settings.beginGroup("terrainGroup");
        const QStringList &groupsNames=settings.childGroups();
        unsigned int index=0;
        while(index<(unsigned int)groupsNames.size())
        {
            const QString &terrainName=groupsNames.at(index);
            settings.beginGroup(terrainName);
                LoadMap::GroupedTerrain groupedTerrain;
                groupedTerrain.height=0;
                groupedTerrain.tileLayer=NULL;

                const unsigned int &height=settings.value("height").toUInt(&ok);
                if(!ok)
                {
                    std::cerr << "height not a number" << std::endl;
                    abort();
                }
                if(height>4)
                {
                    std::cerr << "height out of range" << std::endl;
                    abort();
                }
                const QString &layerString=settings.value("layer").toString();

                //tempory value
                const QString &tmp_transition_tsx=settings.value("transition_tsx").toString();
                std::vector<uint32_t> tmp_transition_tile;
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
                            tmp_transition_tile.push_back(s.toInt(&ok));
                            if(!ok)
                            {
                                std::cerr << "into transition_tile some is not a number: " << s.toStdString() << " from: " << tmp_transition_tile_settings.toStdString() << " (abort)" << std::endl;
                                abort();
                            }
                            tmp_transition_tile_index++;
                        }
                    }
                    if(tmp_transition_tile.size()!=12)
                    {
                        std::cerr << "into transition_tile number should be 12, 8 border + 4 curved border (abort)" << std::endl;
                        abort();
                    }
                }

                groupedTerrain.height=height;
                groupedTerrain.tmp_layerString=layerString;
                groupedTerrain.tmp_transition_tsx=tmp_transition_tsx;
                groupedTerrain.tmp_transition_tile=tmp_transition_tile;
                LoadMap::groupedTerrainList.push_back(groupedTerrain);

            settings.endGroup();
            index++;
        }
        settings.endGroup();
    }
    {
        settings.beginGroup("plants");
        const QStringList &groupsNames=settings.childGroups();
        unsigned int index=0;
        while(index<(unsigned int)groupsNames.size())
        {
            const QString &terrainName=groupsNames.at(index);
            settings.beginGroup(terrainName);
                const QString &tmx=settings.value("tmx").toString();

                QStringList heightmoisurelist=settings.value("heightmoisurelist").toString().split(';');
                unsigned int heightmoisurelistIndex=0;
                while(heightmoisurelistIndex<(unsigned int)heightmoisurelist.size())
                {
                    QStringList heightmoisure=heightmoisurelist.at(heightmoisurelistIndex).split(',');
                    if(heightmoisure.size()!=2)
                    {
                        std::cerr << "heightmoisure.size()!=2" << std::endl;
                        abort();
                    }
                    const unsigned int &height=heightmoisure.at(0).toUInt(&ok);
                    if(!ok)
                    {
                        std::cerr << "height not a number" << std::endl;
                        abort();
                    }
                    if(height>4)
                    {
                        std::cerr << "height not in valid range" << std::endl;
                        abort();
                    }
                    const unsigned int &moisure=heightmoisure.at(1).toUInt(&ok);
                    if(!ok)
                    {
                        std::cerr << "moisure not a number" << std::endl;
                        abort();
                    }
                    if(moisure>6 && moisure<1)
                    {
                        std::cerr << "moisure not in valid range" << std::endl;
                        abort();
                    }
                    MapPlants::mapPlantsOptions[height][moisure-1].tmx=tmx;
                    heightmoisurelistIndex++;
                }
            settings.endGroup();
            index++;
        }
        settings.endGroup();
    }
    mapWidth=settings.value("mapWidth").toUInt(&ok);
    if(ok==false)
    {
        std::cerr << "mapWidth not number into the config file" << std::endl;
        abort();
    }
    mapHeight=settings.value("mapHeight").toUInt(&ok);
    if(ok==false)
    {
        std::cerr << "mapHeight not number into the config file" << std::endl;
        abort();
    }
    mapXCount=settings.value("mapXCount").toUInt(&ok);
    if(ok==false)
    {
        std::cerr << "mapXCount not number into the config file" << std::endl;
        abort();
    }
    mapYCount=settings.value("mapYCount").toUInt(&ok);
    if(ok==false)
    {
        std::cerr << "mapYCount not number into the config file" << std::endl;
        abort();
    }
    seed=settings.value("seed").toUInt(&ok);
    if(ok==false)
    {
        std::cerr << "seed not number into the config file" << std::endl;
        abort();
    }
    tileStep=settings.value("tileStep").toUInt(&ok);
    if(ok==false || tileStep<2)
    {
        std::cerr << "tileStep not number into the config file or lower than 2" << std::endl;
        abort();
    }
    if(mapWidth%tileStep!=0)
    {
        std::cerr << "width of the map need be a multiple of tileStep: " << std::to_string(tileStep) << std::endl;
        abort();
    }
    if(mapHeight%tileStep!=0)
    {
        std::cerr << "height of the map need be a multiple of tileStep: " << std::to_string(tileStep) << std::endl;
        abort();
    }
    displayzone=settings.value("displayzone").toBool();
    dotransition=settings.value("dotransition").toBool();
    dovegetation=settings.value("dovegetation").toBool();
    dominimap=settings.value("dominimap").toBool();

    scale_TerrainMap=settings.value("scale_TerrainMap").toFloat();
    scale_TerrainMoisure=settings.value("scale_TerrainMoisure").toFloat();
    scale_Zone=settings.value("scale_Zone").toFloat();
    miniMapDivisor=settings.value("miniMapDivisor").toFloat();
}
