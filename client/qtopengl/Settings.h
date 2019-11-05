#ifndef SETTINGS_H
#define SETTINGS_H

#include <QSettings>

class Settings
{
public:
    Settings();
    // /data/user/0/org.qtprojet.example.catchchallenger/files
    static QSettings settings;
};

#endif // SETTINGS_H
