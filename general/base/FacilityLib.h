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
    static QString randomPassword(const QString& string,const quint8& length);
    static QStringList skinIdList(const QString& skinPath);
    static QString secondsToString(const quint64 &seconds);
private:
    static QByteArray UTF8EmptyData;
};
}

#endif
