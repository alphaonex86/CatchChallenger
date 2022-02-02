#ifndef RSSNEWS_H
#define RSSNEWS_H

#include <QDateTime>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <string>
#include <vector>

#include "../../../general/tinyXML2/tinyxml2.hpp"

class FeedNews : public QObject {
  Q_OBJECT
 public:
  struct FeedEntry {
    QString title;
    QString link;
    QString description;
    QDateTime pubDate;
  };

  static FeedNews *GetInstance();

  ~FeedNews();
  void checkCache();
  static std::string getText(const std::string &version);
 signals:
  void feedEntryList(const std::vector<FeedNews::FeedEntry> &entryList,
                     std::string error = std::string()) const;

 private:
  QTimer newUpdateTimer;
  QTimer firstUpdateTimer;
  QNetworkAccessManager *qnam;
  QNetworkReply *reply;
  static FeedNews *feedNews;

  explicit FeedNews();
 private slots:
  void downloadFile();
  void httpFinished();
  void loadFeeds(const QByteArray &data);
  void loadRss(const tinyxml2::XMLElement *root);
  void loadAtom(const tinyxml2::XMLElement *root);
};

#endif  // RSSNEWS_H
