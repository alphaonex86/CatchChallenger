#include "FacilityLibClient.hpp"
#include "../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include <QObject>
#include <iostream>

using namespace CatchChallenger;

std::string FacilityLibClient::timeToString(const uint32_t &sec)
{
    std::string timeText;
    if(sec>3600*24*365*50)
        timeText="Time player: bug";
    else if(sec>=3600*24*10)
        timeText=QObject::tr("%n day(s)","",sec/(3600*24)).toStdString();
    else if(sec>=3600*24)
    {
        int d=sec/(3600*24);
        int h=(sec%(3600*24))/3600;
        if(h!=0)
            timeText=QObject::tr("%n day(s) and %1","",d).arg(QObject::tr("%n hour(s)","",h)).toStdString();
        else
            timeText=QObject::tr("%n day(s)","",d).toStdString();
    }
    else if(sec>=3600)
    {
        int h=sec/3600;
        int m=(sec%3600)/60;
        if(m!=0)
            timeText=QObject::tr("%n hour(s) and %1","",h).arg(QObject::tr("%n minute(s)","",m)).toStdString();
        else
            timeText=QObject::tr("%n hour(s)","",h).toStdString();
    }
    else if(sec>=60)
    {
        int m=sec/60;
        int s=sec%60;
        if(s!=0)
            timeText=QObject::tr("%n minute(s) and %1","",m).arg(QObject::tr("%n second(s)","",s)).toStdString();
        else
            timeText=QObject::tr("%n minute(s)","",m).toStdString();
    }
    else
    {
        timeText=QObject::tr("%n second(s)","",sec).toStdString();
    }
    return timeText;
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

std::string FacilityLibClient::reputationRequirementsToText(const CatchChallenger::ReputationRequirements &reputationRequirements)
{
    if(reputationRequirements.reputationId>=CatchChallenger::CommonDatapack::commonDatapack.get_reputation().size())
    {
        std::cerr << "reputationRequirements.reputationId" << reputationRequirements.reputationId
                  << ">=CatchChallenger::CommonDatapack::commonDatapack.reputation.size()"
                  << CatchChallenger::CommonDatapack::commonDatapack.get_reputation().size() << std::endl;
        return QObject::tr("Unknown reputation id: %1").arg(reputationRequirements.reputationId).toStdString();
    }
    const CatchChallenger::Reputation &reputation=CatchChallenger::CommonDatapack::commonDatapack.get_reputation().at(reputationRequirements.reputationId);
    if(QtDatapackClientLoader::datapackLoader->get_reputationExtra().find(reputation.name)==
            QtDatapackClientLoader::datapackLoader->get_reputationExtra().cend())
    {
        std::cerr << "!QtDatapackClientLoader::datapackLoader->reputationExtra.contains("+reputation.name+")" << std::endl;
        return QObject::tr("Unknown reputation name: %1").arg(QString::fromStdString(reputation.name)).toStdString();
    }
    const QtDatapackClientLoader::ReputationExtra &reputationExtra=QtDatapackClientLoader::datapackLoader->get_reputationExtra().at(reputation.name);
    if(reputationRequirements.positif)
    {
        if(reputationRequirements.level>=reputationExtra.reputation_positive.size())
        {
            std::cerr << "No text for level "+std::to_string(reputationRequirements.level)+" for reputation "+reputationExtra.name << std::endl;
            return QStringLiteral("No text for level %1 for reputation %2")
                    .arg(reputationRequirements.level)
                    .arg(QString::fromStdString(reputationExtra.name))
                    .toStdString();
        }
        else
            return reputationExtra.reputation_positive.at(reputationRequirements.level);
    }
    else
    {
        if(reputationRequirements.level>=reputationExtra.reputation_negative.size())
        {
            std::cerr << "No text for level "+std::to_string(reputationRequirements.level)+" for reputation "+reputationExtra.name << std::endl;
            return QStringLiteral("No text for level %1 for reputation %2")
                    .arg(reputationRequirements.level)
                    .arg(QString::fromStdString(reputationExtra.name))
                    .toStdString();
        }
        else
            return reputationExtra.reputation_negative.at(reputationRequirements.level);
    }
}
