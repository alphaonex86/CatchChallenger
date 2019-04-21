#include "SettingsAll.h"
#include "../map-procedural-generation-terrain/LoadMap.h"
#include "LoadMapAll.h"

#include <iostream>
#include <QFile>
#include <QTextStream>
#include <QDebug>

void SettingsAll::putDefaultSettings(QSettings &settings)
{
    //do tile to zone converter
    if(!settings.contains("displaycity"))
        settings.setValue("displaycity",false);
    if(!settings.contains("displayregion"))
        settings.setValue("displayregion",false);
    if(!settings.contains("scale_City"))
        settings.setValue("scale_City",1.0);
    if(!settings.contains("doallmap"))
        settings.setValue("doallmap",false);
    if(!settings.contains("cityRadius"))
        settings.setValue("cityRadius",3);
    if(!settings.contains("maxCityLinks"))
        settings.setValue("maxCityLinks",3);
    if(!settings.contains("levelmapscale"))
        settings.setValue("levelmapscale",0.05);
    if(!settings.contains("levelmapmin"))
        settings.setValue("levelmapmin",2);
    if(!settings.contains("levelmapmax"))
        settings.setValue("levelmapmax",50);

    settings.beginGroup("road");
    settings.beginGroup("ledge");
    if(!settings.contains("doledge"))
        settings.setValue("doledge",true);
    if(!settings.contains("ledgeleft"))
        settings.setValue("ledgeleft",808);
    if(!settings.contains("ledgeright"))
        settings.setValue("ledgeright",810);
    if(!settings.contains("ledgebottom"))
        settings.setValue("ledgebottom",740);
    if(!settings.contains("ledgechance"))
        settings.setValue("ledgechance",0.7);
    if(!settings.contains("tsx"))
        settings.setValue("tsx","tileset/t1.tsx");
    settings.endGroup();

    settings.beginGroup("bot");
    if(!settings.contains("dobot"))
        settings.setValue("dobot",true);
    if(!settings.contains("maxbot"))
        settings.setValue("maxbot",15);
    if(!settings.contains("maxskin"))
        settings.setValue("maxskin", 100);
    if(!settings.contains("cashbase"))
        settings.setValue("cashbase", 100);
    if(!settings.contains("cashexp"))
        settings.setValue("cashexp", 1.5);
    if(!settings.contains("cashmonster"))
        settings.setValue("cashmonster", 0.2);
    settings.endGroup();

    settings.beginGroup("region");
    if(!settings.contains("grass"))
        settings.setValue("grass", "");
    if(!settings.contains("extratileset"))
        settings.setValue("extratileset", "");
    if(!settings.contains("mountain_terrain"))
        settings.setValue("mountain_terrain", "");
    if(!settings.contains("mountain_layer"))
        settings.setValue("mountain_layer", "Collisions");
    if(!settings.contains("mountain_tile"))
        settings.setValue("mountain_tile", "");
    if(!settings.contains("mountain_tsx"))
        settings.setValue("mountain_tsx", "");
    if(!settings.contains("walkway"))
        settings.setValue("walkway", "");
    if(!settings.contains("waterchance"))
        settings.setValue("waterchance", 0.4);
    if(!settings.contains("initialregion"))
        settings.setValue("initialregion", 300);
    if(!settings.contains("walkwayregion"))
        settings.setValue("walkwayregion", 20);
    if(!settings.contains("retrymax"))
        settings.setValue("retrymax", 500);
    settings.endGroup();
    settings.endGroup();

    settings.beginGroup("room");
    settings.beginGroup("furniture");

    settings.endGroup();
    settings.beginGroup("limitation");

    settings.endGroup();
    settings.beginGroup("walls");

    settings.endGroup();
    settings.endGroup();

    settings.beginGroup("wildMonsters");
    settings.beginGroup("0");
    if(!settings.contains("comment"))
        settings.setValue("comment","key is the id, heightmoisurelist entries is height,moisure->mappercent,luckweight");
    if(!settings.contains("heightmoisurelist"))
        settings.setValue("heightmoisurelist","3,1->100,10;3,2->100,10;3,3->100,10;3,4->100,10;3,5->100,10;3,6->100,10");
    settings.endGroup();
    settings.endGroup();

    settings.sync();
}

void SettingsAll::populateSettings(QSettings &settings, SettingsAll::SettingsExtra& config)
{
    config.displaycity=settings.value("displaycity").toBool();
    config.displayregion=settings.value("displayregion").toBool();
    config.scale_City=settings.value("scale_City").toFloat();
    config.doallmap=settings.value("doallmap").toBool();
    config.maxCityLinks=settings.value("maxCityLinks").toUInt();
    config.cityRadius=settings.value("cityRadius").toUInt();

    config.levelmapscale=settings.value("levelmapscale").toFloat();
    config.levelmapmin=settings.value("levelmapmin").toUInt();
    config.levelmapmax=settings.value("levelmapmax").toUInt();

    settings.beginGroup("road");
    settings.beginGroup("ledge");
    config.doledge=settings.value("doledge").toBool();
    config.ledgeleft=settings.value("ledgeleft").toUInt();
    config.ledgeright=settings.value("ledgeright").toUInt();
    config.ledgebottom=settings.value("ledgebottom").toUInt();
    config.ledgechance=settings.value("ledgechance").toFloat();
    settings.endGroup();

    settings.beginGroup("region");
    config.grass = settings.value("grass").toString();
    config.extratileset = settings.value("extratileset").toString();
    config.walkway = settings.value("walkway").toString();


    config.roadWaterChance=settings.value("waterchance").toFloat();
    config.regionTry=settings.value("initialregion").toUInt();
    config.walkwayTry=settings.value("walkwayregion").toUInt();
    config.roadRetry=settings.value("retrymax").toUInt();

    LoadMapAll::RoadMountain mountain;
    mountain.terrain = settings.value("mountain_terrain").toString();
    mountain.layer = settings.value("mountain_layer").toString();
    mountain.tile = settings.value("mountain_tile").toString();
    mountain.tsx = settings.value("mountain_tsx").toString();
    LoadMapAll::mountain = mountain;
    settings.endGroup();
    settings.endGroup();

    RoomSetting room;
    room.furnitures = std::vector<Furnitures> ();
    room.limitations = std::vector<FurnituresLimitations> ();
    room.walls = std::vector<RoomStructure>();

    settings.beginGroup("room");
    settings.beginGroup("furniture");
    for(QString child: settings.childGroups()){
        settings.beginGroup(child);

        Furnitures f;
        f.layer = settings.value("layer", "Collisions").toString();
        f.offsetX = settings.value("offsetX", 0).toInt();
        f.offsetY = settings.value("offsetY", 0).toInt();
        f.width = settings.value("width", 1).toInt();
        f.tags = settings.value("tags").toString().split(",");
        f.templatePath = settings.value("template").toString();

        if(settings.contains("tiles")){
            f.tiles = settings.value("tiles").toString().split(",");
            f.height = settings.value("height", f.tiles.size()/f.width).toInt();

            if(f.tiles.size() == f.width*f.height)
                room.furnitures.push_back(f);
            else
                std::cout << settings.value("tags").toString().toStdString() << " " << settings.value("tiles").toString().toStdString() << " " << f.layer.toStdString() << " " << f.offsetX << " " << f.offsetY << std::endl;
        }else if(!f.templatePath.isEmpty()){
            f.tiles = QStringList();
            f.height = settings.value("height", 1).toInt();
            room.furnitures.push_back(f);
        }

        settings.endGroup();
    }
    settings.endGroup();
    settings.beginGroup("limitation");
    for(QString child: settings.childGroups()){
        settings.beginGroup(child);

        FurnituresLimitations f;
        f.tag = child;
        f.min = settings.value("min", 1).toUInt();
        f.max = settings.value("max", 1).toUInt();
        f.chance = settings.value("chance", 0.5).toFloat();

        room.limitations.push_back(f);
        settings.endGroup();
    }
    settings.endGroup();
    settings.beginGroup("walls");
    for(QString child: settings.childGroups()){
        settings.beginGroup(child);

        RoomStructure f;
        f.layer = settings.value("layer", "Collisions").toString();
        f.offsetX = settings.value("offsetX", 0).toInt();
        f.offsetY = settings.value("offsetY", 0).toInt();

        if(settings.contains("tiles")){
            f.tiles = settings.value("tiles").toString().split(",");
            f.width = settings.value("width", 1).toInt();
            f.height = settings.value("height", f.tiles.size()/f.width).toInt();

            if(f.tiles.size() == f.width*f.height)
                room.walls.push_back(f);
            else
                std::cout << settings.value("tiles").toString().toStdString() << " " << f.layer.toStdString() << " " << f.offsetX << " " << f.offsetY << std::endl;
        }
        settings.endGroup();
    }
    settings.endGroup();

    room.floors = settings.value("floor").toString().split(",");
    room.tilesets = settings.value("tileset").toString().split(",");
    config.room = room;

    settings.endGroup();

    settings.beginGroup("wildMonsters");
    QStringList monsterId=settings.childGroups();
    {
        bool ok=false;
        unsigned int indexGroupId=0;
        while(indexGroupId<(unsigned int)monsterId.size())
        {
            QString idString=monsterId.at(indexGroupId);
            const uint32_t monsterId=idString.toUInt(&ok);
            if(!ok)
                qDebug() << "Syntaxe error into: idString not number: " << idString;
            if(monsterId!=0)
            {
                settings.beginGroup(idString);
                if(settings.contains("heightmoisurelist"))
                {
                    QStringList heightmoisurelist=settings.value("heightmoisurelist").toString().split(";");
                    unsigned int heightmoisureId=0;
                    while(heightmoisureId<(unsigned int)heightmoisurelist.size())
                    {
                        QString heightmoisureEntry=heightmoisurelist.at(heightmoisureId);
                        heightmoisureEntry.replace("->",",");
                        QStringList heightmoisureSplit=heightmoisureEntry.split(",");
                        if(heightmoisureSplit.size()!=4)
                            qDebug() << "Syntaxe error into: heightmoisurelist: " << settings.value("heightmoisurelist").toString() << ": " << heightmoisureEntry;
                        else
                        {
                            const uint32_t height=heightmoisureSplit.at(0).toUInt(&ok)-1;
                            if(!ok)
                                qDebug() << "Syntaxe error into: height not number: " << heightmoisureSplit.at(0);
                            else
                            {
                                const uint32_t moisure=heightmoisureSplit.at(1).toUInt(&ok)-1;
                                if(!ok)
                                    qDebug() << "Syntaxe error into: moisure not number: " << heightmoisureSplit.at(1);
                                else
                                {
                                    const uint32_t mapweight=heightmoisureSplit.at(2).toUInt(&ok);
                                    if(!ok)
                                        qDebug() << "Syntaxe error into: mapweight not number: " << heightmoisureSplit.at(2);
                                    else
                                    {
                                        const uint32_t luckweight=heightmoisureSplit.at(3).toUInt(&ok);
                                        if(!ok)
                                            qDebug() << "Syntaxe error into: height not number: " << heightmoisureSplit.at(3);
                                        else
                                        {
                                            LoadMap::TerrainMonster terrainMonster;
                                            //terrainMonster.luckweight=luckweight;
                                            terrainMonster.mapweight=mapweight;
                                            terrainMonster.monster=monsterId;
                                            if(height>=5)
                                            {
                                                std::cerr << "height>=5" << std::endl;
                                                abort();
                                            }
                                            if(moisure>=6)
                                            {
                                                std::cerr << "moisure>=6" << std::endl;
                                                abort();
                                            }
                                            std::map<unsigned int,std::vector<LoadMap::TerrainMonster> > &terrainMonsters=LoadMap::terrainList[height][moisure].terrainMonsters;
                                            if(terrainMonsters.find(luckweight)==terrainMonsters.cend())
                                                terrainMonsters.insert(std::pair<unsigned int,std::vector<LoadMap::TerrainMonster> >(luckweight,std::vector<LoadMap::TerrainMonster>()));
                                            std::vector<LoadMap::TerrainMonster> &terrainMonstersVector=terrainMonsters[luckweight];
                                            terrainMonstersVector.push_back(terrainMonster);
                                        }
                                    }
                                }
                            }
                        }
                        heightmoisureId++;
                    }
                }
                settings.endGroup();
            }
            indexGroupId++;
        }
    }
    settings.endGroup();

    {
        QFile inputFile("cities.txt");
        if(inputFile.open(QIODevice::ReadOnly))
        {
            QTextStream in(&inputFile);
            while(!in.atEnd())
            {
                QString line = in.readLine();
                if(!line.isEmpty())
                    config.citiesNames.push_back(line.toStdString());
            }
            inputFile.close();
        }
    }
    {
        QFile inputFile("dialog.txt");
        if(inputFile.open(QIODevice::ReadOnly))
        {
            QTextStream in(&inputFile);
            while(!in.atEnd())
            {
                QString line = in.readLine();
                if(!line.isEmpty())
                    config.npcMessage.push_back(line.toStdString());
            }
            inputFile.close();
        }
    }
}
