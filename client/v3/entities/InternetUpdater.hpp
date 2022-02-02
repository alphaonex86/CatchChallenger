#ifndef __EMSCRIPTEN__
  #ifndef INTERNETUPDATER_H
    #define INTERNETUPDATER_H

    #include <QNetworkAccessManager>
    #include <QNetworkReply>
    #include <QObject>
    #include <QString>
    #include <QTimer>

class InternetUpdater : public QObject {
  Q_OBJECT
 public:
  static InternetUpdater *GetInstance();

  static std::string getText(const std::string &version);
    #if defined(_WIN32) || defined(Q_OS_MAC)
  static std::string GetOSDisplayString();
    #endif
 signals:
  void newUpdate(const std::string &version) const;

 private:
  static InternetUpdater *internetUpdater;
  QTimer newUpdateTimer;
  QTimer firstUpdateTimer;
  QNetworkAccessManager qnam;
  QNetworkReply *reply;

  explicit InternetUpdater();
 private slots:
  void downloadFile();
  void httpFinished();
  bool versionIsNewer(const std::string &version);
};

  #endif  // INTERNETUPDATER_H
#endif    // #ifndef __EMSCRIPTEN__
