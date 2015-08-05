#ifndef CATCHCHALLENGER_DATAPACKCHECKSUM_H
#define CATCHCHALLENGER_DATAPACKCHECKSUM_H

#ifndef QT_NO_EMIT
#include <QThread>
#endif

#include <QStringList>
#include <QByteArray>
#include <QList>
#include <QString>

namespace CatchChallenger {
class DatapackChecksum
        #ifndef QT_NO_EMIT
        : public QObject
        #endif
{
    #ifndef QT_NO_EMIT
    Q_OBJECT
    #endif
public:
    #ifndef QT_NO_EMIT
    static QThread thread;
    #endif
    explicit DatapackChecksum();
    ~DatapackChecksum();
    struct FullDatapackChecksumReturn
    {
        QStringList datapackFilesList;
        QByteArray hash;
        QList<quint32> partialHashList;
    };
    static QByteArray doChecksumBase(const QString &datapackPath);
    static QByteArray doChecksumMain(const QString &datapackPath);
    static QByteArray doChecksumSub(const QString &datapackPath);
    static FullDatapackChecksumReturn doFullSyncChecksumBase(const QString &datapackPath);
    static FullDatapackChecksumReturn doFullSyncChecksumMain(const QString &datapackPath);
    static FullDatapackChecksumReturn doFullSyncChecksumSub(const QString &datapackPath);
    #ifndef QT_NO_EMIT
public slots:
    void doDifferedChecksumBase(const QString &datapackPath);
    void doDifferedChecksumMain(const QString &datapackPath);
    void doDifferedChecksumSub(const QString &datapackPath);
signals:
    void datapackChecksumDoneBase(const QStringList &datapackFilesList,const QByteArray &hash,const QList<quint32> &partialHashList);
    void datapackChecksumDoneMain(const QStringList &datapackFilesList,const QByteArray &hash,const QList<quint32> &partialHashList);
    void datapackChecksumDoneSub(const QStringList &datapackFilesList,const QByteArray &hash,const QList<quint32> &partialHashList);
    #endif
};
}

#endif // CATCHCHALLENGER_DATAPACKCHECKSUM_H
