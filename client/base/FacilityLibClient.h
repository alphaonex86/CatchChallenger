#ifndef CATCHCHALLENGER_FACILITYLIBClient_H
#define CATCHCHALLENGER_FACILITYLIBClient_H

#include <QString>
#include <QList>
#include <QStringList>
#include <QHash>
#include <QRect>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>

namespace CatchChallenger {
class FacilityLibClient
{
public:
    static std::string timeToString(const uint32_t &sec);
    static std::vector<std::string> stdvectorstringToQStringList(const std::vector<std::string> &vector);
    static bool rectTouch(QRect r1,QRect r2);
};

}

#endif
