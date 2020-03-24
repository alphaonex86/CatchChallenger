#include "Settings.hpp"
#include <QStandardPaths>
#include <QDir>

QSettings Settings::settings(QStandardPaths::standardLocations(QStandardPaths::DataLocation).first()+"/config.ini",QSettings::IniFormat);
//QSettings Settings::settings;

Settings::Settings()
{
    #ifdef Q_OS_ANDROID
    /*if(QSettings().fileName().startsWith("/.config"))
    {
        QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, QDir::homePath() + "/.config");
    }*/
    #endif
}
