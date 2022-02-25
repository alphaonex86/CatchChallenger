// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_SCENES_SHARED_ZONECATCH_ZONECATCH_HPP_
#define CLIENT_V3_SCENES_SHARED_ZONECATCH_ZONECATCH_HPP_

#include <QTimer>

#include "../../../../../general/base/GeneralStructures.hpp"
#include "../../../../../general/base/GeneralStructuresXml.hpp"
#include "../../../base/ConnectionManager.hpp"
#include "../../../core/Node.hpp"
#include "../../../core/Sprite.hpp"
#include "../../../ui/Dialog.hpp"
#include "../../../ui/Label.hpp"
#include "../../../ui/ListView.hpp"
#include "../../../ui/ThemedButton.hpp"

namespace Scenes {
class ZoneCatch : public QObject, public UI::Dialog {
  Q_OBJECT
 public:
  static ZoneCatch *Create();
  ~ZoneCatch();

  void Draw(QPainter *painter) override;
  void OnEnter() override;
  void OnExit() override;
  void OnScreenResize() override;

  void Close();
  std::string GetZoneName() const;
  bool IsNextCityCatchActive() const;
  void SetZone(std::string zone);

 private:
  ZoneCatch();

  UI::Label *wait_text_;
  UI::Label *wait_time_;
  UI::Button *cancel_button_;

  // city
  QTimer nextCityCatchTimer;
  CatchChallenger::City city;
  QTimer updater_page_zonecatch;
  QDateTime nextCatch;
  QDateTime nextCatchOnScreen;
  std::string zonecatchName;
  bool zonecatch;

  void CityCaptureUpdateTime();
  void UpdatePageZoneCatch();
  void OnCancelClick();
  void CaptureCityYourAreNotLeader();
  void CaptureCityYourLeaderHaveStartInOtherCity(const std::string &zone);
  void CaptureCityPreviousNotFinished();
  void CaptureCityStartBattle(const uint16_t &player_count,
                              const uint16_t &clan_count);
  void CaptureCityStartBotFight(const uint16_t &player_count,
                                const uint16_t &clan_count,
                                const uint16_t &fightId);
  void CaptureCityDelayedStart(const uint16_t &player_count,
                               const uint16_t &clan_count);
  void CaptureCityWin();
  void CityCapture(const uint32_t &remainingTime, const uint8_t &type);
};
}  // namespace Scenes
#endif  // CLIENT_V3_SCENES_SHARED_ZONECATCH_ZONECATCH_HPP_
