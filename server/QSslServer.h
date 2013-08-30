#ifndef QSSLSERVER_H
#define QSSLSERVER_H

#include <QTcpServer>
#include <QSslCertificate>
#include <QSslKey>

class QSslServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit QSslServer(const QSslCertificate &sslCertificate,const QSslKey &sslKey);
protected:
    void incomingConnection(int socketDescriptor);
private:
    QSslCertificate sslCertificate;
    QSslKey sslKey;

};

#endif // QSSLSERVER_H
