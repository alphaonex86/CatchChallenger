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
    virtual void resetAll();
private:
    //the delayed action
    struct DelayedPlantInsert
    {
        quint32 mapId;
        quint16 x;
        quint16 y;
        quint8 plant_id;
        quint16 seconds_to_mature;
    };
    QList<DelayedPlantInsert> delayedPlantInsert;
protected slots:
    //plant
    void insert_plant(const quint32 &mapId,const quint16 &x,const quint16 &y,const quint8 &plant_id,const quint16 &seconds_to_mature);
    void remove_plant(const quint32 &mapId,const quint16 &x,const quint16 &y);
    void seed_planted(const bool &ok);
    void plant_collected(const CatchChallenger::Plant_collect &stat);
    virtual bool canGoTo(const CatchChallenger::Direction &direction,CatchChallenger::Map map,COORD_TYPE x,COORD_TYPE y,const bool &checkCollision);
public slots:
    virtual void datapackParsed();
    virtual void reinject_signals();
private slots:
    void loadBotOnTheMap(Map_full *parsedMap, const quint32 &botId, const quint8 &x, const quint8 &y, const QString &lookAt, const QString &skin);
};

#endif // MAPCONTROLLER_H
