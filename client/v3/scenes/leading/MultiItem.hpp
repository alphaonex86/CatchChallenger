// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_SCENES_LEADING_MULTIITEM_HPP_
#define CLIENT_V3_SCENES_LEADING_MULTIITEM_HPP_

#include "../../base/ConnectionInfo.hpp"
#include "../../ui/SelectableItem.hpp"

namespace Scenes {
class MultiItem : public UI::SelectableItem {
 public:
  static MultiItem *Create(const ConnectionInfo &connexionInfo);
  ~MultiItem();

  void DrawContent(QPainter *painter) override;
  ConnectionInfo Connection() const;

 private:
  explicit MultiItem(const ConnectionInfo &connexionInfo);
  ConnectionInfo m_connexionInfo;
  QString name_;

  void FetchName();
};
}  // namespace Scenes
#endif  // CLIENT_V3_SCENES_LEADING_MULTIITEM_HPP_
