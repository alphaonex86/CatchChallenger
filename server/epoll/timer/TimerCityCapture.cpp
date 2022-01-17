#include "TimerCityCapture.hpp"
#include "../Epoll.hpp"
#include "../ServerPrivateVariablesEpoll.hpp"
#include "../../base/Client.hpp"

#include <iostream>

TimerCityCapture::TimerCityCapture()
{
}

void TimerCityCapture::exec()
{
    CatchChallenger::Client::startTheCityCapture();
}
