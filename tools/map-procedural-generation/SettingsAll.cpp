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

    settings.sync();
}

void SettingsAll::loadSettings(QSettings &settings, bool &displaycity, std::vector<std::string> &citiesNames, float &scale_City, bool &doallmap)
{
    displaycity=settings.value("displaycity").toBool();
    scale_City=settings.value("scale_City").toFloat();
    doallmap=settings.value("doallmap").toBool();

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
