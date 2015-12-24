#include "FacilityLibClient.h"
#include <QObject>

using namespace CatchChallenger;

QString FacilityLibClient::timeToString(const uint32_t &sec)
{
    QString timeText;
    if(sec>3600*24*365*50)
        timeText="Time player: bug";
    else if(sec>=3600*24*10)
        timeText=QObject::tr("%n day(s)","",sec/(3600*24));
    else if(sec>=3600*24)
        timeText=QObject::tr("%n day(s) and %1","",sec/(3600*24)).arg(QObject::tr("%n hour(s)","",(sec%(3600*24))/3600));
    else if(sec>=3600)
        timeText=QObject::tr("%n hour(s) and %1","",sec/3600).arg(QObject::tr("%n minute(s)","",(sec%3600)/60));
    else if(sec>=60)
        timeText=QObject::tr("%n minute(s) and %1","",sec/60).arg(QObject::tr("%n second(s)","",sec%60));
    else
        timeText=QObject::tr("%n second(s)","",sec);
    return timeText;
}

QStringList FacilityLibClient::stdvectorstringToQStringList(const std::vector<std::string> &vector)
{
    QStringList list;
    unsigned int index=0;
    while(index<vector.size())
    {
        list << QString::fromStdString(vector.at(index));
        ++index;
    }
    return list;
}

bool FacilityLibClient::rectTouch(QRect r1,QRect r2)
{
    if (r1.isNull() || r2.isNull())
        return false;

    if((r1.x()+r1.width())<r2.x())
        return false;
    if((r2.x()+r2.width())<r1.x())
        return false;

    if((r1.y()+r1.height())<r2.y())
        return false;
    if((r2.y()+r2.height())<r1.y())
        return false;

    return true;
}
