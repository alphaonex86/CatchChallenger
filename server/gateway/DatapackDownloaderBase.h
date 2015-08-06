#ifndef CATCHCHALLENGER_DatapackDownloaderBase_H
#define CATCHCHALLENGER_DatapackDownloaderBase_H

#include "../../general/base/GeneralVariable.h"
#include "../../client/base/DatapackChecksum.h"

#include <QObject>
#include <QString>
#include <QCoreApplication>
#include <QAbstractSocket>
#include <QString>
#include <QByteArray>
#include <QList>
#include <QPair>
#include <QDir>
#include <QHash>
#include <QFileInfo>
#include <QDateTime>
#include <QCryptographicHash>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QRegularExpression>

#include "../../general/base/DebugClass.h"
#include "../../general/base/GeneralStructures.h"
#include "../../client/base/qt-tar-xz/QXzDecodeThread.h"

namespace CatchChallenger {
class DatapackDownloaderBase : public QObject
{
    Q_OBJECT
public:
    explicit DatapackDownloaderBase(const QString &mDatapackBase);
    virtual ~DatapackDownloaderBase();
    static DatapackDownloaderBase *datapackDownloaderBase;
    std::vector<void *> clientInSuspend;
    void datapackDownloadError();
    void resetAll();
    void datapackFileList(const char * const data,const unsigned int &size);
    void writeNewFileBase(const QString &fileName, const QByteArray &data);

    //datapack related
    void sendDatapackContentBase();
    void test_mirror_base();
    void httpErrorEventBase();
    void decodedIsFinishBase();
    bool mirrorTryNextBase();
    void httpFinishedForDatapackListBase();
    const QStringList listDatapackBase(QString suffix);
    void cleanDatapackBase(QString suffix);

    QByteArray hashBase;
    QByteArray sendedHashBase;
    static QSet<QString> extensionAllowed;
    static QString commandUpdateDatapackBase;
private:
    static QRegularExpression regex_DATAPACK_FILE_REGEX;
    /// \todo group into one thread by change for queue
    QXzDecodeThread xzDecodeThreadBase;
    bool datapackTarXzBase;
    CatchChallenger::DatapackChecksum datapackChecksum;
    int index_mirror_base;
    static QRegularExpression excludePathBase;
    //file list
    struct query_files
    {
        quint8 id;
        QStringList filesName;
    };
    QList<query_files> query_files_list_base;
    bool wait_datapack_content_base;
    QStringList datapackFilesListBase;
    QList<quint32> partialHashListBase;
    static QString text_slash;
    static QString text_dotcoma;
    bool httpError;
    bool httpModeBase;
    const QString mDatapackBase;
    struct UrlInWaiting
    {
        QString fileName;
    };
    QHash<QNetworkReply *,UrlInWaiting> urlInWaitingListBase;
private slots:
    void getHttpFileBase(const QString &url, const QString &fileName);
    void httpFinishedBase();
    void datapackDownloadFinishedBase();
    void datapackChecksumDoneBase(const QStringList &datapackFilesList,const QByteArray &hash, const QList<quint32> &partialHash);
    void haveTheDatapack();
signals:
    void doDifferedChecksumBase(const QString &datapackPath);
};
}

#endif // Protocol_and_data_H
