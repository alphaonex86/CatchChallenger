#ifndef RSSNEWS_H
#define RSSNEWS_H

#include <vector>
#include <string>
#include <QTimer>
#include <QDateTime>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "../../general/base/tinyXML2/tinyxml2.h"

class FeedNews : public QObject
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
    static FeedNews *feedNews;
    static std::string getText(const std::string &version);
signals:
    void feedEntryList(const std::vector<FeedNews::FeedEntry> &entryList, std::string error=std::string()) const;
private:
    QTimer newUpdateTimer;
    QTimer firstUpdateTimer;
    QNetworkAccessManager *qnam;
    QNetworkReply *reply;
private slots:
    void downloadFile();
    void httpFinished();
    void loadFeeds(const QByteArray &data);
    void loadRss(const tinyxml2::XMLElement *root);
    void loadAtom(const tinyxml2::XMLElement *root);
};

#endif // RSSNEWS_H
