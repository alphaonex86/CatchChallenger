#include "QSslServer.h"
#include "../general/base/DebugClass.h"
#include <QSslSocket>

using namespace CatchChallenger;

QSslServer::QSslServer(const QSslCertificate &sslCertificate,const QSslKey &sslKey)
{
    this->sslCertificate=sslCertificate;
    this->sslKey=sslKey;
}

void QSslServer::incomingConnection(qintptr socketDescriptor)
{
   QSslSocket * socket = new QSslSocket;
   socket->setSocketDescriptor(socketDescriptor);
   socket->setPrivateKey(sslKey);
   socket->setLocalCertificate(sslCertificate);
   socket->setPeerVerifyMode(QSslSocket::VerifyNone);
   socket->ignoreSslErrors();
   socket->startServerEncryption();
   connect(socket,static_cast<void(QSslSocket::*)(const QList<QSslError> &errors)>(&QSslSocket::sslErrors),this,&QSslServer::sslErrors);
}

void QSslServer::sslErrors(const QList<QSslError> &errors)
{
    int index=0;
    while(index<errors.size())
    {
        DebugClass::debugConsole(QString("Ssl error: %1").arg(errors.at(index).errorString()));
        index++;
    }
    QSslSocket *socket=qobject_cast<QSslSocket *>(QObject::sender());
    if(socket!=NULL)
        socket->disconnectFromHost();
}
