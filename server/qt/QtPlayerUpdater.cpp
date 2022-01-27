#include "QtPlayerUpdater.hpp"

QtPlayerUpdater::QtPlayerUpdater()
{
    setInterval(1000);
}

void QtPlayerUpdater::setInterval(int ms)
{
    QTimer::setInterval(ms);
}

void QtPlayerUpdater::exec()
{
    PlayerUpdaterBase::exec();
}
