#include "QSslServer.h"
#include "../general/base/DebugClass.h"
#include <QSslSocket>
#include <unistd.h>

using namespace CatchChallenger;

QSslServer::QSslServer(const QSslCertificate &sslCertificate,const QSslKey &sslKey)
{
    this->sslCertificate=sslCertificate;
    this->sslKey=sslKey;
    firstHeader[0x00]=0x01;
}

QSslServer::QSslServer()
{
    firstHeader[0x00]=0x00;
}

void QSslServer::incomingConnection(qintptr socketDescriptor)
{
    QSslSocket * socket = new QSslSocket;
    socket->setSocketDescriptor(socketDescriptor);
    socket->write(firstHeader);
    if(!sslCertificate.isNull())
    {
        socket->setPrivateKey(sslKey);
        socket->setLocalCertificate(sslCertificate);
        std::vector<QSslCertificate> certificates;
        certificates << sslCertificate;
        socket->setCaCertificates(certificates);
        socket->setPeerVerifyMode(QSslSocket::VerifyNone);
        socket->ignoreSslErrors();
        socket->startServerEncryption();
        connect(socket,static_cast<void(QSslSocket::*)(const std::vector<QSslError> &errors)>(&QSslSocket::sslErrors),this,&QSslServer::sslErrors);
    }
    addPendingConnection(socket);
}

void QSslServer::sslErrors(const std::vector<QSslError> &errors)
{
    int index=0;
    while(index<errors.size())
    {
        DebugClass::debugConsole(std::stringLiteral("Ssl error: %1").arg(errors.at(index).errorString()));
        index++;
    }
    QSslSocket *socket=qobject_cast<QSslSocket *>(QObject::sender());
    if(socket!=NULL)
        socket->disconnectFromHost();
}
