#ifndef NORMALSERVERGLOBAL_H
#define NORMALSERVERGLOBAL_H

#include <QSettings>

class NormalServerGlobal
{
public:
    NormalServerGlobal();
    static void checkSettingsFile(QSettings *settings);
};

#endif // NORMALSERVERGLOBAL_H
