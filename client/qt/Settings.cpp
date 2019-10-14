#include "Settings.h"
#include <QStandardPaths>

#ifdef Q_OS_ANDROID
if (QSettings().fileName().startsWith("/.config"))

{ QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, QDir::homePath() + "/.config"); }
#endif
QSettings Settings::settings(QStandardPaths::standardLocations(QStandardPaths::DataLocation).first(),QSettings::IniFormat);

Settings::Settings()
{
}
