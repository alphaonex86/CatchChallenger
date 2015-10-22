#include "FacilityLibClient.h"

using namespace CatchChallenger;

QString FacilityLibClient::timeToString(const uint32_t &playerMonster);
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
}
