#include "BaseWindow.h"
#include "ui_BaseWindow.h"
#include "../../general/base/FacilityLib.h"
#include "DatapackClientLoader.h"

#include <QInputDialog>

using namespace CatchChallenger;

void BaseWindow::cityCaptureUpdateTime()
{
    if(city.capture.frenquency==City::Capture::Frequency_week)
        nextCatch=QDateTime::fromMSecsSinceEpoch(QDateTime::currentMSecsSinceEpoch()+24*3600*7*1000);
    else
        nextCatch=FacilityLib::nextCaptureTime(city);
    nextCityCatchTimer.start(nextCatch.toMSecsSinceEpoch()-QDateTime::currentMSecsSinceEpoch());
}

void BaseWindow::updatePageZoneCatch()
{
    if(QDateTime::currentMSecsSinceEpoch()<nextCatchOnScreen.toMSecsSinceEpoch())
    {
        int sec=(nextCatchOnScreen.toMSecsSinceEpoch()-QDateTime::currentMSecsSinceEpoch())/1000+1;
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
        ui->zonecaptureWaitTime->setText(tr("Remaining time: %1").arg(QStringLiteral("<b>%1</b>").arg(timeText)));
        ui->zonecaptureCancel->setVisible(true);
    }
    else
    {
        ui->zonecaptureCancel->setVisible(false);
        ui->zonecaptureWaitTime->setText("<i>"+tr("In waiting of players list")+"</i>");
        updater_page_zonecatch.stop();
    }
}

void BaseWindow::on_zonecaptureCancel_clicked()
{
    updater_page_zonecatch.stop();
    ui->stackedWidget->setCurrentWidget(ui->page_map);
    CatchChallenger::Api_client_real::client->waitingForCityCapture(true);
    zonecatch=false;
}

void BaseWindow::captureCityYourAreNotLeader()
{
    updater_page_zonecatch.stop();
    ui->stackedWidget->setCurrentWidget(ui->page_map);
    showTip(tr("You are not a clan leader to start a city capture"));
    zonecatch=false;
}

void BaseWindow::captureCityYourLeaderHaveStartInOtherCity(const QString &zone)
{
    updater_page_zonecatch.stop();
    ui->stackedWidget->setCurrentWidget(ui->page_map);
    if(DatapackClientLoader::datapackLoader.zonesExtra.contains(zone))
        showTip(tr("Your clan leader have start a capture for another city")+QStringLiteral(": %1").arg(QStringLiteral("<b>%1</b>").arg(DatapackClientLoader::datapackLoader.zonesExtra[zone].name)));
    else
        showTip(tr("Your clan leader have start a capture for another city"));
    zonecatch=false;
}

void BaseWindow::captureCityPreviousNotFinished()
{
    updater_page_zonecatch.stop();
    ui->stackedWidget->setCurrentWidget(ui->page_map);
    showTip(tr("Previous capture of this city is not finished"));
    zonecatch=false;
}

void BaseWindow::captureCityStartBattle(const quint16 &player_count,const quint16 &clan_count)
{
    ui->zonecaptureCancel->setVisible(false);
    ui->zonecaptureWaitTime->setText("<i>"+tr("%1 and %2 in wainting to capture the city").arg("<b>"+tr("%n player(s)","",player_count)+"</b>").arg("<b>"+tr("%n clan(s)","",clan_count)+"</b>")+"</i>");
    updater_page_zonecatch.stop();
}

void BaseWindow::captureCityStartBotFight(const quint16 &player_count,const quint16 &clan_count,const quint32 &fightId)
{
    ui->zonecaptureCancel->setVisible(false);
    ui->zonecaptureWaitTime->setText("<i>"+tr("%1 and %2 in wainting to capture the city").arg("<b>"+tr("%n player(s)","",player_count)+"</b>").arg("<b>"+tr("%n clan(s)","",clan_count)+"</b>")+"</i>");
    updater_page_zonecatch.stop();
    botFight(fightId);
}

void BaseWindow::captureCityDelayedStart(const quint16 &player_count,const quint16 &clan_count)
{
    ui->zonecaptureCancel->setVisible(false);
    ui->zonecaptureWaitTime->setText("<i>"+tr("In waiting fight.")+" "+tr("%1 and %2 in wainting to capture the city").arg("<b>"+tr("%n player(s)","",player_count)+"</b>").arg("<b>"+tr("%n clan(s)","",clan_count)+"</b>")+"</i>");
    updater_page_zonecatch.stop();
}

void BaseWindow::captureCityWin()
{
    updater_page_zonecatch.stop();
    ui->stackedWidget->setCurrentWidget(ui->page_map);
    if(!zonecatchName.isEmpty())
        showTip(tr("Your clan win the city")+QStringLiteral(": %1").arg(QStringLiteral("<b>%1</b>").arg(zonecatchName)));
    else
        showTip(tr("Your clan win the city"));
    zonecatch=false;
}
