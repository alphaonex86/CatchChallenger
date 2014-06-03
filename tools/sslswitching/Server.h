#ifndef QRAWSERVER_H
#define QRAWSERVER_H

#include <QTcpServer>
#include <QSslCertificate>
#include <QSslKey>

class Server : public QTcpServer
{
    Q_OBJECT
public:
    explicit Server(const QSslCertificate &sslCertificate,const QSslKey &sslKey);
    explicit Server();
protected:
    void incomingConnection(qintptr socketDescriptor);
private:
    QSslCertificate sslCertificate;
    QSslKey sslKey;
    char encodingHeaderValue[1];
};

#endif // QRAWSERVER_H
