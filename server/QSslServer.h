#ifndef QSSLSERVER_H
#define QSSLSERVER_H

#include <QTcpServer>
#include <QSslCertificate>
#include <QSslKey>
#include <QByteArray>

class QSslServer : public QTcpServer
{
public:
    explicit QSslServer(const QSslCertificate &sslCertificate,const QSslKey &sslKey);
    explicit QSslServer();
protected:
    void incomingConnection(qintptr socketDescriptor);
private:
    QSslCertificate sslCertificate;
    QSslKey sslKey;
    QByteArray firstHeader;
private:
    void sslErrors(const QList<QSslError> &errors);
};

#endif // QSSLSERVER_H
