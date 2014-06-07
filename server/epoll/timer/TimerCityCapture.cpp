#include "TimerCityCapture.h"
#include "Epoll.h"

#include "../base/LocalClientHandler.h"

#include <iostream>

TimerCityCapture::TimerCityCapture()
{
}

void TimerCityCapture::exec()
{
    CatchChallenger::LocalClientHandler::startTheCityCapture();
    EpollTimer::exec();
}
