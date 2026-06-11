#ifndef SETTINGSALL_H
#define SETTINGSALL_H

#include <QSettings>
#include <vector>
#include <string>
#include <map>
#include <cstdint>

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

        //[city] avenue/plaza ground comes from a template tmx (template/<name>.tmx):
        //its Walkable fill tile is the path terrain, its OnGrass 3x3 ring the border.
        //Empty template name = no avenue for that city size. useAsBase additionally
        //brushes the template tmx itself, centered, as the city base ground.
        QString cityBigTemplate;
        bool cityBigUseAsBase;
        QString cityMediumTemplate;
        bool cityMediumUseAsBase;
        //sign tile styles ("tileset/idx" like fetchTile) per city type; ONE style is
        //picked per city for all its signs. Empty list = no signs.
        std::vector<std::string> cityBigSignTiles;
        std::vector<std::string> cityMediumSignTiles;

        //[road] cave\*: percent of road chunks turned into a cave (walled corridor)
        unsigned int cavePercent;
        QString caveWallTile;
        QString caveFloorTile;

        //[building] gymTypes="type[:#color]->Mon1,Mon2;...": each gym picks a type;
        //some trainer monsters are replaced by that type's pool, and the gym tileset
        //blue parts are recolored with the type color into dest/map/tileset/.
        std::vector<std::string> gymTypeNames;
        std::vector<QString> gymTypeColors;//"" = no recolored tileset for that type
        std::vector<std::vector<std::string> > gymTypeMonsters;
        //optional datapack monsters/type.xml: when set, type colors are read from it
        QString typeXml;

        //[wildMonsters] <id>\name=: monster display name; generated xml then uses
        //the lowercase name instead of the numeric id (engine resolves both)
        std::map<uint16_t,std::string> monsterNames;
    };

    static void putDefaultSettings(QSettings &settings);
    static void populateSettings(QSettings &settings, SettingsExtra& config);
};

#endif // SETTINGSALL_H
