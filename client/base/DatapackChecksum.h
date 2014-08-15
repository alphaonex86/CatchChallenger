#ifndef CATCHCHALLENGER_DATAPACKCHECKSUM_H
#define CATCHCHALLENGER_DATAPACKCHECKSUM_H

#include <QThread>

namespace CatchChallenger {
class DatapackChecksum : public QThread
{
    Q_OBJECT
public:
    explicit DatapackChecksum();
    ~DatapackChecksum();
    static QByteArray doChecksum(const QString &datapackPath);
public slots:
    void doDifferedChecksum(const QString &datapackPath);
signals:
    void datapackChecksumDone(const QByteArray &hash);
};
}

#endif // CATCHCHALLENGER_DATAPACKCHECKSUM_H
