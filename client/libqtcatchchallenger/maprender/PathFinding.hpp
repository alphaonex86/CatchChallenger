#ifndef PATHFINDING_H
#define PATHFINDING_H

#include "MapVisualiserOrder.hpp"
#ifndef NOTHREADS
#include <QThread>
#else
#include <QObject>
#endif
#include <QMutex>
#include <unordered_map>
#include <vector>
#include <string>
#include "../../general/base/GeneralStructures.hpp"
#include "../../general/base/GeneralVariable.hpp"
#include "../../general/base/CommonMap/CommonMap.hpp"

//forward decl only: searchPath() takes the visible-NPC map (all_map) by pointer
//to mask bot tiles, but PathFinding must NOT link QMap_client's static storage
//(keeps the isolated testpathfinding binary linkable). The .cpp includes
//QMap_client.hpp to read ->botsDisplay (header-only, no out-of-line symbol).
namespace CatchChallenger { class QMap_client; }

class PathFinding
        #ifndef NOTHREADS
        : public QThread
        #else
        : public QObject
        #endif
{
    Q_OBJECT
public:
    explicit PathFinding();
    virtual ~PathFinding();
    enum PathFinding_status : uint8_t
    {
        PathFinding_status_OK,
        PathFinding_status_PathNotFound,
        PathFinding_status_Canceled,
        PathFinding_status_InternalError,
    };

signals:
    void result(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,const uint8_t &x,const uint8_t &y,const std::vector<std::pair<CatchChallenger::Orientation,uint8_t> > &path,const PathFinding::PathFinding_status &status);
    void internalCancel();
    void emitSearchPath(const CATCHCHALLENGER_TYPE_MAPID &destination_map_index,const uint8_t &destination_x,const uint8_t &destination_y,const CATCHCHALLENGER_TYPE_MAPID &current_map_index,const COORD_TYPE &x,const COORD_TYPE &y,const std::unordered_map<CATCHCHALLENGER_TYPE_ITEM,CATCHCHALLENGER_TYPE_ITEM_QUANTITY> &items);
public slots:
    //all_map (defaulted nullptr) supplies the visible-NPC occupancy so the
    //planner routes AROUND bots (even lookAt="move" bots whose tile stays
    //walkable in flat_simplified_map); passed by pointer to avoid linking
    //QMap_client's static storage into the isolated pathfinding test.
    void searchPath(const std::vector<CatchChallenger::CommonMap> &mapList,const CATCHCHALLENGER_TYPE_MAPID &destination_map_index,
                    const COORD_TYPE &destination_x,const COORD_TYPE &destination_y,const CATCHCHALLENGER_TYPE_MAPID &current_map_index,const COORD_TYPE &x,const COORD_TYPE &y,
                    const std::unordered_map<CATCHCHALLENGER_TYPE_ITEM,CATCHCHALLENGER_TYPE_ITEM_QUANTITY> &items,
                    const std::unordered_map<CATCHCHALLENGER_TYPE_MAPID,CatchChallenger::QMap_client *> *all_map=nullptr);
    void internalSearchPath(const CATCHCHALLENGER_TYPE_MAPID &destination_map_index, const COORD_TYPE &destination_x, const COORD_TYPE &destination_y, const CATCHCHALLENGER_TYPE_MAPID &source_map_index, const COORD_TYPE &source_x, const COORD_TYPE &source_y, const std::unordered_map<CATCHCHALLENGER_TYPE_ITEM,CATCHCHALLENGER_TYPE_ITEM_QUANTITY> &items);
    void cancel();
    #ifdef CATCHCHALLENGER_HARDENED
    static void extraControlOnData(const std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > &controlVar,const CatchChallenger::Orientation &orientation);
    #endif
private:
    struct SimplifiedMapForPathFinding
    {
        /* 255 walkable
         * 254 not walkable
         * 253 ParsedLayerLedges_LedgesBottom
         * 252 ParsedLayerLedges_LedgesTop
         * 251 ParsedLayerLedges_LedgesRight
         * 250 ParsedLayerLedges_LedgesLeft
         * 249 dirt
         * 200 - 248 reserved */
        // RAII owner of the per-map collision grid (x + y*width). Was a raw
        // `uint8_t *` allocated with new[] in searchPath() and freed by hand
        // only on internalSearchPath()'s "path not found" branch -- every other
        // exit (the success path, the many border-crossing early returns, and
        // the next call overwriting the member at searchPath line ~169) leaked
        // it (valgrind: definitely lost, 1020 B per map per pathfind). A vector
        // frees on every path automatically (no manual delete[], no double-free
        // on the shallow struct copy).
        std::vector<uint8_t> simplifiedMap;//to know if have the item

        COORD_TYPE width,height;

        struct Map_Border
        {
            struct Map_BorderContent_TopBottom
            {
                CATCHCHALLENGER_TYPE_MAPID map;
                int32_t x_offset;
            };
            struct Map_BorderContent_LeftRight
            {
                CATCHCHALLENGER_TYPE_MAPID map;
                int32_t y_offset;
            };
            Map_BorderContent_TopBottom top;
            Map_BorderContent_TopBottom bottom;
            Map_BorderContent_LeftRight left;
            Map_BorderContent_LeftRight right;
        };
        Map_Border border;
        struct PathToGo
        {
            std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > left;
            std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > right;
            std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > top;
            std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > bottom;
        };
        std::unordered_map<std::pair<uint8_t,uint8_t>,PathToGo,pairhash> pathToGo;
        std::unordered_set<std::pair<uint8_t,uint8_t>,pairhash> pointQueued;
    };

    struct MapPointToParse
    {
        CATCHCHALLENGER_TYPE_MAPID map;
        COORD_TYPE x,y;
    };

    QMutex mutex;
    std::unordered_map<CATCHCHALLENGER_TYPE_MAPID,SimplifiedMapForPathFinding> simplifiedMapList;
    bool tryCancel;
    //std::vector<QMap_client> qmapList;
    std::vector<CatchChallenger::CommonMap> mapList;
public:
    static bool canGoOn(const SimplifiedMapForPathFinding &simplifiedMapForPathFinding,const COORD_TYPE &x, const COORD_TYPE &y);
    static bool canMove(const CatchChallenger::Orientation &orientation, CATCHCHALLENGER_TYPE_MAPID &current_map_index, COORD_TYPE &x, COORD_TYPE &y, const std::unordered_map<CATCHCHALLENGER_TYPE_MAPID, SimplifiedMapForPathFinding> &simplifiedMapList);
};

#endif // PATHFINDING_H
