#ifndef CATCHCHALLENGER_MAPCONTROLLER_H
#define CATCHCHALLENGER_MAPCONTROLLER_H

#include "MapControllerMP.hpp"
#include "../../general/base/lib.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <QTimer>
#include <QGraphicsPixmapItem>
#include <QColor>

class DLL_PUBLIC MapController : public MapControllerMP
{
    Q_OBJECT
public:
    explicit MapController(const bool &centerOnPlayer=true, const bool &debugTags=false, const bool &useCache=true);
    ~MapController();
    virtual void connectAllSignals(CatchChallenger::Api_protocol_Qt *client);
    virtual void resetAll();
    //std::string mapIdToString(const CATCHCHALLENGER_TYPE_MAPID &mapId) const;
    void remove_plant_full(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,const COORD_TYPE &x,const COORD_TYPE &y);
    void insert_plant_full(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,const COORD_TYPE &x,const COORD_TYPE &y,const uint8_t &plant_id,const uint16_t &seconds_to_mature);
    void setColor(const QColor &color, const uint32_t &timeInMS=0);
    virtual bool asyncMapLoaded(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,Map_full * tempMapObject);
private:
    //the delayed action
    struct DelayedPlantInsert
    {
        CATCHCHALLENGER_TYPE_MAPID mapId;
        COORD_TYPE x,y;
        uint8_t plant_id;
        uint16_t seconds_to_mature;
    };
    std::vector<DelayedPlantInsert> delayedPlantInsert;
    std::unordered_map<CATCHCHALLENGER_TYPE_MAPID,std::vector<DelayedPlantInsert> > delayedPlantInsertOnMap;
    struct PlantTimer
    {
        Tiled::MapObject * mapObject;
        uint8_t plant_id;
        uint16_t seconds_to_mature;
    };
    Tiled::SharedTileset botFlags;
    QGraphicsPixmapItem *imageOver;
    bool imageOverAdded;
    QColor actualColor,tempColor,newColor;
    QTimer updateColorTimer;
    int updateColorIntervale;
    QTimer updateBotTimer;
protected slots:
    //plant
    void getPlantTimerEvent();
    bool updatePlantGrowing(CatchChallenger::ClientPlantWithTimer *plant);//return true if is growing
    void insert_plant(const CATCHCHALLENGER_TYPE_MAPID &mapId,const COORD_TYPE &x,const COORD_TYPE &y,const uint8_t &plant_id,const uint16_t &seconds_to_mature);
    void remove_plant(const CATCHCHALLENGER_TYPE_MAPID &mapId,const COORD_TYPE &x,const COORD_TYPE &y);
    void seed_planted(const bool &ok);
    void plant_collected(const CatchChallenger::Plant_collect &stat);
    virtual bool canGoTo(const CatchChallenger::Direction &direction, const CATCHCHALLENGER_TYPE_MAPID &mapIndex, const COORD_TYPE &x,const COORD_TYPE &y, const bool &checkCollision);
    void tryLoadPlantOnMapDisplayed(const CATCHCHALLENGER_TYPE_MAPID &mapIndex);
    void updateGrowing();
    void updateColor();
    void loadPlayerFromCurrentMap();
    void updateBot();
public slots:
    virtual void datapackParsed();
    virtual void datapackParsedMainSub();
    virtual void reinject_signals();
private slots:
    void loadBotOnTheMap(Map_full *parsedMap, const CATCHCHALLENGER_TYPE_BOTID &botId, const COORD_TYPE &x, const COORD_TYPE &y, const std::string &lookAt, const std::string &skin);
};

#endif // MAPCONTROLLER_H
