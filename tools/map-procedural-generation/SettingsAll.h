#ifndef SETTINGSALL_H
#define SETTINGSALL_H

#include <QSettings>
#include <vector>
#include <string>

#include "../map-procedural-generation-terrain/Settings.h"

class SettingsAll
{
public:
    struct RoomStructure
    {
        QString layer;
        QStringList tiles;
        int offsetX;
        int offsetY;
        int width;
        int height;
    };

    struct Furnitures: public RoomStructure
    {
        QStringList tags;
        QString templatePath;
    };

    struct FurnituresLimitations
    {
        QString tag;
        int min;
        int max;
        float chance;
    };

    struct RoomSetting
    {
        std::vector<Furnitures> furnitures;
        std::vector<FurnituresLimitations> limitations;
        std::vector<RoomStructure> walls;
        QStringList floors;
        QStringList tilesets;
    };

    struct SettingsExtra: public Settings::Setting
    {
        bool displaycity;
        bool displayregion;
        std::vector<std::string> citiesNames;
        float scale_City;
        bool doallmap;
        unsigned int maxCityLinks;
        unsigned int cityRadius;
        float levelmapscale;
        unsigned int levelmapmin;
        unsigned int levelmapmax;
        bool doledge;
        unsigned int ledgeleft;
        unsigned int ledgeright;
        unsigned int ledgebottom;
        float ledgechance;
        QString grass;
        QString walkway;
        QString extratileset;
        float roadWaterChance;
        unsigned int regionTry;
        unsigned int walkwayTry;
        unsigned int roadRetry;

        RoomSetting room;
        std::vector<std::string> npcMessage;

        //key-building / bot generation
        bool doGym;
        unsigned int gymTrainers;
        std::vector<unsigned int> shopItems;
        //Bot sprite skins are datapack-specific NAMES (a folder under skin/bot/
        //or skin/fighter/), NOT numeric indices. botSkins is the random pool for
        //road/house/trainer NPCs; the key-building skins pick a fixed look.
        std::vector<std::string> botSkins;
        std::string healSkin;
        std::string shopSkin;
        std::string gymTrainerSkin;
        std::string gymLeaderSkin;
    };

    static void putDefaultSettings(QSettings &settings);
    static void populateSettings(QSettings &settings, SettingsExtra& config);
};

#endif // SETTINGSALL_H
