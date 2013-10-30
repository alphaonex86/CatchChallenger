#ifndef CATCHCHALLENGER_MAPCONTROLLER_H
#define CATCHCHALLENGER_MAPCONTROLLER_H

#include "MapControllerMP.h"
#include "../../../general/base/Api_protocol.h"

#include <QString>
#include <QList>
#include <QStringList>
#include <QHash>
#include <QTimer>

class MapController : public MapControllerMP
{
    Q_OBJECT
public:
    explicit MapController(const bool &centerOnPlayer=true, const bool &debugTags=false, const bool &useCache=true, const bool &OpenGL=false);
    ~MapController();
    static MapController *mapController;
    virtual void connectAllSignals();
    virtual void resetAll();
    QString mapIdToString(const quint32 &mapId) const;
    void remove_plant_full(const QString &map,const quint8 &x,const quint8 &y);
    void insert_plant_full(const QString &map,const quint8 &x,const quint8 &y,const quint8 &plant_id,const quint16 &seconds_to_mature);
private:
    //the delayed action
    struct DelayedPlantInsert
    {
        quint32 mapId;
        quint8 x,y;
        quint8 plant_id;
        quint16 seconds_to_mature;
    };
    QList<DelayedPlantInsert> delayedPlantInsert;
    QMultiHash<QString,DelayedPlantInsert> delayedPlantInsertOnMap;
    struct PlantTimer
    {
        Tiled::MapObject * mapObject;
        quint8 plant_id;
        quint16 seconds_to_mature;
    };
protected slots:
    //plant
    void getPlantTimerEvent();
    bool updatePlantGrowing(CatchChallenger::ClientPlantWithTimer *plant);//return true if is growing
    void insert_plant(const quint32 &mapId,const quint8 &x,const quint8 &y,const quint8 &plant_id,const quint16 &seconds_to_mature);
    void remove_plant(const quint32 &mapId,const quint8 &x,const quint8 &y);
    void seed_planted(const bool &ok);
    void plant_collected(const CatchChallenger::Plant_collect &stat);
    virtual bool canGoTo(const CatchChallenger::Direction &direction,CatchChallenger::Map map,COORD_TYPE x,COORD_TYPE y,const bool &checkCollision);
    void tryLoadPlantOnMapDisplayed(const QString &fileName);
    void updateGrowing();
public slots:
    virtual void datapackParsed();
    virtual void reinject_signals();
private slots:
    void loadBotOnTheMap(MapVisualiserThread::Map_full *parsedMap, const quint32 &botId, const quint8 &x, const quint8 &y, const QString &lookAt, const QString &skin);
};

#endif // MAPCONTROLLER_H
