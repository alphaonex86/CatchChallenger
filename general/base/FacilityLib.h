#ifndef POKECRAFT_FACILITYLIB_H
#define POKECRAFT_FACILITYLIB_H

#include <QByteArray>
#include <QString>
#include <QStringList>

namespace Pokecraft {
class FacilityLib
{
public:
    static QByteArray toUTF8(const QString &text);
    static QStringList listFolder(const QString& folder,const QString& suffix="");
};
}

#endif
