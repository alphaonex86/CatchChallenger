// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_UI_GRIDVIEW_HPP_
#define CLIENT_V3_UI_GRIDVIEW_HPP_

#include <vector>

#include "ListView.hpp"

namespace UI {
class GridView : public ListView {
 public:
  static GridView *Create(Node *parent = nullptr);

  void SetItemSize(qreal width, qreal height);

 protected:
  explicit GridView(Node *parent = nullptr);
  void ReCalculateSize() override;

 private:
  QSizeF item_size_;
};
}  // namespace UI
#endif  // CLIENT_V3_UI_GRIDVIEW_HPP_
