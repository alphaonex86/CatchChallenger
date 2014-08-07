#include "PathFinding.h"

PathFinding::PathFinding() :
    tryCancel(false)
{
    start();
    moveToThread(this);
    connect(this,&PathFinding::internalCancel,this,&PathFinding::cancel,Qt::BlockingQueuedConnection);
    connect(this,&PathFinding::emitSearchPath,this,&PathFinding::internalSearchPath,Qt::BlockingQueuedConnection);
}

PathFinding::~PathFinding()
{
    emit internalCancel();
}

void PathFinding::searchPath(QList<MapVisualiserThread::Map_full> mapList)
{
    this->mapList=mapList;
    emit emitSearchPath();
}

void PathFinding::internalSearchPath()
{
    QList<QPair<CatchChallenger::Direction,quint8> > path;
    emit result(path);
}

void PathFinding::cancel()
{
    tryCancel=true;
}
