#ifndef NORMALSERVERGLOBAL_H
#define NORMALSERVERGLOBAL_H

#include <QSettings>
#include <QString>

class NormalServerGlobal
{
public:
    NormalServerGlobal();
    static void checkSettingsFile(QSettings * const settings, const QString &datapack_basePath);
};

#endif // NORMALSERVERGLOBAL_H
