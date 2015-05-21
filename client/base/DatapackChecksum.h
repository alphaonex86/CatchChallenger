#ifndef CATCHCHALLENGER_DATAPACKCHECKSUM_H
#define CATCHCHALLENGER_DATAPACKCHECKSUM_H

#include <QThread>

namespace CatchChallenger {
class DatapackChecksum : public QObject
{
    Q_OBJECT
public:
    static QThread thread;
    explicit DatapackChecksum();
    ~DatapackChecksum();
    static QByteArray doChecksumBase(const QString &datapackPath, bool onlyFull=false);
    static QByteArray doChecksumMain(const QString &datapackPath, bool onlyFull=false);
    static QByteArray doChecksumSub(const QString &datapackPath, bool onlyFull=false);
public slots:
    void doDifferedChecksumBase(const QString &datapackPath);
    void doDifferedChecksumMain(const QString &datapackPath);
    void doDifferedChecksumSub(const QString &datapackPath);
signals:
    void datapackChecksumDoneBase(const QStringList &datapackFilesList,const QByteArray &hash,const QList<quint32> &partialHashList);
    void datapackChecksumDoneMain(const QStringList &datapackFilesList,const QByteArray &hash,const QList<quint32> &partialHashList);
    void datapackChecksumDoneSub(const QStringList &datapackFilesList,const QByteArray &hash,const QList<quint32> &partialHashList);
};
}

#endif // CATCHCHALLENGER_DATAPACKCHECKSUM_H
