#include "Settings.h"

#include "MapPlants.h"

#include <iostream>

void Settings::putDefaultSettings(QSettings &settings)
{
    //do tile to zone converter
    settings.beginGroup("terrain");
        settings.beginGroup("water");
            if(!settings.contains("tsx"))
                settings.setValue("tsx","animation.tsx");
            if(!settings.contains("tileId"))
                settings.setValue("tileId",0);
            if(!settings.contains("heightmoisurelist"))
                settings.setValue("heightmoisurelist","0,1;0,2;0,3;0,4;0,5;0,6");//the index
            if(!settings.contains("layer"))
                settings.setValue("layer","Water");
        settings.endGroup();
        settings.beginGroup("grass");
            if(!settings.contains("tsx"))
                settings.setValue("tsx","terra.tsx");
            if(!settings.contains("tileId"))
                settings.setValue("tileId",0);
            if(!settings.contains("heightmoisurelist"))
                settings.setValue("heightmoisurelist","1,1;1,2;1,3;1,4;1,5;1,6;2,1;2,2;2,3;2,4;2,5;2,6;3,1;3,2;3,3;3,4;3,5;3,6");//the index
            if(!settings.contains("layer"))
                settings.setValue("layer","Walkable");
        settings.endGroup();
        settings.beginGroup("mountain");
            if(!settings.contains("tsx"))
                settings.setValue("tsx","terra.tsx");
            if(!settings.contains("tileId"))
                settings.setValue("tileId",353);
            if(!settings.contains("heightmoisurelist"))
                settings.setValue("heightmoisurelist","4,1;4,2;4,3;4,4;4,5;4,6");//the index
            if(!settings.contains("layer"))
                settings.setValue("layer","Walkable");
        settings.endGroup();
    settings.endGroup();
    settings.beginGroup("plants");
        settings.beginGroup("flowers");
            if(!settings.contains("tmx"))
                settings.setValue("tmx","flowers");
            if(!settings.contains("heightmoisurelist"))
                settings.setValue("heightmoisurelist","1,1");
        settings.endGroup();
        settings.beginGroup("grass");
            if(!settings.contains("tmx"))
                settings.setValue("tmx","grass");
            if(!settings.contains("heightmoisurelist"))
                settings.setValue("heightmoisurelist","1,2");
        settings.endGroup();
        settings.beginGroup("tree-1");
            if(!settings.contains("tmx"))
                settings.setValue("tmx","tree-1");
            if(!settings.contains("heightmoisurelist"))
                settings.setValue("heightmoisurelist","1,3");
        settings.endGroup();
        settings.beginGroup("tree-2");
            if(!settings.contains("tmx"))
                settings.setValue("tmx","tree-2");
            if(!settings.contains("heightmoisurelist"))
                settings.setValue("heightmoisurelist","1,4");
        settings.endGroup();
        settings.beginGroup("tree-3");
            if(!settings.contains("tmx"))
                settings.setValue("tmx","tree-3");
            if(!settings.contains("heightmoisurelist"))
                settings.setValue("heightmoisurelist","1,5;1,6");
        settings.endGroup();
    settings.endGroup();
    settings.beginGroup("transition");
        settings.beginGroup("waterborder");
            if(!settings.contains("from_type"))
                settings.setValue("from_type","water");
            if(!settings.contains("to_type"))
                settings.setValue("to_type","grass");
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
    if(!settings.contains("dotransition"))
        settings.setValue("dotransition",true);
    if(!settings.contains("dovegetation"))
        settings.setValue("dovegetation",true);
    if(!settings.contains("tileStep"))
        settings.setValue("tileStep",4);

    settings.sync();
}

void Settings::loadSettings(QSettings &settings,unsigned int &mapWidth,unsigned int &mapHeight,unsigned int &mapXCount,unsigned int &mapYCount,unsigned int &seed,bool &displayzone,std::vector<LoadMap::TerrainTransition> &terrainTransitionList,
                  bool &dotransition,bool &dovegetation,unsigned int &tileStep)
{
    bool ok;
    {
        for(int height=0;height<5;height++)
            for(int moisure=0;moisure<6;moisure++)
            {
                LoadMap::terrainList[height][moisure].tile=NULL;
                LoadMap::terrainList[height][moisure].tileLayer=NULL;
                LoadMap::terrainList[height][moisure].tileId=0;
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
                    LoadMap::terrainList[height][moisure-1].tsx=tsx;
                    LoadMap::terrainList[height][moisure-1].tileId=tileId;
                    LoadMap::terrainList[height][moisure-1].layerString=layerString;
                    LoadMap::terrainList[height][moisure-1].terrainName=terrainName;
                    heightmoisurelistIndex++;
                }
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
    {
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
                    terrainTransition.tmp_from_type=settings.value("from_type").toString();
                    {
                        const QStringList &to_type_list=settings.value("to_type").toString().split(',');
                        unsigned int to_type_index=0;
                        while(to_type_index<(unsigned int)to_type_list.size())
                        {
                            terrainTransition.tmp_to_type.push_back(to_type_list.at(to_type_index));
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
    }
}
