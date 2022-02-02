// Copyright 2021 <CatchChallenger>
#ifndef CLIENT_V3_SCENES_SHARED_PLAYER_QUESTITEM_HPP_
#define CLIENT_V3_SCENES_SHARED_PLAYER_QUESTITEM_HPP_

#include "../../../ui/SelectableItem.hpp"

namespace Scenes {
class QuestItem : public UI::SelectableItem {
 public:
  ~QuestItem();
  static QuestItem *Create(QString text);

  void DrawContent(QPainter *painter) override;

 private:
  QString text_;

  QuestItem(QString text);
};
}  // namespace Scenes
#endif  // CLIENT_V3_SCENES_SHARED_PLAYER_QUESTITEM_HPP_
