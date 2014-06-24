#ifndef RSSNEWS_H
#define RSSNEWS_H

#include <QThread>
#include <QList>
#include <QString>
#include <QTimer>
#include <QDateTime>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class RssNews : public QThread
{
    Q_OBJECT
public:
    struct RssEntry
    {
        QString title;
        QString link;
        QString description;
        QDateTime pubDate;
    };
    explicit RssNews();
    ~RssNews();
    static RssNews *rssNews;
    static QString getText(const QString &version);
signals:
    void rssEntryList(const QList<RssNews::RssEntry> &entryList) const;
private:
    QTimer newUpdateTimer;
    QTimer firstUpdateTimer;
    QNetworkAccessManager *qnam;
    QNetworkReply *reply;
private slots:
    void downloadFile();
    void httpFinished();
    void loadRss(const QByteArray &data);
};

#endif // RSSNEWS_H
