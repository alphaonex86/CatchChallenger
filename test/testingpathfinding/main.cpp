// test/testingpathfinding/main.cpp
//
// Pure-C++ unit test for client/libqtcatchchallenger/maprender/
// PathFinding.cpp. No network, no event loop exec, no syscalls beyond
// reading the city.tmx fixture once, no DB, no datapack parsing.
//
// What it reproduces
// ------------------
// In game, clicking a far tile asks PathFinding for a route. The route
// used to be computed with Q_UNUSED(items): the planner walked straight
// through a "walkOn" zone that requires an item the player doesn't
// carry. The client only discovers this WHILE walking and refuses with
//   "You can't enter to this zone without the correct item"
// (MapVisualiserPlayerWithFight::canGoTo -> BlockedOn_ZoneItem).
//
// Test shape
// ----------
// A single fixed CatchChallenger::MapServer object (is-a CommonMap, see
// StubMapServer.hpp) is loaded from city.tmx for realistic dimensions,
// then given a deterministic collision grid: two open halves separated
// by a wall whose ONLY gap is an item-gated walkOn zone (item 4901).
//
//   * GATED   (player has no item): a correct planner must NOT route
//               through the gap -> PathFinding_status_PathNotFound.
//               Returning a path that crosses the gap reproduces the
//               bug -> FAIL.
//   * WITHITEM (player carries 4901): the gap is passable -> a path is
//               found and, replayed step by step, it crosses the gap.
//   * OPEN     (no wall at all): sanity that the planner + this harness
//               agree on a trivially reachable destination.
//
// PathFinding is exercised through its public slots. It is compiled
// here with -DNOTHREADS so it is a plain QObject living in this thread;
// searchPath() fills the internal simplified-map, then we call
// internalSearchPath() directly (same thread) and read the result
// synchronously via a tiny QObject slot — no lambdas (project rule),
// no queued-connection metatype marshalling, no QCoreApplication::exec.

#include <QCoreApplication>
#include <QObject>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
#include <unordered_map>

#include "StubMapServer.hpp"
#include "../../general/base/Map_loader.hpp"
#include "../../general/base/GeneralStructures.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include "../../client/libqtcatchchallenger/maprender/PathFinding.hpp"

static const char *CITY_TMX =
    "/home/user/Desktop/CatchChallenger/CatchChallenger-datapack/map/main/test/city.tmx";

// Item id that gates the zone. 4901 is "Admin key / Tuber" in the
// datapack's quest.xml — value is irrelevant here, only non-zero
// matters (0 == "no item required").
static const CATCHCHALLENGER_TYPE_ITEM GATE_ITEM = 4901;

static int g_pass = 0;
static int g_fail = 0;

static void ok(const std::string &name, const std::string &detail)
{
    std::printf("PASS %s %s\n", name.c_str(), detail.c_str());
    g_pass++;
}
static void ko(const std::string &name, const std::string &detail)
{
    std::printf("FAIL %s %s\n", name.c_str(), detail.c_str());
    g_fail++;
}

// Captures PathFinding::result synchronously.
class ResultCatcher : public QObject
{
    Q_OBJECT
public:
    bool got;
    PathFinding::PathFinding_status status;
    std::vector<std::pair<CatchChallenger::Orientation,uint8_t> > path;
    explicit ResultCatcher() : QObject(NULL), got(false),
        status(PathFinding::PathFinding_status_InternalError) {}
public slots:
    void onResult(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,const uint8_t &x,const uint8_t &y,
                  const std::vector<std::pair<CatchChallenger::Orientation,uint8_t> > &p,
                  const PathFinding::PathFinding_status &s)
    {
        Q_UNUSED(mapIndex); Q_UNUSED(x); Q_UNUSED(y);
        got=true;
        status=s;
        path=p;
    }
};

// Replay a returned path from (sx,sy). Reports the final tile and
// whether tile (gx,gy) — the gated gap — was ever stepped on.
static void replay(const std::vector<std::pair<CatchChallenger::Orientation,uint8_t> > &path,
                   int sx,int sy,int gx,int gy,
                   int &endX,int &endY,bool &crossedGate)
{
    int x=sx,y=sy;
    crossedGate=(x==gx && y==gy);
    unsigned int i=0;
    while(i<path.size())
    {
        const CatchChallenger::Orientation o=path.at(i).first;
        uint8_t c=path.at(i).second;
        uint8_t s=0;
        while(s<c)
        {
            if(o==CatchChallenger::Orientation_top)
                y--;
            else if(o==CatchChallenger::Orientation_bottom)
                y++;
            else if(o==CatchChallenger::Orientation_right)
                x++;
            else if(o==CatchChallenger::Orientation_left)
                x--;
            if(x==gx && y==gy)
                crossedGate=true;
            s++;
        }
        i++;
    }
    endX=x;
    endY=y;
}

// Build the deterministic collision grid on top of the loaded map.
// g\zoneVal at the wall gap; everything else 255 (walkable) except the
// wall row (254) which is impassable apart from the gap.
static void buildGrid(CatchChallenger::MapServer &m,bool withWall,
                      int wallRow,int gapX,uint8_t zoneVal)
{
    const int W=m.width;
    const int H=m.height;
    m.flat_simplified_map.assign((size_t)W*H,255);
    if(withWall)
    {
        int x=0;
        while(x<W)
        {
            m.flat_simplified_map[(size_t)x+(size_t)wallRow*W]=254;
            x++;
        }
        m.flat_simplified_map[(size_t)gapX+(size_t)wallRow*W]=zoneVal;
        // zones[] is indexed by the flat_simplified_map cell value.
        if((int)m.zones.size()<=zoneVal)
            m.zones.resize((size_t)zoneVal+1);
        m.zones[zoneVal].walkOn.clear();
        m.zones[zoneVal].walkOn.push_back(0);// -> monstersCollision[0]
    }
}

// One PathFinding run, fully synchronous.
static PathFinding::PathFinding_status runPath(
        CatchChallenger::MapServer &m,
        int sx,int sy,int dx,int dy,
        const std::unordered_map<CATCHCHALLENGER_TYPE_ITEM,CATCHCHALLENGER_TYPE_ITEM_QUANTITY> &items,
        std::vector<std::pair<CatchChallenger::Orientation,uint8_t> > &outPath)
{
    std::vector<CatchChallenger::CommonMap> mapList;
    mapList.push_back(m);// slice to CommonMap; zones + flat copied

    PathFinding pf;
    ResultCatcher catcher;
    QObject::connect(&pf,&PathFinding::result,&catcher,&ResultCatcher::onResult,
                     Qt::DirectConnection);

    // searchPath() fills pf's internal simplifiedMapList (and applies
    // the item-gate masking under test), then posts a queued
    // emitSearchPath we deliberately never deliver. We invoke
    // internalSearchPath() ourselves on this thread so result() fires
    // synchronously into ResultCatcher.
    pf.searchPath(mapList,0,(uint8_t)dx,(uint8_t)dy,0,(uint8_t)sx,(uint8_t)sy,items);
    pf.internalSearchPath(0,(uint8_t)dx,(uint8_t)dy,0,(uint8_t)sx,(uint8_t)sy,items);

    outPath=catcher.path;
    if(!catcher.got)
        return PathFinding::PathFinding_status_InternalError;
    return catcher.status;
}

int main(int argc,char *argv[])
{
    QCoreApplication app(argc,argv);

    // --- the fixed internal MapServer object reference ---
    static CatchChallenger::MapServer mapServer;

    CatchChallenger::Map_loader loader;
    if(!loader.tryLoadMap(CITY_TMX,mapServer,false))
    {
        ko("load_city_tmx",std::string("tryLoadMap failed: ")+loader.errorString());
        std::printf("%d passed, %d failed\n",g_pass,g_fail);
        return 1;
    }
    // tryLoadMap() leaves mapFinal.width/height untouched — the server
    // fills them in loadAllMapsAndLink(); a lone-map load must copy them
    // from map_to_send (same as tools/map2png).
    mapServer.width=(uint8_t)loader.map_to_send.width;
    mapServer.height=(uint8_t)loader.map_to_send.height;
    if(mapServer.width<8 || mapServer.height<8)
    {
        ko("load_city_tmx","map too small for the fixture");
        std::printf("%d passed, %d failed\n",g_pass,g_fail);
        return 1;
    }
    ok("load_city_tmx",std::to_string(mapServer.width)+"x"+std::to_string(mapServer.height));

    // city.tmx carries border links (its edges resolve back to map index 0
    // once the lone map is loaded), so PathFinding's edge-wrap would route
    // around the deterministic wall via the top/bottom/left/right edge —
    // defeating the "the gated gap is the ONLY crossing" premise of this
    // fixture. Sever every border so the wall genuinely isolates the two
    // halves; 65535 == "no neighbouring map" (see Map_Border.hpp).
    mapServer.border.top.mapIndex=65535;
    mapServer.border.bottom.mapIndex=65535;
    mapServer.border.left.mapIndex=65535;
    mapServer.border.right.mapIndex=65535;

    const int W=mapServer.width;
    const int H=mapServer.height;
    const int wallRow=H/2;
    const int gapX=W/2;
    const uint8_t zoneVal=5;// <200 -> PathFinding::canGoOn() treats it walkable
    // predefined start / stop, opposite sides of the wall.
    const int sx=gapX, sy=wallRow-3;
    const int dx=gapX, dy=wallRow+3;

    // Inject a controlled item-gated walkOn collision (index 0).
    {
        std::vector<CatchChallenger::MonstersCollision> mc;
        CatchChallenger::MonstersCollision e;
        e.type=CatchChallenger::MonstersCollisionType_WalkOn;
        e.item=GATE_ITEM;
        mc.push_back(e);
        CatchChallenger::CommonDatapack::commonDatapack.testing_setMonstersCollision(mc);
    }

    std::unordered_map<CATCHCHALLENGER_TYPE_ITEM,CATCHCHALLENGER_TYPE_ITEM_QUANTITY> noItem;
    std::unordered_map<CATCHCHALLENGER_TYPE_ITEM,CATCHCHALLENGER_TYPE_ITEM_QUANTITY> withItem;
    withItem[GATE_ITEM]=1;

    std::vector<std::pair<CatchChallenger::Orientation,uint8_t> > path;
    int ex=0,ey=0; bool crossed=false;

    // --- OPEN: no wall, trivially reachable ---
    buildGrid(mapServer,false,wallRow,gapX,zoneVal);
    {
        PathFinding::PathFinding_status st=runPath(mapServer,sx,sy,dx,dy,noItem,path);
        if(st==PathFinding::PathFinding_status_OK && !path.empty())
            ok("open_path","reachable destination routed");
        else
            ko("open_path","planner failed on an open map (harness/planner broken)");
    }

    // --- WITHITEM: player carries the gate item, gap is passable ---
    buildGrid(mapServer,true,wallRow,gapX,zoneVal);
    {
        PathFinding::PathFinding_status st=runPath(mapServer,sx,sy,dx,dy,withItem,path);
        replay(path,sx,sy,gapX,wallRow,ex,ey,crossed);
        if(st==PathFinding::PathFinding_status_OK && crossed)
            ok("withitem_path","gap traversed when item carried");
        else
            ko("withitem_path","expected a path through the gate when the item is held"
                               " (st="+std::to_string((int)st)+")");
    }

    // --- GATED: no item. The bug under test. ---
    buildGrid(mapServer,true,wallRow,gapX,zoneVal);
    {
        PathFinding::PathFinding_status st=runPath(mapServer,sx,sy,dx,dy,noItem,path);
        replay(path,sx,sy,gapX,wallRow,ex,ey,crossed);
        if(st==PathFinding::PathFinding_status_OK && crossed)
            ko("gated_path",
               "PathFinding routed through an item-gated zone without the item — "
               "reproduces \"You can't enter to this zone without the correct item\"");
        else if(st==PathFinding::PathFinding_status_PathNotFound)
            ok("gated_path","item-gated zone correctly excluded from the route");
        else
            ko("gated_path","unexpected status="+std::to_string((int)st));
    }

    // --- LEAST-DIRECTION-CHANGE: the defining property of this planner. ---
    //
    // CatchChallenger's PathFinding is NOT a conventional shortest-path search.
    // Many equal-length Manhattan routes exist between two tiles; a plain BFS/A*
    // would happily return a STAIRCASE (right,down,right,down,...) — same number
    // of tiles, but a direction change on every step. This planner instead
    // minimises the number of DIRECTION CHANGES (context switches): it keeps, per
    // resolved cell, four run-length-compressed variants (path ending Left / Right
    // / Top / Bottom) and, at the destination, returns the variant with the FEWEST
    // RUNS. Fewer runs == fewer turns == a straighter, cheaper-to-walk path (and it
    // compresses far better on the wire: one long run vs many 1-step runs).
    //
    // So on an OPEN map, going from (x,y) to (x+4,y+4) must come back as TWO runs
    // (ONE turn) — an L shape: four steps along one axis, then four along the
    // other, e.g. [{right,4},{bottom,4}] or [{bottom,4},{right,4}] — NOT the
    // 8-run staircase a shortest-path search would accept as "equally optimal".
    buildGrid(mapServer,false,wallRow,gapX,zoneVal);// open grid, all walkable
    {
        const int lx=5,ly=5;// well inside the map, room for +4/+4
        PathFinding::PathFinding_status st=runPath(mapServer,lx,ly,lx+4,ly+4,noItem,path);
        replay(path,lx,ly,-1,-1,ex,ey,crossed);
        bool lshape=false;
        if(path.size()==2)
        {
            // the two runs must be on DIFFERENT axes (one horizontal, one
            // vertical): an L, not two colinear runs or a staircase.
            const CatchChallenger::Orientation a=path.at(0).first;
            const CatchChallenger::Orientation b=path.at(1).first;
            const bool aH=(a==CatchChallenger::Orientation_left||a==CatchChallenger::Orientation_right);
            const bool bH=(b==CatchChallenger::Orientation_left||b==CatchChallenger::Orientation_right);
            lshape=(aH!=bH);
        }
        if(st==PathFinding::PathFinding_status_OK && ex==lx+4 && ey==ly+4
                && path.size()==2 && lshape)
            ok("least_direction_change","(x,y)->(x+4,y+4) routed as an L (2 runs, ONE turn) "
                                        "-- minimises context switches, not a staircase");
        else
            ko("least_direction_change","expected a 2-run L path (least direction change); got "
                +std::to_string(path.size())+" run(s) (st="+std::to_string((int)st)
                +", end=("+std::to_string(ex)+","+std::to_string(ey)+"))");
    }

    std::printf("%d passed, %d failed\n",g_pass,g_fail);
    return g_fail==0 ? 0 : 1;
}

#include "main.moc"
