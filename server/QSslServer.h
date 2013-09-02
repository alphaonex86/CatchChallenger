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
    void incomingConnection(qintptr socketDescriptor);
private:
    QSslCertificate sslCertificate;
    QSslKey sslKey;
private slots:
    void sslErrors(const QList<QSslError> &errors);
};

#endif // QSSLSERVER_H
