#include "TimerCityCapture.h"
#include "../Epoll.h"

#include "../../base/Client.h"

#include <iostream>

TimerCityCapture::TimerCityCapture()
{
}

void TimerCityCapture::exec()
{
    CatchChallenger::Client::startTheCityCapture();
    EpollTimer::exec();
}
