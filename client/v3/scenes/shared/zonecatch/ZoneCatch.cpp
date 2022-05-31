// Copyright 2021 CatchChallenger
#include "ZoneCatch.hpp"

#include <iostream>

#include "../../../../../general/base/CommonDatapackServerSpec.hpp"
#include "../../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../../FacilityLibClient.hpp"
#include "../../../Globals.hpp"
#include "../../../core/AssetsLoader.hpp"
#include "../../../core/Logger.hpp"
#include "../../../entities/PlayerInfo.hpp"
#include "../../../entities/Utils.hpp"
#include "../../../ui/Row.hpp"

using Scenes::ZoneCatch;
using std::placeholders::_1;
using std::placeholders::_2;

ZoneCatch::ZoneCatch() {
  SetDialogSize(600, 500);

  wait_text_ = UI::Label::Create(this);
  wait_time_ = UI::Label::Create(this);
  cancel_button_ = UI::YellowButton::Create(this);
  cancel_button_->SetText(tr("Cancel"));

  city.capture.day = CatchChallenger::City::Capture::Day::Monday;
  city.capture.frenquency =
      CatchChallenger::City::Capture::Frequency::Frequency_week;
  city.capture.hour = 0;
  city.capture.minute = 0;
  zonecatch = false;
  zonecatchName.clear();
}

ZoneCatch::~ZoneCatch() {}

ZoneCatch *ZoneCatch::Create() { return new (std::nothrow) ZoneCatch(); }

void ZoneCatch::Draw(QPainter *painter) { UI::Dialog::Draw(painter); }

void ZoneCatch::OnEnter() {
  UI::Dialog::OnEnter();

  auto client = ConnectionManager::GetInstance()->client;

  if (!connect(&nextCityCatchTimer, &QTimer::timeout, this,
               &ZoneCatch::CityCaptureUpdateTime))
    abort();
  if (!connect(&updater_page_zonecatch, &QTimer::timeout, this,
               &ZoneCatch::UpdatePageZoneCatch))
    abort();
  if (!connect(client,
               &CatchChallenger::Api_client_real::QtcaptureCityYourAreNotLeader,
               this, &ZoneCatch::CaptureCityYourAreNotLeader))
    abort();
  if (!connect(client,
               &CatchChallenger::Api_client_real::
                   QtcaptureCityYourLeaderHaveStartInOtherCity,
               this, &ZoneCatch::CaptureCityYourLeaderHaveStartInOtherCity))
    abort();
  if (!connect(
          client,
          &CatchChallenger::Api_client_real::QtcaptureCityPreviousNotFinished,
          this, &ZoneCatch::CaptureCityPreviousNotFinished))
    abort();
  if (!connect(client,
               &CatchChallenger::Api_client_real::QtcaptureCityStartBattle,
               this, &ZoneCatch::CaptureCityStartBattle))
    abort();
  if (!connect(client,
               &CatchChallenger::Api_client_real::QtcaptureCityStartBotFight,
               this, &ZoneCatch::CaptureCityStartBotFight))
    abort();
  if (!connect(client,
               &CatchChallenger::Api_client_real::QtcaptureCityDelayedStart,
               this, &ZoneCatch::CaptureCityDelayedStart))
    abort();
  if (!connect(client, &CatchChallenger::Api_client_real::QtcaptureCityWin,
               this, &ZoneCatch::CaptureCityWin))
    abort();
  if (!connect(client, &CatchChallenger::Api_client_real::QtcityCapture, this,
               &ZoneCatch::CityCapture))
    abort();
}

void ZoneCatch::OnExit() {
  auto client = ConnectionManager::GetInstance()->client;

  disconnect(&nextCityCatchTimer, &QTimer::timeout, this,
             &ZoneCatch::CityCaptureUpdateTime);
  disconnect(&updater_page_zonecatch, &QTimer::timeout, this,
             &ZoneCatch::UpdatePageZoneCatch);
  disconnect(client,
             &CatchChallenger::Api_client_real::QtcaptureCityYourAreNotLeader,
             this, &ZoneCatch::CaptureCityYourAreNotLeader);
  disconnect(client,
             &CatchChallenger::Api_client_real::
                 QtcaptureCityYourLeaderHaveStartInOtherCity,
             this, &ZoneCatch::CaptureCityYourLeaderHaveStartInOtherCity);
  disconnect(
      client,
      &CatchChallenger::Api_client_real::QtcaptureCityPreviousNotFinished, this,
      &ZoneCatch::CaptureCityPreviousNotFinished);
  disconnect(client,
             &CatchChallenger::Api_client_real::QtcaptureCityStartBattle, this,
             &ZoneCatch::CaptureCityStartBattle);
  disconnect(client,
             &CatchChallenger::Api_client_real::QtcaptureCityStartBotFight,
             this, &ZoneCatch::CaptureCityStartBotFight);
  disconnect(client,
             &CatchChallenger::Api_client_real::QtcaptureCityDelayedStart, this,
             &ZoneCatch::CaptureCityDelayedStart);
  disconnect(client, &CatchChallenger::Api_client_real::QtcaptureCityWin, this,
             &ZoneCatch::CaptureCityWin);
  disconnect(client, &CatchChallenger::Api_client_real::QtcityCapture, this,
             &ZoneCatch::CityCapture);
  UI::Dialog::OnExit();
}

void ZoneCatch::OnScreenResize() {
  UI::Dialog::OnScreenResize();

  auto content = ContentBoundary();

  wait_text_->SetPos(content.x(), content.y());
  wait_time_->SetPos(content.x(), wait_text_->Bottom() + 10);

  cancel_button_->SetSize(150, 40);
  cancel_button_->SetPixelSize(14);
  cancel_button_->SetPos(
      content.x() + content.width() / 2 - cancel_button_->Width() / 2,
      wait_time_->Bottom() + 10);
}

void ZoneCatch::SetZone(std::string zone) {
  if (QtDatapackClientLoader::GetInstance()->get_zonesExtra().find(zone) !=
      QtDatapackClientLoader::GetInstance()->get_zonesExtra().cend()) {
    zonecatchName =
        QtDatapackClientLoader::GetInstance()->get_zonesExtra().at(zone).name;
    wait_text_->SetText(
        tr("You are waiting to capture %1")
            .arg(QString("%1").arg(QString::fromStdString(zonecatchName))));
  } else {
    zonecatchName.clear();
    wait_text_->SetText(tr("You are waiting to capture a zone"));
  }
  updater_page_zonecatch.start(1000);
  nextCatchOnScreen = nextCatch;
  zonecatch = true;
  ConnectionManager::GetInstance()->client->waitingForCityCapture(false);
  UpdatePageZoneCatch();
}

void ZoneCatch::CityCaptureUpdateTime() {
  if (city.capture.frenquency ==
      CatchChallenger::City::Capture::Frequency_week) {
    nextCatch = QDateTime::fromMSecsSinceEpoch(
        QDateTime::currentMSecsSinceEpoch() + 24 * 3600 * 7 * 1000);
  } else {
    nextCatch = QDateTime::fromMSecsSinceEpoch(
        CatchChallenger::FacilityLib::nextCaptureTime(city));
  }
  nextCityCatchTimer.start(static_cast<uint32_t>(
      nextCatch.toMSecsSinceEpoch() - QDateTime::currentMSecsSinceEpoch()));
}

void ZoneCatch::UpdatePageZoneCatch() {
  if (QDateTime::currentMSecsSinceEpoch() <
      nextCatchOnScreen.toMSecsSinceEpoch()) {
    int sec = static_cast<uint32_t>(nextCatchOnScreen.toMSecsSinceEpoch() -
                                    QDateTime::currentMSecsSinceEpoch()) /
                  1000 +
              1;
    std::string timeText;
    if (sec > 3600 * 24 * 365 * 50) {
      timeText = "Time player: bug";
    } else {
      timeText = CatchChallenger::FacilityLibClient::timeToString(sec);
    }
    wait_time_->SetText(tr("Remaining time: %1")
                            .arg(QStringLiteral("<b>%1</b>")
                                     .arg(QString::fromStdString(timeText))));
    cancel_button_->SetVisible(true);
  } else {
    cancel_button_->SetVisible(false);
    wait_time_->SetText("<i>" + tr("In waiting of players list") + "</i>");
    updater_page_zonecatch.stop();
  }
}

void ZoneCatch::OnCancelClick() {
  updater_page_zonecatch.stop();
  zonecatch = false;
}

void ZoneCatch::CaptureCityYourAreNotLeader() {
  updater_page_zonecatch.stop();
  zonecatch = false;
}

void ZoneCatch::CaptureCityYourLeaderHaveStartInOtherCity(
    const std::string &zone) {
  updater_page_zonecatch.stop();
  zonecatch = false;
}

void ZoneCatch::CaptureCityPreviousNotFinished() {
  updater_page_zonecatch.stop();
  zonecatch = false;
}

void ZoneCatch::CaptureCityStartBattle(const uint16_t &player_count,
                                       const uint16_t &clan_count) {
  updater_page_zonecatch.stop();
}

void ZoneCatch::CaptureCityStartBotFight(const uint16_t &player_count,
                                         const uint16_t &clan_count,
                                         const uint16_t &fightId) {
  updater_page_zonecatch.stop();
}

void ZoneCatch::CaptureCityDelayedStart(const uint16_t &player_count,
                                        const uint16_t &clan_count) {
  updater_page_zonecatch.stop();
}

void ZoneCatch::CaptureCityWin() {
  updater_page_zonecatch.stop();
  zonecatch = false;
}

void ZoneCatch::CityCapture(const uint32_t &remainingTime,
                            const uint8_t &type) {
  if (remainingTime == 0) {
    nextCityCatchTimer.stop();
    std::cout << "City capture disabled" << std::endl;
    return;  // disabled
  }
  nextCityCatchTimer.start(remainingTime * 1000);
  nextCatch = QDateTime::fromMSecsSinceEpoch(
      QDateTime::currentMSecsSinceEpoch() + remainingTime * 1000);
  city.capture.frenquency = (CatchChallenger::City::Capture::Frequency)type;
  city.capture.day =
      (CatchChallenger::City::Capture::Day)QDateTime::currentDateTime()
          .addSecs(remainingTime)
          .date()
          .dayOfWeek();
  city.capture.hour = static_cast<uint8_t>(
      QDateTime::currentDateTime().addSecs(remainingTime).time().hour());
  city.capture.minute = static_cast<uint8_t>(
      QDateTime::currentDateTime().addSecs(remainingTime).time().minute());
}

std::string ZoneCatch::GetZoneName() const { return zonecatchName; }

bool ZoneCatch::IsNextCityCatchActive() const {
  return nextCityCatchTimer.isActive();
}
