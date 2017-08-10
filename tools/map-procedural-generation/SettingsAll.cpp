#include "SettingsAll.h"

#include <iostream>
#include <QFile>
#include <QTextStream>

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
