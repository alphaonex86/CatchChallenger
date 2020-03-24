#include "TimerCityCapture.hpp"
#include "../Epoll.hpp"

#include "../../base/Client.hpp"

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
