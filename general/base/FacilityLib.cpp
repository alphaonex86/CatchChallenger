#include "FacilityLib.h"

using namespace Pokecraft;

QByteArray FacilityLib::toUTF8(const QString &text)
{
	if(text.isEmpty() || text.size()>255)
		return QByteArray();
	QByteArray returnedData,data;
	data=text.toUtf8();
	if(data.size()==0 || data.size()>255)
		return QByteArray();
	returnedData[0]=data.size();
	returnedData+=data;
	return returnedData;
}

