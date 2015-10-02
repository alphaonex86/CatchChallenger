#ifndef QSSLSERVER_H
#define QSSLSERVER_H

#include <QTcpServer>
#include <QSslCertificate>
#include <QSslKey>
#include <std::vector<char>>

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
    std::vector<char> firstHeader;
private:
    void sslErrors(const std::vector<QSslError> &errors);
};

#endif // QSSLSERVER_H
