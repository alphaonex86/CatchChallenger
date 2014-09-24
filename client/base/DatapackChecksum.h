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
    static QByteArray doChecksum(const QString &datapackPath, bool onlyFull=false);
public slots:
    void doDifferedChecksum(const QString &datapackPath);
signals:
    void datapackChecksumDone(const QStringList &datapackFilesList,const QByteArray &hash,const QList<quint32> &partialHashList);
};
}

#endif // CATCHCHALLENGER_DATAPACKCHECKSUM_H
