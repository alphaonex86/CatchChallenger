#include "ServerPrivateVariablesEventLoop.hpp"

ServerPrivateVariablesEventLoop ServerPrivateVariablesEventLoop::serverPrivateVariablesEventLoop;

ServerPrivateVariablesEventLoop::ServerPrivateVariablesEventLoop()
{
    timer_to_send_insert_move_remove = nullptr;
    nextTimeStampsToCaptureCity=0;
}
