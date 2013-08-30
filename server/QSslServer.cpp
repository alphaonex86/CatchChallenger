#include "QSslServer.h"
#include <QSslSocket>

QSslServer::QSslServer(const QSslCertificate &sslCertificate,const QSslKey &sslKey)
{
    this->sslCertificate=sslCertificate;
    this->sslKey=sslKey;
}

void QSslServer::incomingConnection(int socketDescriptor)
{
   QSslSocket * socket = new QSslSocket;
   socket->setSocketDescriptor(socketDescriptor);
   socket->setPrivateKey(sslKey);
   socket->setLocalCertificate(sslCertificate);
   socket->setPeerVerifyMode(QSslSocket::VerifyNone);
   socket->ignoreSslErrors();
   socket->startServerEncryption();
}
