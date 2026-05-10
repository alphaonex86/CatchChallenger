#include "TimerCityCapture.hpp"
#include "../../base/Client.hpp"

#include <iostream>

TimerCityCapture::TimerCityCapture()
{
}

void TimerCityCapture::exec()
{
    CatchChallenger::Client::startTheCityCapture();
}
