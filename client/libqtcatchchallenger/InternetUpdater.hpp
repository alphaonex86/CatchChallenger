#ifndef __EMSCRIPTEN__
#ifndef INTERNETUPDATER_H
#define INTERNETUPDATER_H

#include <QObject>
#include <QString>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "../../general/base/lib.h"

class DLL_PUBLIC InternetUpdater : public QObject
{
    Q_OBJECT
public:
    explicit InternetUpdater();
    static InternetUpdater *internetUpdater;
    static std::string getText(const std::string &version);
    #if defined(_WIN32) || defined(Q_OS_MAC)
    static std::string GetOSDisplayString();
    #endif
signals:
    void newUpdate(const std::string &version) const;
    void errorUpdate(const std::string &errorString) const;
    void noNewUpdate() const;
private:
    QTimer newUpdateTimer;
    QTimer firstUpdateTimer;
    QNetworkAccessManager qnam;
    QNetworkReply *reply;
private slots:
    void downloadFile();
    void httpFinished();
    bool versionIsNewer(const std::string &version);
};

#endif // INTERNETUPDATER_H
#endif // #ifndef __EMSCRIPTEN__
