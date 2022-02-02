// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_UI_COLUMN_HPP_
#define CLIENT_V3_UI_COLUMN_HPP_

#include <vector>

#include "../core/Node.hpp"

namespace UI {
class Column : public Node {
 public:
  static Column *Create(Node *parent = nullptr);
  ~Column();

  void SetItemSpacing(int value);
  void ReCalculateSize();
  void AddChild(Node *node);
  /** Remove a child from the node */
  void RemoveChild(Node *node);

  void Draw(QPainter *painter) override;
  void OnChildResize(Node *node) override;
  void RegisterEvents() override;
  void UnRegisterEvents() override;

 protected:
  int spacing_;

  explicit Column(Node *parent = nullptr);

 private:
  bool is_processing_resize_;
};
}  // namespace UI
#endif  // CLIENT_V3_UI_ROW_HPP_
