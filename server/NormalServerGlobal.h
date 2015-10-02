#ifndef NORMALSERVERGLOBAL_H
#define NORMALSERVERGLOBAL_H

#include <QSettings>
#include <std::string>

class NormalServerGlobal
{
public:
    NormalServerGlobal();
    static void checkSettingsFile(QSettings * const settings, const std::string &datapack_basePath);
};

#endif // NORMALSERVERGLOBAL_H
