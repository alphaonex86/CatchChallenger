#ifndef RSSNEWS_H
#define RSSNEWS_H

#include <QThread>
#include <QList>
#include <QString>
#include <QTimer>
#include <QDateTime>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QDomElement>

class FeedNews : public QThread
{
    Q_OBJECT
public:
    struct FeedEntry
    {
        QString title;
        QString link;
        QString description;
        QDateTime pubDate;
    };
    explicit FeedNews();
    ~FeedNews();
    static FeedNews *rssNews;
    static QString getText(const QString &version);
signals:
    void feedEntryList(const QList<FeedNews::FeedEntry> &entryList, QString error=QString()) const;
private:
    QTimer newUpdateTimer;
    QTimer firstUpdateTimer;
    QNetworkAccessManager *qnam;
    QNetworkReply *reply;
private slots:
    void downloadFile();
    void httpFinished();
    void loadFeeds(const QByteArray &data);
    void loadRss(const QDomElement &root);
    void loadAtom(const QDomElement &root);
};

#endif // RSSNEWS_H
