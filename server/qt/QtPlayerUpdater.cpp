#include "QtPlayerUpdater.hpp"

QtPlayerUpdater::QtPlayerUpdater()
{

}

void QtPlayerUpdater::setInterval(int ms)
{
    QTimer::setInterval(ms);
}

void QtPlayerUpdater::exec()
{
    PlayerUpdaterBase::exec();
}
