#include "Settings.h"
#include <QStandardPaths>
#include <QDir>

QSettings Settings::settings(QStandardPaths::standardLocations(QStandardPaths::DataLocation).first(),QSettings::IniFormat);

Settings::Settings()
{
    #ifdef Q_OS_ANDROID
    /*if(QSettings().fileName().startsWith("/.config"))
    {
        QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, QDir::homePath() + "/.config");
    }*/
    #endif
}
