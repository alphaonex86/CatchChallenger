#ifndef PATHFINDING_H
#define PATHFINDING_H

#include <QObject>
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
    void emitSearchPath();
public slots:
    void searchPath(QList<MapVisualiserThread::Map_full> mapList);
    void internalSearchPath();
    void cancel();
private:
    bool tryCancel;
    QList<MapVisualiserThread::Map_full> mapList;
};

#endif // PATHFINDING_H
