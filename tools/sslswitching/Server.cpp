#include "Server.h"
#include <QSslSocket>
#include <unistd.h>

Server::Server(const QSslCertificate &sslCertificate,const QSslKey &sslKey)
{
    this->sslCertificate=sslCertificate;
    this->sslKey=sslKey;
    encodingHeaderValue[0]=0x01;
}

Server::Server()
{
    encodingHeaderValue[0]=0x00;
}

void Server::incomingConnection(qintptr socketDescriptor)
{
    QSslSocket * socket = new QSslSocket;
    socket->setSocketDescriptor(socketDescriptor);
    socket->write(encodingHeaderValue,sizeof(encodingHeaderValue));
    if(!sslCertificate.isNull())
    {
        socket->setPrivateKey(sslKey);
        socket->setLocalCertificate(sslCertificate);
        QList<QSslCertificate> certificates;
        certificates << sslCertificate;
        socket->setCaCertificates(certificates);
        socket->setPeerVerifyMode(QSslSocket::VerifyNone);
        socket->ignoreSslErrors();
        socket->startServerEncryption();
    }
    addPendingConnection(socket);
}
