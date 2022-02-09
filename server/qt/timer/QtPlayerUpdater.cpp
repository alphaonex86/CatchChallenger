#include "QtPlayerUpdater.hpp"

QtPlayerUpdater::QtPlayerUpdater()
{
    setInterval(1000);
    if(!connect(this,&QTimer::timeout,this,&QtPlayerUpdater::exec,Qt::QueuedConnection))
        abort();
    start();
}

void QtPlayerUpdater::setInterval(int ms)
{
    QTimer::setInterval(ms);
}

void QtPlayerUpdater::exec()
{
    PlayerUpdaterBase::exec();
}
