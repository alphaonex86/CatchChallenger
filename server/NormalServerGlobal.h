#ifndef NORMALSERVERGLOBAL_H
#define NORMALSERVERGLOBAL_H

#include <string>

class NormalServerGlobal
{
public:
    NormalServerGlobal();
    static void displayInfo();
    static void checkSettingsFile(QSettings * const settings, const std::string &datapack_basePath);
};

#endif // NORMALSERVERGLOBAL_H
