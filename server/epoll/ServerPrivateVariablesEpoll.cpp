#include "ServerPrivateVariablesEpoll.hpp"

ServerPrivateVariablesEpoll ServerPrivateVariablesEpoll::serverPrivateVariablesEpoll;

ServerPrivateVariablesEpoll::ServerPrivateVariablesEpoll()
{
    timer_to_send_insert_move_remove = nullptr;
    nextTimeStampsToCaptureCity=0;
}
