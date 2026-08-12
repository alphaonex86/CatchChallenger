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
    struct SettingsExtra: public Settings::Setting
    {
        bool displaycity;
        bool displayregion;
        //[General] cleanTileset: after the run, drop from the staging pool
        //(dest/map/tileset/, dest/map/main/tileset/) every tileset no generated map
        //references. The pool is refilled at each startup from the tool's own
        //tileset/ and from --datapack, so this is never destructive.
        bool cleanTileset;
        //[General] cityDebug: add an Object layer "City" to all.tmx holding, per
        //town, the polygon of the hole it was laid out in and a ONE LINE label
        //with its name, size and key variables.
        bool cityDebug;
        //[General] terrainDebug: add an Object layer "Terrain" holding the OUTLINE
        //polygon of every sea and lake, so what the generator calls a sea can be
        //seen in Tiled.
        bool terrainDebug;

        //[water] a WATER BODY is a connected component of the Water layer.
        //seaMinTiles is what separates a SEA (big enough to sail, and the only
        //thing a water path is routed on) from a LAKE — a pond in a field must
        //never become a shipping lane. bodyDebugMinTiles only filters the debug
        //overlay, so a world map is not buried under puddle outlines.
        unsigned int waterSeaMinTiles;
        //a body of at least lakeMinTiles (and under seaMinTiles) is a LAKE;
        //anything smaller is a puddle and is not named at all
        unsigned int waterLakeMinTiles;
        //debug outlines are traced on a DOWNSAMPLED grid of this many tiles per
        //block: at tile resolution a noisy coastline gives over a million corners
        unsigned int waterBodyDebugStep;
        //[water] how many sea routes to build, as a percent of the number of land
        //roads ("a water path should be X fewer than land"); a chunk is sailable
        //when at least chunkSeaPercent of it is sea; boatPercent of the routes are
        //a closed boat crossing instead of a swimmable channel.
        unsigned int waterPathPercentOfLand;
        unsigned int waterChunkSeaPercent;
        unsigned int waterBoatPercent;
        //how many chunks from a town the coast may be for it to count as a PORT
        unsigned int waterHarbourChunkRadius;
        //A SHORTCUT crossing: two towns already joined by land, but so far apart
        //on the road graph that the player has to tour the whole world to go from
        //one to the other. It is built when the road detour is at least
        //shortcutMinDetour chunks AND the sea route costs at most
        //shortcutMaxPercent of it. Everything else stays forbidden: a sea link
        //between two towns the road already joins closely buys nothing.
        unsigned int waterShortcutMinDetour;
        unsigned int waterShortcutMaxPercent;
        //[water] the CHANNEL painted across a sea chunk: a corridor of water
        //halfWidth tiles either side of the travel axis, walled by borderTile
        //rock. The wall is a CONTINUOUS chain whose position wanders by up to
        //wanderAmplitude tiles, so it does not read as a drawn corridor; what
        //lies beyond it is never reachable.
        QString waterBorderTile;
        unsigned int waterChannelHalfWidth;
        unsigned int waterWanderAmplitude;
        //ISLANDS inside the channel: islandPercent of the chunks get one, of at
        //least islandMinTiles tiles, mountain core, ringed by at most
        //islandSandMax tiles of sand. islandLandablePercent of them are walkable
        //islets, the rest are bare rock scenery.
        unsigned int waterIslandPercent;
        unsigned int waterIslandMinTiles;
        unsigned int waterIslandSandMax;
        unsigned int waterIslandLandablePercent;
        //swimmers met on the channel
        unsigned int waterMinFighter;
        unsigned int waterMaxFighter;
        //the two ships of tileset/ships.tsx, as "tsx/id,width,height" (the sprite
        //is a block of the sheet, stamped straight onto the Collisions layer —
        //there is no tmx for them). shipDecoration is moored in the channel as
        //scenery; shipUsable is the boat of a closed crossing and carries the
        //push-teleport to the far shore.
        QString waterShipDecoration;
        //how many sea chunks get the moored ship: it is scenery, it must stay rare
        unsigned int waterShipDecorationPercent;
        QString waterShipUsable;
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
        //[road] extraSpacePercent*: how much of a road chunk is filled with
        //walkable content (path + grass/water blobs) beyond the minimum needed to
        //join its borders. Low = a simple track crossing an untouched landscape,
        //high = most of the chunk is walkable. Rolled once per chunk with a
        //TRIANGULAR distribution, so most roads sit near the middle of the range
        //and the extremes stay rare; variance scales how far a chunk may go from
        //that middle (0 pins every chunk to it, 100 uses the whole range).
        unsigned int roadExtraSpacePercentMin;
        unsigned int roadExtraSpacePercentMax;
        unsigned int roadExtraSpacePercentVariance;
        unsigned int regionTry;
        unsigned int walkwayTry;
        unsigned int roadRetry;

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

        //[city] small\ medium\ big\: everything that depends on the SIZE of a town,
        //one entry per CityType (index 0 small, 1 medium, 2 big — the LoadMapAll
        //CityType order; SettingsAll cannot include LoadMapAll.h, it is the other
        //way round).
        struct CitySize
        {
            //avenue/plaza ground comes from a template tmx (template/<name>.tmx):
            //its Walkable fill tile is the path terrain, its OnGrass 3x3 ring the
            //border. Empty name = no avenue for that size. useAsBase additionally
            //brushes the template tmx itself, centered, as the city base ground.
            QString templateName;
            bool useAsBase;
            //sign tile styles ("tileset/idx" like fetchTile); ONE style is picked
            //per city for all its signs. Empty list = no signs.
            std::vector<std::string> signTiles;
            //The city HOLE: percent of the chunk, centered, the town is laid out in.
            //A small town spread over the whole 44x44 chunk read as an empty field,
            //so it gets a SMALLER hole and the terrain vegetation grows back around
            //it. Always clamped to leave the vegetation border ring of the chunk.
            unsigned int holePercent;
            //Max share of that hole the building COLLISION footprints may take. A
            //building that would push the total over is denied. Upper limit only —
            //it cannot be hit exactly.
            unsigned int densityPercent;
            //How many buildings to try to place, NOT counting the heal and the
            //market (they are always placed first). Best effort: densityPercent and
            //the free ground still decide.
            unsigned int minBuilding;
        };
        CitySize citySize[3];
        //[city] flatten*: the height/moisure noise under a town is forced to its
        //DOMINANT terrain, so a city is never cut in half by two terrains, then it
        //ramps back to the natural noise over flattenFalloff tiles - the gradient
        //restarts at the town level, so no mountain wall lands against the border.
        //flattenShape: "rectangle" (the whole chunk), "circle" or "octagon"; all
        //three are built as a polygon. flattenMargin shrinks the shape in tiles.
        bool cityFlatten;
        QString cityFlattenShape;
        unsigned int cityFlattenMargin;
        float cityFlattenFalloff;

        //[road] cave\*: percent of road chunks turned into a cave. The overworld
        //keeps its NATURAL terrain — only a small pocket + the cave mouth
        //(entranceTile) appears at each road connection; the walled corridor is a
        //separate <chunk>-cave.tmx interior map reached through the mouth.
        //wallTile may be a comma list of 9 tiles (3x3 repeating block).
        //Some caves go DEEPER (up to maxDepth levels linked by stair tiles, +2
        //monster levels per floor) and may hold ground items (itemTile visual).
        unsigned int cavePercent;
        QString caveWallTile;
        QString caveFloorTile;
        QString caveEntranceTile;//mouth ON a cliff facing bottom
        QString caveEntranceTopTile;//mouth ON a cliff facing top (terra 19 style)
        QString caveExitBottomTile;//interior exit to the bottom, on the ring line, push (terra 402)
        QString caveExitTopTile;//interior exit to the top, walkable gap in the ring, on-it (terra 403)
        QString caveStairDownTile;
        QString caveStairUpTile;
        QString caveItemTile;
        unsigned int caveMaxDepth;
        unsigned int caveItemPercent;
        std::vector<unsigned int> caveItems;

        //[building] gymTypes="type[:#color]->Mon1,Mon2;...": each gym picks a type;
        //some trainer monsters are replaced by that type's pool, and the gym tileset
        //blue parts are recolored with the type color into dest/map/tileset/.
        std::vector<std::string> gymTypeNames;
        std::vector<QString> gymTypeColors;//"" = no recolored tileset for that type
        std::vector<std::vector<std::string> > gymTypeMonsters;
        //[building] cityTypeTerrains="type->terrainKeyword,..;..": a city takes the
        //ELEMENT TYPE whose terrain keywords match its surroundings (water city by
        //the sea, stone in the mountains, plant in the grass...), counts kept
        //balanced; the gym type always matches the city type
        std::vector<std::pair<std::string,std::vector<std::string> > > cityTypeTerrains;
        //[building] cityStyleTerrains="<style>-city->terrainKeyword,..;..": which
        //template/ style folder the filler houses of a city are drawn from. A
        //style folder with no rule is used as a random fallback.
        std::vector<std::pair<std::string,std::vector<std::string> > > cityStyleTerrains;
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
