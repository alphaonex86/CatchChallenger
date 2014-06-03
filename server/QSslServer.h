#ifndef QSSLSERVER_H
#define QSSLSERVER_H

#include <QTcpServer>
#include <QSslCertificate>
#include <QSslKey>
#include <QByteArray>

class QSslServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit QSslServer(const QSslCertificate &sslCertificate,const QSslKey &sslKey);
    explicit QSslServer();
protected:
    void incomingConnection(qintptr socketDescriptor);
private:
    QSslCertificate sslCertificate;
    QSslKey sslKey;
    QByteArray firstHeader;
private slots:
    void sslErrors(const QList<QSslError> &errors);
};

#endif // QSSLSERVER_H
