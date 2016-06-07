#include "TimerCityCapture.h"
#include "../Epoll.h"

#include "../../base/Client.h"

#include <iostream>

TimerCityCapture::TimerCityCapture()
{
}

void TimerCityCapture::exec()
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    CatchChallenger::Client::startTheCityCapture();
    #endif
}
