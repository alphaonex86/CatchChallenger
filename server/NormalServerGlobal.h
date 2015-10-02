#ifndef NORMALSERVERGLOBAL_H
#define NORMALSERVERGLOBAL_H

#include <std::unordered_settings>
#include <std::string>

class NormalServerGlobal
{
public:
    NormalServerGlobal();
    static void checkSettingsFile(std::unordered_settings * const settings, const std::string &datapack_basePath);
};

#endif // NORMALSERVERGLOBAL_H
