#include "SettingsAll.h"
#include "../map-procedural-generation-terrain/LoadMap.h"

#include <iostream>
#include <QFile>
#include <QTextStream>
#include <QDebug>

void SettingsAll::putDefaultSettings(QSettings &settings)
{
    //do tile to zone converter
    if(!settings.contains("displaycity"))
        settings.setValue("displaycity",false);
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

    settings.beginGroup("wildMonsters");
    settings.beginGroup("0");
    if(!settings.contains("comment"))
        settings.setValue("comment","key is the id, heightmoisurelist entries is height,moisure->mapweight,luckweight");
    if(!settings.contains("heightmoisurelist"))
        settings.setValue("heightmoisurelist","3,1->10,10;3,2->10,10;3,3->10,10;3,4->10,10;3,5->10,10;3,6->10,10");
    settings.endGroup();
    settings.endGroup();

    settings.sync();
}

void SettingsAll::loadSettings(QSettings &settings, bool &displaycity, std::vector<std::string> &citiesNames, float &scale_City, bool &doallmap,
                               unsigned int &maxCityLinks,unsigned int &cityRadius,
                               float &levelmapscale, unsigned int &levelmapmin, unsigned int &levelmapmax)
{
    displaycity=settings.value("displaycity").toBool();
    scale_City=settings.value("scale_City").toFloat();
    doallmap=settings.value("doallmap").toBool();
    maxCityLinks=settings.value("maxCityLinks").toUInt();
    cityRadius=settings.value("cityRadius").toUInt();

    levelmapscale=settings.value("levelmapscale").toFloat();
    levelmapmin=settings.value("levelmapmin").toUInt();
    levelmapmax=settings.value("levelmapmax").toUInt();

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
                    citiesNames.push_back(line.toStdString());
            }
            inputFile.close();
        }
    }
}
