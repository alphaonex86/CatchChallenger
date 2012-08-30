#ifndef POKECRAFT_FACILITYLIB_H
#define POKECRAFT_FACILITYLIB_H

#include <QString>

namespace Pokecraft {
class FacilityLib
{
public:
	static QByteArray toUTF8(const QString &text);
};
}

#endif
