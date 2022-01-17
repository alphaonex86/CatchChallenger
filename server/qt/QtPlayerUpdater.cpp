#include "QtPlayerUpdater.hpp"

void QtPlayerUpdater::setInterval(int ms)
{
    QTimer::setInterval(ms);
}

void QtPlayerUpdater::exec()
{
    PlayerUpdaterBase::exec();
}
