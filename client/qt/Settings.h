#ifndef SETTINGS_H
#define SETTINGS_H

#include <QSettings>

class Settings
{
public:
    Settings();
    static QSettings settings;
};

#endif // SETTINGS_H
