#include "QFakeServer.h"
#include "QFakeSocket.h"

#include <QHostAddress>

using namespace Pokecraft;

QFakeServer QFakeServer::server;

QFakeServer::QFakeServer()
{
	m_isListening=false;
}

void QFakeServer::addPendingConnection(QFakeSocket * socket)
{
	QPair<QFakeSocket *,QFakeSocket *> newEntry;
	newEntry.first=socket;
	newEntry.second=new QFakeSocket();
	{
		QMutexLocker locker(&mutex);
		m_listOfConnexion << newEntry;
		m_pendingConnection << newEntry;
	}
	newEntry.first->theOtherSocket=newEntry.second;
	newEntry.second->theOtherSocket=newEntry.first;
	newEntry.second->connectToHost(QHostAddress::LocalHost,9999);
	newEntry.second->connectToHostImplementation();
	connect(newEntry.first,SIGNAL(disconnected()),this,SLOT(disconnectedSocket()));
	emit newConnection();
}

bool QFakeServer::hasPendingConnections()
{
	QMutexLocker locker(&mutex);
	return m_pendingConnection.size()>0;
}

bool QFakeServer::isListening() const
{
	return m_isListening;
}

bool QFakeServer::listen()
{
	m_isListening=true;
	return true;
}

QFakeSocket * QFakeServer::nextPendingConnection()
{
	QMutexLocker locker(&mutex);
	QFakeSocket * socket=m_pendingConnection.first().second;
	m_pendingConnection.removeFirst();
	return socket;
}

void QFakeServer::close()
{
	m_isListening=false;
	QMutexLocker locker(&mutex);
	int loop_size=m_listOfConnexion.size();
	while(0<loop_size)
	{
		m_listOfConnexion.first().first->disconnectFromHost();
		loop_size--;
	}
}

void QFakeServer::disconnectedSocket()
{
	QMutexLocker locker(&mutex);
	int index=0;
	int loop_size=m_listOfConnexion.size();
	while(index<loop_size)
	{
		if(m_listOfConnexion.at(index).first->theOtherSocket==NULL)
		{
			m_listOfConnexion.removeAt(index);
			loop_size--;
		}
		else
			index++;
	}
	loop_size=m_pendingConnection.size();
	while(index<loop_size)
	{
		if(m_pendingConnection.at(index).first->theOtherSocket==NULL)
		{
			m_pendingConnection.removeAt(index);
			loop_size--;
		}
		else
			index++;
	}
}
