#include "PlayerUpdater.h"
#include "../../general/base/DebugClass.h"
#include "EventDispatcher.h"

using namespace Pokecraft;
PlayerUpdater::PlayerUpdater()
{
	connected_players=0;
	sended_connected_players=0;
	next_send_timer.setSingleShot(true);
	next_send_timer.setInterval(250);
	connect(this,SIGNAL(send_addConnectedPlayer()),this,SLOT(internal_addConnectedPlayer()),Qt::QueuedConnection);
	connect(this,SIGNAL(send_removeConnectedPlayer()),this,SLOT(internal_removeConnectedPlayer()),Qt::QueuedConnection);
	connect(&next_send_timer,SIGNAL(timeout()),this,SLOT(send_timer()),Qt::QueuedConnection);
}

void PlayerUpdater::addConnectedPlayer()
{
	DebugClass::debugConsole("PlayerUpdater::addConnectedPlayer()");
	emit send_addConnectedPlayer();
}

void PlayerUpdater::removeConnectedPlayer()
{
	DebugClass::debugConsole("PlayerUpdater::removeConnectedPlayer()");
	emit send_removeConnectedPlayer();
}

void PlayerUpdater::internal_addConnectedPlayer()
{
	connected_players++;
	if(!next_send_timer.isActive())
		next_send_timer.start();
}

void PlayerUpdater::internal_removeConnectedPlayer()
{
	connected_players--;
	if(!next_send_timer.isActive())
		next_send_timer.start();
}

void PlayerUpdater::send_timer()
{
	if(sended_connected_players!=connected_players && EventDispatcher::generalData.serverSettings.commmonServerSettings.sendPlayerNumber)
	{
		sended_connected_players=connected_players;
		emit newConnectedPlayer(connected_players);
	}
}
