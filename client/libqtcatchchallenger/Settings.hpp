#ifndef SETTINGS_H
#define SETTINGS_H

#include <QSettings>
#include "../../general/base/lib.h"

class DLL_PUBLIC Settings
{
public:
    Settings();
    // /data/user/0/org.qtprojet.example.catchchallenger/files
    static QSettings *settings;//without pointer is out of control and file is created everywhere
    static void init();
};

#endif // SETTINGS_H
