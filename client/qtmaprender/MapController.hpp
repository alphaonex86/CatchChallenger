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
    std::string mapIdToString(const uint32_t &mapId) const;
    void remove_plant_full(const std::string &map,const uint8_t &x,const uint8_t &y);
    void insert_plant_full(const std::string &map,const uint8_t &x,const uint8_t &y,const uint8_t &plant_id,const uint16_t &seconds_to_mature);
    void setColor(const QColor &color, const uint32_t &timeInMS=0);
    virtual bool asyncMapLoaded(const std::string &fileName,Map_full * tempMapObject);
private:
    //the delayed action
    struct DelayedPlantInsert
    {
        uint32_t mapId;
        uint8_t x,y;
        uint8_t plant_id;
        uint16_t seconds_to_mature;
    };
    std::vector<DelayedPlantInsert> delayedPlantInsert;
    std::unordered_map<std::string,std::vector<DelayedPlantInsert> > delayedPlantInsertOnMap;
    struct PlantTimer
    {
        Tiled::MapObject * mapObject;
        uint8_t plant_id;
        uint16_t seconds_to_mature;
    };
    Tiled::Tileset *botFlags;
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
    void insert_plant(const uint32_t &mapId,const uint8_t &x,const uint8_t &y,const uint8_t &plant_id,const uint16_t &seconds_to_mature);
    void remove_plant(const uint32_t &mapId,const uint8_t &x,const uint8_t &y);
    void seed_planted(const bool &ok);
    void plant_collected(const CatchChallenger::Plant_collect &stat);
    virtual bool canGoTo(const CatchChallenger::Direction &direction,CatchChallenger::CommonMap map,COORD_TYPE x,COORD_TYPE y,const bool &checkCollision);
    void tryLoadPlantOnMapDisplayed(const std::string &fileName);
    void updateGrowing();
    void updateColor();
    void loadPlayerFromCurrentMap();
    void updateBot();
public slots:
    virtual void datapackParsed();
    virtual void datapackParsedMainSub();
    virtual void reinject_signals();
private slots:
    void loadBotOnTheMap(Map_full *parsedMap, const uint32_t &botId, const uint8_t &x, const uint8_t &y, const std::string &lookAt, const std::string &skin);
protected:
    static std::string text_random;
    static std::string text_loop;
    static std::string text_move;
    static std::string text_left;
    static std::string text_right;
    static std::string text_top;
    static std::string text_bottom;
    static std::string text_slash;
    static std::string text_type;
    static std::string text_fightRange;
    static std::string text_fight;
    static std::string text_fightid;
    static std::string text_bot;
    static std::string text_slashtrainerpng;
    static std::string text_DATAPACK_BASE_PATH_SKIN;
};

#endif // MAPCONTROLLER_H
