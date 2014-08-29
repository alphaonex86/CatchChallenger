#ifndef PATHFINDING_H
#define PATHFINDING_H

#include <QObject>
#include <QMutex>
#include "../../general/base/GeneralStructures.h"
#include "../render/MapVisualiserThread.h"

class PathFinding : public QThread
{
    Q_OBJECT
public:
    explicit PathFinding();
    virtual ~PathFinding();
signals:
    void result(QList<QPair<CatchChallenger::Direction,quint8> > path);
    void internalCancel();
    void emitSearchPath(const QString &destination_map,const quint8 &destination_x,const quint8 &destination_y,const QString &current_map,const quint8 &x,const quint8 &y,const QHash<quint16,quint32> &items);
public slots:
    void searchPath(const QHash<QString, MapVisualiserThread::Map_full *> &all_map,const QString &destination_map,const quint8 &destination_x,const quint8 &destination_y,const QString &current_map,const quint8 &x,const quint8 &y,const QHash<quint16,quint32> &items);
    void internalSearchPath(const QString &destination_map,const quint8 &destination_x,const quint8 &destination_y,const QString &current_map,const quint8 &x,const quint8 &y,const QHash<quint16,quint32> &items);
    void cancel();
private:
    struct SimplifiedMapForPathFinding
    {
        bool *walkable;
        quint8 *monstersCollisionMap;//to know if have the item
        bool *dirt;
        quint8 *ledges;

        quint8 width,height;

        struct Map_Border
        {
            struct Map_BorderContent_TopBottom
            {
                SimplifiedMapForPathFinding *map;
                qint32 x_offset;
            };
            struct Map_BorderContent_LeftRight
            {
                SimplifiedMapForPathFinding *map;
                qint32 y_offset;
            };
            Map_BorderContent_TopBottom top;
            Map_BorderContent_TopBottom bottom;
            Map_BorderContent_LeftRight left;
            Map_BorderContent_LeftRight right;
        };
        Map_Border border;
        struct PathToGo
        {
            QList<QPair<CatchChallenger::Orientation,quint8/*step number*/> > left;
            QList<QPair<CatchChallenger::Orientation,quint8/*step number*/> > right;
            QList<QPair<CatchChallenger::Orientation,quint8/*step number*/> > top;
            QList<QPair<CatchChallenger::Orientation,quint8/*step number*/> > bottom;
        };
        QHash<QPair<quint8,quint8>,PathToGo> pathToGo;
    };

    struct MapPointToParse
    {
        QString map;
        quint8 x,y;
    };

    QMutex mutex;
    QHash<QString,SimplifiedMapForPathFinding> simplifiedMapList;
    bool tryCancel;
    QList<MapVisualiserThread::Map_full> mapList;
};

#endif // PATHFINDING_H
