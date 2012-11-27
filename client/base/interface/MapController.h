#ifndef POKECRAFT_MAPCONTROLLER_H
#define POKECRAFT_MAPCONTROLLER_H

#include "MapControllerMP.h"
#include "../../client/base/Api_protocol.h"

#include <QString>
#include <QList>
#include <QStringList>
#include <QHash>
#include <QTimer>

class MapController : public MapControllerMP
{
    Q_OBJECT
public:
    explicit MapController(Pokecraft::Api_protocol *client, const bool &centerOnPlayer=true, const bool &debugTags=false, const bool &useCache=true, const bool &OpenGL=false);
    ~MapController();

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
    struct DelayedPlantRemove
    {
        quint32 mapId;
        quint16 x;
        quint16 y;
    };
    QList<DelayedPlantRemove> delayedPlantRemove;

    struct Plant
    {
        Tiled::MapObject * mapObject;
        quint8 x,y;
        quint8 plant_id;
        quint32 mapId;
    };
    QList<Plant> plantList;
protected slots:
    //plant
    void insert_plant(const quint32 &mapId,const quint16 &x,const quint16 &y,const quint8 &plant_id,const quint16 &seconds_to_mature);
    void remove_plant(const quint32 &mapId,const quint16 &x,const quint16 &y);
    void seed_planted(const bool &ok);
    void plant_collected(const Pokecraft::Plant_collect &stat);
public slots:
    virtual void datapackParsed();
signals:
    //plant, can do action only if the previous is finish
    void useSeed(const quint8 &plant_id);
    void collectMaturePlant();
};

#endif // MAPCONTROLLER_H
