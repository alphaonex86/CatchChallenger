#ifndef SETTINGSALL_H
#define SETTINGSALL_H

#include <QSettings>
#include <vector>

class SettingsAll
{
public:
    static void putDefaultSettings(QSettings &settings);
    static void loadSettings(QSettings &settings, bool &displaycity, std::vector<std::string> &citiesNames, float &scale_City,bool &doallmap);
};

#endif // SETTINGSALL_H
