#ifndef SETTINGS_H
#define SETTINGS_H

#include <QSettings>
#include <vector>
#include "LoadMap.h"

class Settings
{
public:
    struct Setting
    {
        unsigned int mapWidth;
        unsigned int mapHeight;
        unsigned int mapXCount;
        unsigned int mapYCount;
        unsigned int seed;
        bool displayzone;
        bool dotransition;
        bool dovegetation;
        unsigned int tileStep;
        float scale_TerrainMap;
        float scale_TerrainHeat;
        float scale_TerrainMoisure;
        float scale_Zone;
        bool dominimap;
        float miniMapDivisor;
    };

    static void putDefaultSettings(QSettings &settings);
    static void populateSettings(QSettings &settings, Setting& config);
};

#endif // SETTINGS_H
