#include "QSslServer.hpp"
#include <QSslSocket>
#include <QSslConfiguration>
#include <unistd.h>
#include <iostream>

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
        QList<QSslCertificate> certificates;
        certificates << sslCertificate;
        //socket->sslConfiguration().setCaCertificates(certificates);->wrong
        QSslConfiguration configuration = QSslConfiguration::defaultConfiguration();
        configuration.addCaCertificates(certificates);
        QSslConfiguration::setDefaultConfiguration(configuration);
        //socket->setCaCertificates(certificates);
        socket->setPeerVerifyMode(QSslSocket::VerifyNone);
        socket->ignoreSslErrors();
        socket->startServerEncryption();
        connect(socket,static_cast<void(QSslSocket::*)(const QList<QSslError> &errors)>(&QSslSocket::sslErrors),this,&QSslServer::sslErrors);
    }
    addPendingConnection(socket);
}

void QSslServer::sslErrors(const QList<QSslError> &errors)
{
    int index=0;
    while(index<errors.size())
    {
        std::cerr << "Ssl error: " << errors.at(index).errorString().toStdString() << std::endl;
        index++;
    }
    QSslSocket *socket=qobject_cast<QSslSocket *>(QObject::sender());
    if(socket!=NULL)
        socket->disconnectFromHost();
}
